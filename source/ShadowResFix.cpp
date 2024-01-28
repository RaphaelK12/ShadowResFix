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
#include "D3DX9Mesh.h"
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

extern m_IDirect3DTexture9* NormalTex;
extern m_IDirect3DTexture9* DiffuseTex;
extern m_IDirect3DTexture9* SpecularTex;
extern m_IDirect3DTexture9* DepthTex;
extern m_IDirect3DTexture9* StencilTex;
extern m_IDirect3DTexture9* BloomTex;

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

bool DisableADAPTIVETESS_X = 1;
bool EnableDepthOverwrite = 1;
bool UseSMAA = 1;
bool UseMSAA = 0;
bool UseSSAA = 1;
bool UseSSAO = 1;
bool UseBokeh = 0;
bool UseMBlur = 0;

float ResSSAA = 1;
float AoDistance = 100;
float FocusPoint[4] = { 350, 0, 0, 0 };
float FocusScale[4] = { 300, 0, 0, 0 };


extern uint8_t* baseAddress;

float DEPTHBIAS = 0.f;
float SLOPESCALEDEPTHBIAS = 0.f;

bool doPostFx = false;
extern bool fixCoronaDepth;
extern int UsePostFx[4]; // postfx type
extern int UseDebugTextures ;


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

m_IDirect3DTexture9* rainDepth = nullptr;

// postfx textures
IDirect3DTexture9* pHDRTex =  nullptr; // game hdr texture
IDirect3DTexture9* pHDRTex2 =  nullptr; // main temp texture

 // smaa textures
IDirect3DTexture9* areaTex =  nullptr; // loaded from file
IDirect3DTexture9* searchTex =  nullptr; // loaded from file
IDirect3DTexture9* edgesTex =  nullptr; // gen
IDirect3DTexture9* blendTex =  nullptr; // gen

// old, used to add bias into shadows
extern std::list<std::string> shadowGen;


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
bool isShaderPostFx(m_IDirect3DPixelShader9* shader) {
    if(shader) {
        switch(shader->id) {
            case 815:
            case 817:
            case 819:
            case 821:
            case 827:
            case 829:
            case 831:
            {
                return true;
                break;
            }
            default:
            {
                return false;
                break;
            }
        }
    }
    return false;
}

IDirect3DPixelShader9* PostFxPS = nullptr;

IDirect3DPixelShader9* SMAA_EdgeDetection = nullptr;
IDirect3DPixelShader9* SMAA_BlendingWeightsCalculation = nullptr;
IDirect3DPixelShader9* SMAA_NeighborhoodBlending = nullptr;

IDirect3DVertexShader9* SMAA_EdgeDetectionVS = nullptr;
IDirect3DVertexShader9* SMAA_BlendingWeightsCalculationVS = nullptr;
IDirect3DVertexShader9* SMAA_NeighborhoodBlendingVS = nullptr;

IDirect3DPixelShader9* DOF_ps = nullptr;

IDirect3DPixelShader9* SSAO_ps = nullptr;
IDirect3DPixelShader9* SSAO_ps2 = nullptr;
IDirect3DVertexShader9* SSAO_vs = nullptr;


std::vector<m_IDirect3DPixelShader9*> PostFxList;

extern bool afterpostfx;
IDirect3DTexture9* mainDepthTex =  nullptr; // gen
IDirect3DTexture9* oldDepthTex =  nullptr; // gen
IDirect3DSurface9* mainDepth =  nullptr; // gen
IDirect3DSurface9* oldDepth =  nullptr; // gen

IDirect3DTexture9* aoHalfTex = 0;
IDirect3DTexture9* halfDepthDsTex = 0;

