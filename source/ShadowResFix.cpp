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
bool UseSSAO = 0;
bool UseBokeh = 0;
bool UseMBlur = 0;

float ResSSAA = 1;
float AoDistance = 100;
float FocusPoint[4] = { 350, 0, 0, 0 };
float FocusScale[4] = { 300, 0, 0, 0 };
float parametersAA[4] = { 0.25f , 0.125f , 0.0f, 0.0f };
// gWorld[4] = { 0.f };
// gWorldView[4] = { 0.f };
// gWorldViewProj[4] = { 0.f };
// gViewInverse[4] = { 0.f };

float SunCentre[4] = { 0.f };
float SunColor [4] = { 0.f };
float SunDirection[4] = { 0.f };
float SS_params[4] = { 1.5f, 0.95f, 0.01f, 0.975f };
float SS_params2[4] = { 0.5f, 1.f, 2.f, 1.5f };
float gWorld[4*4] = { 0.f };
float gWorldView[4*4] = { 0.f };
float gWorldViewProj[4*4] = { 0.f };
float gViewInverse[4*4] = { 0.f };
float gShadowMatrix[4*4] = { 0.f };

extern uint8_t* baseAddress;

float DEPTHBIAS = 0.f;
float SLOPESCALEDEPTHBIAS = 0.f;

bool doPostFx = false;
extern bool fixCoronaDepth;
extern int UsePostFxAA[4]; // postfx type
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

std::vector<m_IDirect3DPixelShader9*> ps_4;
std::vector<m_IDirect3DVertexShader9*> vs_4;

// list to disable depth write
std::vector<IDirect3DPixelShader9*> ps2;
//extern int getSignature(IDirect3DPixelShader9* pShader, std::vector<uint8_t>& pattern);

extern std::vector<uint8_t> patternFF;// ShaderId

// main fxc list with a list of shaders
std::vector<std::vector<m_IDirect3DPixelShader9*>> fx_ps;
// main fxc list with a list of shaders
std::vector<std::vector<m_IDirect3DVertexShader9*>> fx_vs;

// found texture list
std::list<m_IDirect3DTexture9*> textureList;

m_IDirect3DTexture9* rainDepth = nullptr;

extern m_IDirect3DTexture9* NormalTex;
extern m_IDirect3DTexture9* DiffuseTex;
extern m_IDirect3DTexture9* SpecularTex;
extern m_IDirect3DTexture9* DepthTex;
extern m_IDirect3DTexture9* StencilTex;
extern m_IDirect3DTexture9* BloomTex;

// postfx textures
IDirect3DTexture9* pHDRTex =  nullptr; // game hdr texture
IDirect3DTexture9* pHDRTex2 =  nullptr; // main temp texture
IDirect3DTexture9* pHDRTex3 =  nullptr; // main temp texture

IDirect3DTexture9* pShadowBlurTex1 =  nullptr; // main shadow temp texture
IDirect3DTexture9* pShadowBlurTex2 =  nullptr; // main shadow temp texture

IDirect3DTexture9* pHalfHDRTex =  nullptr; // game half res screen texture
IDirect3DTexture9* pQuarterHDRTex =  nullptr; //  game 1/4 res screen texture
IDirect3DTexture9* pHDRDownsampleTex =  nullptr; // main downsampled texture
IDirect3DTexture9* pHDRDownsampleTex2 =  nullptr; // main downsampled texture

 // smaa textures
IDirect3DTexture9* areaTex  =  nullptr; // loaded from file
IDirect3DTexture9* searchTex=  nullptr; // loaded from file
IDirect3DTexture9* edgesTex =  nullptr; // smaa gen
IDirect3DTexture9* blendTex =  nullptr; // smaa gen
IDirect3DVolumeTexture9* bluenoisevolume =  nullptr; // loaded from file
extern bool afterpostfx;
IDirect3DTexture9* mainDepthTex = nullptr; // gen
IDirect3DTexture9* oldDepthTex = nullptr; // gen
IDirect3DSurface9* mainDepth = nullptr; // gen
IDirect3DSurface9* oldDepth = nullptr; // gen

IDirect3DTexture9* aoHalfTex = 0;
IDirect3DTexture9* aoTex = 0;
IDirect3DTexture9* halfDepthDsTex = 0;
IDirect3DTexture9* stencilDownsampled = nullptr; // gen
IDirect3DTexture9* depthStenciltex = nullptr; // gen

// temp set and used in postfx
IDirect3DTexture9* renderTargetTex = 0;
IDirect3DTexture9* textureRead = 0;
IDirect3DSurface9* renderTargetSurf = nullptr;
IDirect3DSurface9* surfaceRead = nullptr;

IDirect3DTexture9* OldShadowAtlas = nullptr;
IDirect3DTexture9* NewShadowAtlas = nullptr;
IDirect3DSurface9* OldShadowAtlasSurf = nullptr;



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

IDirect3DPixelShader9* FxaaPS = nullptr;
IDirect3DPixelShader9* SunShafts_PS = nullptr;
IDirect3DPixelShader9* SunShafts2_PS = nullptr;
IDirect3DPixelShader9* SunShafts3_PS = nullptr;
IDirect3DPixelShader9* SunShafts4_PS = nullptr;
IDirect3DPixelShader9* SSDownsampler_PS = nullptr;
IDirect3DPixelShader9* SSDownsampler2_PS = nullptr;
IDirect3DPixelShader9* SSAdd_PS = nullptr;

IDirect3DPixelShader9* SMAA_EdgeDetection = nullptr;
IDirect3DPixelShader9* SMAA_BlendingWeightsCalculation = nullptr;
IDirect3DPixelShader9* SMAA_NeighborhoodBlending = nullptr;

IDirect3DVertexShader9* SMAA_EdgeDetectionVS = nullptr;
IDirect3DVertexShader9* SMAA_BlendingWeightsCalculationVS = nullptr;
IDirect3DVertexShader9* SMAA_NeighborhoodBlendingVS = nullptr;

IDirect3DPixelShader9* DOF_ps = nullptr;
IDirect3DPixelShader9* dof_blur_ps = nullptr;
IDirect3DPixelShader9* dof_coc_ps = nullptr;

IDirect3DPixelShader9* depth_of_field_ps = nullptr;
IDirect3DPixelShader9* depth_of_field_tent_ps = nullptr;
IDirect3DPixelShader9* depth_of_field_blur_ps = nullptr;
IDirect3DPixelShader9* depth_of_field_coc_ps  = nullptr;
IDirect3DPixelShader9* stipple_filter_ps = nullptr;
IDirect3DPixelShader9* motionblur_ps = nullptr;

IDirect3DPixelShader9* SSAO_ps = nullptr;
IDirect3DPixelShader9* SSAO_ps2 = nullptr;
IDirect3DVertexShader9* SSAO_vs = nullptr;
IDirect3DPixelShader9* downsampler_ps = nullptr;

IDirect3DPixelShader9* DeferredShadowGen_ps = nullptr;
IDirect3DPixelShader9* DeferredShadowBlur1_ps = nullptr;
IDirect3DPixelShader9* DeferredShadowBlur2_ps = nullptr;
IDirect3DPixelShader9* DeferredShadowBlur3_ps = nullptr;
IDirect3DPixelShader9* DeferredShadowUse1_ps = nullptr;
IDirect3DPixelShader9* DeferredShadowUse2_ps = nullptr;




IDirect3DPixelShader9* MSAA_CustomResolve_ps = nullptr;
IDirect3DPixelShader9* MSAA_Render_ps = nullptr;

extern LPDIRECT3DPIXELSHADER9       ImGuiPS;
extern LPDIRECT3DVERTEXSHADER9      ImGuiVS;

extern IDirect3DPixelShader9* CompilePixelShaderFromFile(const char* filename, const char* mainFunction, const char* name, m_IDirect3DDevice9Ex* m_pDeviceEx, bool isAsm, bool addToList);
extern IDirect3DVertexShader9* CompileVertexShaderFromFile(const char* filename, const char* mainFunction, const char* name, m_IDirect3DDevice9Ex* m_pDeviceEx, bool isAsm, bool addToList);

