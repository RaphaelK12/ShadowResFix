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
extern BOOL gFixCascadedShadowMapResolution;
extern BOOL gFixRainDrops;

struct stTextue {

};

struct stPixelShader {

};

struct stVertexShader {

};

struct stSurface {

};


std::vector<stTextue*> textureList;
std::vector<stPixelShader*> psList;
std::vector<stVertexShader*> vsList;
std::vector<stSurface*> surfaceList;

HRESULT m_IDirect3DDevice9Ex::CreateTexture(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) {
	if(
		(Format == D3DFORMAT(1515474505u) && Width >= 256 && Width == Height && Levels == 1) //512x512 = 1024x256
		) {
	}
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

	//if(gFixRainDrops)
	if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth / gWindowDivisor && Height == gWindowHeight / gWindowDivisor && ppTexture != 0 && (*ppTexture) != 0) {
		pHDRTexQuarter = (*ppTexture);
	}

	return hr;
}

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
	return ProxyInterface->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
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
	return ProxyInterface->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}

HRESULT m_IDirect3DDevice9Ex::SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) {
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