IDirect3DPixelShader9* MSAA_CustomResolve_ps = nullptr;
IDirect3DPixelShader9* MSAA_Render_ps = nullptr;



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
    uint8_t* waterQualityBA0xD612C0 = &dummyVar;
    uint8_t* ShadowQualityBA0xD612B8 = &dummyVar;
    uint8_t* NightShadowBA0xD612B4 = &dummyVar;

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
        //Width = gWindowWidth; 
        //Height = gWindowHeight;
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


    static int cc = 0;
    if(UseSSAA && Width == gWindowWidth && Height == gWindowHeight) {
        if(ResSSAA < 1.f &&
           (Format == D3DFMT_D24FS8 ||
            Format == D3DFMT_D24S8 ||
            Format == D3DFMT_D24X8 ||
            Format == D3DFMT_D32 ||
            Format == D3DFORMAT(1515474505u) // INTZ
            )
           ) {
            //if(mainDepthTex == nullptr) {
                 //ProxyInterface->CreateTexture(Width, Height, Levels, Usage, Format, Pool, &mainDepthTex, pSharedHandle);
                 //std::string nm = "Screen";
                 //mainDepthTex = new m_IDirect3DTexture9(mainDepthTex, this, Width, Height, Levels, Usage, Format, Pool, nm, true);
                 //mainDepthTex->GetSurfaceLevel(0, &mainDepth);
            //}

            if(cc == 0) {
                Width *= ResSSAA;
                Height *= ResSSAA;
                cc++;
            }
            else {
                Width *= ResSSAA;
                Height *= ResSSAA;
            }
        }
        else {
            Width *= ResSSAA;
            Height *= ResSSAA;
        }
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
    if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth / 4 && Height == gWindowHeight / 4) {
        BloomTex = static_cast<m_IDirect3DTexture9*>(*ppTexture);
    }
    if(Format == D3DFMT_A16B16G16R16F && ppTexture && Width == (gWindowWidth * (UseSSAA ? ResSSAA : 1)) && Height == (gWindowHeight * (UseSSAA ? ResSSAA : 1))) {
        pHDRTex = (*ppTexture);

        // Release before, just to be sure
        SAFE_RELEASE(pHDRTex2);
        SAFE_RELEASE(areaTex);
        SAFE_RELEASE(searchTex);
        SAFE_RELEASE(edgesTex);
        SAFE_RELEASE(blendTex);

        SAFE_RELEASE(aoHalfTex);
        SAFE_RELEASE(halfDepthDsTex);

        // create new texture to postfx
        HRESULT hr = ProxyInterface->CreateTexture(Width, Height, Levels, Usage, Format, Pool, &pHDRTex2, 0);
        if(SUCCEEDED(hr) && pHDRTex2) {
            m_IDirect3DTexture9* tex = nullptr;

            tex = new m_IDirect3DTexture9(pHDRTex2, this, Width, Height, Levels, Usage, Format, Pool, name, addToList);
            pHDRTex2 = tex;
            tex->name = "Screen";

            hr = D3DXCreateTextureFromFileExA(this, "update/shaders/AreaTex.png", 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &areaTex);
            if(hr != S_OK) {
                hr = D3DXCreateTextureFromFileExA(this, "shaders/AreaTex.png", 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &areaTex);
                if(hr != S_OK) {
                    hr = D3DXCreateTextureFromFileExA(this, "AreaTex.png", 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &areaTex);
                }
            }
            hr = D3DXCreateTextureFromFileExA(this, "update/shaders/SearchTex.png", 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &searchTex);
            if(hr != S_OK) {
                hr = D3DXCreateTextureFromFileExA(this, "shaders/SearchTex.png", 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &searchTex);
                if(hr != S_OK) {
                    hr = D3DXCreateTextureFromFileExA(this, "SearchTex.png", 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &searchTex);
                }
            }
            if(searchTex) {
                ((m_IDirect3DTexture9*) searchTex)->name = "MainPostfx";
            }
            if(areaTex) { 
                ((m_IDirect3DTexture9*) areaTex)->name = "MainPostfx";
            }
            if(hr == S_OK) {
                hr = CreateTexture(Width, Height, Levels, Usage, D3DFMT_X8R8G8B8, Pool, &edgesTex, 0);
                if(hr == S_OK && edgesTex) {
                    ((m_IDirect3DTexture9*) edgesTex)->name = "MainPostfx";
                }
                hr = CreateTexture(Width, Height, Levels, Usage, D3DFMT_A8R8G8B8, Pool, &blendTex, 0);
                if(hr == S_OK && blendTex) {
                    ((m_IDirect3DTexture9*) blendTex)->name = "MainPostfx";
                }
                hr = CreateTexture(Width / 2, Height / 2, Levels, Usage, D3DFMT_X8R8G8B8, Pool, &aoHalfTex, 0);
                if(hr == S_OK && aoHalfTex) {
                    ((m_IDirect3DTexture9*) aoHalfTex)->name = "MainPostfx";
                }
                hr = CreateTexture(Width / 2, Height / 2, Levels, Usage, D3DFMT_R16F, Pool, &halfDepthDsTex, 0);
                if(hr == S_OK && halfDepthDsTex) {
                    ((m_IDirect3DTexture9*) halfDepthDsTex)->name = "MainPostfx";
                }
            }
        }
    }
    if(hr != S_OK) {
        printf("as");
    }
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetPixelShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {
    IDirect3DPixelShader9* pShader = 0;
    m_IDirect3DPixelShader9* pshader = 0;
    float vec[4] = { 0.f };

    GetPixelShader(&pShader);
    HRESULT hr = 0;
    if(pShader) {
        pshader = static_cast<m_IDirect3DPixelShader9*>(pShader);
        for(int i = 0; i < (int) Vector4fCount * 4; i++) {
            if(pConstantData[i] != 0.f)
                m_IDirect3DPixelShader9::globalConstants[StartRegister][i] = pConstantData[i];
        }
        memcpy(pshader->constants[0], m_IDirect3DPixelShader9::globalConstants[0],4*4*250);

        //for(int i = 0; i < Vector4fCount; i++) {
        //	for(int j = 0; j < 4; j++) {
        //		pshader->constants[StartRegister][i * j] = pConstantData[i * j];
        //	}
        //}
    }

    if(!ProxyInterface) {
        return S_FALSE;
    }
    if(UseSSAA && pConstantData[0] == 1.f / gWindowWidth && pConstantData[1] == 1.f / gWindowHeight && Vector4fCount==1) {
        vec[0] = pConstantData[0] * (1.f/ ResSSAA);
        vec[1] = pConstantData[1] * (1.f / ResSSAA);
        vec[2] = pConstantData[2] * ResSSAA;
        vec[3] = pConstantData[3] * ResSSAA;
        return ProxyInterface->SetPixelShaderConstantF(StartRegister, vec, Vector4fCount);
    }
    if(UseSSAA && pConstantData[0] == gWindowWidth && pConstantData[1] == gWindowHeight && Vector4fCount == 1) {
        vec[0] = pConstantData[0] * ResSSAA;
        vec[1] = pConstantData[1] * ResSSAA;
        vec[2] = pConstantData[2] * (1.f / ResSSAA);
        vec[3] = pConstantData[3] * (1.f / ResSSAA);
        return ProxyInterface->SetPixelShaderConstantF(StartRegister, vec, Vector4fCount);
    }
    if(gFixCascadedShadowMapResolution) {
        if(StartRegister == 53 && Vector4fCount == 1 && oldCascadesWidth != 0 && oldCascadesHeight != 0 &&
           pConstantData[0] == 1.f / oldCascadesWidth && pConstantData[1] == 1.f / oldCascadesHeight) {
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
        if(pshader) {
            //for(int i = 0; i < (int) Vector4fCount * 4; i++) {
            //    pshader->constants[StartRegister][i] = pConstantData[i];
            //}

            for(int i = 0; i < (int) Vector4fCount * 4; i++) {
                if(pConstantData[i] != 0.f)
                    m_IDirect3DVertexShader9::globalConstants[StartRegister][i] = pConstantData[i];
            }
            memcpy(pshader->constants[0], m_IDirect3DVertexShader9::globalConstants[0], 4 * 4 * 250);

            //if(pshader->id == 15 &&gbuffertexturesampler3) {
            //    SetTexture(0, gbuffertexturesampler3);
            //}
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
    if(0 && Stage == 0 && pTexture) {
        IDirect3DVertexShader9* pShader = nullptr;
        m_IDirect3DVertexShader9* pShader2 = nullptr;
        GetVertexShader(&pShader);
        if(pShader) {
            pShader2 = static_cast<m_IDirect3DVertexShader9*>(pShader);
            if(pShader2) {
            }
        }
    }
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
        if(pTexture2 && pTexture2->Width == gWindowWidth /*&& pTexture2->Format == D3DFMT_A2B10G10R10*/) {
            IDirect3DPixelShader9* pShader = nullptr;
            m_IDirect3DPixelShader9* pShader2 = nullptr;
            GetPixelShader(&pShader);
            if(pShader) {
                pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
                if(pShader2) {
                    // diffuse 0
                    if((Stage == 0 && pShader2->id == 5 + 1 ) ||
                       (Stage == 0 && pShader2->id == 5 + 2 ) ||
                       (Stage == 0 && pShader2->id == 5 + 3 ) ||
                       (Stage == 0 && pShader2->id == 5 + 4 ) ||
                       (Stage == 0 && pShader2->id == 5 + 6 ) ||
                       (Stage == 0 && pShader2->id == 5 + 7 ) ||
                       (Stage == 0 && pShader2->id == 5 + 8 ) ||
                       (Stage == 0 && pShader2->id == 5 + 9 ) ||
                       (Stage == 0 && pShader2->id == 5 + 10) ||
                       (Stage == 0 && pShader2->id == 5 + 11) ||
                       (Stage == 0 && pShader2->id == 5 + 12) ||
                       (Stage == 0 && pShader2->id == 5 + 14) ||
                       (Stage == 0 && pShader2->id == 5 + 15) ||
                       (Stage == 1 && pShader2->id == 5 + 16) ||
                       (Stage == 1 && pShader2->id == 5 + 17) ||
                       (Stage == 0 && pShader2->id == 5 + 31)) {
                        DiffuseTex = pTexture2;
                    }
                    // normal 1
                    if((Stage == 1 && pShader2->id == 5 + 1 ) ||
                       (Stage == 1 && pShader2->id == 5 + 2 ) ||
                       (Stage == 1 && pShader2->id == 5 + 3 ) ||
                       (Stage == 1 && pShader2->id == 5 + 4 ) ||
                       (Stage == 1 && pShader2->id == 5 + 6 ) ||
                       (Stage == 1 && pShader2->id == 5 + 7 ) ||
                       (Stage == 1 && pShader2->id == 5 + 8 ) ||
                       (Stage == 1 && pShader2->id == 5 + 9 ) ||
                       (Stage == 1 && pShader2->id == 5 + 10) ||
                       (Stage == 1 && pShader2->id == 5 + 11) ||
                       (Stage == 1 && pShader2->id == 5 + 12) ||
                       (Stage == 1 && pShader2->id == 5 + 14) ||
                       (Stage == 1 && pShader2->id == 5 + 15) ||
                       (Stage == 2 && pShader2->id == 5 + 16) ||
                       (Stage == 2 && pShader2->id == 5 + 17) ||
                       (Stage == 1 && pShader2->id == 5 + 29) ||
                       (Stage == 0 && pShader2->id == 5 + 30) ||
                       (Stage == 0 && pShader2->id == 5 + 32)) {
                        NormalTex = pTexture2;
                    }
                    // specular 2
                    if((Stage == 2 && pShader2->id == 5 + 1 ) ||
                       (Stage == 2 && pShader2->id == 5 + 2 ) ||
                       (Stage == 2 && pShader2->id == 5 + 3 ) ||
                       (Stage == 2 && pShader2->id == 5 + 4 ) ||
                       (Stage == 2 && pShader2->id == 5 + 6 ) ||
                       (Stage == 2 && pShader2->id == 5 + 7 ) ||
                       (Stage == 2 && pShader2->id == 5 + 8 ) ||
                       (Stage == 2 && pShader2->id == 5 + 9 ) ||
                       (Stage == 2 && pShader2->id == 5 + 14) ||
                       (Stage == 4 && pShader2->id == 5 + 16) ||
                       (Stage == 4 && pShader2->id == 5 + 17) ||
                       (Stage == 0 && pShader2->id == 5 + 33)) {
                        SpecularTex = pTexture2;
                    }
                    // depth 3
                    if((Stage == 4 && pShader2->id == 5 + 1 ) ||
                       (Stage == 4 && pShader2->id == 5 + 2 ) ||
                       (Stage == 4 && pShader2->id == 5 + 3 ) ||
                       (Stage == 4 && pShader2->id == 5 + 4 ) ||
                       (Stage == 4 && pShader2->id == 5 + 6 ) ||
                       (Stage == 4 && pShader2->id == 5 + 7 ) ||
                       (Stage == 4 && pShader2->id == 5 + 8 ) ||
                       (Stage == 4 && pShader2->id == 5 + 9 ) ||
                       (Stage == 2 && pShader2->id == 5 + 10) ||
                       (Stage == 2 && pShader2->id == 5 + 11) ||
                       (Stage == 2 && pShader2->id == 5 + 12) ||
                       (Stage == 4 && pShader2->id == 5 + 14) ||
                       (Stage == 2 && pShader2->id == 5 + 15) ||
                       (Stage == 5 && pShader2->id == 5 + 16) ||
                       (Stage == 5 && pShader2->id == 5 + 17) ||
                       (Stage == 1 && pShader2->id == 5 + 18) ||
                       (Stage == 1 && pShader2->id == 5 + 19) ||
                       (Stage == 0 && pShader2->id == 5 + 20) ||
                       (Stage == 0 && pShader2->id == 5 + 21) ||
                       (Stage == 0 && pShader2->id == 5 + 22) ||
                       (Stage == 0 && pShader2->id == 5 + 23) ||
                       (Stage == 1 && pShader2->id == 5 + 28) ||
                       (Stage == 2 && pShader2->id == 5 + 29) ||
                       (Stage == 1 && pShader2->id == 5 + 30) ||
                       (Stage == 1 && pShader2->id == 5 + 35) ||
                       (Stage == 0 && pShader2->id == 5 + 36) ||
                       (Stage == 0 && pShader2->id == 5 + 11)) {
                        DepthTex = pTexture2;
                    }
                    // stencil
                    if((Stage == 5 && pShader2->id == 5 + 2 ) ||
                       (Stage == 5 && pShader2->id == 5 + 4 ) ||
                       (Stage == 5 && pShader2->id == 5 + 7 ) ||
                       (Stage == 5 && pShader2->id == 5 + 8 ) ||
                       (Stage == 5 && pShader2->id == 5 + 9 ) ||
                       (Stage == 4 && pShader2->id == 5 + 11) ||
                       (Stage == 4 && pShader2->id == 5 + 12) ||
                       (Stage == 5 && pShader2->id == 5 + 14) ||
                       (Stage == 2 && pShader2->id == 5 + 28)) {
                        StencilTex = pTexture2;
                    }
                }
            }
        }

    }
    return ProxyInterface->SetTexture(Stage, pTexture);
}

HRESULT m_IDirect3DDevice9Ex::CreatePixelShader(THIS_ CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) {
    HRESULT hr = ProxyInterface->CreatePixelShader(pFunction, ppShader);

    if(SUCCEEDED(hr) && ppShader) {
        *ppShader = new m_IDirect3DPixelShader9(*ppShader, this, SC_FXC);
        
    }
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreatePixelShader2(THIS_ CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader, ShaderCreationMode extra) {
    HRESULT hr = ProxyInterface->CreatePixelShader(pFunction, ppShader);

    if(SUCCEEDED(hr) && ppShader) {
        *ppShader = new m_IDirect3DPixelShader9(*ppShader, this, extra);
    }
    
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateVertexShader(THIS_ CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader) {
    HRESULT hr = ProxyInterface->CreateVertexShader(pFunction, ppShader);

    if(SUCCEEDED(hr) && ppShader) {
        *ppShader = new m_IDirect3DVertexShader9(*ppShader, this, SC_FXC);
    }    
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateVertexShader2(THIS_ CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader, ShaderCreationMode extra) {
    HRESULT hr = ProxyInterface->CreateVertexShader(pFunction, ppShader);

    if(SUCCEEDED(hr) && ppShader) {
        *ppShader = new m_IDirect3DVertexShader9(*ppShader, this, extra);
    }
    
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
            GetVertexShaderConstantF(0, &pShader2->constants[0][0], 233);
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
            if(UsePostFx) {
                if(isShaderPostFx(pShader2)) {
                    doPostFx = true;
                }
            }
        }
        else {
            pShader = static_cast<m_IDirect3DPixelShader9*>(pShader)->GetProxyInterface();
        }
    }

    HRESULT hr = ProxyInterface->SetPixelShader(pShader);

    if(pShader) {
        if(pShader2) {
            if(EnableDepthOverwrite) {
                if(pShader2->overwriteDepth) {
                    ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, pShader2->depthWrite);
                }
                else {
                    ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
                }
            }
            GetPixelShaderConstantF(0, &pShader2->constants[0][0], 233);
        }
        else {
            if(EnableDepthOverwrite) {
                ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, last);
            }
        }
    }


    if(1 && (!SMAA_EdgeDetection || !SMAA_BlendingWeightsCalculation || !SMAA_NeighborhoodBlending ||
             !SMAA_EdgeDetectionVS || !SMAA_BlendingWeightsCalculationVS || !SMAA_NeighborhoodBlendingVS)) {
        FILE* f = nullptr;
        UINT size = 0;
        f = fopen("update/shaders/SMAA2.hlsl", "r");
        if(!f) {
            f = fopen("shaders/SMAA2.hlsl", "r");
        }
        if(!f) {
            f = fopen("SMAA2.hlsl", "r");
        }
        if(!f) {
            return hr;
        }
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buff = new char[size + 5];
        memset(buff, 0, size + 5);
        if(!buff) {
            fclose(f);
            return hr;
        }
        UINT size2 = fread(buff, 1, size, f);

        fclose(f);
        HRESULT hr2 = S_FALSE;
        ID3DXBuffer* bf1 = nullptr;
        ID3DXBuffer* bf2 = nullptr;
        ID3DXConstantTable* ppConstantTable = nullptr;
        IDirect3DPixelShader9* shader = nullptr;
        if(!SMAA_EdgeDetection || !SMAA_EdgeDetectionVS) {
            { // SMAAColorEdgeDetectionPS
                HRESULT hr3 = D3DXCompileShader(buff, size, 0, 0, "SMAAColorEdgeDetectionPS", "ps_3_0", 0, &bf1, &bf2, &ppConstantTable);
                HRESULT hr2 = S_FALSE;
                IDirect3DPixelShader9* shader = nullptr;
                if(hr3 == S_OK) {
                    hr2 = CreatePixelShader2((DWORD*) bf1->GetBufferPointer(), &SMAA_EdgeDetection, SC_NEW);
                    if(hr2 != S_OK || !SMAA_EdgeDetection) {
                        Log::Error("Failed to create pixel shader hlsl: ");
                        SAFE_RELEASE(SMAA_EdgeDetection);
                    }
                    m_IDirect3DPixelShader9* shader2 = static_cast<m_IDirect3DPixelShader9*> (SMAA_EdgeDetection);
                    if(SMAA_EdgeDetection && shader2) {
                        shader2->oName = "SMAAColorEdgeDetectionPS";
                        shader2->loadedFx = buff;
                        ps_2.push_back(shader2);
                    }
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
                else {
                    Log::Error("Failed to compile pixel shader hlsl: ");
                    Log::Error((char*) bf2->GetBufferPointer());
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
            }
            {// SMAAEdgeDetectionVS
                HRESULT hr3 = D3DXCompileShader(buff, size, 0, 0, "SMAAEdgeDetectionVS", "vs_3_0", 0, &bf1, &bf2, &ppConstantTable);
                HRESULT hr2 = S_FALSE;
                IDirect3DVertexShader9* shader = nullptr;
                if(hr3 == S_OK) {
                    hr2 = CreateVertexShader2((DWORD*) bf1->GetBufferPointer(), &SMAA_EdgeDetectionVS, SC_NEW);
                    if(hr2 != S_OK || !SMAA_EdgeDetectionVS) {
                        Log::Error("Failed to create pixel shader hlsl: ");
                        SAFE_RELEASE(SMAA_EdgeDetectionVS);
                    }
                    m_IDirect3DVertexShader9* shader2 = static_cast<m_IDirect3DVertexShader9*> (SMAA_EdgeDetectionVS);
                    if(SMAA_EdgeDetectionVS && shader2) {
                        shader2->oName = "SMAAEdgeDetectionVS";
                        shader2->loadedFx = buff;
                        vs_2.push_back(shader2);
                    }
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
                else {
                    Log::Error("Failed to compile pixel shader hlsl: ");
                    Log::Error((char*) bf2->GetBufferPointer());
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
            }
        }
        if(!SMAA_BlendingWeightsCalculation || !SMAA_BlendingWeightsCalculationVS) {
            {
                HRESULT hr3 = D3DXCompileShader(buff, size, 0, 0, "SMAABlendingWeightCalculationPS", "ps_3_0", 0, &bf1, &bf2, &ppConstantTable);
                HRESULT hr2 = S_FALSE;
                IDirect3DPixelShader9* shader = nullptr;
                if(hr3 == S_OK) {
                    hr2 = CreatePixelShader2((DWORD*) bf1->GetBufferPointer(), &SMAA_BlendingWeightsCalculation, SC_NEW);
                    if(hr2 != S_OK || !SMAA_BlendingWeightsCalculation) {
                        Log::Error("Failed to create pixel shader hlsl: ");
                        SAFE_RELEASE(SMAA_BlendingWeightsCalculation);
                    }
                    m_IDirect3DPixelShader9* shader2 = static_cast<m_IDirect3DPixelShader9*> (SMAA_BlendingWeightsCalculation);
                    if(SMAA_BlendingWeightsCalculation && shader2) {
                        shader2->oName = "SMAABlendingWeightCalculationPS";
                        shader2->loadedFx = buff;
                        ps_2.push_back(shader2);
                    }
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
                else {
                    Log::Error("Failed to compile pixel shader hlsl: ");
                    Log::Error((char*) bf2->GetBufferPointer());
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
            }
            {
                HRESULT hr3 = D3DXCompileShader(buff, size, 0, 0, "SMAABlendingWeightCalculationVS", "vs_3_0", 0, &bf1, &bf2, &ppConstantTable);
                HRESULT hr2 = S_FALSE;
                IDirect3DVertexShader9* shader = nullptr;
                if(hr3 == S_OK) {
                    hr2 = CreateVertexShader2((DWORD*) bf1->GetBufferPointer(), &SMAA_BlendingWeightsCalculationVS, SC_NEW);
                    if(hr2 != S_OK || !SMAA_BlendingWeightsCalculationVS) {
                        Log::Error("Failed to create pixel shader hlsl: ");
                        SAFE_RELEASE(SMAA_BlendingWeightsCalculationVS);
                    }
                    m_IDirect3DVertexShader9* shader2 = static_cast<m_IDirect3DVertexShader9*> (SMAA_BlendingWeightsCalculationVS);
                    if(SMAA_BlendingWeightsCalculationVS && shader2) {
                        shader2->oName = "SMAABlendingWeightCalculationVS";
                        shader2->loadedFx = buff;
                        vs_2.push_back(shader2);
                    }
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
                else {
                    Log::Error("Failed to compile pixel shader hlsl: ");
                    Log::Error((char*) bf2->GetBufferPointer());
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
            }
        }
        if(!SMAA_NeighborhoodBlending || !SMAA_NeighborhoodBlendingVS) {
            {
                HRESULT hr3 = D3DXCompileShader(buff, size, 0, 0, "SMAANeighborhoodBlendingPS", "ps_3_0", 0, &bf1, &bf2, &ppConstantTable);
                HRESULT hr2 = S_FALSE;
                IDirect3DPixelShader9* shader = nullptr;
                if(hr3 == S_OK) {
                    hr2 = CreatePixelShader2((DWORD*) bf1->GetBufferPointer(), &SMAA_NeighborhoodBlending, SC_NEW);
                    if(hr2 != S_OK || !SMAA_NeighborhoodBlending) {
                        Log::Error("Failed to create pixel shader hlsl: ");
                        SAFE_RELEASE(SMAA_NeighborhoodBlending);
                    }
                    m_IDirect3DPixelShader9* shader2 = static_cast<m_IDirect3DPixelShader9*> (SMAA_NeighborhoodBlending);
                    if(SMAA_NeighborhoodBlending && shader2) {
                        shader2->oName = "SMAANeighborhoodBlendingPS";
                        shader2->loadedFx = buff;
                        ps_2.push_back(shader2);
                    }
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
                else {
                    Log::Error("Failed to compile pixel shader hlsl: ");
                    Log::Error((char*) bf2->GetBufferPointer());
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
            }
            {
                HRESULT hr3 = D3DXCompileShader(buff, size, 0, 0, "SMAANeighborhoodBlendingVS", "vs_3_0", 0, &bf1, &bf2, &ppConstantTable);
                HRESULT hr2 = S_FALSE;
                IDirect3DVertexShader9* shader = nullptr;
                if(hr3 == S_OK) {
                    hr2 = CreateVertexShader2((DWORD*) bf1->GetBufferPointer(), &SMAA_NeighborhoodBlendingVS, SC_NEW);
                    if(hr2 != S_OK || !SMAA_NeighborhoodBlendingVS) {
                        Log::Error("Failed to create pixel shader hlsl: ");
                        SAFE_RELEASE(SMAA_NeighborhoodBlendingVS);
                    }
                    m_IDirect3DVertexShader9* shader2 = static_cast<m_IDirect3DVertexShader9*> (SMAA_NeighborhoodBlendingVS);
                    if(SMAA_NeighborhoodBlendingVS && shader2) {
                        shader2->oName = "SMAANeighborhoodBlendingVS";
                        shader2->loadedFx = buff;
                        vs_2.push_back(shader2);
                    }
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
                else {
                    Log::Error("Failed to compile pixel shader hlsl: ");
                    Log::Error((char*) bf2->GetBufferPointer());
                    SAFE_RELEASE(bf1);
                    SAFE_RELEASE(bf2);
                    SAFE_RELEASE(ppConstantTable);
                }
            }
        }
    }

    if(1 && (!DOF_ps)) {
        FILE* f = nullptr;
        UINT size = 0;
        f = fopen("update/shaders/DOF_ps.hlsl", "r");
        if(!f) {
            f = fopen("shaders/DOF_ps.hlsl", "r");
        }
        if(!f) {
            f = fopen("DOF_ps.hlsl", "r");
        }
        if(!f) {
            return hr;
        }
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buff = new char[size + 5];
        memset(buff, 0, size + 5);
        if(!buff) {
            fclose(f);
            return hr;
        }
        UINT size2 = fread(buff, 1, size, f);

        fclose(f);
        HRESULT hr2 = S_FALSE;
        ID3DXBuffer* bf1 = nullptr;
        ID3DXBuffer* bf2 = nullptr;
        ID3DXConstantTable* ppConstantTable = nullptr;
        IDirect3DPixelShader9* shader = nullptr;
        { // DOF_ps
            HRESULT hr3 = D3DXCompileShader(buff, size, 0, 0, "main", "ps_3_0", 0, &bf1, &bf2, &ppConstantTable);
            HRESULT hr2 = S_FALSE;
            IDirect3DPixelShader9* shader = nullptr;
            if(hr3 == S_OK) {
                hr2 = CreatePixelShader2((DWORD*) bf1->GetBufferPointer(), &DOF_ps, SC_NEW);
                if(hr2 != S_OK || !DOF_ps) {
                    Log::Error("Failed to create DOF pixel shader hlsl: ");
                    SAFE_RELEASE(DOF_ps);
                }
                m_IDirect3DPixelShader9* shader2 = static_cast<m_IDirect3DPixelShader9*> (DOF_ps);
                if(DOF_ps && shader2) {
                    shader2->oName = "DOF_ps";
                    shader2->loadedFx = buff;
                    ps_2.push_back(shader2);
                }
                SAFE_RELEASE(bf1);
                SAFE_RELEASE(bf2);
                SAFE_RELEASE(ppConstantTable);
            }
            else {
                Log::Error("Failed to compile DOF pixel shader hlsl: ");
                Log::Error((char*) bf2->GetBufferPointer());
                SAFE_RELEASE(bf1);
                SAFE_RELEASE(bf2);
                SAFE_RELEASE(ppConstantTable);
            }
        }
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {
    HRESULT hr = 0;

    // fix from AssaultKifle47
    if(State == D3DRS_ADAPTIVETESS_X && DisableADAPTIVETESS_X) {
        Value = 0;
    }

    //if(afterpostfx == true && (State == D3DRS_ZWRITEENABLE|| State == D3DRS_ZENABLE)) {
    //    ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    //    ProxyInterface->SetRenderState(D3DRS_ZENABLE, 0);
    //}

    IDirect3DPixelShader9* pShader = 0;
    m_IDirect3DPixelShader9* pShader2 = 0;
    GetPixelShader(&pShader);
    pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
    
    if(State == D3DRS_ZWRITEENABLE) {
        last = Value;
    }

    hr = ProxyInterface->SetRenderState(State, Value);
    if(State == D3DRS_ZWRITEENABLE && pShader2 && EnableDepthOverwrite) {
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

struct VERTEX {
    D3DXVECTOR3 pos;
    D3DXVECTOR2 tex1;
};
DWORD CoronaDepth = 0;
const DWORD VertexFVF = D3DFVF_XYZ | D3DFVF_TEX1;
DWORD OldFVF = D3DFVF_XYZ | D3DFVF_TEX1;
extern VERTEX SmaaVertexArray[4];
#define PostfxTextureCount 15
IDirect3DBaseTexture9* prePostFx[PostfxTextureCount] = { 0 };
DWORD Samplers[PostfxTextureCount] = { D3DTEXF_LINEAR };
extern bool usePrimitiveUp;

 bool useDof = true;

extern bool sRGB_A ;
extern bool sRGB_a ;
extern bool sRGB_B ;
extern bool sRGB_b ;
extern bool sRGB_C ;
extern bool sRGB_c ;

extern float PostFxFog[4];

extern bool fixDistantOutlineUsingDXVK;
int DofSamples[4] = { 20 };

HRESULT m_IDirect3DDevice9Ex::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
    m_IDirect3DPixelShader9* pShader2 = nullptr;
    IDirect3DPixelShader9* pShader = nullptr;
    static float vec4[4] = { 0.f };
    HRESULT hr = S_FALSE;
    IDirect3DSurface9* pHDRSurface = nullptr;
    IDirect3DSurface9* pHDRSurface2 = nullptr;
    IDirect3DSurface9* backBuffer = nullptr;
    IDirect3DSurface9* edgesSurf = nullptr;
    IDirect3DSurface9* blendSurf = nullptr;
    DWORD OldSRGB = 0;
    DWORD OldSampler = 0;


    if(useDof && DOF_ps && doPostFx) {
        for(int i = 0; i < PostfxTextureCount; i++) {
            ProxyInterface->GetTexture(i, &prePostFx[i]);
            GetSamplerState(i, D3DSAMP_MAGFILTER, &Samplers[i]);
        }
        GetPixelShader(&pShader);
        if(pShader) {
            pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
            if(pShader2) {
                if(isShaderPostFx(pShader2)) {
                    if(UseSSAA) {
                        SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                    }
                    doPostFx = false; // useless?

                    ProxyInterface->GetRenderTarget(0, &backBuffer);
                    if(backBuffer && pHDRTex && pHDRTex2) {
                        // save sampler state

                        pHDRTex2->GetSurfaceLevel(0, &pHDRSurface2);
                        pHDRTex->GetSurfaceLevel(0, &pHDRSurface);

                        // DOF
                        if(1) {
                            // game postfx or DOF
                            //SetPixelShader(pShader2);

                            SetRenderTarget(0, pHDRSurface2);
                            Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);
                            hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                            SetPixelShader(DOF_ps);

                            //vec4[0] = ((gWindowWidth * 1) * (UseSSAA ? ResSSAA : 1));
                            //vec4[1] = ((gWindowHeight * 1) * (UseSSAA ? ResSSAA : 1));
                            //vec4[2] = 1.f / ((gWindowWidth * 1) * (UseSSAA ? ResSSAA : 1));
                            //vec4[3] = 1.f / ((gWindowHeight * 1) * (UseSSAA ? ResSSAA : 1));
                            //ProxyInterface->SetPixelShaderConstantF(210, vec4, 1);
                            //vec4[1] = AoDistance;
                            //ProxyInterface->SetPixelShaderConstantF(211, vec4, 1);

                            ProxyInterface->SetPixelShaderConstantF(212, FocusPoint, 1);
                            ProxyInterface->SetPixelShaderConstantF(213, FocusScale, 1);
                            ProxyInterface->SetPixelShaderConstantI(4, DofSamples, 1);
                            //if(gNearFarPlane) {
                            //    ProxyInterface->SetPixelShaderConstantF(128, NearFarPlane, 1);
                            //}
                            //SetTexture(1, 0);
                            //SetTexture(1, 0);
                            ProxyInterface->SetTexture(2, 0);

                            ProxyInterface->SetRenderTarget(0, backBuffer);
                            SetTexture(2, pHDRTex2);
                            //SetTexture(1, DiffuseTex);
                            //SetTexture(2, NormalTex);
                            //SetTexture(3, BloomTex);
                            //SetTexture(4, SpecularTex);
                            //SetTexture(5, DepthTex);
                            //SetTexture(6, StencilTex);
                            

                            ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                            //ProxyInterface->SetTexture(0, 0);
                            ProxyInterface->SetTexture(1, 0);
                            ProxyInterface->SetTexture(2, 0);
                            //ProxyInterface->SetTexture(3, 0);

                            SetPixelShader(pShader);
                        }


                        SAFE_RELEASE(backBuffer);
                        SAFE_RELEASE(pHDRSurface2);
                        SAFE_RELEASE(pHDRSurface);
                        if(UseSSAA && doPostFx) {
                            SetSamplerState(0, D3DSAMP_MAGFILTER, OldSampler);
                        }

                        // restore sampler state
                        for(int i = 0; i < PostfxTextureCount; i++) {
                            if(prePostFx[i]) {
                                ProxyInterface->SetTexture(i, prePostFx[i]);
                                SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
                                SAFE_RELEASE(prePostFx[i]);
                            }
                        }

                        return hr;
                    }
                    SAFE_RELEASE(backBuffer);
                }
            }
        }
        for(int i = 0; i < PostfxTextureCount; i++) {
            if(prePostFx[i]) {
                ProxyInterface->SetTexture(i, prePostFx[i]);
                SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
                SAFE_RELEASE(prePostFx[i]);
            }
        }
    }

    if(UseSSAO && SSAO_ps && SSAO_vs && doPostFx) {
        for(int i = 0; i < PostfxTextureCount; i++) {
            ProxyInterface->GetTexture(i, &prePostFx[i]);
            GetSamplerState(i, D3DSAMP_MAGFILTER, &Samplers[i]);
        }
        GetPixelShader(&pShader);
        if(pShader) {
            pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
            if(pShader2) {
                if(isShaderPostFx(pShader2)) {
                    if(UseSSAA) {
                        SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                    }
                    doPostFx = false; // useless?

                    ProxyInterface->GetRenderTarget(0, &backBuffer);
                    if(backBuffer && pHDRTex && pHDRTex2) {
                        // save sampler state

                        pHDRTex2->GetSurfaceLevel(0, &pHDRSurface2);
                        pHDRTex->GetSurfaceLevel(0, &pHDRSurface);

                        // SSAO
                        if(1) {
                            // game postfx or SSAO
                            //SetPixelShader(pShader2);

                            SetRenderTarget(0, pHDRSurface2);
                            Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);
                            hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                            SetPixelShader(SSAO_ps);

                            vec4[0] = ((gWindowWidth * 1) * (UseSSAA ? ResSSAA : 1));
                            vec4[1] = ((gWindowHeight * 1) * (UseSSAA ? ResSSAA : 1));
                            vec4[2] = 1.f / ((gWindowWidth * 1) * (UseSSAA ? ResSSAA : 1));
                            vec4[3] = 1.f / ((gWindowHeight * 1) * (UseSSAA ? ResSSAA : 1));
                            ProxyInterface->SetPixelShaderConstantF(210, vec4, 1);
                            vec4[1] = AoDistance;
                            ProxyInterface->SetPixelShaderConstantF(211, vec4, 1);
                            //SetTexture(1, 0);
                            //SetTexture(1, 0);
                            ProxyInterface->SetTexture(2, 0);

                            SetRenderTarget(0, pHDRSurface);
                            //SetTexture(2, pHDRTex2);

                            ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                            //ProxyInterface->SetTexture(0, 0);
                            ProxyInterface->SetTexture(1, 0);
                            ProxyInterface->SetTexture(2, 0);
                            //ProxyInterface->SetTexture(3, 0);

                            SetPixelShader(SSAO_ps2);

                            ProxyInterface->SetRenderTarget(0, backBuffer);
                            switch(UseDebugTextures) {

                                //"None", 
                                //"Difuse", 
                                //"normal",
                                //"specular", 
                                //"depth", 
                                //"SMAA edges", 
                                //"stencil"

                                case 0:
                                    SetTexture(2, pHDRTex2);
                                    break;
                                case 1:
                                    SetTexture(2, DiffuseTex);
                                    break;
                                case 2:
                                    SetTexture(2, NormalTex);
                                    break;
                                case 3:
                                    SetTexture(2, BloomTex);
                                    break;
                                case 4:
                                    SetTexture(2, SpecularTex);
                                    break;
                                case 5:
                                    SetTexture(2, DepthTex);
                                    break;
                                case 6:
                                    SetTexture(2, StencilTex);
                                    break;
                                default:
                                    SetTexture(2, pHDRTex2);
                                    break;
                            }
                            SetTexture(1, pHDRTex);

                            ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                            ProxyInterface->SetTexture(1, 0);
                            ProxyInterface->SetTexture(2, 0);
                            SetPixelShader(pShader);
                        }


                        SAFE_RELEASE(backBuffer);
                        SAFE_RELEASE(pHDRSurface2);
                        SAFE_RELEASE(pHDRSurface);
                        if(UseSSAA && doPostFx) {
                            SetSamplerState(0, D3DSAMP_MAGFILTER, OldSampler);
                        }
                        //ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
                        //ProxyInterface->SetRenderState(D3DRS_ZENABLE, 0);
                        //hr = ProxyInterface->GetDepthStencilSurface(&oldDepth);
                        //Clear(0, 0, D3DCLEAR_ZBUFFER, 0, 0, 0);

                        //hr = ProxyInterface->SetDepthStencilSurface(mainDepth);
                        //Clear(0, 0, D3DCLEAR_ZBUFFER, 0, 0, 0);

                        //afterpostfx = true;

                        // restore sampler state
                        for(int i = 0; i < PostfxTextureCount; i++) {
                            if(prePostFx[i]) {
                                ProxyInterface->SetTexture(i, prePostFx[i]);
                                SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
                                SAFE_RELEASE(prePostFx[i]);
                            }
                        }

                        return hr;
                    }
                    SAFE_RELEASE(backBuffer);
                }
            }
        }
        for(int i = 0; i < PostfxTextureCount; i++) {
            if(prePostFx[i]) {
                ProxyInterface->SetTexture(i, prePostFx[i]);
                SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
                SAFE_RELEASE(prePostFx[i]);
            }
        }
    }

    if(fixDistantOutlineUsingDXVK) {
        GetPixelShader(&pShader);
        if(pShader) {
            pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
            if(pShader2 && pShader2->id == 822 || pShader2->id == 823) {
                DWORD oldSampler = 0;
                GetSamplerState(0, D3DSAMP_MAGFILTER, &oldSampler);
                SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
                hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                SetSamplerState(0, D3DSAMP_MAGFILTER, oldSampler);
                return hr;
            }
        }
    }

    if(UseSSAA && doPostFx) {
        GetSamplerState(0, D3DSAMP_MAGFILTER, &OldSampler);
        SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    }

    if(UsePostFx[0] > 0 && doPostFx) {
        GetPixelShader(&pShader);
        if(pShader) {
            pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
            if(pShader2) {
                if(isShaderPostFx(pShader2)) {
                    if(UseSSAA) {
                        SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                    }
                    doPostFx = false; // useless?

                    ProxyInterface->GetRenderTarget(0, &backBuffer);
                    if(backBuffer && pHDRTex && pHDRTex2) {
                        // save sampler state
                        //for(int i = 0; i < PostfxTextureCount; i++) {
                        //    GetTexture(i, &prePostFx[i]);
                        //    GetSamplerState(i, D3DSAMP_MAGFILTER, &Samplers[i]);
                        //}

                        pHDRTex2->GetSurfaceLevel(0, &pHDRSurface2);

                        // FXAA
                        if((UsePostFx[0] == 1 ) && PostFxPS) {
                            // game postfx
                            GetRenderState(D3DRS_SRGBWRITEENABLE, &OldSRGB); // save srgb state
                            //SetPixelShader(pShader2);

                            SetRenderTarget(0, pHDRSurface2);
                            Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);
                                SetRenderState(D3DRS_SRGBWRITEENABLE, (sRGB_A==true?1:0));
                                SetSamplerState(2, D3DSAMP_SRGBTEXTURE, (sRGB_a == true ? 1 : 0));
                            hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                            SetPixelShader(PostFxPS);

                            ProxyInterface->SetRenderTarget(0, backBuffer);
                            SetTexture(2, pHDRTex2);

                                SetRenderState(D3DRS_SRGBWRITEENABLE, (sRGB_C == true ? 1 : 0));
                                SetSamplerState(2, D3DSAMP_SRGBTEXTURE, (sRGB_c == true ? 1 : 0));

                            hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                            SetSamplerState(2, D3DSAMP_SRGBTEXTURE, 0);

                            ProxyInterface->SetTexture(2, 0);
                            SetPixelShader(pShader);
                            SetRenderState(D3DRS_SRGBWRITEENABLE, OldSRGB);// restore srgb state

                        }

                        // SMAA
                        if(UsePostFx[0] > 1 &&
                           SMAA_EdgeDetection && SMAA_BlendingWeightsCalculation && SMAA_NeighborhoodBlending &&
                           SMAA_EdgeDetectionVS && SMAA_BlendingWeightsCalculationVS && SMAA_NeighborhoodBlendingVS &&
                           areaTex && searchTex && edgesTex && blendTex && pHDRSurface2
                           ) {
                            vec4[0] = 1.f / (gWindowWidth * (UseSSAA ? ResSSAA : 1));
                            vec4[1] = 1.f / (gWindowHeight * (UseSSAA ? ResSSAA : 1));
                            vec4[2] = (gWindowWidth * (UseSSAA ? ResSSAA : 1));
                            vec4[3] = (gWindowHeight * (UseSSAA ? ResSSAA : 1));
                            ProxyInterface->SetPixelShaderConstantF(24, vec4, 1);
                            ProxyInterface->SetVertexShaderConstantF(24, vec4, 1);
                            {
                                edgesTex->GetSurfaceLevel(0, &edgesSurf);
                                blendTex->GetSurfaceLevel(0, &blendSurf);


                                // game postfx
                                //SetPixelShader(pShader2);
                                SetRenderTarget(0, pHDRSurface2);
                                Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);
                                hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);


                                // SMAA_EdgeDetection
                                GetRenderState(D3DRS_SRGBWRITEENABLE, &OldSRGB); // save srgb state
                                SetRenderState(D3DRS_SRGBWRITEENABLE, 0);
                                ProxyInterface->GetFVF(&OldFVF);
                                if(usePrimitiveUp)
                                    ProxyInterface->SetFVF(VertexFVF);
                                SetPixelShader(SMAA_EdgeDetection);
                                SetVertexShader(SMAA_EdgeDetectionVS);
                                ProxyInterface->SetPixelShaderConstantF(24, vec4, 1);
                                ProxyInterface->SetVertexShaderConstantF(24, vec4, 1);
                                SetRenderTarget(0, edgesSurf);
                                SetTexture(0, pHDRTex2);
                                SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                                SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                                SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                                SetSamplerState(3, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
                                SetSamplerState(4, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

                                Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);
                                SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 1);
                                if(usePrimitiveUp)
                                    ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, SmaaVertexArray, sizeof(VERTEX));
                                else
                                    ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                                SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);

                                ProxyInterface->SetTexture(0, 0);
                                //SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 1);


                                // SMAA_BlendingWeightsCalculation
                                SetPixelShader(SMAA_BlendingWeightsCalculation);
                                SetVertexShader(SMAA_BlendingWeightsCalculationVS);
                                ProxyInterface->SetPixelShaderConstantF(24, vec4, 1);
                                ProxyInterface->SetVertexShaderConstantF(24, vec4, 1);
                                SetRenderTarget(0, blendSurf);
                                SetTexture(1, edgesTex);
                                SetTexture(2, areaTex);
                                SetTexture(3, searchTex);
                                Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);
                                if(usePrimitiveUp)
                                    ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, SmaaVertexArray, sizeof(VERTEX));
                                else
                                    ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                                ProxyInterface->SetTexture(1, 0);
                                ProxyInterface->SetTexture(2, 0);
                                ProxyInterface->SetTexture(3, 0);

                                //SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);                                
                                SetRenderState(D3DRS_SRGBWRITEENABLE, OldSRGB);// restore srgb state

                                // SMAA_NeighborhoodBlending
                                SetPixelShader(SMAA_NeighborhoodBlending);
                                SetVertexShader(SMAA_NeighborhoodBlendingVS);
                                ProxyInterface->SetPixelShaderConstantF(24, vec4, 1);
                                ProxyInterface->SetVertexShaderConstantF(24, vec4, 1);
                                vec4[0] = UsePostFx[0];
                                vec4[1] = 1;
                                vec4[2] = 3;
                                vec4[3] = 4;
                                ProxyInterface->SetPixelShaderConstantF(5, vec4, 1);
                                ProxyInterface->SetRenderTarget(0, backBuffer);
                                //Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);

                                SetTexture(0, pHDRTex2);
                                SetTexture(1, edgesTex);
                                SetTexture(4, blendTex);

                                SetRenderState(D3DRS_SRGBWRITEENABLE, 0);

                                if(usePrimitiveUp)
                                    ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, SmaaVertexArray, sizeof(VERTEX));
                                else
                                    ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                                ProxyInterface->SetTexture(1, 0);
                                ProxyInterface->SetTexture(4, 0);

                                SetPixelShader(pShader);
                                ProxyInterface->SetFVF(OldFVF);


                                SAFE_RELEASE(edgesSurf);
                                SAFE_RELEASE(blendSurf);
                            }
                        }

                        SAFE_RELEASE(backBuffer);
                        SAFE_RELEASE(pHDRSurface2);
                        if(UseSSAA && doPostFx) {
                            SetSamplerState(0, D3DSAMP_MAGFILTER, OldSampler);
                        }
                        //ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 0);
                        //ProxyInterface->SetRenderState(D3DRS_ZENABLE, 0);
                        //hr = ProxyInterface->GetDepthStencilSurface(&oldDepth);
                        //Clear(0, 0, D3DCLEAR_ZBUFFER, 0, 0, 0);
                        //hr = ProxyInterface->SetDepthStencilSurface(mainDepth);
                        //Clear(0, 0, D3DCLEAR_ZBUFFER, 0, 0, 0);
                        //afterpostfx = true;
                        // restore sampler state
                        for(int i = 0; i < PostfxTextureCount; i++) {
                            if(prePostFx[i]) {
                                SetTexture(i, prePostFx[i]);
                                //SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
                                SAFE_RELEASE(prePostFx[i]);
                            }
                            SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
                        }

                        return hr;
                    }
                    SAFE_RELEASE(backBuffer);
                }
            }
        }
    }
    if(UseSSAA && doPostFx) {
        SetSamplerState(0, D3DSAMP_MAGFILTER, OldSampler);
    }

    // Fixes coronas being rendered through objects in water reflections.
    if(fixCoronaDepth) {
        GetRenderState(D3DRS_ZENABLE, &CoronaDepth);
        IDirect3DVertexShader9* vShader = 0;
        GetVertexShader(&vShader);
        if(vShader) {
            m_IDirect3DVertexShader9* vshader2 = static_cast<m_IDirect3DVertexShader9*>(vShader);
            if(vshader2) {
                if(vshader2->id == 15) {
                    SetRenderState(D3DRS_ZENABLE, 1);
                    hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                    SetRenderState(D3DRS_ZENABLE, CoronaDepth);
                    return hr;
                }
            }
        }
    }

    hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
    return hr;
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




