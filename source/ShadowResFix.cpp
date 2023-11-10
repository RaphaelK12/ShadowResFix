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
#include "Utils.h"

// cascaded shadows resolution
extern BOOL gFixCascadedShadowMapResolution;
UINT oldCascadesWidth = 0;
UINT oldCascadesHeight = 0;
UINT newCascadesWidthX2 = 0;
UINT newCascadesHeightX2 = 0;

// rain drops
extern IDirect3DTexture9* pRainDropsRefractionHDRTex;
extern bool gFixRainDrops;
extern UINT gWindowWidth; // see dllmain.cpp-> CreateDevice/Reset
extern UINT gWindowHeight;
extern UINT gWindowDivisor;

// Near Far Plane
extern bool gNearFarPlane;
extern float NearFarPlane[4];
static bool bNFfound = false;

// Tre eLeaves Wind Sway
extern bool gTreeLeavesSwayInTheWind;
float WindSwayValue[4] = { 0 };	// time saved values ​​for wind sway
float vec[4] = { 0.f };	// temp vec4, util for debug, pConstantData does not show values

// write to depth buffer state
extern UINT gFixEmissiveTransparency;
DWORD last = 0;
bool useing = 0;

// misc
extern UINT gReflectionResMult;
extern UINT gLightResMult;
extern float shaderReturnColor[4];
extern BOOL gFixWashedMirror;

extern uint8_t* baseAddress;

float DEPTHBIAS = 0.f;
float SLOPESCALEDEPTHBIAS = 0.f;

struct stTextue {
    UINT Cnt;
    UINT Width;
    UINT Height;
    UINT Levels;
    DWORD Usage;
    D3DFORMAT Format;
    D3DPOOL Pool;
    IDirect3DTexture9* pTexture;
    stTextue(UINT _Cnt, UINT _Width, UINT _Height, UINT _Levels, DWORD _Usage,
             D3DFORMAT _Format, D3DPOOL _Pool, IDirect3DTexture9* _pTexture) :
        Cnt(_Cnt),
        Width(_Width),
        Height(_Height),
        Levels(_Levels),
        Usage(_Usage),
        Format(_Format),
        Pool(_Pool),
        pTexture(_pTexture) {
    }
};

// found game shaders with signatures
std::vector<m_IDirect3DPixelShader9*> ps;
// found game shaders with signatures
std::vector<m_IDirect3DVertexShader9*> vs;

// any shader without signature, and any edited shader
std::vector<m_IDirect3DPixelShader9*> ps_2;
// any shader without signature, and any edited shader
std::vector<m_IDirect3DVertexShader9*> vs_2;

// list to disable depth write
std::vector<IDirect3DPixelShader9*> ps2;
//extern int getSignature(IDirect3DPixelShader9* pShader, std::vector<uint8_t>& pattern);

extern std::vector<uint8_t> patternZS;// ShaderId
extern std::vector<uint8_t> patternFF;// ShaderId
extern std::vector<uint8_t> pattern2; // depth disable

// main fxc list with a list of shaders
std::vector<std::vector<m_IDirect3DPixelShader9*>> fx_ps;
// main fxc list with a list of shaders
std::vector<std::vector<m_IDirect3DVertexShader9*>> fx_vs;

// found texture list
std::list<m_IDirect3DTexture9*> textureList;

extern std::list<std::string> shadowGen;

bool doPostFx = false;

bool isShaderInAnyList(m_IDirect3DPixelShader9* shader) {
    for(auto& f : fx_ps) {
        for(auto p : f) {
            m_IDirect3DPixelShader9* pShader = p;
            if(p == shader)
                return true;
        }
    }
    for(auto p : ps_2) {
        if(p == shader) {
            return true;
        }
    }
    return false;
}
bool isShaderInAnyList(m_IDirect3DVertexShader9* shader) {
    for(auto& f : fx_vs) {
        for(auto p : f) {
            m_IDirect3DVertexShader9* pShader = p;
            if(p == shader)
                return true;
        }
    }
    for(auto p : vs_2) {
        if(p == shader) {
            return true;
        }
    }
    return false;
}

