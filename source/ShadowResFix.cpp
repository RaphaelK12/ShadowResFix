/*
	ShadowResFix
	Fix cascading shadow mapping resolution, game renders the shadow map 
	four times into a 512x512 resolution texture, once for each cascade, 
	then copies each 512x512 texture side by side into a 1024x256 resolution 
	texture, the problem is that we lost half of the details, the fix is 
	​​simple, create a 2048x512 texture instead of a 1024x256 one. This fix 
	doesn't cause any performance penalties because the hard work is 
	generating the four 512x512 cascades that the game already does anyway, 
	so simply using a 2048x512 texture will use almost the same processing 
	power as a 1024x256 texture, but will have the double the details.
*/


#include "d3d9.h"

UINT oldCascadesWidth = 0;
UINT oldCascadesHeight = 0;
UINT newCascadesWidthX2 = 0;
UINT newCascadesHeightX2 = 0;

IDirect3DTexture9* pHDRTexQuarter = 0;

UINT gWindowWidth = 1366; // see dllmain.cpp-> CreateDevice/Reset
UINT gWindowHeight = 768;
UINT gWindowDivisor = 1;
// gTreeLeavesSwayInTheWind
extern BOOL gTreeLeavesSwayInTheWind;
extern BOOL gNearFarPlane;
extern BOOL gFixCascadedShadowMapResolution;
extern BOOL gFixRainDrops;
extern UINT gFixEmissiveTransparency;
extern UINT ReflectionResMult;

struct stTextue {
	UINT Cnt;
	UINT Width;
	UINT Height;
	UINT Levels;
	DWORD Usage;
	D3DFORMAT Format;
	D3DPOOL Pool;
	IDirect3DTexture9* pTexture;
};

struct stPixelShader {
	IDirect3DPixelShader9* pShader;
	UINT len;
	DWORD* pFunction;

	IDirect3DPixelShader9* pNewShader;
	UINT Newlen;
	DWORD* pNewFunction;
	std::string Source;
};

struct stVertexShader {
	IDirect3DVertexShader9* pShader;
	UINT len;
	DWORD* pFunction;

	IDirect3DVertexShader9* pNewShader;
	UINT Newlen;
	DWORD* pNewFunction;
	std::string Source;
};

struct stSurface {

};

std::vector<stTextue*> textureList;
std::vector<stPixelShader*> psList;
std::vector<stVertexShader*> vsList;
std::vector<stSurface*> surfaceList;

std::vector<uint8_t> pattern = {
	0x5F, 0x46, 0x75, 0x63, 0x6B, 0x59, 0x6F, 0x75, 0x5A, 0x6F, 0x6C, 0x69, 0x6B, 0x61, 0x5F, 0x01
};

std::list<IDirect3DPixelShader9*> ps;


HRESULT m_IDirect3DDevice9Ex::CreateTexture(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) {
	if(ReflectionResMult > 1 && ReflectionResMult <= 16) {
		if(Format == D3DFMT_A16B16G16R16F && Width == Height && Width >= 128 && Width <= 1024 && Levels == 4 && Usage == 1) {
			Width *= ReflectionResMult;
			Height *= ReflectionResMult;
		}
		if(Format == D3DFORMAT(1515474505u) && Width == Height && Width >= 128 && Width <= 1024 && Levels == 4 && Usage == 2) {
			Width *= ReflectionResMult;
			Height *= ReflectionResMult;
		}
	}

	
	//if(
	//	(Format == D3DFORMAT(1515474505u) && Width >= 256 && Width == Height && Levels == 1) //512x512 = 1024x256
	//	) {
	//}
	//if(Format == D3DFMT_DXT1 && Width == 32 && Height == 32 && Levels == 1) {
		//Width == 64;
		//Height == 64;
	//}
	//if(Format == D3DFMT_L8 && Width == 32 && Height == 32 && Levels == 1) {
		//Width == 64;
		//Height == 64;
	//}
	if(gFixCascadedShadowMapResolution) {
		if(Format == D3DFORMAT(D3DFMT_R16F) && Height >= 256 && Width == Height * 4 && Levels == 1) {
			// old res
			oldCascadesWidth = Width;
			oldCascadesHeight = Height;
			// x2
			Width *= 2;
			Height *= 2;
			// new res
			newCascadesWidthX2 = Width;
			newCascadesHeightX2 = Height;
		}
	}

	HRESULT hr = ProxyInterface->CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
	
	if(SUCCEEDED(hr) && ppTexture) {
		*ppTexture = new m_IDirect3DTexture9(*ppTexture, this);
	}

	//if(gFixRainDrops) // from AssaultKifle47
	if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth / gWindowDivisor && Height == gWindowHeight / gWindowDivisor && ppTexture != 0 && (*ppTexture) != 0) {
		pHDRTexQuarter = (*ppTexture);
	}

	return hr;
}