std::vector<float> mtx;

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
    if(((Format == D3DFMT_R16F || Format == D3DFMT_R32F) && Height == /*256*/ 128 << *ShadowQualityBA0xD612B8 && Width == Height * 4 && Levels == 1)) {
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
        if(((Format == D3DFMT_R16F || Format == D3DFMT_R32F) && Height >= 256 && Width == Height * 4 && Levels == 1)) {
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
        textureList.remove((m_IDirect3DTexture9*) pHalfHDRTex);
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
        if(gFixWashedMirror)
            Format = D3DFMT_A16B16G16R16F;
    }
    if((Format == D3DFMT_A2B10G10R10 || Format == D3DFMT_A2R10G10B10 || Format == D3DFMT_A2B10G10R10_XR_BIAS || 
        Format == D3DFMT_A2W10V10U10) && Width == gWindowWidth && Height == gWindowHeight) {
        addToList = true;
        name = "gBuffer";
        if(gFixWashedMirror)
            Format = D3DFMT_A16B16G16R16F;
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
    //if( Format == D3DFMT_D24S8) {
    //    Format = D3DFMT_D24FS8;
    //};
    HRESULT hr = ProxyInterface->CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);

    if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth / 4 && Height == gWindowHeight / 4) {
        if(pQuarterHDRTex == nullptr) {
            pQuarterHDRTex = (*ppTexture);
            ((m_IDirect3DTexture9*) pQuarterHDRTex)->name = "MainPostfx";
            name = "MainPostfx";
        }
    }
    m_IDirect3DTexture9* tex = nullptr;
    if(SUCCEEDED(hr) && ppTexture) {
        tex = new m_IDirect3DTexture9(*ppTexture, this, Width, Height, Levels, Usage, Format, Pool, name, addToList);
        *ppTexture = tex;
    }

    if(tex != nullptr && (Format == D3DFMT_R16F || Format == D3DFMT_R32F) && Height == /*256*/ 128 << *ShadowQualityBA0xD612B8 && Width == Height * 4 && Levels == 1) {
        OldShadowAtlas = tex;
        SAFE_RELEASE(NewShadowAtlas);
        HRESULT hr = ProxyInterface->CreateTexture(Width, Height, Levels,                       D3DUSAGE_DEPTHSTENCIL, D3DFMT_D24X8, Pool, &NewShadowAtlas, pSharedHandle);
        NewShadowAtlas = new m_IDirect3DTexture9(NewShadowAtlas, this, Width, Height, Levels,   D3DUSAGE_DEPTHSTENCIL, D3DFMT_D24X8, Pool, name, addToList);
        //HRESULT hr = ProxyInterface->CreateTexture(Width, Height, Levels,                       D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT) MAKEFOURCC('D','F','2','4'), Pool, &NewShadowAtlas, pSharedHandle);
        //NewShadowAtlas = new m_IDirect3DTexture9(NewShadowAtlas, this, Width, Height, Levels,   D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT) MAKEFOURCC('D','F','2','4'), Pool, name, addToList);
    }



    //RainDropsFix from AssaultKifle47
    if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth / gWindowDivisor && Height == gWindowHeight / gWindowDivisor && ppTexture != 0 && (*ppTexture) != 0) {
        pRainDropsRefractionHDRTex = (*ppTexture);
    }
    if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth / 4 && Height == gWindowHeight / 4) {
        BloomTex = static_cast<m_IDirect3DTexture9*>(*ppTexture);
    }
    if(Format == D3DFMT_A16B16G16R16F && Width == gWindowWidth / 2 && Height == gWindowHeight / 2) {
        pHalfHDRTex = static_cast<m_IDirect3DTexture9*>(*ppTexture);
    }
    if(Format == D3DFMT_A16B16G16R16F && ppTexture && Width == (gWindowWidth * (UseSSAA ? ResSSAA : 1)) && Height == (gWindowHeight * (UseSSAA ? ResSSAA : 1))) {
        pHDRTex = (*ppTexture);


        textureList.remove((m_IDirect3DTexture9*) pHDRTex2);
        textureList.remove((m_IDirect3DTexture9*) pHDRTex3);
        textureList.remove((m_IDirect3DTexture9*) areaTex);
        textureList.remove((m_IDirect3DTexture9*) searchTex);
        textureList.remove((m_IDirect3DTexture9*) edgesTex);
        textureList.remove((m_IDirect3DTexture9*) blendTex);
        textureList.remove((m_IDirect3DTexture9*) bluenoisevolume);
        textureList.remove((m_IDirect3DTexture9*) aoHalfTex);
        textureList.remove((m_IDirect3DTexture9*) aoTex);
        textureList.remove((m_IDirect3DTexture9*) halfDepthDsTex);
        textureList.remove((m_IDirect3DTexture9*) stencilDownsampled);
        textureList.remove((m_IDirect3DTexture9*) pHDRDownsampleTex);
        textureList.remove((m_IDirect3DTexture9*) pHDRDownsampleTex2);
        textureList.remove((m_IDirect3DTexture9*) depthStenciltex);
        textureList.remove((m_IDirect3DTexture9*) pShadowBlurTex1);
        textureList.remove((m_IDirect3DTexture9*) pShadowBlurTex2);

        // Release before, just to be sure
        SAFE_RELEASE(pHDRTex2);
        SAFE_RELEASE(pHDRTex3);
        SAFE_RELEASE(areaTex);
        SAFE_RELEASE(searchTex);
        SAFE_RELEASE(edgesTex);
        SAFE_RELEASE(blendTex);
        SAFE_RELEASE(bluenoisevolume);

        SAFE_RELEASE(aoTex);
        SAFE_RELEASE(aoHalfTex);
        SAFE_RELEASE(halfDepthDsTex);
        SAFE_RELEASE(stencilDownsampled);
        SAFE_RELEASE(pHDRDownsampleTex);
        SAFE_RELEASE(pHDRDownsampleTex2);
        SAFE_RELEASE(depthStenciltex);
        SAFE_RELEASE(pShadowBlurTex1);
        SAFE_RELEASE(pShadowBlurTex2);

        // create new texture to postfx
        HRESULT hr = ProxyInterface->CreateTexture(Width, Height, Levels, Usage, Format, Pool, &pHDRTex2, 0);
        if(SUCCEEDED(hr) && pHDRTex2) {
            m_IDirect3DTexture9* tex = nullptr;

            std::string nm = "Screen";
            tex = new m_IDirect3DTexture9(pHDRTex2, this, Width, Height, Levels, Usage, Format, Pool, nm, true);
            tex->name = "Screen";
            pHDRTex2 = tex;
        }
        hr = ProxyInterface->CreateTexture(Width, Height, Levels, Usage, Format, Pool, &pHDRTex3, 0);
        if(SUCCEEDED(hr) && pHDRTex3) {
            m_IDirect3DTexture9* tex = nullptr;

            std::string nm = "Screen";
            tex = new m_IDirect3DTexture9(pHDRTex3, this, Width, Height, Levels, Usage, Format, Pool, nm, true);
            tex->name = "Screen";
            pHDRTex3 = tex;

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
            hr = D3DXCreateVolumeTextureFromFileExA(this, "update/shaders/bluenoisevolume.dds", 0, 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &bluenoisevolume);
            if(hr != S_OK) {
                //hr = D3DXCreateTextureFromFileExA(this, "shaders/bluenoisevolume.dds", 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &bluenoisevolume);
                hr = D3DXCreateVolumeTextureFromFileExA(this, "shaders/bluenoisevolume.dds", 0, 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &bluenoisevolume);
                if(hr != S_OK) {
                    //hr = D3DXCreateTextureFromFileExA(this, "bluenoisevolume.dds", 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &bluenoisevolume);
                    hr = D3DXCreateVolumeTextureFromFileExA(this, "bluenoisevolume.dds", 0, 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, D3DX_FILTER_LINEAR, D3DCOLOR_ARGB(150, 100, 100, 100), NULL, NULL, &bluenoisevolume);
                }
            }
            if(searchTex) {
                ((m_IDirect3DTexture9*) searchTex)->name = "Loaded";
            }
            if(areaTex) { 
                ((m_IDirect3DTexture9*) areaTex)->name = "Loaded";
            }
            if(bluenoisevolume) {
                ((m_IDirect3DTexture9*) bluenoisevolume)->name = "Loaded";
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
                hr = CreateTexture(Width, Height, Levels, Usage, D3DFMT_X8R8G8B8, Pool, &aoTex, 0);
                if(hr == S_OK && aoTex) {
                    ((m_IDirect3DTexture9*) aoTex)->name = "MainPostfx";
                }
                hr = CreateTexture(Width / 2, Height / 2, Levels, Usage, D3DFMT_R16F, Pool, &halfDepthDsTex, 0);
                if(hr == S_OK && halfDepthDsTex) {
                    ((m_IDirect3DTexture9*) halfDepthDsTex)->name = "MainPostfx";
                }
                hr = CreateTexture(Width , Height , Levels, Usage, D3DFMT_R16F, Pool, &stencilDownsampled, 0);
                if(hr == S_OK && stencilDownsampled) {
                    ((m_IDirect3DTexture9*) stencilDownsampled)->name = "MainPostfx";
                }            
                hr = CreateTexture(Width /2, Height/2 , Levels /*1*/, Usage, D3DFMT_A32B32G32R32F, Pool, &pHDRDownsampleTex, 0);
                if(hr == S_OK && pHDRDownsampleTex) {
                    ((m_IDirect3DTexture9*) pHDRDownsampleTex)->name = "MainPostfx";
                }
                hr = CreateTexture(Width /2, Height/2 , Levels /*1*/, Usage, D3DFMT_A32B32G32R32F, Pool, &pHDRDownsampleTex2, 0);
                if(hr == S_OK && pHDRDownsampleTex2) {
                    ((m_IDirect3DTexture9*) pHDRDownsampleTex2)->name = "MainPostfx";
                }

                hr = CreateTexture(Width, Height, Levels, Usage, D3DFMT_G16R16F, Pool, &pShadowBlurTex1, 0);
                if(hr == S_OK && pShadowBlurTex1) {
                    ((m_IDirect3DTexture9*) pShadowBlurTex1)->name = "MainPostfx";
                }
                hr = CreateTexture(Width, Height, Levels, Usage, D3DFMT_G16R16F, Pool, &pShadowBlurTex2, 0);
                if(hr == S_OK && pShadowBlurTex2) {
                    ((m_IDirect3DTexture9*) pShadowBlurTex2)->name = "MainPostfx";
                }

                hr = CreateTexture(Width /2, Height/2 , Levels /*1*/, 0, D3DFMT_D24S8, Pool, &depthStenciltex, 0);
                if(hr == S_OK && depthStenciltex) {
                    ((m_IDirect3DTexture9*) depthStenciltex)->name = "MainPostfx";
                }
            }
        }
    }
    if(hr != S_OK) {
        printf("printf for debug only\n");
    }
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetPixelShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {
    IDirect3DPixelShader9* pShader = 0;
    m_IDirect3DPixelShader9* pShader2 = 0;
    float vec[4] = { 0.f };

    GetPixelShader(&pShader);
    HRESULT hr = 0;
    if(pShader) {
        pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
        if(pShader2) {
            for(int i = 0; i < (int) Vector4fCount * 4; i++) {
                //if(pConstantData[i] != 0.f)
                m_IDirect3DPixelShader9::globalConstants[StartRegister][i] = pConstantData[i];
            }
            memcpy(pShader2->constants[0], m_IDirect3DPixelShader9::globalConstants[0], 4 * 4 * 250);
            if(StartRegister == 65 && Vector4fCount >= 1) {
                switch(pShader2->id) {
                    case 65:
                    case 66:
                    {
                        if(pConstantData[1] != 0.f && (pConstantData[1] == -pConstantData[3] || pConstantData[1] == pConstantData[3]))
                            memcpy(SunDirection, pConstantData, 4 * 4);
                    }
                }
            }
            if(StartRegister == 64 && Vector4fCount >= 1) {
                switch(pShader2->id) {
                    case 65:
                    case 66:
                    {
                        if(pConstantData[0] != 0.f)
                            memcpy(SunCentre, pConstantData, 4 * 4);
                    }
                }
            }
            if(StartRegister == 66 && Vector4fCount >= 1) {
                switch(pShader2->id) {
                    case 65:
                    case 66:
                    {
                        if(pConstantData[0] != 0.f)
                            memcpy(SunColor, pConstantData, 4 * 4);
                    }
                }
            }
            if(Vector4fCount >= 4) {
                if(StartRegister == 12) {
                    switch(pShader2->id) {
                        case 7:
                        case 8:
                        case 12:
                        case 13:
                        case 15:
                        case 16:
                        case 17:
                        case 21:
                        case 23:
                        case 34:
                        case 35:
                        case 148:
                        case 158:
                        case 281:
                        case 550:
                        {
                            if(pConstantData[0] != 0.f)
                                memcpy(gViewInverse, pConstantData, 4 * 4 * 4);
                        }
                    }
                    if(StartRegister == 60) {
                        switch(pShader2->id) {
                            case 7:
                            case 8:
                            case 148:
                            case 158:
                            case 281:
                            case 550:
                            {
                                if(pConstantData[0] != 0.f)
                                    memcpy(gShadowMatrix, pConstantData, 4 * 4 * 4);
                            }
                        }
                    }
                }
            }
        }

        if(!ProxyInterface) {
            return S_FALSE;
        }
        if(UseSSAA && pConstantData[0] == 1.f / gWindowWidth && pConstantData[1] == 1.f / gWindowHeight && Vector4fCount == 1) {
            vec[0] = pConstantData[0] * (1.f / ResSSAA);
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
                if(pShader2 && pShader2->disable) {
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
        if(pShader2 && pShader2->disable) {
            ProxyInterface->SetPixelShaderConstantF(220, shaderReturnColor, 1);
        }
        return hr;
    }
    hr = ProxyInterface->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetVertexShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) {
    IDirect3DVertexShader9* pShader = 0;
    GetVertexShader(&pShader);
    if(pShader) {
        m_IDirect3DVertexShader9* pShader2 = static_cast<m_IDirect3DVertexShader9*>(pShader);
        if(pShader2) {
            if(Vector4fCount >= 4) {
                // gWorld[4] = { 0.f };
                // gWorldView[4] = { 0.f };
                // gWorldViewProj[4] = { 0.f };
                // gViewInverse[4] = { 0.f };
                if(StartRegister == 0) {
                    switch(pShader2->id) {
                        case 29:
                        case 34:
                        {
                            if(Vector4fCount == 16) {
                                if(mtx.size() < (Vector4fCount * 4));
                                mtx.resize(Vector4fCount * 4);
                                memcpy(&mtx[0], pConstantData, 4 * 4 * Vector4fCount);
                                memcpy(gWorld, pConstantData, 4 * 4 * 4);
                            }
                        }
                    }
                }
                if(StartRegister == 4) {
                    switch(pShader2->id) {
                        case 34:
                        {
                            if(pConstantData[0] != 0.f)
                                memcpy(gWorldView, pConstantData, 4 * 4 * 4);
                        }
                    }
                }
                if(StartRegister == 8) {
                    switch(pShader2->id) {
                        case  29:
                        {
                            if(pConstantData[0] != 0.f)
                                memcpy(gWorldViewProj, pConstantData, 4 * 4 * 4);
                        }
                    }
                }
                if(StartRegister == 12) {
                    switch(pShader2->id) {
                        case 10:
                        case 14:
                        case 15:
                        case 20:
                        case 21:
                        case 22:
                        case 28:
                        case 534:
                        case 537:
                        case 538:
                        case 539:
                        case 540:
                        case 541:
                        {
                            if(pConstantData[0] != 0.f)
                                memcpy(gViewInverse, pConstantData, 4 * 4 * 4);
                        }
                    }
                }
                if(StartRegister == 60) {
                    switch(pShader2->id) {
                        case 2:
                        case 3:
                        case 20:
                        case 130:
                        case 537:
                        case 538:
                        case 539:
                        case 540:
                        case 809:
                        case 831:
                        {
                            if(pConstantData[0] != 0.f)
                                memcpy(gShadowMatrix, pConstantData, 4 * 4 * 4);
                        }
                    }
                }
            }

            for(int i = 0; i < (int) Vector4fCount * 4; i++) {
                //if(pConstantData[i] != 0.f)
                m_IDirect3DVertexShader9::globalConstants[StartRegister][i] = pConstantData[i];
            }
            memcpy(pShader2->constants[0], m_IDirect3DVertexShader9::globalConstants[0], (4 * 4 * 250));
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
    if(gFixRainDrops && Stage == 1 && pTexture == 0 && DWORD(pRainDropsRefractionHDRTex) > 0x4) {
        IDirect3DVertexShader9* pShader = nullptr;
        m_IDirect3DVertexShader9* pShader2 = nullptr;
        GetVertexShader(&pShader);
        if(pShader) {
            pShader2 = static_cast<m_IDirect3DVertexShader9*>(pShader);
            if(pShader2 && (pShader2->id == 553 || pShader2->id == 554)) {
                pTexture = pRainDropsRefractionHDRTex;
            }
        }
    }
    // fix from AssaultKifle47
    //if(gFixRainDrops && Stage == 1 && pTexture == 0 && DWORD(pRainDropsRefractionHDRTex)>0x4) {
    //    pTexture = pRainDropsRefractionHDRTex;
    //}
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

int getBinShaderSize(CONST DWORD* pFunction) {
    unsigned int sz = 0;
    while(pFunction[sz] != 65535) {
        sz++;
    }
    return sz+1;
}



HRESULT m_IDirect3DDevice9Ex::CreatePixelShader(THIS_ CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) {
    HRESULT hr = ProxyInterface->CreatePixelShader(pFunction, ppShader);

    if(SUCCEEDED(hr) && ppShader) {

        *ppShader = new m_IDirect3DPixelShader9(*ppShader, this, SC_FXC);
        if((*ppShader) && (*ppShader) != ImGuiPS)
            ps_4.push_back(static_cast<m_IDirect3DPixelShader9*>(*ppShader));
        //unsigned int sz = 0;
        //unsigned int sz2 = getBinShaderSize(pFunction);
        //(*ppShader)->GetFunction(0, &sz);
        //sz = sz / 4;
        //if(sz != sz2) {
        //    printf("%i %i", sz, sz2);
        //}
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
        if((*ppShader) && (*ppShader) != ImGuiVS)
            vs_4.push_back(static_cast<m_IDirect3DVertexShader9*>(*ppShader));
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
            if(UsePostFxAA) {
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

    static bool firstShader = true;
    if(firstShader) {
        firstShader = false;
        if(!downsampler_ps)
            downsampler_ps = CompilePixelShaderFromFile("SSAO3_ps.hlsl", "mainDownsample", "SSAO3_Downsample_ps", this, false, true);
        if(!SSAO_vs)
            SSAO_vs = CompileVertexShaderFromFile("SSAO_vs.asm", 0, "SSAO_vs.asm", this, true, true);

        if(!SMAA_EdgeDetection)
            SMAA_EdgeDetection = CompilePixelShaderFromFile("SMAA2.hlsl", "SMAAColorEdgeDetectionPS", "SMAAColorEdgeDetectionPS", this, false, true);
        if(!SMAA_BlendingWeightsCalculation)
            SMAA_BlendingWeightsCalculation = CompilePixelShaderFromFile("SMAA2.hlsl", "SMAABlendingWeightCalculationPS", "SMAABlendingWeightCalculationPS", this, false, true);
        if(!SMAA_NeighborhoodBlending)
            SMAA_NeighborhoodBlending = CompilePixelShaderFromFile("SMAA2.hlsl", "SMAANeighborhoodBlendingPS", "SMAANeighborhoodBlendingPS", this, false, true);

        if(!SMAA_EdgeDetectionVS)
            SMAA_EdgeDetectionVS = CompileVertexShaderFromFile("SMAA2.hlsl", "SMAAEdgeDetectionVS", "SMAAEdgeDetectionVS", this, false, true);
        if(!SMAA_BlendingWeightsCalculationVS)
            SMAA_BlendingWeightsCalculationVS = CompileVertexShaderFromFile("SMAA2.hlsl", "SMAABlendingWeightCalculationVS", "SMAABlendingWeightCalculationVS", this, false, true);
        if(!SMAA_NeighborhoodBlendingVS)
            SMAA_NeighborhoodBlendingVS = CompileVertexShaderFromFile("SMAA2.hlsl", "SMAANeighborhoodBlendingVS", "SMAANeighborhoodBlendingVS", this, false, true);

        if(!DOF_ps)
            DOF_ps = CompilePixelShaderFromFile("DOF_ps.hlsl", "main", "DOF_ps.hlsl", this, false, true);

        if(0 && (!SMAA_EdgeDetection || !SMAA_BlendingWeightsCalculation || !SMAA_NeighborhoodBlending ||
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

        if(0 && (!DOF_ps)) {
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
    }
    return hr;
}

int bReversedDepth = 0;

HRESULT m_IDirect3DDevice9Ex::Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    if(bReversedDepth) {
        if(Z == 1.f)
            Z = 0.f;
    }
    return ProxyInterface->Clear(Count, pRects, Flags, Color, Z, Stencil);
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

    if(bReversedDepth != 0 && State == D3DRS_ZFUNC) {
        if(Value == D3DCMP_GREATER) {
            Value = D3DCMP_LESS;
        }
        if(Value == D3DCMP_GREATEREQUAL) {
            Value = D3DCMP_LESSEQUAL;
        }
        if(Value == D3DCMP_LESS) {
            Value = D3DCMP_GREATER;
        }
        if(Value == D3DCMP_LESSEQUAL) {
            Value = D3DCMP_GREATEREQUAL;
        }
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


#define PostfxTextureCount 15
IDirect3DBaseTexture9* prePostFx[PostfxTextureCount] = { 0 };
DWORD Samplers[PostfxTextureCount] = { D3DTEXF_LINEAR };
extern bool usePrimitiveUp;

 bool useDof = false;
 int UseSunShafts = 1;
 int SS_NumSamples[4] = { 20, 20, 20, 20 };
 int useDepthOfField = 3;
 bool useStippleFilter = true;
 bool useNewShadowAtlas = false;

extern float PostFxFog[4];
float NoiseSale[4] = { 1.f / 256, 0.3, -0.5, 0 };

float BilateralDepthTreshold[4] = { 0.003, 0.002, 0.004, 0.005 };

extern bool fixDistantOutlineUsingDXVK;
extern bool useMotionBlur;
int DofSamples[4] = { 20 };
bool downsampleStencil = 0;
int DrawCallsCount = 0;
bool useLinear = 1;

// 0 off, 1 horizontal, 2 vertical, 3 horizontal e vertical.
int useDefferedShadows = 0;



void swapbuffers() {
    auto temptex = renderTargetTex;
    renderTargetTex = textureRead;
    textureRead = temptex;

    auto tempsurf = renderTargetSurf;
    renderTargetSurf = surfaceRead;
    surfaceRead = tempsurf;
}

static HRESULT PostFx3(m_IDirect3DDevice9Ex* hk_device, LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE PrimitiveType,
                UINT StartVertex, UINT PrimitiveCount, IDirect3DPixelShader9* pShader) {
    m_IDirect3DPixelShader9* pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);

    IDirect3DVertexShader9* vShader = nullptr;
    pDevice->GetVertexShader(&vShader);
    //m_IDirect3DVertexShader9* vShader2 = static_cast<m_IDirect3DVertexShader9*>(vShader);

    IDirect3DSurface9* pHDRSurface = nullptr;
    IDirect3DSurface9* pHDRSurface2 = nullptr;
    IDirect3DSurface9* pHDRSurface3 = nullptr;
    IDirect3DSurface9* pHDRDownsampleSurf = nullptr;
    IDirect3DSurface9* pHDRDownsampleSurf2 = nullptr;
    IDirect3DSurface9* backBuffer = nullptr;
    IDirect3DSurface9* edgesSurf = nullptr;
    IDirect3DSurface9* blendSurf = nullptr;
    IDirect3DSurface9* stencilDownsampledSurf = nullptr;
    IDirect3DSurface9* ppZStencilSurface = nullptr;
    IDirect3DSurface9* ppZStencilSurface2 = nullptr;
    IDirect3DSurface9* aoSurface = nullptr;

    D3DVOLUME_DESC volumeDescription;

    HRESULT hr = S_FALSE;

    DWORD OldSRGB = 0;
    DWORD OldSampler = 0;

    static float vec4[4] = { 0.f };
    static float blueTimerVec4[4] = { 0.f };
    static int blueTimer = 0;

    // SetPixelShaderConstantI only set the first value, so I need 4 int4 to set 1 int4
    int SS_NumSamplesa[4] = { SS_NumSamples[0], 0, 1, 0 };
    int SS_NumSamplesb[4] = { SS_NumSamples[1], 0, 1, 0 };
    int SS_NumSamplesc[4] = { SS_NumSamples[2], 0, 1, 0 };
    int SS_NumSamplesd[4] = { SS_NumSamples[3], 0, 1, 0 };

    int numPasses = useStippleFilter + UseSSAO + useDepthOfField + useDof + (UsePostFxAA[0] > 0);
    static float SS_NumSamples2[4] = { 0.f };
    SS_NumSamples2[0] = SS_NumSamples[0];
    SS_NumSamples2[1] = SS_NumSamples[1];
    SS_NumSamples2[2] = SS_NumSamples[2];
    SS_NumSamples2[3] = SS_NumSamples[3];

    // save render state
    for(int i = 0; i < PostfxTextureCount; i++) {
        pDevice->GetTexture(i, &prePostFx[i]);
        pDevice->GetSamplerState(i, D3DSAMP_MAGFILTER, &Samplers[i]);
    }

    if(bluenoisevolume) {
        bluenoisevolume->GetLevelDesc(0, &volumeDescription);

        blueTimer++;
        blueTimer = blueTimer % (volumeDescription.Depth + 1);
        blueTimerVec4[0] = gWindowWidth / float(volumeDescription.Width);
        blueTimerVec4[1] = gWindowHeight / float(volumeDescription.Height);
        blueTimerVec4[2] = blueTimer / float(volumeDescription.Depth);
        blueTimerVec4[3] = blueTimer;
    }

    pDevice->SetPixelShaderConstantF(218, blueTimerVec4, 1);
    pDevice->SetPixelShaderConstantF(144, NoiseSale, 1);
    pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHalfHDRTex)->GetProxyInterface());
    if(bluenoisevolume)
        pDevice->SetTexture(9, ((m_IDirect3DTexture9*) bluenoisevolume)->GetProxyInterface());

    if(UseSSAA) {
        hk_device->GetSamplerState(0, D3DSAMP_MAGFILTER, &OldSampler);
        hk_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    }

    // main postfx passes
    {
        IDirect3DTexture9* mainTex = 0;
        if(pHDRTex2 && pHDRTex && aoTex) {
            renderTargetTex = pHDRTex2;
            hk_device->GetTexture(2, (IDirect3DBaseTexture9**) &mainTex);
            textureRead = mainTex;

            pDevice->GetRenderTarget(0, &backBuffer);
            renderTargetTex->GetSurfaceLevel(0, &renderTargetSurf);
            textureRead->GetSurfaceLevel(0, &surfaceRead);
            aoTex->GetSurfaceLevel(0, &aoSurface);
            pHDRTex3->GetSurfaceLevel(0, &pHDRSurface3);
            IDirect3DSurface9* pShadowBlurSurf1 = nullptr;
            IDirect3DSurface9* pShadowBlurSurf2 = nullptr;

            pShadowBlurTex1->GetSurfaceLevel(0, &pShadowBlurSurf1);
            pShadowBlurTex2->GetSurfaceLevel(0, &pShadowBlurSurf2);


            // ready for new post processing?
            if(backBuffer && renderTargetSurf && surfaceRead && pShadowBlurSurf1 && pShadowBlurSurf2) {
                if(UseSSAO && SSAO_ps) {
                    if(UseSSAA) {
                        pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                    }

                    if(downsampleStencil) {
                        stencilDownsampled->GetSurfaceLevel(0, &stencilDownsampledSurf);
                        hk_device->SetRenderTarget(0, stencilDownsampledSurf);
                        hk_device->SetPixelShader(downsampler_ps);
                        hk_device->Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);
                        hr = pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        pDevice->SetPixelShader(pShader2->GetProxyInterface());
                        SAFE_RELEASE(stencilDownsampledSurf);
                    }

                    hk_device->SetPixelShader(SSAO_ps);
                    vec4[0] = ((gWindowWidth * 1) * (UseSSAA ? ResSSAA : 1));
                    vec4[1] = ((gWindowHeight * 1) * (UseSSAA ? ResSSAA : 1));
                    vec4[2] = 1.f / ((gWindowWidth * 1) * (UseSSAA ? ResSSAA : 1));
                    vec4[3] = 1.f / ((gWindowHeight * 1) * (UseSSAA ? ResSSAA : 1));
                    pDevice->SetPixelShaderConstantF(210, vec4, 1);
                    vec4[1] = AoDistance;
                    pDevice->SetPixelShaderConstantF(211, vec4, 1);
                    //pDevice->SetTexture(2, 0);
                    hk_device->SetRenderTarget(0, pShadowBlurSurf1);
                    pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                    pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                    pDevice->SetTexture(3, 0);

                    //hk_device->SetPixelShader(DeferredShadowBlur1_ps);
                    pDevice->SetPixelShaderConstantF(140, BilateralDepthTreshold, 1);
                    //hk_device->SetRenderTarget(0, pShadowBlurSurf2);
                    //pDevice->SetTexture(11, static_cast<m_IDirect3DTexture9*>(pShadowBlurTex1)->GetProxyInterface());
                    //pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                    //hk_device->SetPixelShader(DeferredShadowBlur2_ps);
                    //hk_device->SetRenderTarget(0, pShadowBlurSurf1);
                    //pDevice->SetTexture(11, static_cast<m_IDirect3DTexture9*>(pShadowBlurTex2)->GetProxyInterface());
                    //pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                    hk_device->SetPixelShader(DeferredShadowBlur3_ps);
                    hk_device->SetRenderTarget(0, pShadowBlurSurf2);
                    pDevice->SetTexture(11, static_cast<m_IDirect3DTexture9*>(pShadowBlurTex1)->GetProxyInterface());
                    pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                    pDevice->SetTexture(11, 0);

                    hk_device->SetPixelShader(SSAO_ps2);
                    hk_device->SetRenderTarget(0, renderTargetSurf);
                    switch(UseDebugTextures) {
                        //"None", 
                        //"Difuse", 
                        //"normal",
                        //"specular", 
                        //"depth", 
                        //"SMAA edges", 
                        //"stencil"
                        case 0:
                            pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                            break;
                        case 1:
                            pDevice->SetTexture(2, DiffuseTex->GetProxyInterface());
                            break;
                        case 2:
                            pDevice->SetTexture(2, NormalTex->GetProxyInterface());
                            break;
                        case 3:
                            pDevice->SetTexture(2, BloomTex->GetProxyInterface());
                            break;
                        case 4:
                            pDevice->SetTexture(2, SpecularTex->GetProxyInterface());
                            break;
                        case 5:
                            pDevice->SetTexture(2, DepthTex->GetProxyInterface());
                            break;
                        case 6:
                            pDevice->SetTexture(2, StencilTex->GetProxyInterface());
                            break;
                        default:
                            pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                            break;
                    }
                    pDevice->SetTexture(3, ((m_IDirect3DTexture9*) pShadowBlurTex2)->GetProxyInterface());
                    pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                    swapbuffers();
                    pDevice->SetTexture(1, prePostFx[1]);
                    pDevice->SetTexture(3, prePostFx[3]);
                    pDevice->SetTexture(11, prePostFx[11]);
                    //pDevice->SetTexture(2, 0);
                    pDevice->SetPixelShader(pShader2->GetProxyInterface());
                }

                if(useDepthOfField == 1 && depth_of_field_ps) {
                    hk_device->SetPixelShader(depth_of_field_ps);
                    hk_device->SetRenderTarget(0, renderTargetSurf);
                    pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                    pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                    swapbuffers();
                    //pDevice->SetTexture(2, 0);
                    pDevice->SetPixelShader(pShader2->GetProxyInterface());
                }

                if(useDepthOfField == 2 && depth_of_field_blur_ps && depth_of_field_tent_ps && depth_of_field_coc_ps) {
                    pDevice->GetDepthStencilSurface(&ppZStencilSurface);
                    depthStenciltex->GetSurfaceLevel(0, &ppZStencilSurface2);
                    pHDRDownsampleTex->GetSurfaceLevel(0, &pHDRDownsampleSurf);
                    pHDRDownsampleTex2->GetSurfaceLevel(0, &pHDRDownsampleSurf2);

                    if(ppZStencilSurface && ppZStencilSurface2 && pHDRDownsampleSurf && pHDRDownsampleSurf2

                       ) {
                        hk_device->SetSamplerState(8, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(8, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(8, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

                        pDevice->SetDepthStencilSurface(((m_IDirect3DSurface9*) ppZStencilSurface2)->GetProxyInterface());

                        hk_device->SetPixelShader(depth_of_field_blur_ps);
                        hk_device->SetRenderTarget(0, pHDRDownsampleSurf2);
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        hk_device->SetPixelShader(depth_of_field_tent_ps);
                        hk_device->SetRenderTarget(0, pHDRDownsampleSurf);
                        pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHDRDownsampleTex2)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        hk_device->SetPixelShader(depth_of_field_coc_ps);
                        pDevice->SetDepthStencilSurface(ppZStencilSurface);
                        hk_device->SetRenderTarget(0, renderTargetSurf);
                        pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHDRDownsampleTex)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        swapbuffers();

                        pDevice->SetPixelShader(pShader2->GetProxyInterface());
                        SAFE_RELEASE(pHDRDownsampleSurf);
                        SAFE_RELEASE(pHDRDownsampleSurf2);

                        SAFE_RELEASE(ppZStencilSurface);
                        SAFE_RELEASE(ppZStencilSurface2);
                    }

                }

                if(useDepthOfField == 3 && dof_blur_ps && dof_coc_ps) {
                    pDevice->GetDepthStencilSurface(&ppZStencilSurface);
                    depthStenciltex->GetSurfaceLevel(0, &ppZStencilSurface2);
                    pHDRDownsampleTex->GetSurfaceLevel(0, &pHDRDownsampleSurf);
                    pHDRDownsampleTex2->GetSurfaceLevel(0, &pHDRDownsampleSurf2);

                    if(ppZStencilSurface && ppZStencilSurface2 && pHDRDownsampleSurf && pHDRDownsampleSurf2

                       ) {
                        hk_device->SetSamplerState(8, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(8, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(8, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

                        pDevice->SetDepthStencilSurface(((m_IDirect3DSurface9*) ppZStencilSurface2)->GetProxyInterface());

                        hk_device->SetPixelShader(dof_blur_ps);
                        hk_device->SetRenderTarget(0, pHDRDownsampleSurf);
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHalfHDRTex)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        hk_device->SetPixelShader(dof_coc_ps);
                        pDevice->SetDepthStencilSurface(ppZStencilSurface);
                        hk_device->SetRenderTarget(0, renderTargetSurf);
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHDRDownsampleTex)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        swapbuffers();
                        pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHalfHDRTex)->GetProxyInterface());

                        pDevice->SetPixelShader(pShader2->GetProxyInterface());
                        SAFE_RELEASE(pHDRDownsampleSurf);
                        SAFE_RELEASE(pHDRDownsampleSurf2);

                        SAFE_RELEASE(ppZStencilSurface);
                        SAFE_RELEASE(ppZStencilSurface2);
                    }

                }

                if(useDepthOfField == 4 && DOF_ps) {
                    numPasses--;
                    hk_device->SetPixelShader(DOF_ps);
                    pDevice->SetPixelShaderConstantF(212, FocusPoint, 1);
                    pDevice->SetPixelShaderConstantF(213, FocusScale, 1);
                    pDevice->SetPixelShaderConstantI(4, DofSamples, 1);
                    //pDevice->SetTexture(2, 0);

                    hk_device->SetRenderTarget(0, renderTargetSurf);

                    pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                    pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                    swapbuffers();
                    //pDevice->SetTexture(2, 0);
                    pDevice->SetPixelShader(pShader2->GetProxyInterface());
                }

                if(useMotionBlur && motionblur_ps) {
                    hk_device->SetPixelShader(motionblur_ps);
                    hk_device->SetRenderTarget(0, renderTargetSurf);
                    pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                    pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                    swapbuffers();
                    pDevice->SetPixelShader(pShader2->GetProxyInterface());
                }

                if(useStippleFilter && stipple_filter_ps) {
                    hk_device->SetPixelShader(stipple_filter_ps);
                    hk_device->SetRenderTarget(0, renderTargetSurf);
                    pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                    pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                    swapbuffers();
                    pDevice->SetPixelShader(pShader2->GetProxyInterface());
                }

                // fxaa / smaa
                if(UsePostFxAA[0] > 0) {
                    numPasses--;
                    if(UseSSAA) {
                        pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                    }

                    // FXAA
                    if((UsePostFxAA[0] == 1) && FxaaPS) {
                        pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &OldSRGB); // save srgb state
                        hk_device->SetPixelShader(FxaaPS);
                        pDevice->SetPixelShaderConstantF(214, parametersAA, 1); // need to edit FXAA shader to be set

                        hk_device->SetRenderTarget(0, renderTargetSurf);

                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        hr = pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        swapbuffers();
                        pDevice->SetSamplerState(2, D3DSAMP_SRGBTEXTURE, 0);
                        //pDevice->SetTexture(2, 0);
                        pDevice->SetPixelShader(pShader2->GetProxyInterface());
                        pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, OldSRGB);// restore srgb state
                    }

                    // SMAA
                    if(UsePostFxAA[0] > 1 &&
                       SMAA_EdgeDetection && SMAA_BlendingWeightsCalculation && SMAA_NeighborhoodBlending &&
                       SMAA_EdgeDetectionVS && SMAA_BlendingWeightsCalculationVS && SMAA_NeighborhoodBlendingVS &&
                       areaTex && searchTex && edgesTex && blendTex
                       ) {
                        DWORD oldSample = 0;
                        vec4[0] = 1.f / (gWindowWidth * (UseSSAA ? ResSSAA : 1));
                        vec4[1] = 1.f / (gWindowHeight * (UseSSAA ? ResSSAA : 1));
                        vec4[2] = (gWindowWidth * (UseSSAA ? ResSSAA : 1));
                        vec4[3] = (gWindowHeight * (UseSSAA ? ResSSAA : 1));
                        pDevice->SetPixelShaderConstantF(24, vec4, 1);
                        pDevice->SetPixelShaderConstantF(24, vec4, 1);
                        pDevice->SetVertexShaderConstantF(24, vec4, 1);
                        edgesTex->GetSurfaceLevel(0, &edgesSurf);
                        blendTex->GetSurfaceLevel(0, &blendSurf);

                        // SMAA_EdgeDetection
                        pDevice->GetRenderState(D3DRS_SRGBWRITEENABLE, &OldSRGB); // save srgb state
                        pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, 0);
                        hk_device->SetPixelShader(SMAA_EdgeDetection);
                        hk_device->SetVertexShader(SMAA_EdgeDetectionVS);
                        pDevice->SetPixelShaderConstantF(24, vec4, 1);
                        pDevice->SetVertexShaderConstantF(24, vec4, 1);
                        hk_device->SetRenderTarget(0, edgesSurf);
                        //pDevice->SetTexture(2, 0);
                        pDevice->SetTexture(0, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->GetSamplerState(3, D3DSAMP_MAGFILTER, &oldSample);
                        pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        pDevice->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        pDevice->SetSamplerState(3, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
                        pDevice->SetSamplerState(4, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        pDevice->Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);
                        pDevice->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 1);
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        pDevice->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);
                        //pDevice->SetTexture(0, 0);

                        // SMAA_BlendingWeightsCalculation
                        hk_device->SetPixelShader(SMAA_BlendingWeightsCalculation);
                        hk_device->SetVertexShader(SMAA_BlendingWeightsCalculationVS);
                        pDevice->SetPixelShaderConstantF(24, vec4, 1);
                        pDevice->SetVertexShaderConstantF(24, vec4, 1);
                        hk_device->SetRenderTarget(0, blendSurf);
                        pDevice->SetTexture(1, ((m_IDirect3DTexture9*) edgesTex)->GetProxyInterface());
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) areaTex)->GetProxyInterface());
                        pDevice->SetTexture(3, ((m_IDirect3DTexture9*) searchTex)->GetProxyInterface());
                        pDevice->Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        //pDevice->SetTexture(0, 0);
                        //pDevice->SetTexture(1, 0);
                        //pDevice->SetTexture(2, 0);
                        //pDevice->SetTexture(3, 0);
                        pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, OldSRGB);// restore srgb state

                        // SMAA_NeighborhoodBlending
                        hk_device->SetPixelShader(SMAA_NeighborhoodBlending);
                        hk_device->SetVertexShader(SMAA_NeighborhoodBlendingVS);
                        pDevice->SetPixelShaderConstantF(24, vec4, 1);
                        pDevice->SetVertexShaderConstantF(24, vec4, 1);
                        vec4[0] = UsePostFxAA[0];
                        vec4[1] = 1;
                        vec4[2] = 3;
                        vec4[3] = 4;
                        pDevice->SetPixelShaderConstantF(5, vec4, 1);

                        hk_device->SetRenderTarget(0, renderTargetSurf);

                        //pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->SetTexture(0, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->SetTexture(1, ((m_IDirect3DTexture9*) edgesTex)->GetProxyInterface());
                        pDevice->SetTexture(4, ((m_IDirect3DTexture9*) blendTex)->GetProxyInterface());
                        pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, 0);
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        swapbuffers();
                        pDevice->SetTexture(0, prePostFx[0]);
                        pDevice->SetTexture(1, prePostFx[1]);
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->SetTexture(3, prePostFx[3]);
                        pDevice->SetTexture(4, prePostFx[4]);
                        pDevice->SetSamplerState(3, D3DSAMP_MAGFILTER, oldSample);
                        pDevice->SetPixelShader(pShader2->GetProxyInterface());
                        pDevice->SetVertexShader(vShader);
                        SAFE_RELEASE(edgesSurf);
                        SAFE_RELEASE(blendSurf);
                    }
                }

                // one pass half res
                if(UseSunShafts == 1 && SSDownsampler_PS && depth_of_field_tent_ps && SunShafts_PS) {
                    pDevice->GetDepthStencilSurface(&ppZStencilSurface);
                    depthStenciltex->GetSurfaceLevel(0, &ppZStencilSurface2);
                    pHDRDownsampleTex->GetSurfaceLevel(0, &pHDRDownsampleSurf);
                    pHDRDownsampleTex2->GetSurfaceLevel(0, &pHDRDownsampleSurf2);

                    if(ppZStencilSurface && ppZStencilSurface2 && pHDRDownsampleSurf && pHDRDownsampleSurf2) {
                        {
                            hk_device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                            hk_device->SetSamplerState(8, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                            hk_device->SetSamplerState(8, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                            hk_device->SetSamplerState(8, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                            hk_device->SetSamplerState(9, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                            hk_device->SetSamplerState(9, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                            hk_device->SetSamplerState(9, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                            hk_device->SetSamplerState(11, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                            hk_device->SetSamplerState(11, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
                            hk_device->SetSamplerState(11, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
                            hk_device->SetSamplerState(13, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                            hk_device->SetSamplerState(13, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                            hk_device->SetSamplerState(13, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);

                            pDevice->SetPixelShaderConstantF(96, SS_params, 1);
                            pDevice->SetPixelShaderConstantF(99, SS_params2, 1);
                            pDevice->SetPixelShaderConstantF(97, SunDirection, 1);
                            pDevice->SetPixelShaderConstantF(98, SunColor, 1);
                            pDevice->SetPixelShaderConstantF(100, gWorld, 4);
                            pDevice->SetPixelShaderConstantF(104, gWorldView, 4);
                            pDevice->SetPixelShaderConstantF(108, gWorldViewProj, 4);
                            pDevice->SetPixelShaderConstantF(112, gViewInverse, 4);
                            pDevice->SetPixelShaderConstantF(116, gShadowMatrix, 4);
                            pDevice->SetPixelShaderConstantF(100, &mtx[0], mtx.size() / 4);
                            pDevice->SetPixelShaderConstantF(120, SS_NumSamples2, 1);
                            pDevice->SetPixelShaderConstantI(5, SS_NumSamplesa, 1);
                            pDevice->SetPixelShaderConstantI(6, SS_NumSamplesb, 1);
                            pDevice->SetPixelShaderConstantI(7, SS_NumSamplesc, 1);
                            pDevice->SetPixelShaderConstantI(8, SS_NumSamplesd, 1);
                        }

                        pDevice->SetPixelShaderConstantF(218, blueTimerVec4, 1);
                        pDevice->SetPixelShaderConstantF(144, NoiseSale, 1);
                        pDevice->SetDepthStencilSurface(((m_IDirect3DSurface9*) ppZStencilSurface2)->GetProxyInterface());

                        pDevice->SetTexture(13, 0);
                        pDevice->SetTexture(11, 0);
                        pDevice->SetTexture(8, 0);

                        hk_device->SetPixelShader(SSDownsampler_PS);
                        hk_device->SetRenderTarget(0, pHDRDownsampleSurf2);
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->SetTexture(13, ((m_IDirect3DTexture9*) DiffuseTex)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        hk_device->SetPixelShader(depth_of_field_tent_ps);
                        hk_device->SetRenderTarget(0, pHDRDownsampleSurf);
                        pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHDRDownsampleTex2)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        hk_device->SetPixelShader(SunShafts_PS);
                        hk_device->SetRenderTarget(0, pHDRDownsampleSurf2);
                        pDevice->SetTexture(8, 0);
                        pDevice->SetTexture(11, ((m_IDirect3DTexture9*) pHDRDownsampleTex)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        pDevice->SetTexture(8, 0);
                        pDevice->SetTexture(11, 0);
                        pDevice->SetTexture(13, 0);

                        hk_device->SetPixelShader(SSAdd_PS);
                        pDevice->SetDepthStencilSurface(ppZStencilSurface);
                        hk_device->SetRenderTarget(0, renderTargetSurf);
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHalfHDRTex)->GetProxyInterface());
                        pDevice->SetTexture(11, ((m_IDirect3DTexture9*) pHDRDownsampleTex2)->GetProxyInterface());
                        if(bluenoisevolume)
                            pDevice->SetTexture(9, ((m_IDirect3DTexture9*) bluenoisevolume)->GetProxyInterface());

                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        swapbuffers();
                        pDevice->SetTexture(13, 0);
                        pDevice->SetTexture(11, 0);
                        pDevice->SetTexture(8, 0);

                        pDevice->SetPixelShader(pShader2->GetProxyInterface());
                        SAFE_RELEASE(pHDRDownsampleSurf);
                        SAFE_RELEASE(pHDRDownsampleSurf2);

                        SAFE_RELEASE(ppZStencilSurface);
                        SAFE_RELEASE(ppZStencilSurface2);
                    }
                }

                // 2 passes half res
                if(UseSunShafts == 2 && SSDownsampler_PS && depth_of_field_tent_ps && SunShafts_PS && SunShafts2_PS) {
                    pDevice->GetDepthStencilSurface(&ppZStencilSurface);
                    depthStenciltex->GetSurfaceLevel(0, &ppZStencilSurface2);
                    pHDRDownsampleTex->GetSurfaceLevel(0, &pHDRDownsampleSurf);
                    pHDRDownsampleTex2->GetSurfaceLevel(0, &pHDRDownsampleSurf2);

                    if(ppZStencilSurface && ppZStencilSurface2 && pHDRDownsampleSurf && pHDRDownsampleSurf2) {
                        hk_device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(8, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(8, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(8, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(9, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(9, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(9, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(11, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(11, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
                        hk_device->SetSamplerState(11, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
                        hk_device->SetSamplerState(13, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(13, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(13, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);

                        pDevice->SetPixelShaderConstantF(96, SS_params, 1);
                        pDevice->SetPixelShaderConstantF(99, SS_params2, 1);
                        pDevice->SetPixelShaderConstantF(97, SunDirection, 1);
                        pDevice->SetPixelShaderConstantF(98, SunColor, 1);
                        pDevice->SetPixelShaderConstantF(100, gWorld, 4);
                        pDevice->SetPixelShaderConstantF(104, gWorldView, 4);
                        pDevice->SetPixelShaderConstantF(108, gWorldViewProj, 4);
                        pDevice->SetPixelShaderConstantF(112, gViewInverse, 4);
                        pDevice->SetPixelShaderConstantF(116, gShadowMatrix, 4);
                        pDevice->SetPixelShaderConstantF(100, &mtx[0], mtx.size() / 4);
                        pDevice->SetPixelShaderConstantF(120, SS_NumSamples2, 1);
                        pDevice->SetPixelShaderConstantI(5, SS_NumSamplesa, 1);
                        pDevice->SetPixelShaderConstantI(6, SS_NumSamplesb, 1);
                        pDevice->SetPixelShaderConstantI(7, SS_NumSamplesc, 1);
                        pDevice->SetPixelShaderConstantI(8, SS_NumSamplesd, 1);

                        pDevice->SetPixelShaderConstantF(218, blueTimerVec4, 1);
                        pDevice->SetPixelShaderConstantF(144, NoiseSale, 1);
                        pDevice->SetDepthStencilSurface(((m_IDirect3DSurface9*) ppZStencilSurface2)->GetProxyInterface());

                        pDevice->SetTexture(13, 0);
                        pDevice->SetTexture(11, 0);
                        pDevice->SetTexture(8, 0);

                        hk_device->SetPixelShader(SSDownsampler_PS);
                        hk_device->SetRenderTarget(0, pHDRDownsampleSurf2);
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->SetTexture(13, ((m_IDirect3DTexture9*) DiffuseTex)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        hk_device->SetPixelShader(depth_of_field_tent_ps);
                        hk_device->SetRenderTarget(0, pHDRDownsampleSurf);
                        pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHDRDownsampleTex2)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        hk_device->SetPixelShader(SunShafts_PS);
                        hk_device->SetRenderTarget(0, pHDRDownsampleSurf2);
                        pDevice->SetTexture(8, 0);
                        pDevice->SetTexture(11, ((m_IDirect3DTexture9*) pHDRDownsampleTex)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        hk_device->SetPixelShader(SunShafts2_PS);
                        hk_device->SetRenderTarget(0, pHDRDownsampleSurf);
                        pDevice->SetTexture(11, ((m_IDirect3DTexture9*) pHDRDownsampleTex2)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        pDevice->SetTexture(8, 0);
                        pDevice->SetTexture(11, 0);
                        pDevice->SetTexture(13, 0);

                        hk_device->SetPixelShader(SSAdd_PS);
                        pDevice->SetDepthStencilSurface(ppZStencilSurface);
                        hk_device->SetRenderTarget(0, renderTargetSurf);
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHalfHDRTex)->GetProxyInterface());
                        pDevice->SetTexture(11, ((m_IDirect3DTexture9*) pHDRDownsampleTex)->GetProxyInterface());
                        if(bluenoisevolume)
                            pDevice->SetTexture(9, ((m_IDirect3DTexture9*) bluenoisevolume)->GetProxyInterface());

                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        swapbuffers();
                        pDevice->SetTexture(13, 0);
                        pDevice->SetTexture(11, 0);
                        pDevice->SetTexture(8, 0);

                        pDevice->SetPixelShader(pShader2->GetProxyInterface());
                        SAFE_RELEASE(pHDRDownsampleSurf);
                        SAFE_RELEASE(pHDRDownsampleSurf2);

                        SAFE_RELEASE(ppZStencilSurface);
                        SAFE_RELEASE(ppZStencilSurface2);
                    }
                }

                // full screen
                if(UseSunShafts == 3 && depth_of_field_blur_ps && depth_of_field_tent_ps && depth_of_field_coc_ps && SunShafts3_PS) {
                    pDevice->GetDepthStencilSurface(&ppZStencilSurface);
                    depthStenciltex->GetSurfaceLevel(0, &ppZStencilSurface2);
                    pHDRDownsampleTex->GetSurfaceLevel(0, &pHDRDownsampleSurf);
                    pHDRDownsampleTex2->GetSurfaceLevel(0, &pHDRDownsampleSurf2);

                    if(ppZStencilSurface && ppZStencilSurface2 && pHDRDownsampleSurf && pHDRDownsampleSurf2 && pHDRSurface3) {
                        hk_device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(8, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(8, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(8, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(9, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(9, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(9, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(11, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(11, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(11, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(13, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(13, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(13, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);

                        hk_device->SetPixelShader(SSDownsampler2_PS);
                        hk_device->SetRenderTarget(0, pHDRSurface3);
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                        pDevice->SetTexture(13, ((m_IDirect3DTexture9*) DiffuseTex)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        hk_device->SetPixelShader(SunShafts3_PS);
                        hk_device->SetRenderTarget(0, renderTargetSurf);

                        pDevice->SetPixelShaderConstantF(96, SS_params, 1);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(99, SS_params2, 1);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(97, SunDirection, 1);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(98, SunColor, 1);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(100, gWorld, 4);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(104, gWorldView, 4);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(108, gWorldViewProj, 4);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(112, gViewInverse, 4);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(116, gShadowMatrix, 4);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(100, &mtx[0], mtx.size() / 4);//   [4] = { 0.f };

                        pDevice->SetPixelShaderConstantF(120, SS_NumSamples2, 1);
                        pDevice->SetPixelShaderConstantI(5, SS_NumSamplesa, 1);
                        pDevice->SetPixelShaderConstantI(6, SS_NumSamplesb, 1);
                        pDevice->SetPixelShaderConstantI(7, SS_NumSamplesc, 1);
                        pDevice->SetPixelShaderConstantI(8, SS_NumSamplesd, 1);

                        pDevice->SetPixelShaderConstantF(218, blueTimerVec4, 1);
                        pDevice->SetPixelShaderConstantF(144, NoiseSale, 1);
                        //pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHalfHDRTex)->GetProxyInterface());
                        pDevice->SetTexture(11, ((m_IDirect3DTexture9*) pHDRTex3)->GetProxyInterface());
                        if(bluenoisevolume)
                            pDevice->SetTexture(9, ((m_IDirect3DTexture9*) bluenoisevolume)->GetProxyInterface());

                        //pDevice->SetTexture(11, ((m_IDirect3DTexture9*) pHDRDownsampleTex)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        swapbuffers();

                        pDevice->SetPixelShader(pShader2->GetProxyInterface());
                        SAFE_RELEASE(pHDRDownsampleSurf);
                        SAFE_RELEASE(pHDRDownsampleSurf2);

                        SAFE_RELEASE(ppZStencilSurface);
                        SAFE_RELEASE(ppZStencilSurface2);
                    }
                }
                
                // full screen 2 passes
                if(UseSunShafts == 4 && depth_of_field_blur_ps && depth_of_field_tent_ps && depth_of_field_coc_ps && SunShafts4_PS) {
                    pDevice->GetDepthStencilSurface(&ppZStencilSurface);
                    depthStenciltex->GetSurfaceLevel(0, &ppZStencilSurface2);
                    pHDRDownsampleTex->GetSurfaceLevel(0, &pHDRDownsampleSurf);
                    pHDRDownsampleTex2->GetSurfaceLevel(0, &pHDRDownsampleSurf2);

                    if(ppZStencilSurface && ppZStencilSurface2 && pHDRDownsampleSurf && pHDRDownsampleSurf2) {
                        hk_device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(8, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(8, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(8, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(9, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(9, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(9, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(11, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(11, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(11, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(13, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                        hk_device->SetSamplerState(13, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
                        hk_device->SetSamplerState(13, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);

                        if(!(useDepthOfField == 2)) {
                            pDevice->SetDepthStencilSurface(((m_IDirect3DSurface9*) ppZStencilSurface2)->GetProxyInterface());

                            hk_device->SetPixelShader(depth_of_field_blur_ps);
                            hk_device->SetRenderTarget(0, pHDRDownsampleSurf2);
                            pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                            pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                            hk_device->SetPixelShader(depth_of_field_tent_ps);
                            hk_device->SetRenderTarget(0, pHDRDownsampleSurf);
                            pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHDRDownsampleTex2)->GetProxyInterface());
                            pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                            pDevice->SetDepthStencilSurface(ppZStencilSurface);
                        }
                        hk_device->SetRenderTarget(0, renderTargetSurf);
                        pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());

                        hk_device->SetPixelShader(SunShafts4_PS);

                        pDevice->SetPixelShaderConstantF(96, SS_params, 1);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(99, SS_params2, 1);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(97, SunDirection, 1);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(98, SunColor, 1);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(100, gWorld, 4);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(104, gWorldView, 4);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(108, gWorldViewProj, 4);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(112, gViewInverse, 4);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(116, gShadowMatrix, 4);//   [4] = { 0.f };
                        pDevice->SetPixelShaderConstantF(100, &mtx[0], mtx.size() / 4);//   [4] = { 0.f };

                        pDevice->SetPixelShaderConstantF(120, SS_NumSamples2, 1);
                        pDevice->SetPixelShaderConstantI(5, SS_NumSamplesa, 1);
                        pDevice->SetPixelShaderConstantI(6, SS_NumSamplesb, 1);
                        pDevice->SetPixelShaderConstantI(7, SS_NumSamplesc, 1);
                        pDevice->SetPixelShaderConstantI(8, SS_NumSamplesd, 1);

                        pDevice->SetPixelShaderConstantF(218, blueTimerVec4, 1);
                        pDevice->SetPixelShaderConstantF(144, NoiseSale, 1);
                        pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHalfHDRTex)->GetProxyInterface());
                        if(bluenoisevolume)
                            pDevice->SetTexture(9, ((m_IDirect3DTexture9*) bluenoisevolume)->GetProxyInterface());

                        pDevice->SetTexture(11, ((m_IDirect3DTexture9*) pHDRDownsampleTex)->GetProxyInterface());
                        pDevice->SetTexture(13, ((m_IDirect3DTexture9*) DiffuseTex)->GetProxyInterface());
                        pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        swapbuffers();

                        pDevice->SetPixelShader(pShader2->GetProxyInterface());
                        SAFE_RELEASE(pHDRDownsampleSurf);
                        SAFE_RELEASE(pHDRDownsampleSurf2);

                        SAFE_RELEASE(ppZStencilSurface);
                        SAFE_RELEASE(ppZStencilSurface2);
                    }
                }

                // game postfx
                {
                    pDevice->SetRenderTarget(0, backBuffer);

                    for(int i = 0; i < 4; i++) {
                        if(prePostFx[i]) {
                            pDevice->SetTexture(i, prePostFx[i]);
                        }
                        pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
                    }
                    hk_device->SetSamplerState(8, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

                    pDevice->SetPixelShaderConstantF(218, blueTimerVec4, 1);
                    pDevice->SetPixelShaderConstantF(144, NoiseSale, 1);
                    pDevice->SetTexture(8, ((m_IDirect3DTexture9*) pHalfHDRTex)->GetProxyInterface());
                    if(bluenoisevolume)
                        pDevice->SetTexture(9, ((m_IDirect3DTexture9*) bluenoisevolume)->GetProxyInterface());

                    pDevice->SetTexture(2, ((m_IDirect3DTexture9*) textureRead)->GetProxyInterface());
                    hk_device->Clear(0, 0, D3DCLEAR_TARGET, 0, 0, 0);
                    hr = pDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                }
                SAFE_RELEASE(backBuffer);
                SAFE_RELEASE(renderTargetSurf);
                SAFE_RELEASE(surfaceRead);
                SAFE_RELEASE(aoSurface);
                SAFE_RELEASE(mainTex);
                SAFE_RELEASE(pHDRSurface3);

                if(UseSSAA) {
                    pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, OldSampler);
                }
                for(int i = 0; i < PostfxTextureCount; i++) {
                    if(prePostFx[i]) {
                        pDevice->SetTexture(i, prePostFx[i]);
                        //pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
                        SAFE_RELEASE(prePostFx[i]);
                    }
                    pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
                }

                return S_OK;
            }

            SAFE_RELEASE(backBuffer);
            SAFE_RELEASE(renderTargetSurf);
            SAFE_RELEASE(surfaceRead);
            SAFE_RELEASE(aoSurface);
            SAFE_RELEASE(mainTex);
            SAFE_RELEASE(pShadowBlurSurf1);
            SAFE_RELEASE(pShadowBlurSurf2);

            if(UseSSAA) {
                pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, OldSampler);
            }
            for(int i = 0; i < PostfxTextureCount; i++) {
                if(prePostFx[i]) {
                    pDevice->SetTexture(i, prePostFx[i]);
                    pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
                    SAFE_RELEASE(prePostFx[i]);
                }
                pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
            }

            return S_FALSE;
        }
    }

    for(int i = 0; i < PostfxTextureCount; i++) {
        if(prePostFx[i]) {
            pDevice->SetTexture(i, prePostFx[i]);
            pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, Samplers[i]);
            SAFE_RELEASE(prePostFx[i]);
        }
    }
    return S_FALSE;
}


HRESULT m_IDirect3DDevice9Ex::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
    DrawCallsCount++;
    HRESULT hr = S_FALSE;
    m_IDirect3DPixelShader9* pShader2 = nullptr;
    IDirect3DPixelShader9* pShader = nullptr;

    IDirect3DVertexShader9* vShader = nullptr;
    m_IDirect3DVertexShader9* vshader2 = nullptr;
    GetVertexShader(&vShader);
    if(vShader) {
        vshader2 = static_cast<m_IDirect3DVertexShader9*>(vShader);
        if(vshader2) {
            vshader2->replaceConstants();
        }
    }

    GetPixelShader(&pShader);
    if(pShader) {
        pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
        if(pShader2) {
            pShader2->replaceConstants();
            // post fx
            if(isShaderPostFx(pShader2)) {
                HRESULT result = PostFx3(this, ProxyInterface, PrimitiveType, StartVertex, PrimitiveCount, pShader);
                if(result == S_OK) {
                    return result;
                }
            }
            // atmoscat clouds
            else if((pShader2->id == 65 || pShader2->id == 66) && DiffuseTex != nullptr) {
                IDirect3DSurface9* DiffuseSurf = nullptr;
                DiffuseTex->GetProxyInterface()->GetSurfaceLevel(0, &DiffuseSurf);
                if(DiffuseSurf) {
                    IDirect3DSurface9* pTexture = 0;

                    ProxyInterface->GetRenderTarget(1, &pTexture);
                    ProxyInterface->SetRenderTarget(1, DiffuseSurf);
                    //if(pShader2->compiledShaders[(int)ShaderUse::SU_LASM]) {
                    //    ProxyInterface->SetPixelShader(static_cast<m_IDirect3DPixelShader9*>(pShader2->compiledShaders[ShaderUse::SU_LASM])->GetProxyInterface());
                    //}
                    hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                    ProxyInterface->SetPixelShader(pShader2->GetProxyInterface());
                    ProxyInterface->SetRenderTarget(1, pTexture);

                    //ProxyInterface->SetRenderTarget(1, 0);
                    SAFE_RELEASE(pTexture);
                    SAFE_RELEASE(DiffuseSurf);
                    return hr;
                }
                printf("%i", pShader2->id);
            }
            // cascade convert
            else if(useNewShadowAtlas == true && pShader2->id == 1 && NewShadowAtlas) {
                IDirect3DSurface9* oldDepth = nullptr;
                IDirect3DSurface9* newDepth = nullptr;
                NewShadowAtlas->GetSurfaceLevel(0, &newDepth);
                if(newDepth) {
                    D3DVIEWPORT9 newViewport = { 0 };
                    D3DVIEWPORT9 lastViewport = { 0 };
                    ProxyInterface->GetViewport(&newViewport);
                    ProxyInterface->GetViewport(&lastViewport);
                    ProxyInterface->GetDepthStencilSurface(&oldDepth);
                    SetDepthStencilSurface(newDepth);
                    DWORD last_ZENABLE      = 0;
                    DWORD last_ZWRITEENABLE = 0;
                    DWORD last_ZFUNC        = 0;
                    ProxyInterface->GetRenderState(D3DRS_ZENABLE        , &last_ZENABLE     );
                    ProxyInterface->GetRenderState(D3DRS_ZWRITEENABLE   , &last_ZWRITEENABLE);
                    ProxyInterface->GetRenderState(D3DRS_ZFUNC          , &last_ZFUNC       );

                    ProxyInterface->SetRenderState(D3DRS_ZENABLE        , D3DZB_TRUE);
                    ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE   , D3DZB_TRUE);
                    ProxyInterface->SetRenderState(D3DRS_ZFUNC          , D3DCMP_ALWAYS);
                    newViewport.MaxZ = 1000;
                    newViewport.MinZ = -1000;
                    ProxyInterface->SetViewport(&newViewport);
                    hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                    ProxyInterface->SetViewport(&lastViewport);
                    ProxyInterface->SetRenderState(D3DRS_ZENABLE        , last_ZENABLE     );
                    ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE   , last_ZWRITEENABLE);
                    ProxyInterface->SetRenderState(D3DRS_ZFUNC          , last_ZFUNC       );
                    ProxyInterface->SetDepthStencilSurface(oldDepth);
                    SAFE_RELEASE(newDepth);
                    SAFE_RELEASE(oldDepth);
                    return hr;
                }
            }
            // deferred 1 e 2
            else if(pShader2->id == 6 || pShader2->id == 7) {
                IDirect3DSurface9* pShadowBlurSurf1 = nullptr;
                IDirect3DSurface9* pShadowBlurSurf2 = nullptr;
                IDirect3DSurface9* OldRenderTarget = nullptr;
                IDirect3DTexture9* Oldtexture = nullptr;

                if(useDefferedShadows > 0 && pShadowBlurTex1 && pShadowBlurTex2) {
                    pShadowBlurTex1->GetSurfaceLevel(0, &pShadowBlurSurf1);
                    pShadowBlurTex2->GetSurfaceLevel(0, &pShadowBlurSurf2);
                    ProxyInterface->GetRenderTarget(0, &OldRenderTarget);
                    if(pShadowBlurSurf1 && pShadowBlurSurf2 && OldRenderTarget) {
                        SetRenderTarget(0, pShadowBlurSurf1);
                        SetPixelShader(DeferredShadowGen_ps);
                        ProxyInterface->SetPixelShaderConstantF(140, BilateralDepthTreshold, 1);
                        DWORD old_SAMP_MAGFILTER = 0;
                        ProxyInterface->GetSamplerState(11, D3DSAMP_MAGFILTER, &old_SAMP_MAGFILTER);
                        ProxyInterface->GetTexture(11, (IDirect3DBaseTexture9**) & Oldtexture);
                        ProxyInterface->SetSamplerState(11, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

                        if(useNewShadowAtlas == true && NewShadowAtlas) {
                            ProxyInterface->SetTexture(15, static_cast<m_IDirect3DTexture9*>(NewShadowAtlas)->GetProxyInterface());
                            //To enable Fetch - 4 on a texture sampler(sampler 0 in this example) :
                        #define FETCH4_ENABLE ((DWORD)MAKEFOURCC('G', 'E', 'T', '4'))
                        #define FETCH4_DISABLE ((DWORD)MAKEFOURCC('G', 'E', 'T', '1'))
                            DWORD old_SAMP_MIPMAPLODBIAS = 0;
                            DWORD old_SAMP_MAGFILTER = 0;
                            DWORD old_SAMP_MINFILTER = 0;

                            // Enable Fetch-4 on sampler 0 by overloading the MIPMAPLODBIAS render state
                            ProxyInterface->GetSamplerState(15, D3DSAMP_MIPMAPLODBIAS, &old_SAMP_MIPMAPLODBIAS);
                            ProxyInterface->GetSamplerState(15, D3DSAMP_MAGFILTER, &old_SAMP_MAGFILTER);
                            ProxyInterface->GetSamplerState(15, D3DSAMP_MINFILTER, &old_SAMP_MINFILTER);

                            ProxyInterface->SetSamplerState(15, D3DSAMP_MIPMAPLODBIAS, FETCH4_ENABLE);
                            // Set point sampling filtering (required for Fetch-4 to work)
                            ProxyInterface->SetSamplerState(15, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                            ProxyInterface->SetSamplerState(15, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
                            ProxyInterface->SetSamplerState(15, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
                            hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                            ProxyInterface->SetSamplerState(15, D3DSAMP_MIPMAPLODBIAS, old_SAMP_MIPMAPLODBIAS);
                            ProxyInterface->SetSamplerState(15, D3DSAMP_MAGFILTER, old_SAMP_MAGFILTER);
                            ProxyInterface->SetSamplerState(15, D3DSAMP_MINFILTER, old_SAMP_MINFILTER);

                            ProxyInterface->SetTexture(15, static_cast<m_IDirect3DTexture9*>(OldShadowAtlas)->GetProxyInterface());
                        }
                        else
                            ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                        if(useDefferedShadows == 1) {
                            ProxyInterface->SetRenderTarget(0, static_cast<m_IDirect3DSurface9*>(pShadowBlurSurf2)->GetProxyInterface());
                            ProxyInterface->SetTexture(11, static_cast<m_IDirect3DTexture9*>(pShadowBlurTex1)->GetProxyInterface());
                            SetPixelShader(DeferredShadowBlur1_ps);
                            ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        }
                        if(useDefferedShadows == 2) {
                            SetRenderTarget(0, pShadowBlurSurf2);
                            ProxyInterface->SetTexture(11, static_cast<m_IDirect3DTexture9*>(pShadowBlurTex1)->GetProxyInterface());
                            SetPixelShader(DeferredShadowBlur2_ps);
                            ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        }
                        if(useDefferedShadows == 3) {
                            SetPixelShader(DeferredShadowBlur1_ps);
                            ProxyInterface->SetTexture(11, static_cast<m_IDirect3DTexture9*>(pShadowBlurTex1)->GetProxyInterface());
                            SetRenderTarget(0, pShadowBlurSurf2);
                            ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                            SetPixelShader(DeferredShadowBlur2_ps);
                            ProxyInterface->SetTexture(11, static_cast<m_IDirect3DTexture9*>(pShadowBlurTex2)->GetProxyInterface());
                            SetRenderTarget(0, pShadowBlurSurf1);
                            ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        }
                        if(useDefferedShadows == 4) {
                            SetRenderTarget(0, pShadowBlurSurf2);
                            ProxyInterface->SetTexture(11, static_cast<m_IDirect3DTexture9*>(pShadowBlurTex1)->GetProxyInterface());
                            SetPixelShader(DeferredShadowBlur3_ps);
                            ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        }

                        ProxyInterface->SetRenderTarget(0, OldRenderTarget);
                        if(useDefferedShadows == 3)
                            ProxyInterface->SetTexture(11, static_cast<m_IDirect3DTexture9*>(pShadowBlurTex1)->GetProxyInterface());

                        else
                            ProxyInterface->SetTexture(11, static_cast<m_IDirect3DTexture9*>(pShadowBlurTex2)->GetProxyInterface());

                        if(pShader2->id == 6)
                            SetPixelShader(DeferredShadowUse1_ps);
                        else
                            SetPixelShader(DeferredShadowUse2_ps);
                        ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                        ProxyInterface->SetTexture(11, Oldtexture);
                        //ProxyInterface->SetTexture(11, 0);
                        SetPixelShader(pShader);
                        ProxyInterface->SetSamplerState(11, D3DSAMP_MAGFILTER, old_SAMP_MAGFILTER);
                        SAFE_RELEASE(pShadowBlurSurf1);
                        SAFE_RELEASE(pShadowBlurSurf2);
                        SAFE_RELEASE(OldRenderTarget);
                        SAFE_RELEASE(Oldtexture);
                        return hr;
                    }
                    SAFE_RELEASE(pShadowBlurSurf1);
                    SAFE_RELEASE(pShadowBlurSurf2);
                    SAFE_RELEASE(OldRenderTarget);
                    SAFE_RELEASE(Oldtexture);
                    //SetPixelShader(pShader);
                    ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                    return hr;
                }

                if(useNewShadowAtlas == true && NewShadowAtlas) {
                    ProxyInterface->SetTexture(15, static_cast<m_IDirect3DTexture9*>(NewShadowAtlas)->GetProxyInterface());
                    //To enable Fetch - 4 on a texture sampler(sampler 0 in this example) :
                #define FETCH4_ENABLE ((DWORD)MAKEFOURCC('G', 'E', 'T', '4'))
                #define FETCH4_DISABLE ((DWORD)MAKEFOURCC('G', 'E', 'T', '1'))
                    DWORD old_SAMP_MIPMAPLODBIAS = 0;
                    DWORD old_SAMP_MAGFILTER = 0;
                    DWORD old_SAMP_MINFILTER = 0;

                    // Enable Fetch-4 on sampler 0 by overloading the MIPMAPLODBIAS render state
                    ProxyInterface->GetSamplerState(15, D3DSAMP_MIPMAPLODBIAS, &old_SAMP_MIPMAPLODBIAS);
                    ProxyInterface->GetSamplerState(15, D3DSAMP_MAGFILTER, &old_SAMP_MAGFILTER);
                    ProxyInterface->GetSamplerState(15, D3DSAMP_MINFILTER, &old_SAMP_MINFILTER);

                    //ProxyInterface->SetSamplerState(15, D3DSAMP_MIPMAPLODBIAS, FETCH4_ENABLE);
                    // Set point sampling filtering (required for Fetch-4 to work)
                    ProxyInterface->SetSamplerState(15, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
                    ProxyInterface->SetSamplerState(15, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
                    ProxyInterface->SetSamplerState(15, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
                    hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

                    ProxyInterface->SetSamplerState(15, D3DSAMP_MIPMAPLODBIAS, old_SAMP_MIPMAPLODBIAS);
                    ProxyInterface->SetSamplerState(15, D3DSAMP_MAGFILTER, old_SAMP_MAGFILTER);
                    ProxyInterface->SetSamplerState(15, D3DSAMP_MINFILTER, old_SAMP_MINFILTER);

                    ProxyInterface->SetTexture(15, static_cast<m_IDirect3DTexture9*>(OldShadowAtlas)->GetProxyInterface());
                    return hr;
                }

                // use new deferred
                SetPixelShader(pShader);
                ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                return hr;
            }
        }
    }

    if(fixDistantOutlineUsingDXVK) {
        if(pShader2 && (pShader2->id == 822 || pShader2->id == 823)) {
            DWORD oldSampler = 0;
            GetSamplerState(0, D3DSAMP_MAGFILTER, &oldSampler);
            SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
            hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
            SetSamplerState(0, D3DSAMP_MAGFILTER, oldSampler);
            return hr;
        }
    }

    // Fixes coronas being rendered through objects in water reflections.
    if(fixCoronaDepth) {
        if(vshader2) {
            //vshader2->replaceConstants();
            if(vshader2->id == 15) {
                DWORD CoronaDepth = 0;
                GetRenderState(D3DRS_ZENABLE, &CoronaDepth);
                SetRenderState(D3DRS_ZENABLE, 1);
                hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
                SetRenderState(D3DRS_ZENABLE, CoronaDepth);
                return hr;
            }
        }
    }

    hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::DrawIndexedPrimitive(THIS_ D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {
    DrawCallsCount++;
    m_IDirect3DPixelShader9* pShader2 = nullptr;
    IDirect3DPixelShader9* pShader = nullptr;

    IDirect3DVertexShader9* vShader = nullptr;
    m_IDirect3DVertexShader9* vshader2 = nullptr;
    GetVertexShader(&vShader);
    if(vShader) {
        vshader2 = static_cast<m_IDirect3DVertexShader9*>(vShader);
        if(vshader2) {
            vshader2->replaceConstants();
        }
    }

    GetPixelShader(&pShader);
    if(pShader) {
        pShader2 = static_cast<m_IDirect3DPixelShader9*>(pShader);
        if(pShader2) {
            pShader2->replaceConstants();
        }
    }
    return ProxyInterface->DrawIndexedPrimitive(Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT m_IDirect3DDevice9Ex::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    DrawCallsCount++;
    return ProxyInterface->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT m_IDirect3DDevice9Ex::DrawRectPatch(UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    DrawCallsCount++;
    return ProxyInterface->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
}

HRESULT m_IDirect3DDevice9Ex::DrawTriPatch(UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    DrawCallsCount++;
    return ProxyInterface->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
}

HRESULT m_IDirect3DDevice9Ex::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    return ProxyInterface->DrawIndexedPrimitiveUP(PrimitiveType, MinIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}