HRESULT m_IDirect3DDevice9Ex::CreateTexture(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) {
    bool addToList = false;
    std::string name = "nonamed";
    if(true) {
        if((
            Format == D3DFMT_A16B16G16R16F ||
            Format == D3DFMT_A32B32G32R32F
            ) && Width >= 1 && Height >= 1) {
            addToList = true;
            name = "Any rgba float";
        }

        if((

            Format == D3DFMT_A16B16G16R16 ||
            Format == D3DFMT_A2B10G10R10 ||
            Format == D3DFMT_A2B10G10R10_XR_BIAS ||
            Format == D3DFMT_A2R10G10B10 ||
            Format == D3DFMT_A2W10V10U10 ||
            Format == D3DFMT_A1R5G5B5 ||
            Format == D3DFMT_A4R4G4B4 ||
            Format == D3DFMT_A8B8G8R8 ||
            Format == D3DFMT_A8R3G3B2 ||
            Format == D3DFMT_A8R8G8B8 ||
            Format == D3DFMT_Q16W16V16U16 ||
            Format == D3DFMT_Q8W8V8U8 ||
            Format == D3DFMT_MULTI2_ARGB8
            ) && Width >= 1 && Height >= 1) {
            addToList = true;
            name = "Any rgba";
        }

        if((
            Format == D3DFMT_X1R5G5B5 ||
            Format == D3DFMT_X4R4G4B4 ||
            Format == D3DFMT_X8B8G8R8 ||
            Format == D3DFMT_X8L8V8U8 ||
            Format == D3DFMT_X8R8G8B8 ||
            Format == D3DFMT_R3G3B2 ||
            Format == D3DFMT_R5G6B5 ||
            Format == D3DFMT_R8G8B8
            ) && Width >= 1 && Height >= 1) {
            addToList = true;
            name = "Any rgb";
        }

        if((
            Format == D3DFMT_D15S1 ||
            Format == D3DFMT_D16 ||
            Format == D3DFMT_D16_LOCKABLE ||
            Format == D3DFMT_D24FS8 ||
            Format == D3DFMT_D24S8 ||
            Format == D3DFMT_D24X4S4 ||
            Format == D3DFMT_D24X8 ||
            Format == D3DFMT_D32 ||
            Format == D3DFMT_D32F_LOCKABLE ||
            Format == D3DFMT_D32_LOCKABLE ||
            Format == D3DFMT_S8_LOCKABLE ||
            Format == D3DFMT_D16_LOCKABLE ||
            Format == D3DFORMAT(1515474505u) // INTZ
            ) && Width >= 1 && Height >= 1) {
            addToList = true;
            name = "Any depth stencil";
        }

        if((
            Format == D3DFMT_A4L4 ||
            Format == D3DFMT_A8L8 ||
            Format == D3DFMT_A8P8 ||
            Format == D3DFMT_G16R16 ||
            Format == D3DFMT_V16U16 ||
            Format == D3DFMT_V8U8 ||
            Format == D3DFMT_G8R8_G8B8 ||
            Format == D3DFMT_R8G8_B8G8 ||
            Format == D3DFMT_UYVY ||
            Format == D3DFMT_YUY2 ||
            Format == D3DFMT_L6V5U5 ||
            Format == D3DFMT_CxV8U8
            ) && Width >= 1 && Height >= 1) {
            addToList = true;
            name = "Any RG LA";
        }

        if((
            Format == D3DFMT_G16R16F ||
            Format == D3DFMT_G32R32F
            ) && Width >= 1 && Height >= 1) {
            addToList = true;
            name = "Any RG float";
        }

        if((
            Format == D3DFMT_R16F
            ) && Width >= 1 && Height >= 1) {
            addToList = true;
            name = "Any R float 16";
        }

        if((
            Format == D3DFMT_R32F
            ) && Width >= 1 && Height >= 1) {
            addToList = true;
            name = "Any R float 32";
        }

        if((
            Format == D3DFMT_INDEX16 ||
            Format == D3DFMT_INDEX32 ||
            Format == D3DFMT_L16 ||
            Format == D3DFMT_A1 ||
            Format == D3DFMT_A8 ||
            Format == D3DFMT_BINARYBUFFER ||
            Format == D3DFMT_L8 ||
            Format == D3DFMT_P8 ||
            Format == D3DFMT_VERTEXDATA
            ) && Width >= 1 && Height >= 1) {
            addToList = true;
            name = "Any R";
        }
    }
    static uint8_t dummyVar = 1;

    uint8_t* ReflexQualityBA0xD612BC = &dummyVar;
    uint8_t* waterQualityBA0xD612C0  = &dummyVar;
    uint8_t* ShadowQualityBA0xD612B8 = &dummyVar;
    uint8_t* NightShadowBA0xD612B4   = &dummyVar;

    int32_t GameVersion = 0;
    Utils::GetGameVersion(GameVersion);

    if(GameVersion == 1200) {
        ReflexQualityBA0xD612BC = (baseAddress + 0xD612BC);
        waterQualityBA0xD612C0 = (baseAddress + 0xD612C0);
        ShadowQualityBA0xD612B8 = (baseAddress + 0xD612B8);
        NightShadowBA0xD612B4 = (baseAddress + 0xD612B4);
    }

    int reflexresshift = *ReflexQualityBA0xD612BC > 0 ? *ReflexQualityBA0xD612BC + 2 : 0;

    if(Format == D3DFMT_A16B16G16R16F && Width == Height && Width == 16 << reflexresshift && Levels == 4 && Usage == 1) {
        addToList = true;
        name = "reflex";
    }
    if(Format == D3DFORMAT(1515474505u) && Width == Height && Width == 16 << reflexresshift && Levels == 4 && Usage == 2) {
        addToList = true;
        name = "reflex";
    }

    if(gReflectionResMult >= 1 && gReflectionResMult <= 16) {
        if(Format == D3DFMT_A16B16G16R16F && Width == Height && Width == 16 << reflexresshift && Levels == 4 && Usage == 1) {
            Width *= gReflectionResMult;
            Height *= gReflectionResMult;
        }
        if(Format == D3DFORMAT(1515474505u) && Width == Height && Width == 16 << reflexresshift && Levels == 4 && Usage == 2) {
            Width *= gReflectionResMult;
            Height *= gReflectionResMult;
        }
    }

    if(
        (Format == D3DFORMAT(1515474505u) && Width == 256 << *ShadowQualityBA0xD612B8 && Width == Height && Levels == 1) ||
        (Format == D3DFMT_R32F && Height == 256 << *ShadowQualityBA0xD612B8 && Width == Height && Levels == 1 && Usage == 1)) //512x512 = 1024x256
    {
        addToList = true;
        name = "shadow cascade";
    }
    if((Format == D3DFMT_R16F && Height == 256 << *ShadowQualityBA0xD612B8 && Width == Height * 4 && Levels == 1)) {
        addToList = true;
        name = "shadow atlas";
    }
    if((Format == D3DFMT_G16R16F || Format == D3DFMT_G32R32F) &&
       ((Height == (64 * 4) << *ShadowQualityBA0xD612B8 && (Width == Height)))) {
        addToList = true;
        name = "light atlas";
        Width *= gLightResMult;
        Height *= gLightResMult;
    }
    if((Format == D3DFMT_G16R16F || Format == D3DFMT_G32R32F || Format == D3DFORMAT(1515474505u)) &&
       (Height == 64 << *ShadowQualityBA0xD612B8 && (Width == Height || Width == Height * 4))) {
        addToList = true;
        name = "light atlas";
        Width *= gLightResMult;
        Height *= gLightResMult;
    }

    if((Format == D3DFMT_A16B16G16R16F || Format == D3DFORMAT(1515474505u)) && Width == 128 << *waterQualityBA0xD612C0 && Height == Width && Levels == 1) {
        addToList = true;
        name = "wate reflex";
    }

    if(gFixCascadedShadowMapResolution) {
        if((Format == D3DFMT_R16F && Height >= 256 && Width == Height * 4 && Levels == 1)) {
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

    if((Width >= gWindowWidth - 100 && Width <= gWindowWidth + 100) && (Height >= gWindowHeight - 100 && Height <= gWindowHeight + 100)) {
        addToList = true;
        name = "Screen Size2";
    }
    if(Width == gWindowWidth && Height == gWindowHeight) {
        addToList = true;
        name = "Screen Size";
    }
    if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth && Height == gWindowHeight) {
        addToList = true;
        name = "Screen";
    }
    if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth / 2 && Height == gWindowHeight / 2) {
        addToList = true;
        name = "Screen / 2";
    }
    if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth / 4 && Height == gWindowHeight / 4) {
        addToList = true;
        name = "Screen / 4";
    }
    if((Format == D3DFMT_A2B10G10R10 || Format == D3DFMT_A2R10G10B10 || Format == D3DFMT_A2B10G10R10_XR_BIAS || Format == D3DFMT_A2W10V10U10)) {
        addToList = true;
        name = "gBuffer 2?";
    }
    if((Format == D3DFMT_A2B10G10R10 || Format == D3DFMT_A2R10G10B10 || Format == D3DFMT_A2B10G10R10_XR_BIAS || Format == D3DFMT_A2W10V10U10) && Width == gWindowWidth && Height == gWindowHeight) {
        addToList = true;
        name = "gBuffer";
    }
    int mirrorresshift = *ReflexQualityBA0xD612BC > 0 ? *ReflexQualityBA0xD612BC - 1 : 0;
    if(Format == D3DFMT_A2R10G10B10 && Width == Height && Height == 256 << mirrorresshift) {
        addToList = true;
        name = "Mirror Tex";
        if(gFixWashedMirror)
            Format = D3DFMT_A16B16G16R16F;
    }
    if(Format == D3DFORMAT(1515474505u) && Width == Height && Height == 256 << mirrorresshift) {
        addToList = true;
        name = "Mirror Tex";
    }

    if(Format == D3DFMT_L8 && Width == 32 && Height == 512) {
        addToList = false;
    }

    HRESULT hr = ProxyInterface->CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);

    if(SUCCEEDED(hr) && ppTexture) {
        m_IDirect3DTexture9* tex = nullptr;
        tex = new m_IDirect3DTexture9(*ppTexture, this, Width, Height, Levels, Usage, Format, Pool, name, addToList);
        *ppTexture = tex;
    }

    //RainDropsFix from AssaultKifle47
    if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth / gWindowDivisor && Height == gWindowHeight / gWindowDivisor && ppTexture != 0 && (*ppTexture) != 0) {
        pRainDropsRefractionHDRTex = (*ppTexture);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetPixelShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {
    IDirect3DPixelShader9* pShader = 0;
    m_IDirect3DPixelShader9* pshader = 0;
    if(!ProxyInterface) {
        return S_FALSE;
    }
    GetPixelShader(&pShader);
    HRESULT hr = 0;
    if(pShader) {
        pshader = static_cast<m_IDirect3DPixelShader9*>(pShader);
        for(int i = 0; i < (int) Vector4fCount * 4; i++) {
            if(pConstantData[i] != 0.f)
                pshader->constants[StartRegister][i] = pConstantData[i];
        }
        //for(int i = 0; i < Vector4fCount; i++) {
        //	for(int j = 0; j < 4; j++) {
        //		pshader->constants[StartRegister][i * j] = pConstantData[i * j];
        //	}
        //}
    }
    if(gFixCascadedShadowMapResolution) {
        if(StartRegister == 53 && Vector4fCount == 1 && oldCascadesWidth != 0 && oldCascadesHeight != 0 &&
           pConstantData[0] == 1.f / oldCascadesWidth && pConstantData[1] == 1.f / oldCascadesHeight) {
            float vec[4] = { 0.f };
            vec[0] = 1.f / newCascadesWidthX2;
            vec[1] = 1.f / newCascadesHeightX2;
            vec[2] = 1.f / newCascadesHeightX2;
            vec[3] = pConstantData[3];
            // set texture pixel size to pixel shader
            hr = ProxyInterface->SetPixelShaderConstantF(StartRegister, vec, Vector4fCount);
            if(pshader && pshader->disable) {
                ProxyInterface->SetPixelShaderConstantF(220, shaderReturnColor, 1);
            }
            return hr;
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
    if(bNFfound && gNearFarPlane) {
        ProxyInterface->SetPixelShaderConstantF(128, NearFarPlane, 1);
    }
    if(bNFfound && gNearFarPlane && StartRegister == 128 && Vector4fCount == 1)
        hr = ProxyInterface->SetPixelShaderConstantF(StartRegister, NearFarPlane, Vector4fCount);
    else
        hr = ProxyInterface->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
    if(pshader && pshader->disable) {
        ProxyInterface->SetPixelShaderConstantF(220, shaderReturnColor, 1);
    }
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetVertexShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {
    IDirect3DVertexShader9* pShader = 0;
    GetVertexShader(&pShader);
    if(pShader) {
        m_IDirect3DVertexShader9* pshader = static_cast<m_IDirect3DVertexShader9*>(pShader);
        for(int i = 0; i < (int) Vector4fCount * 4; i++) {
            pshader->constants[StartRegister][i] = pConstantData[i];
        }
    }
    if(gTreeLeavesSwayInTheWind) {
        // Force leaves to wind sway
        if(StartRegister == 51 && Vector4fCount == 1) {
            vec[0] = pConstantData[0];
            vec[1] = pConstantData[1];
            vec[2] = pConstantData[2];
            vec[3] = pConstantData[3];
            if(pConstantData[3] > 7.f) {	// the maximum value found that isn't time is 7,
                // so if time is a value that increases,
                // any value above 7 is the time I'm looking for.
                WindSwayValue[0] = vec[0];
                WindSwayValue[1] = vec[1];
                WindSwayValue[2] = vec[2];
                WindSwayValue[3] = vec[3];
            }
            if(pConstantData[0] == 1.f && pConstantData[1] == 1.f && pConstantData[2] == 1.f && pConstantData[3] == 1.f
               //&& ((unsigned int) (void*) pConstantData) == (0x103f2c0u) // only works with rgless
               ) {
                vec[0] = WindSwayValue[0];	// like sin/cos/frac (game time in seconds)
                vec[1] = WindSwayValue[1];	// like sin/cos/frac (game time in seconds)
                vec[2] = WindSwayValue[2];	// like sin/cos/frac (game time in seconds)
                vec[3] = WindSwayValue[3];	// game time in seconds
            }
            return ProxyInterface->SetVertexShaderConstantF(StartRegister, vec, Vector4fCount);
        }
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
        ProxyInterface->SetVertexShaderConstantF(128, NearFarPlane, 1);
    }
    if(bNFfound && gNearFarPlane && StartRegister == 128 && Vector4fCount == 1)
        hr = ProxyInterface->SetVertexShaderConstantF(StartRegister, NearFarPlane, Vector4fCount);
    else
        hr = ProxyInterface->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) {
    // fix from AssaultKifle47
    if(gFixRainDrops && Stage == 1 && pTexture == 0 && pRainDropsRefractionHDRTex) {
        pTexture = pRainDropsRefractionHDRTex;
    }
    if(pTexture) {
        m_IDirect3DTexture9* pTexture2 = static_cast<m_IDirect3DTexture9*>(pTexture);
        if(pTexture2)
            pTexture2->useCounter++;
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
        *ppShader = new m_IDirect3DPixelShader9(*ppShader, this, SC_FXC);
        //IDirect3DPixelShader9* pShader = (*ppShader);
        //{
        //    IDirect3DPixelShader9* pShader = (*ppShader);
        //    static std::vector<uint8_t> pbFunc;
        //    UINT len;
        //    pShader->GetFunction(nullptr, &len);
        //    if(pbFunc.size() < len) {
        //        pbFunc.resize(len + len % 4);
        //    }
        //    pShader->GetFunction(pbFunc.data(), &len);
        //    int cnt = 0;
        //    for(int i = 0; i < (int) pbFunc.size(); i++) {
        //        for(int j = 0; j < (int) pattern2.size() - 1; j++) {
        //            if(pbFunc[i + j] == pattern2[j]) {
        //                cnt = j;
        //                continue;
        //            }
        //            else {
        //                cnt = 0;
        //                break;
        //            }
        //        }
        //        if(cnt >= (int) pattern2.size() - 2) {
        //            cnt = i;
        //            break;
        //        }
        //    }
        //    int c = 0;
        //    if(cnt > 0) {
        //        c = *((int*) &pbFunc[cnt + pattern2.size() - 1]);
        //        ps2.push_back(pShader);
        //    }
        //}
    }
    //if(!ppShader || !(*ppShader)) {
    //    printf("!");
    //    return hr;
    //}
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreatePixelShader2(THIS_ CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader, ShaderCreationMode extra) {
    HRESULT hr = ProxyInterface->CreatePixelShader(pFunction, ppShader);

    if(SUCCEEDED(hr) && ppShader) {
        *ppShader = new m_IDirect3DPixelShader9(*ppShader, this, extra);
        //IDirect3DPixelShader9* pShader = (*ppShader);
        //{
        //    IDirect3DPixelShader9* pShader = (*ppShader);
        //    static std::vector<uint8_t> pbFunc;
        //    UINT len;
        //    pShader->GetFunction(nullptr, &len);
        //    if(pbFunc.size() < len) {
        //        pbFunc.resize(len + len % 4);
        //    }
        //    pShader->GetFunction(pbFunc.data(), &len);
        //    int cnt = 0;
        //    for(int i = 0; i < (int) pbFunc.size(); i++) {
        //        for(int j = 0; j < (int) pattern2.size() - 1; j++) {
        //            if(pbFunc[i + j] == pattern2[j]) {
        //                cnt = j;
        //                continue;
        //            }
        //            else {
        //                cnt = 0;
        //                break;
        //            }
        //        }
        //        if(cnt >= (int) pattern2.size() - 2) {
        //            cnt = i;
        //            break;
        //        }
        //    }
        //    int c = 0;
        //    if(cnt > 0) {
        //        c = *((int*) &pbFunc[cnt + pattern2.size() - 1]);
        //        ps2.push_back(pShader);
        //    }
        //}
    }
    //if(!ppShader || !(*ppShader)) {
    //    printf("!");
    //    return hr;
    //}
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateVertexShader(THIS_ CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader) {
    HRESULT hr = ProxyInterface->CreateVertexShader(pFunction, ppShader);

    if(SUCCEEDED(hr) && ppShader) {
        *ppShader = new m_IDirect3DVertexShader9(*ppShader, this, SC_FXC);
    }
    //if(!ppShader || !(*ppShader)) {
    //    printf("!");
    //    return hr;
    //}

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateVertexShader2(THIS_ CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader, ShaderCreationMode extra) {
    HRESULT hr = ProxyInterface->CreateVertexShader(pFunction, ppShader);

    if(SUCCEEDED(hr) && ppShader) {
        *ppShader = new m_IDirect3DVertexShader9(*ppShader, this, extra);
    }
    //if(!ppShader || !(*ppShader)) {
    //    printf("!");
    //    return hr;
    //}

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetVertexShader(THIS_ IDirect3DVertexShader9* pShader) {
    m_IDirect3DVertexShader9* pShader2 = nullptr;
    if(pShader) {
        pShader2 = static_cast<m_IDirect3DVertexShader9*>(pShader);
        if(pShader2) {
            pShader2->used++;
            if(pShader2->disable && pShader2->dummyShader) {
                pShader = static_cast<m_IDirect3DVertexShader9*>(pShader2->dummyShader)->GetProxyInterface();
            }
            else if(pShader2->usingShader != SU_FXC && pShader2->compiledShaders[pShader2->usingShader]) {
                pShader = static_cast<m_IDirect3DVertexShader9*>(pShader2->compiledShaders[pShader2->usingShader])->GetProxyInterface();
            }
            else {
                pShader = static_cast<m_IDirect3DVertexShader9*>(pShader)->GetProxyInterface();
            }
        }
        else {
            pShader = static_cast<m_IDirect3DVertexShader9*>(pShader)->GetProxyInterface();
        }
    }
    HRESULT hr = ProxyInterface->SetVertexShader(pShader);

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetPixelShader(THIS_ IDirect3DPixelShader9* pShader) {
    m_IDirect3DPixelShader9* pShader2 = nullptr;
    if(pShader) {
        pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
        if(pShader2) {
            pShader2->used++;
            if(pShader2->disable && pShader2->dummyShader) {
                pShader = static_cast<m_IDirect3DPixelShader9*>(pShader2->dummyShader)->GetProxyInterface();
            }
            else if(pShader2->usingShader != SU_FXC && pShader2->compiledShaders[pShader2->usingShader]) {
                pShader = static_cast<m_IDirect3DPixelShader9*>(pShader2->compiledShaders[pShader2->usingShader])->GetProxyInterface();
            }
            else {
                pShader = pShader2->GetProxyInterface();
            }
            if(pShader2->id >= 814 && pShader2->id <= 831) {
                doPostFx = true;
            }
        }
        else {
            pShader = static_cast<m_IDirect3DPixelShader9*>(pShader)->GetProxyInterface();
        }
    }

    HRESULT hr = ProxyInterface->SetPixelShader(pShader);

    if(pShader) {
        if(pShader2) {
            if(pShader2->overwriteDepth) {
                ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, pShader2->depthWrite);
            }
            else {
                ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
            }
            GetPixelShaderConstantF(0, &pShader2->constants[0][0], 233);
        }
        else {
            ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
        }
    }
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {
    HRESULT hr = 0;

    // fix from AssaultKifle47
    if(State == D3DRS_ADAPTIVETESS_X) {
        Value = 0;
    }

    IDirect3DPixelShader9* pShader = 0;
    m_IDirect3DPixelShader9* pShader2 = 0;
    GetPixelShader(&pShader);
    pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
    
    if(State == D3DRS_ZWRITEENABLE) {
        last = Value;
    }

    hr = ProxyInterface->SetRenderState(State, Value);
    if(State == D3DRS_ZWRITEENABLE && pShader2) {
        if(pShader2->overwriteDepth) {
            if(pShader2->depthWrite) {
                ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 1);
            }
            else {
                ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
            }
        }
    }
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::DrawIndexedPrimitive(THIS_ D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {
    return ProxyInterface->DrawIndexedPrimitive(Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT m_IDirect3DDevice9Ex::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    return ProxyInterface->DrawIndexedPrimitiveUP(PrimitiveType, MinIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT m_IDirect3DDevice9Ex::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
    //IDirect3DPixelShader9* pShader = 0;
    //m_IDirect3DPixelShader9* pShader2 = 0;
    //if(doPostFx) {
    //    GetPixelShader(&pShader);
    //    if(pShader) {
    //        pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
    //        if(pShader2) {
    //            if(pShader2->id >= 814 && pShader2->id <= 831) {
    //                doPostFx = false;
    //            }
    //        }
    //    }
    //}
    return ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
}

HRESULT m_IDirect3DDevice9Ex::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    return ProxyInterface->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT m_IDirect3DDevice9Ex::DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    return ProxyInterface->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
}

HRESULT m_IDirect3DDevice9Ex::DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    return ProxyInterface->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
}