extern float NearFarPlane[4];
static BOOL bNFfound = false;
HRESULT m_IDirect3DDevice9Ex::SetPixelShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {
	if(gFixCascadedShadowMapResolution) {
		if(StartRegister == 53 && Vector4fCount == 1 && oldCascadesWidth != 0 && oldCascadesHeight != 0 &&
			pConstantData[0] == 1.f / oldCascadesWidth && pConstantData[1] == 1.f / oldCascadesHeight) {
			float vec[4] = { 0.f };
			vec[0] = 1.f / newCascadesWidthX2;
			vec[1] = 1.f / newCascadesHeightX2;
			vec[2] = 1.f / newCascadesHeightX2;
			vec[3] = pConstantData[3];
			// set pixel size to pixel shader
			return ProxyInterface->SetPixelShaderConstantF(StartRegister, vec, Vector4fCount);
		};
	}
	if(gNearFarPlane) {
		if(StartRegister == 128 && Vector4fCount == 1 && pConstantData[0] <= 1.f && pConstantData[1] >= 5.f && pConstantData[1] <= 15000.f) {
			NearFarPlane[0] = pConstantData[0];
			NearFarPlane[1] = pConstantData[1];
			NearFarPlane[2] = pConstantData[2];
			NearFarPlane[3] = pConstantData[3];
			bNFfound = true;
		}
	}
	HRESULT hr = 0;
	if(bNFfound && gNearFarPlane) {
		ProxyInterface->SetPixelShaderConstantF(128, NearFarPlane, 1);
	}
	if(bNFfound && gNearFarPlane && StartRegister == 128 && Vector4fCount == 1)
		hr = ProxyInterface->SetPixelShaderConstantF(StartRegister, NearFarPlane, Vector4fCount);
	else
		hr = ProxyInterface->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
	return hr;
}	

static float sVec[4] = { 0 };	// time saved values ​​for wind sway
static float vec[4] = { 0.f };	// temp vec4, util for debug, pConstantData does not show values
HRESULT m_IDirect3DDevice9Ex::SetVertexShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {
	if(gTreeLeavesSwayInTheWind) {
		// Force leaves to wind sway
		if(StartRegister == 51 && Vector4fCount == 1) {
			vec[0] = pConstantData[0];
			vec[1] = pConstantData[1];
			vec[2] = pConstantData[2];
			vec[3] = pConstantData[3];
			if(vec[3] > 7.f) {	// the maximum value found that isn't time is 7, 
				// so if time is a value that increases, 
				// any value above 7 is the time I'm looking for.
				sVec[0] = vec[0];
				sVec[1] = vec[1];
				sVec[2] = vec[2];
				sVec[3] = vec[3];
			}
			if(pConstantData[0] == 1.f && pConstantData[1] == 1.f && pConstantData[2] == 1.f && pConstantData[3] == 1.f
			   //&& ((unsigned int) (void*) pConstantData) == (0x103f2c0u) // only works with rgless
			   ) {
				vec[0] = sVec[0];	// like sin/cos/frac (game time in seconds)
				vec[1] = sVec[1];	// like sin/cos/frac (game time in seconds)
				vec[2] = sVec[2];	// like sin/cos/frac (game time in seconds)
				vec[3] = sVec[3];	// game time in seconds
			}
			return ProxyInterface->SetVertexShaderConstantF(StartRegister, vec, Vector4fCount);
		}
	}
	if(gNearFarPlane) {
		if(StartRegister == 128 && Vector4fCount == 1 && pConstantData[0] <= 1.f && pConstantData[1] >= 500.f && pConstantData[1] <= 15000.f) {
			NearFarPlane[0] = pConstantData[0];
			NearFarPlane[1] = pConstantData[1];
			NearFarPlane[2] = pConstantData[2];
			NearFarPlane[3] = pConstantData[3];
			bNFfound = true;
		}
	}
	HRESULT hr = 0;
	if(bNFfound && gNearFarPlane) {
		ProxyInterface->SetVertexShaderConstantF(128, NearFarPlane, 1);
	}
	if(bNFfound && gNearFarPlane && StartRegister == 128 && Vector4fCount == 1)
		hr = ProxyInterface->SetVertexShaderConstantF(StartRegister, NearFarPlane, Vector4fCount);
	else
		hr = ProxyInterface->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
	return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) {
	// from AssaultKifle47
	if(gFixRainDrops && Stage == 1 && pTexture == 0 && pHDRTexQuarter) {
		pTexture = pHDRTexQuarter;
	}
	if(pTexture) {
		switch(pTexture->GetType()) {
			case D3DRTYPE_TEXTURE:
				pTexture = static_cast<m_IDirect3DTexture9*>(pTexture)->GetProxyInterface();
				break;
			case D3DRTYPE_VOLUMETEXTURE:
				pTexture = static_cast<m_IDirect3DVolumeTexture9*>(pTexture)->GetProxyInterface();
				break;
			case D3DRTYPE_CUBETEXTURE:
				pTexture = static_cast<m_IDirect3DCubeTexture9*>(pTexture)->GetProxyInterface();
				break;
			default:
				return D3DERR_INVALIDCALL;
		}
	}
	return ProxyInterface->SetTexture(Stage, pTexture);
}

HRESULT m_IDirect3DDevice9Ex::CreatePixelShader(THIS_ CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) {
	HRESULT hr = ProxyInterface->CreatePixelShader(pFunction, ppShader);

	if(SUCCEEDED(hr) && ppShader) {
		*ppShader = new m_IDirect3DPixelShader9(*ppShader, this);

		{
			IDirect3DPixelShader9* pShader = (*ppShader);
			static std::vector<uint8_t> pbFunc;
			UINT len;
			pShader->GetFunction(nullptr, &len);
			if(pbFunc.size() < len) {
				pbFunc.resize(len + len % 4);
			}
			pShader->GetFunction(pbFunc.data(), &len);
			int cnt = 0;
			for(int i = 0; i < pbFunc.size(); i++) {
				for(int j = 0; j < pattern.size() - 1; j++) {
					if(pbFunc[i + j] == pattern[j]) {
						cnt = j;
						continue;
					}
					else {
						cnt = 0;
						break;
					}
				}
				if(cnt >= pattern.size() - 2) {
					cnt = i;
					break;
				}
			}
			int c = 0;
			if(cnt > 0) {
				c = int(pbFunc[cnt + pattern.size() - 1]);
				ps.push_back(pShader);
				//uint8_t* str = &pbFunc.data()[cnt];
				//printf("%i, %i\n", int(pbFunc[cnt + pattern.size() - 1]), int(pbFunc[cnt + pattern.size() - 2]));
			}
		}
	}
	return hr;
}

DWORD last = 0;
BOOL useing = 0;

HRESULT m_IDirect3DDevice9Ex::SetPixelShader(THIS_ IDirect3DPixelShader9* pShader) {
	auto it = std::find(ps.begin(), ps.end(), pShader);
	if(pShader) {
		pShader = static_cast<m_IDirect3DPixelShader9*>(pShader)->GetProxyInterface();
	}
	if(it != ps.end() && gFixEmissiveTransparency == 1) {
		if(!useing) {
			ProxyInterface->GetRenderState(D3DRS_ZWRITEENABLE, &last);
			ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
		}
		useing = 1;
	}
	else if(gFixEmissiveTransparency == 1) {
		if(useing) {
			useing = 0;
			ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
		}
	}
	return ProxyInterface->SetPixelShader(pShader);
}

HRESULT m_IDirect3DDevice9Ex::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {
	// from AssaultKifle47
	if(State == D3DRS_ADAPTIVETESS_X) {
		Value = 0;
	}

	IDirect3DPixelShader9* pShader = 0;
	GetPixelShader(&pShader);
	auto it = std::find(ps.begin(), ps.end(), pShader);

	//if(State == D3DRS_ZWRITEENABLE && gFixEmissiveTransparency == 1) {
	//	last = Value;
	//}

	if(State == D3DRS_ZWRITEENABLE && /*it != ps.end() &&*/ useing == 1 && gFixEmissiveTransparency == 1) {
		last = Value;
		Value = 0;
		//ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
	}
	if(State == D3DRS_ZWRITEENABLE && it != ps.end() && gFixEmissiveTransparency == 2) {
		Value = 0;
		//ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
	}
	//if(State == D3DRS_ZWRITEENABLE && it != ps.end() && gFixEmissiveTransparency == 3) {
	//	Value = 0;
	//}
	//if(State == D3DRS_ZWRITEENABLE && it != ps.end() && gFixEmissiveTransparency == 4) {
	//	ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
	//}

	return ProxyInterface->SetRenderState(State, Value);
}


HRESULT m_IDirect3DDevice9Ex::DrawIndexedPrimitive(THIS_ D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {
	//if(gFixEmissiveTransparency == 1) {
	//	IDirect3DPixelShader9* pShader = 0;
	//	GetPixelShader(&pShader);
	//	auto it = std::find(ps.begin(), ps.end(), pShader);
	//	if(it != ps.end() && gFixEmissiveTransparency == 1) {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	//	}
	//	else {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
	//	}
	//}
	return ProxyInterface->DrawIndexedPrimitive(Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT m_IDirect3DDevice9Ex::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
	//if(gFixEmissiveTransparency == 1) {
	//	IDirect3DPixelShader9* pShader = 0;
	//	GetPixelShader(&pShader);
	//	auto it = std::find(ps.begin(), ps.end(), pShader);
	//	if(it != ps.end() && gFixEmissiveTransparency == 1) {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	//	}
	//	else {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
	//	}
	//}
	return ProxyInterface->DrawIndexedPrimitiveUP(PrimitiveType, MinIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT m_IDirect3DDevice9Ex::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
	//if(gFixEmissiveTransparency == 1) {
	//	IDirect3DPixelShader9* pShader = 0;
	//	GetPixelShader(&pShader);
	//	auto it = std::find(ps.begin(), ps.end(), pShader);
	//	if(it != ps.end() && gFixEmissiveTransparency == 1) {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	//	}
	//	else {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
	//	}
	//}
	return ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
}

HRESULT m_IDirect3DDevice9Ex::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
	//if(gFixEmissiveTransparency == 1) {
	//	IDirect3DPixelShader9* pShader = 0;
	//	GetPixelShader(&pShader);
	//	auto it = std::find(ps.begin(), ps.end(), pShader);
	//	if(it != ps.end() && gFixEmissiveTransparency == 1) {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	//	}
	//	else {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
	//	}
	//}
	return ProxyInterface->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT m_IDirect3DDevice9Ex::DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
	//if(gFixEmissiveTransparency == 1) {
	//	IDirect3DPixelShader9* pShader = 0;
	//	GetPixelShader(&pShader);
	//	auto it = std::find(ps.begin(), ps.end(), pShader);
	//	if(it != ps.end() && gFixEmissiveTransparency == 1) {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	//	}
	//	else {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
	//	}
	//}
	return ProxyInterface->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
}

HRESULT m_IDirect3DDevice9Ex::DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
	//if(gFixEmissiveTransparency == 1) {
	//	IDirect3DPixelShader9* pShader = 0;
	//	GetPixelShader(&pShader);
	//	auto it = std::find(ps.begin(), ps.end(), pShader);
	//	if(it != ps.end() && gFixEmissiveTransparency == 1) {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
	//	}
	//	else {
	//		ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
	//	}
	//}
	return ProxyInterface->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
}







