/**
* Copyright (C) 2020 Elisha Riedlinger
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*/

#include "shlobj_core.h"
#include "d3d9.h"
#include "d3dx9.h"
#include "iathook.h"
#include <Windows.h>
#include "ShadowResFix.h"
#include "Log.h"

std::string Log::LogText;
std::ofstream Log::mLogFile;
bool Log::isDirt = true;

#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()

extern float ResSSAA;


HWND g_hFocusWindow = NULL;

bool bSaveSettingsOnExit = true;

bool bForceWindowedMode;
bool bUsePrimaryMonitor;
bool bCenterWindow;
bool bBorderlessFullscreen;
bool bAlwaysOnTop;
bool bDoNotNotifyOnTaskSwitch;
bool bDisplayFPSCounter;
float fFPSLimit;
int nFullScreenRefreshRateInHz;

BOOL gEnableProxyLibrary;
BOOL bHook_SHGetFolderPath;
BOOL bHookDirect3DCreate9;
BOOL InitProxyFunctions;
char ProxyLibrary[MAX_PATH] = { 0 };

bool gFixRainDrops = 0;
UINT gWindowWidth = 0;
UINT gWindowHeight = 0;
UINT gWindowDivisor = 4; // rain drops blur
IDirect3DTexture9* pRainDropsRefractionHDRTex = 0;

bool gNearFarPlane = 1;
float NearFarPlane[4] = { 0 };

extern bool DisableADAPTIVETESS_X;
extern bool EnableDepthOverwrite;
extern bool AlowPauseGame;

extern bool UseSSAA;

bool gTreeLeavesSwayInTheWind = 0;
BOOL gFixCascadedShadowMapResolution = 0;
BOOL gFixWashedMirror = 0;
UINT gFixEmissiveTransparency = 0;
UINT gReflectionResMult = 1;
UINT gLightResMult = 1;


extern std::vector<m_IDirect3DPixelShader9*> ps;
extern std::vector<m_IDirect3DVertexShader9*> vs;
extern std::vector<const char*> shader_names_ps;
extern std::vector<const char*> shader_names_vs;
extern std::vector<std::vector<m_IDirect3DPixelShader9*>> fx_ps;
extern std::vector<std::vector<m_IDirect3DVertexShader9*>> fx_vs;

void* gDinpu8Device_vtbl[32] = {};

// smaa postfx vertex array
struct VERTEX {
    D3DXVECTOR3 pos;
    D3DXVECTOR2 tex1;
};
VERTEX SmaaVertexArray[4] = { D3DXVECTOR3(0,0,0),  D3DXVECTOR2(0,0)};



struct stModule {
    std::string name;
    HMODULE hmodule;
    std::list<std::string> functions;
    stModule(std::string _name, HMODULE _hmodule) : name(_name), hmodule(_hmodule) {}
};

class FrameLimiter {
private:
    static inline double TIME_Frequency = 0.0;
    static inline double TIME_Ticks = 0.0;
    static inline double TIME_Frametime = 0.0;

public:
    static inline ID3DXFont* pFPSFont = nullptr;
    static inline ID3DXFont* pTimeFont = nullptr;

public:
    enum FPSLimitMode { FPS_NONE, FPS_REALTIME, FPS_ACCURATE };
    static void Init(FPSLimitMode mode) {
        LARGE_INTEGER frequency;

        QueryPerformanceFrequency(&frequency);
        static constexpr auto TICKS_PER_FRAME = 1;
        auto TICKS_PER_SECOND = (TICKS_PER_FRAME * fFPSLimit);
        if(mode == FPS_ACCURATE) {
            TIME_Frametime = 1000.0 / (double) fFPSLimit;
            TIME_Frequency = (double) frequency.QuadPart / 1000.0; // ticks are milliseconds
        }
        else // FPS_REALTIME
        {
            TIME_Frequency = (double) frequency.QuadPart / (double) TICKS_PER_SECOND; // ticks are 1/n frames (n = fFPSLimit)
        }
        Ticks();
    }
    static DWORD Sync_RT() {
        DWORD lastTicks, currentTicks;
        LARGE_INTEGER counter;

        QueryPerformanceCounter(&counter);
        lastTicks = (DWORD) TIME_Ticks;
        TIME_Ticks = (double) counter.QuadPart / TIME_Frequency;
        currentTicks = (DWORD) TIME_Ticks;

        return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
    }
    static DWORD Sync_SLP() {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        double millis_current = (double) counter.QuadPart / TIME_Frequency;
        double millis_delta = millis_current - TIME_Ticks;
        if(TIME_Frametime <= millis_delta) {
            TIME_Ticks = millis_current;
            return 1;
        }
        else if(TIME_Frametime - millis_delta > 2.0) // > 2ms
            Sleep(1); // Sleep for ~1ms
        else
            Sleep(0); // yield thread's time-slice (does not actually sleep)

        return 0;
    }
    static void ShowFPS(LPDIRECT3DDEVICE9EX device) {
        static std::list<int> m_times;

        //https://github.com/microsoft/VCSamples/blob/master/VC2012Samples/Windows%208%20samples/C%2B%2B/Windows%208%20app%20samples/Direct2D%20geometry%20realization%20sample%20(Windows%208)/C%2B%2B/FPSCounter.cpp#L279
        LARGE_INTEGER frequency;
        LARGE_INTEGER time;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&time);

        if(m_times.size() == 50)
            m_times.pop_front();
        m_times.push_back(static_cast<int>(time.QuadPart));

        uint32_t fps = 0;
        if(m_times.size() >= 2)
            fps = static_cast<uint32_t>(0.5f + (static_cast<float>(m_times.size() - 1) * static_cast<float>(frequency.QuadPart)) / static_cast<float>(m_times.back() - m_times.front()));

        static int space = 0;
        if(!pFPSFont || !pTimeFont) {
            D3DDEVICE_CREATION_PARAMETERS cparams;
            RECT rect;

            if(!device) {
                Log::Error("device == 0");
            }

            device->GetCreationParameters(&cparams);
            GetClientRect(cparams.hFocusWindow, &rect);

            D3DXFONT_DESC fps_font;
            ZeroMemory(&fps_font, sizeof(D3DXFONT_DESC));
            fps_font.Height = rect.bottom / 20;
            fps_font.Width = 0;
            fps_font.Weight = 400;
            fps_font.MipLevels = 0;
            fps_font.Italic = 0;
            fps_font.CharSet = DEFAULT_CHARSET;
            fps_font.OutputPrecision = OUT_DEFAULT_PRECIS;
            fps_font.Quality = ANTIALIASED_QUALITY;
            fps_font.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
            wchar_t FaceName[] = L"Arial";
            memcpy(&fps_font.FaceName, &FaceName, sizeof(FaceName));

            D3DXFONT_DESC time_font = fps_font;
            time_font.Height = rect.bottom / 35;
            space = fps_font.Height + 5;

            if(D3DXCreateFontIndirect(device, &fps_font, &pFPSFont) != D3D_OK)
                return;

            if(D3DXCreateFontIndirect(device, &time_font, &pTimeFont) != D3D_OK)
                return;
        }
        else {
            auto DrawTextOutline = [] (ID3DXFont* pFont, LONG X, LONG Y, D3DXCOLOR dColor, CONST PCHAR cString, ...) {
                const D3DXCOLOR BLACK(D3DCOLOR_XRGB(0, 0, 0));
                CHAR cBuffer[101] = "";

                va_list oArgs;
                va_start(oArgs, cString);
                _vsnprintf((cBuffer + strlen(cBuffer)), (sizeof(cBuffer) - strlen(cBuffer) - 1), cString, oArgs);
                va_end(oArgs);

                RECT Rect[5] =
                {
                    { X - 1, Y, X + 500, Y + 50 },
                    { X, Y - 1, X + 500, Y + 50 },
                    { X + 1, Y, X + 500, Y + 50 },
                    { X, Y + 1, X + 500, Y + 50 },
                    { X, Y, X + 500, Y + 50 },
                };

                if(dColor != BLACK) {
                    for(auto i = 0; i < 4; i++)
                        pFont->DrawText(NULL, cBuffer, -1, &Rect[i], DT_NOCLIP, BLACK);
                }

                pFont->DrawText(NULL, cBuffer, -1, &Rect[4], DT_NOCLIP, dColor);
            };

            static char str_format_fps[] = "%02d";
            static char str_format_time[] = "%.01f ms";
            static char str_format_nfp[] = "%.03f, %.03f";
            static const D3DXCOLOR YELLOW(D3DCOLOR_XRGB(0xF7, 0xF7, 0));
            DrawTextOutline(pFPSFont, 10, 10, YELLOW, str_format_fps, fps);
            DrawTextOutline(pTimeFont, 10, space, YELLOW, str_format_time, (1.0f / fps) * 1000.0f);
            DrawTextOutline(pTimeFont, 10, space + 15, YELLOW, str_format_nfp, NearFarPlane[0], NearFarPlane[1]);
            DrawTextOutline(pTimeFont, 10, space + 30, YELLOW, str_format_fps, gFixEmissiveTransparency);
        }
    }

private:
    static void Ticks() {
        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);
        TIME_Ticks = (double) counter.QuadPart / TIME_Frequency;
    }
};

std::list<stModule> hmoduleList;

std::vector<HWND> windows;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD lpdwProcessId;
    GetWindowThreadProcessId(hwnd, &lpdwProcessId);
    if(lpdwProcessId == lParam && hwnd) {
        if(IsWindowVisible(hwnd))
            windows.push_back(hwnd);
        return FALSE;
    }
    return TRUE;
}

void addFunction(std::string name, HMODULE hmodule) {
    for(auto& i : hmoduleList) {
        if(i.hmodule == hmodule) {
            i.functions.push_back(name);
        }
    }
}
void* get_function_address2(HMODULE module, const char* function_name) {
    if(module != NULL) {
        //Get the address of the function
        return GetProcAddress(module, function_name);
    }
    else {
        return NULL;
    }
}
PCSTR pcwstr_to_pcstr(PCWSTR input) {
    static char cstrbuffer[1024] = { 0 };
    // Get the required cstrbuffer size for the conversion
    int size = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
    if(size == 0) {
        // Error occurred
        return NULL;
    }
    // Perform the conversion
    int result = WideCharToMultiByte(CP_UTF8, 0, input, -1, cstrbuffer, size, NULL, NULL);
    if(result == 0) {
        // Error occurred
        return NULL;
    }
    // Return the converted string
    return cstrbuffer;
}

ShadowResFix* gShadowResFix = nullptr;

HANDLE gMainThreadHandle = nullptr;

uint8_t* baseAddress = nullptr;

FrameLimiter::FPSLimitMode mFPSLimitMode = FrameLimiter::FPSLimitMode::FPS_NONE;

extern IDirect3DTexture9* pHDRTex ;
extern IDirect3DTexture9* pHDRTex2;

extern std::list<m_IDirect3DTexture9*> textureList;
extern m_IDirect3DTexture9* rainDepth  ;
extern IDirect3DTexture9* pHDRTex      ;
extern IDirect3DTexture9* pHDRTex2     ;
extern IDirect3DTexture9* areaTex      ;
extern IDirect3DTexture9* searchTex    ;
extern IDirect3DTexture9* edgesTex     ;
extern IDirect3DTexture9* blendTex     ;



typedef BOOL(WINAPI* ClipCursor_Ptr)(const RECT* lpRect);

typedef HRESULT(__stdcall DInput8DeviceGetDeviceStateT)(IDirectInputDevice8*, DWORD, LPVOID);
typedef HRESULT(__stdcall DInput8DeviceAcquireT)(IDirectInputDevice8*);

typedef ATOM(__stdcall* RegisterClassA_fn)(const WNDCLASSA*);
typedef ATOM(__stdcall* RegisterClassW_fn)(const WNDCLASSW*);
typedef ATOM(__stdcall* RegisterClassExA_fn)(const WNDCLASSEXA*);
typedef ATOM(__stdcall* RegisterClassExW_fn)(const WNDCLASSEXW*);

typedef HRESULT(__stdcall* SHGetFolderPathA_Ptr)(_Reserved_ HWND hwnd, _In_ int csidl, _In_opt_ HANDLE hToken, _In_ DWORD dwFlags, _Out_writes_(MAX_PATH) LPSTR pszPath);
typedef HRESULT(__stdcall* SHGetFolderPathW_Ptr)(_Reserved_ HWND hwnd, _In_ int csidl, _In_opt_ HANDLE hToken, _In_ DWORD dwFlags, _Out_writes_(MAX_PATH) LPWSTR pszPath);

typedef HMODULE(WINAPI* LoadLibraryA_Ptr)(LPCSTR lpLibFileName);
typedef HMODULE(WINAPI* LoadLibraryW_Ptr)(LPCWSTR lpLibFileName);
typedef HMODULE(WINAPI* LoadLibraryExW_Ptr)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HMODULE(WINAPI* LoadLibraryExA_Ptr)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef FARPROC(WINAPI* GetProcAddress_Ptr)(HMODULE hModule, LPCSTR lpProcName);

typedef HRESULT(WINAPI* DirectInput8Create_ptr)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);


Direct3DShaderValidatorCreate9Proc              o_pDirect3DShaderValidatorCreate9 = nullptr;
PSGPErrorProc                                   o_pPSGPError = nullptr;
PSGPSampleTextureProc                           o_pPSGPSampleTexture = nullptr;
D3DPERF_BeginEventProc                          o_pD3DPERF_BeginEvent = nullptr;
D3DPERF_EndEventProc                            o_pD3DPERF_EndEvent = nullptr;
D3DPERF_GetStatusProc                           o_pD3DPERF_GetStatus = nullptr;
D3DPERF_QueryRepeatFrameProc                    o_pD3DPERF_QueryRepeatFrame = nullptr;
D3DPERF_SetMarkerProc                           o_pD3DPERF_SetMarker = nullptr;
D3DPERF_SetOptionsProc                          o_pD3DPERF_SetOptions = nullptr;
D3DPERF_SetRegionProc                           o_pD3DPERF_SetRegion = nullptr;
DebugSetLevelProc                               o_pDebugSetLevel = nullptr;
DebugSetMuteProc                                o_pDebugSetMute = nullptr;
Direct3D9EnableMaximizedWindowedModeShimProc    o_pDirect3D9EnableMaximizedWindowedModeShim = nullptr;
Direct3DCreate9Proc                             o_pDirect3DCreate9 = nullptr;
Direct3DCreate9ExProc                           o_pDirect3DCreate9Ex = nullptr;

DirectInput8Create_ptr                          o_DirectInput8Create = nullptr;


RegisterClassA_fn o_RegisterClassA = nullptr;
RegisterClassW_fn o_RegisterClassW = nullptr;
RegisterClassExA_fn o_RegisterClassExA = nullptr;
RegisterClassExW_fn o_RegisterClassExW = nullptr;

LoadLibraryA_Ptr	           o_LoadLibraryA   = nullptr;
LoadLibraryW_Ptr	           o_LoadLibraryW   = nullptr;
LoadLibraryExW_Ptr	           o_LoadLibraryExW = nullptr;
LoadLibraryExA_Ptr	           o_LoadLibraryExA = nullptr;
GetProcAddress_Ptr             o_GetProcAddress = nullptr;

WNDPROC o_WndProc = nullptr;
WNDPROC WndProc = nullptr;

SHGetFolderPathA_Ptr o_SHGetFolderPathA = nullptr;
SHGetFolderPathW_Ptr o_SHGetFolderPathW = nullptr;

DInput8DeviceGetDeviceStateT* o_DInput8DeviceGetDeviceState = nullptr;
DInput8DeviceAcquireT* o_DInput8DeviceAcquire = nullptr;
ClipCursor_Ptr    o_ClipCursor = nullptr;

bool UsePresentToRenderUI = false;

// SMAA vertex array
void CreateSmaaVertexArray() {
    //FLOAT Half = 1.0f;
    //FLOAT fWidth5 = (FLOAT) Half;
    //FLOAT fHeight5 = (FLOAT) Half;

    //FLOAT fTexWidth1 = (FLOAT) 1;
    //FLOAT fTexHeight1 = (FLOAT) 1;

    //FLOAT fWidthMod = 1.0f / (FLOAT) 10;
    //FLOAT fHeightMod = 1.0f / (FLOAT) 10;

    //SmaaVertexArray[0].pos =  D3DXVECTOR3(fWidth5, -Half, 0.0f);
    //SmaaVertexArray[0].tex1 = D3DXVECTOR2(fTexWidth1, fTexHeight1);
    //SmaaVertexArray[1].pos =  D3DXVECTOR3(fWidth5, fHeight5, 0.0f);
    //SmaaVertexArray[1].tex1 = D3DXVECTOR2(fTexWidth1, 0);
    //SmaaVertexArray[2].pos =  D3DXVECTOR3(-Half, -Half, 0.0f);
    //SmaaVertexArray[2].tex1 = D3DXVECTOR2(0, fTexHeight1);
    //SmaaVertexArray[3].pos =  D3DXVECTOR3(-Half, fHeight5, 0.0f);
    //SmaaVertexArray[3].tex1 = D3DXVECTOR2(0, 0);

    // corrected texcoord
    D3DXVECTOR2 pixelSize = D3DXVECTOR2(1.0f / float(gWindowWidth * (UseSSAA? ResSSAA:1)), 1.0f / float(gWindowHeight * (UseSSAA ? ResSSAA : 1)));
    SmaaVertexArray[0].pos = D3DXVECTOR3(-1.0f - pixelSize.x, 1.0f + pixelSize.y, 0.5f);
    SmaaVertexArray[0].tex1 = D3DXVECTOR2(0.0f, 0.0f);
    SmaaVertexArray[1].pos = D3DXVECTOR3(1.0f - pixelSize.x, 1.0f + pixelSize.y, 0.5f);
    SmaaVertexArray[1].tex1 = D3DXVECTOR2(1.0f, 0.0f);
    SmaaVertexArray[2].pos = D3DXVECTOR3(-1.0f - pixelSize.x, -1.0f + pixelSize.y, 0.5f);
    SmaaVertexArray[2].tex1 = D3DXVECTOR2(0.0f, 1.0f);
    SmaaVertexArray[3].pos = D3DXVECTOR3(1.0f - pixelSize.x, -1.0f + pixelSize.y, 0.5f);
    SmaaVertexArray[3].tex1 = D3DXVECTOR2(1.0f, 1.0f);
}

HRESULT m_IDirect3DDevice9Ex::Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) {
    if(mFPSLimitMode == FrameLimiter::FPSLimitMode::FPS_REALTIME)
        while(!FrameLimiter::Sync_RT());
    else if(mFPSLimitMode == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
        while(!FrameLimiter::Sync_SLP());

    if(UsePresentToRenderUI) {
        if(bDisplayFPSCounter)
            FrameLimiter::ShowFPS(ProxyInterface);

        // Need to keep dx9 state the same as before using ImGui functions
        DWORD		_D3DRS_ZWRITEENABLE = FALSE;
        DWORD		_D3DRS_ALPHATESTENABLE = FALSE;
        D3DCULL		_D3DRS_CULLMODE = D3DCULL_NONE;
        DWORD		_D3DRS_ZENABLE = FALSE;
        DWORD		_D3DRS_ALPHABLENDENABLE = TRUE;
        D3DBLENDOP	_D3DRS_BLENDOP = D3DBLENDOP_ADD;
        D3DBLEND	_D3DRS_SRCBLEND = D3DBLEND_SRCALPHA;
        D3DBLEND	_D3DRS_DESTBLEND = D3DBLEND_INVSRCALPHA;
        DWORD		_D3DRS_SEPARATEALPHABLENDENABLE = FALSE;
        D3DBLEND	_D3DRS_SRCBLENDALPHA = D3DBLEND_ONE;
        D3DBLEND	_D3DRS_DESTBLENDALPHA = D3DBLEND_ONE;
        DWORD		_D3DRS_SCISSORTESTENABLE = FALSE;

        ProxyInterface->GetRenderState(D3DRS_ZWRITEENABLE, (DWORD*) &_D3DRS_ZWRITEENABLE);
        ProxyInterface->GetRenderState(D3DRS_ALPHATESTENABLE, (DWORD*) &_D3DRS_ALPHATESTENABLE);
        ProxyInterface->GetRenderState(D3DRS_CULLMODE, (DWORD*) &_D3DRS_CULLMODE);
        ProxyInterface->GetRenderState(D3DRS_ZENABLE, (DWORD*) &_D3DRS_ZENABLE);
        ProxyInterface->GetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD*) &_D3DRS_ALPHABLENDENABLE);
        ProxyInterface->GetRenderState(D3DRS_BLENDOP, (DWORD*) &_D3DRS_BLENDOP);
        ProxyInterface->GetRenderState(D3DRS_SRCBLEND, (DWORD*) &_D3DRS_SRCBLEND);
        ProxyInterface->GetRenderState(D3DRS_DESTBLEND, (DWORD*) &_D3DRS_DESTBLEND);
        ProxyInterface->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, (DWORD*) &_D3DRS_SEPARATEALPHABLENDENABLE);
        ProxyInterface->GetRenderState(D3DRS_SRCBLENDALPHA, (DWORD*) &_D3DRS_SRCBLENDALPHA);
        ProxyInterface->GetRenderState(D3DRS_DESTBLENDALPHA, (DWORD*) &_D3DRS_DESTBLENDALPHA);
        ProxyInterface->GetRenderState(D3DRS_SCISSORTESTENABLE, (DWORD*) &_D3DRS_SCISSORTESTENABLE);

        gShadowResFix->OnBeforeD3D9DeviceEndScene(this);

        ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, (DWORD) _D3DRS_ZWRITEENABLE);
        ProxyInterface->SetRenderState(D3DRS_ALPHATESTENABLE, (DWORD) _D3DRS_ALPHATESTENABLE);
        ProxyInterface->SetRenderState(D3DRS_CULLMODE, (DWORD) _D3DRS_CULLMODE);
        ProxyInterface->SetRenderState(D3DRS_ZENABLE, (DWORD) _D3DRS_ZENABLE);
        ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD) _D3DRS_ALPHABLENDENABLE);
        ProxyInterface->SetRenderState(D3DRS_BLENDOP, (DWORD) _D3DRS_BLENDOP);
        ProxyInterface->SetRenderState(D3DRS_SRCBLEND, (DWORD) _D3DRS_SRCBLEND);
        ProxyInterface->SetRenderState(D3DRS_DESTBLEND, (DWORD) _D3DRS_DESTBLEND);
        ProxyInterface->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, (DWORD) _D3DRS_SEPARATEALPHABLENDENABLE);
        ProxyInterface->SetRenderState(D3DRS_SRCBLENDALPHA, (DWORD) _D3DRS_SRCBLENDALPHA);
        ProxyInterface->SetRenderState(D3DRS_DESTBLENDALPHA, (DWORD) _D3DRS_DESTBLENDALPHA);
        ProxyInterface->SetRenderState(D3DRS_SCISSORTESTENABLE, (DWORD) _D3DRS_SCISSORTESTENABLE);

        if(GetAsyncKeyState(VK_F3) & 0x01) {
            gFixEmissiveTransparency++;
            gFixEmissiveTransparency = gFixEmissiveTransparency % 3;
        }
        ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    }

    return ProxyInterface->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT m_IDirect3DDevice9Ex::PresentEx(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags) {
    if(mFPSLimitMode == FrameLimiter::FPSLimitMode::FPS_REALTIME)
        while(!FrameLimiter::Sync_RT());
    else if(mFPSLimitMode == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
        while(!FrameLimiter::Sync_SLP());

    if(UsePresentToRenderUI) {
        if(bDisplayFPSCounter)
            FrameLimiter::ShowFPS(ProxyInterface);

        // Need to keep dx9 state the same as before using ImGui functions
        DWORD		_D3DRS_ZWRITEENABLE = FALSE;
        DWORD		_D3DRS_ALPHATESTENABLE = FALSE;
        D3DCULL		_D3DRS_CULLMODE = D3DCULL_NONE;
        DWORD		_D3DRS_ZENABLE = FALSE;
        DWORD		_D3DRS_ALPHABLENDENABLE = TRUE;
        D3DBLENDOP	_D3DRS_BLENDOP = D3DBLENDOP_ADD;
        D3DBLEND	_D3DRS_SRCBLEND = D3DBLEND_SRCALPHA;
        D3DBLEND	_D3DRS_DESTBLEND = D3DBLEND_INVSRCALPHA;
        DWORD		_D3DRS_SEPARATEALPHABLENDENABLE = FALSE;
        D3DBLEND	_D3DRS_SRCBLENDALPHA = D3DBLEND_ONE;
        D3DBLEND	_D3DRS_DESTBLENDALPHA = D3DBLEND_ONE;
        DWORD		_D3DRS_SCISSORTESTENABLE = FALSE;

        ProxyInterface->GetRenderState(D3DRS_ZWRITEENABLE, (DWORD*) &_D3DRS_ZWRITEENABLE);
        ProxyInterface->GetRenderState(D3DRS_ALPHATESTENABLE, (DWORD*) &_D3DRS_ALPHATESTENABLE);
        ProxyInterface->GetRenderState(D3DRS_CULLMODE, (DWORD*) &_D3DRS_CULLMODE);
        ProxyInterface->GetRenderState(D3DRS_ZENABLE, (DWORD*) &_D3DRS_ZENABLE);
        ProxyInterface->GetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD*) &_D3DRS_ALPHABLENDENABLE);
        ProxyInterface->GetRenderState(D3DRS_BLENDOP, (DWORD*) &_D3DRS_BLENDOP);
        ProxyInterface->GetRenderState(D3DRS_SRCBLEND, (DWORD*) &_D3DRS_SRCBLEND);
        ProxyInterface->GetRenderState(D3DRS_DESTBLEND, (DWORD*) &_D3DRS_DESTBLEND);
        ProxyInterface->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, (DWORD*) &_D3DRS_SEPARATEALPHABLENDENABLE);
        ProxyInterface->GetRenderState(D3DRS_SRCBLENDALPHA, (DWORD*) &_D3DRS_SRCBLENDALPHA);
        ProxyInterface->GetRenderState(D3DRS_DESTBLENDALPHA, (DWORD*) &_D3DRS_DESTBLENDALPHA);
        ProxyInterface->GetRenderState(D3DRS_SCISSORTESTENABLE, (DWORD*) &_D3DRS_SCISSORTESTENABLE);

        gShadowResFix->OnBeforeD3D9DeviceEndScene(this);

        ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, (DWORD) _D3DRS_ZWRITEENABLE);
        ProxyInterface->SetRenderState(D3DRS_ALPHATESTENABLE, (DWORD) _D3DRS_ALPHATESTENABLE);
        ProxyInterface->SetRenderState(D3DRS_CULLMODE, (DWORD) _D3DRS_CULLMODE);
        ProxyInterface->SetRenderState(D3DRS_ZENABLE, (DWORD) _D3DRS_ZENABLE);
        ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD) _D3DRS_ALPHABLENDENABLE);
        ProxyInterface->SetRenderState(D3DRS_BLENDOP, (DWORD) _D3DRS_BLENDOP);
        ProxyInterface->SetRenderState(D3DRS_SRCBLEND, (DWORD) _D3DRS_SRCBLEND);
        ProxyInterface->SetRenderState(D3DRS_DESTBLEND, (DWORD) _D3DRS_DESTBLEND);
        ProxyInterface->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, (DWORD) _D3DRS_SEPARATEALPHABLENDENABLE);
        ProxyInterface->SetRenderState(D3DRS_SRCBLENDALPHA, (DWORD) _D3DRS_SRCBLENDALPHA);
        ProxyInterface->SetRenderState(D3DRS_DESTBLENDALPHA, (DWORD) _D3DRS_DESTBLENDALPHA);
        ProxyInterface->SetRenderState(D3DRS_SCISSORTESTENABLE, (DWORD) _D3DRS_SCISSORTESTENABLE);

        if(GetAsyncKeyState(VK_F3) & 0x01) {
            gFixEmissiveTransparency++;
            gFixEmissiveTransparency = gFixEmissiveTransparency % 3;
        }
        ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    }

    return ProxyInterface->PresentEx(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

HRESULT m_IDirect3DDevice9Ex::BeginScene() {
    auto hr = ProxyInterface->BeginScene();
    //ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    return hr;
}

HRESULT m_IDirect3DDevice9Ex::EndScene() {
    if(!UsePresentToRenderUI) {
        if(bDisplayFPSCounter)
            FrameLimiter::ShowFPS(ProxyInterface);

        // Need to keep dx9 state the same as before using ImGui functions
        DWORD		_D3DRS_ZWRITEENABLE = FALSE;
        DWORD		_D3DRS_ALPHATESTENABLE = FALSE;
        D3DCULL		_D3DRS_CULLMODE = D3DCULL_NONE;
        DWORD		_D3DRS_ZENABLE = FALSE;
        DWORD		_D3DRS_ALPHABLENDENABLE = TRUE;
        D3DBLENDOP	_D3DRS_BLENDOP = D3DBLENDOP_ADD;
        D3DBLEND	_D3DRS_SRCBLEND = D3DBLEND_SRCALPHA;
        D3DBLEND	_D3DRS_DESTBLEND = D3DBLEND_INVSRCALPHA;
        DWORD		_D3DRS_SEPARATEALPHABLENDENABLE = FALSE;
        D3DBLEND	_D3DRS_SRCBLENDALPHA = D3DBLEND_ONE;
        D3DBLEND	_D3DRS_DESTBLENDALPHA = D3DBLEND_ONE;
        DWORD		_D3DRS_SCISSORTESTENABLE = FALSE;

        ProxyInterface->GetRenderState(D3DRS_ZWRITEENABLE, (DWORD*) &_D3DRS_ZWRITEENABLE);
        ProxyInterface->GetRenderState(D3DRS_ALPHATESTENABLE, (DWORD*) &_D3DRS_ALPHATESTENABLE);
        ProxyInterface->GetRenderState(D3DRS_CULLMODE, (DWORD*) &_D3DRS_CULLMODE);
        ProxyInterface->GetRenderState(D3DRS_ZENABLE, (DWORD*) &_D3DRS_ZENABLE);
        ProxyInterface->GetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD*) &_D3DRS_ALPHABLENDENABLE);
        ProxyInterface->GetRenderState(D3DRS_BLENDOP, (DWORD*) &_D3DRS_BLENDOP);
        ProxyInterface->GetRenderState(D3DRS_SRCBLEND, (DWORD*) &_D3DRS_SRCBLEND);
        ProxyInterface->GetRenderState(D3DRS_DESTBLEND, (DWORD*) &_D3DRS_DESTBLEND);
        ProxyInterface->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, (DWORD*) &_D3DRS_SEPARATEALPHABLENDENABLE);
        ProxyInterface->GetRenderState(D3DRS_SRCBLENDALPHA, (DWORD*) &_D3DRS_SRCBLENDALPHA);
        ProxyInterface->GetRenderState(D3DRS_DESTBLENDALPHA, (DWORD*) &_D3DRS_DESTBLENDALPHA);
        ProxyInterface->GetRenderState(D3DRS_SCISSORTESTENABLE, (DWORD*) &_D3DRS_SCISSORTESTENABLE);

        gShadowResFix->OnBeforeD3D9DeviceEndScene(this);

        ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, (DWORD) _D3DRS_ZWRITEENABLE);
        ProxyInterface->SetRenderState(D3DRS_ALPHATESTENABLE, (DWORD) _D3DRS_ALPHATESTENABLE);
        ProxyInterface->SetRenderState(D3DRS_CULLMODE, (DWORD) _D3DRS_CULLMODE);
        ProxyInterface->SetRenderState(D3DRS_ZENABLE, (DWORD) _D3DRS_ZENABLE);
        ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD) _D3DRS_ALPHABLENDENABLE);
        ProxyInterface->SetRenderState(D3DRS_BLENDOP, (DWORD) _D3DRS_BLENDOP);
        ProxyInterface->SetRenderState(D3DRS_SRCBLEND, (DWORD) _D3DRS_SRCBLEND);
        ProxyInterface->SetRenderState(D3DRS_DESTBLEND, (DWORD) _D3DRS_DESTBLEND);
        ProxyInterface->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, (DWORD) _D3DRS_SEPARATEALPHABLENDENABLE);
        ProxyInterface->SetRenderState(D3DRS_SRCBLENDALPHA, (DWORD) _D3DRS_SRCBLENDALPHA);
        ProxyInterface->SetRenderState(D3DRS_DESTBLENDALPHA, (DWORD) _D3DRS_DESTBLENDALPHA);
        ProxyInterface->SetRenderState(D3DRS_SCISSORTESTENABLE, (DWORD) _D3DRS_SCISSORTESTENABLE);

        if(GetAsyncKeyState(VK_F3) & 0x01) {
            gFixEmissiveTransparency++;
            gFixEmissiveTransparency = gFixEmissiveTransparency % 3;
        }
        ProxyInterface->SetRenderState(D3DRS_ZWRITEENABLE, 1);
    }

    HRESULT hr = ProxyInterface->EndScene();

    return hr;
}

void ForceWindowed(D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode = NULL) {
    HWND hwnd = pPresentationParameters->hDeviceWindow ? pPresentationParameters->hDeviceWindow : g_hFocusWindow;
    HMONITOR monitor = MonitorFromWindow((!bUsePrimaryMonitor && hwnd) ? hwnd : GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
    int DesktopResX = info.rcMonitor.right - info.rcMonitor.left;
    int DesktopResY = info.rcMonitor.bottom - info.rcMonitor.top;

    int left = (int) info.rcMonitor.left;
    int top = (int) info.rcMonitor.top;

    if(!bBorderlessFullscreen) {
        left += (int) (((float) DesktopResX / 2.0f) - ((float) pPresentationParameters->BackBufferWidth / 2.0f));
        top += (int) (((float) DesktopResY / 2.0f) - ((float) pPresentationParameters->BackBufferHeight / 2.0f));
    }

    pPresentationParameters->Windowed = 1;

    // This must be set to default (0) on windowed mode as per D3D9 spec
    pPresentationParameters->FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

    // If exists, this must match the rate in PresentationParameters
    if(pFullscreenDisplayMode != NULL)
        pFullscreenDisplayMode->RefreshRate = D3DPRESENT_RATE_DEFAULT;

    // This flag is not available on windowed mode as per D3D9 spec
    pPresentationParameters->PresentationInterval &= ~D3DPRESENT_DONOTFLIP;

    if(hwnd != NULL) {
        UINT uFlags = SWP_SHOWWINDOW;
        if(bBorderlessFullscreen) {
            LONG lOldStyle = GetWindowLong(hwnd, GWL_STYLE);
            LONG lOldExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
            LONG lNewStyle = lOldStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_DLGFRAME);
            lNewStyle |= (lOldStyle & WS_CHILD) ? 0 : WS_POPUP;
            LONG lNewExStyle = lOldExStyle & ~(WS_EX_CONTEXTHELP | WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW);
            lNewExStyle |= WS_EX_APPWINDOW;

            if(lNewStyle != lOldStyle) {
                SetWindowLong(hwnd, GWL_STYLE, lNewStyle);
                uFlags |= SWP_FRAMECHANGED;
            }
            if(lNewExStyle != lOldExStyle) {
                SetWindowLong(hwnd, GWL_EXSTYLE, lNewExStyle);
                uFlags |= SWP_FRAMECHANGED;
            }
            SetWindowPos(hwnd, bAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, left, top, DesktopResX, DesktopResY, uFlags);
        }
        else {
            if(!bCenterWindow)
                uFlags |= SWP_NOMOVE;

            SetWindowPos(hwnd, bAlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, left, top, pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight, uFlags);
        }
    }
}

void ForceFullScreenRefreshRateInHz(D3DPRESENT_PARAMETERS* pPresentationParameters) {
    if(!pPresentationParameters->Windowed) {
        std::vector<int> list;
        DISPLAY_DEVICE dd;
        dd.cb = sizeof(DISPLAY_DEVICE);
        DWORD deviceNum = 0;
        while(EnumDisplayDevices(NULL, deviceNum, &dd, 0)) {
            DISPLAY_DEVICE newdd = { 0 };
            newdd.cb = sizeof(DISPLAY_DEVICE);
            DWORD monitorNum = 0;
            DEVMODE dm = { 0 };
            while(EnumDisplayDevices(dd.DeviceName, monitorNum, &newdd, 0)) {
                for(auto iModeNum = 0; EnumDisplaySettings(NULL, iModeNum, &dm) != 0; iModeNum++)
                    list.emplace_back(dm.dmDisplayFrequency);
                monitorNum++;
            }
            deviceNum++;
        }

        std::sort(list.begin(), list.end());
        if(nFullScreenRefreshRateInHz > list.back() || nFullScreenRefreshRateInHz < list.front() || nFullScreenRefreshRateInHz < 0)
            pPresentationParameters->FullScreen_RefreshRateInHz = list.back();
        else
            pPresentationParameters->FullScreen_RefreshRateInHz = nFullScreenRefreshRateInHz;
    }
}


HRESULT m_IDirect3D9Ex::CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface) {
    pRainDropsRefractionHDRTex = 0;
    pRainDropsRefractionHDRTex = 0;
    if(bForceWindowedMode) {
        g_hFocusWindow = hFocusWindow;
        ForceWindowed(pPresentationParameters);
    }

    if(nFullScreenRefreshRateInHz)
        ForceFullScreenRefreshRateInHz(pPresentationParameters);

    if(FrameLimiter::pFPSFont)
        FrameLimiter::pFPSFont->Release();
    if(FrameLimiter::pTimeFont)
        FrameLimiter::pTimeFont->Release();
    FrameLimiter::pFPSFont = nullptr;
    FrameLimiter::pTimeFont = nullptr;

    HRESULT hr = ProxyInterface->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
    gWindowWidth = pPresentationParameters->BackBufferWidth;
    gWindowHeight = pPresentationParameters->BackBufferHeight;

    if(SUCCEEDED(hr) && ppReturnedDeviceInterface) {
        *ppReturnedDeviceInterface = new m_IDirect3DDevice9Ex((IDirect3DDevice9Ex*) *ppReturnedDeviceInterface, this, IID_IDirect3DDevice9);
    }


    CreateSmaaVertexArray();


    return hr;
}

HRESULT m_IDirect3D9Ex::CreateDeviceEx(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode, IDirect3DDevice9Ex** ppReturnedDeviceInterface) {
    pRainDropsRefractionHDRTex = 0;
    if(bForceWindowedMode) {
        g_hFocusWindow = hFocusWindow;
        ForceWindowed(pPresentationParameters, pFullscreenDisplayMode);
    }

    if(nFullScreenRefreshRateInHz)
        ForceFullScreenRefreshRateInHz(pPresentationParameters);

    if(FrameLimiter::pFPSFont)
        FrameLimiter::pFPSFont->Release();
    if(FrameLimiter::pTimeFont)
        FrameLimiter::pTimeFont->Release();
    FrameLimiter::pFPSFont = nullptr;
    FrameLimiter::pTimeFont = nullptr;

    HRESULT hr = ProxyInterface->CreateDeviceEx(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode, ppReturnedDeviceInterface);

    if(SUCCEEDED(hr) && ppReturnedDeviceInterface) {
        *ppReturnedDeviceInterface = new m_IDirect3DDevice9Ex(*ppReturnedDeviceInterface, this, IID_IDirect3DDevice9Ex);
    }

    CreateSmaaVertexArray();

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) {
    pRainDropsRefractionHDRTex = 0;
    textureList.remove(rainDepth);
    textureList.remove((m_IDirect3DTexture9*) pHDRTex);
    textureList.remove((m_IDirect3DTexture9*) pHDRTex2);
    textureList.remove((m_IDirect3DTexture9*) areaTex);
    textureList.remove((m_IDirect3DTexture9*) searchTex);
    textureList.remove((m_IDirect3DTexture9*) edgesTex);
    textureList.remove((m_IDirect3DTexture9*) blendTex);

    SAFE_RELEASE(rainDepth);
    SAFE_RELEASE(pHDRTex);
    SAFE_RELEASE(pHDRTex2);
    SAFE_RELEASE(areaTex);
    SAFE_RELEASE(searchTex);
    SAFE_RELEASE(edgesTex);
    SAFE_RELEASE(blendTex);

    if(bForceWindowedMode)
        ForceWindowed(pPresentationParameters);

    if(nFullScreenRefreshRateInHz)
        ForceFullScreenRefreshRateInHz(pPresentationParameters);

    if(FrameLimiter::pFPSFont)
        FrameLimiter::pFPSFont->OnLostDevice();
    if(FrameLimiter::pTimeFont)
        FrameLimiter::pTimeFont->OnLostDevice();

    gShadowResFix->OnBeforeD3D9DeviceReset(this);

    auto hRet = ProxyInterface->Reset(pPresentationParameters);

    gShadowResFix->OnAfterD3D9DeviceReset();

    gWindowWidth = pPresentationParameters->BackBufferWidth;
    gWindowHeight = pPresentationParameters->BackBufferHeight;

    if(SUCCEEDED(hRet)) {
        if(FrameLimiter::pFPSFont)
            FrameLimiter::pFPSFont->OnResetDevice();
        if(FrameLimiter::pTimeFont)
            FrameLimiter::pTimeFont->OnResetDevice();
    }
    Log::Info("Device9Ex->Reset();");

    CreateSmaaVertexArray();

    return hRet;
}

HRESULT m_IDirect3DDevice9Ex::ResetEx(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode) {
    pRainDropsRefractionHDRTex = 0;

    textureList.remove(rainDepth);
    textureList.remove((m_IDirect3DTexture9*)pHDRTex);
    textureList.remove((m_IDirect3DTexture9*)pHDRTex2);
    textureList.remove((m_IDirect3DTexture9*)areaTex);
    textureList.remove((m_IDirect3DTexture9*)searchTex);
    textureList.remove((m_IDirect3DTexture9*)edgesTex);
    textureList.remove((m_IDirect3DTexture9*)blendTex);

    SAFE_RELEASE(rainDepth);
    SAFE_RELEASE(pHDRTex);
    SAFE_RELEASE(pHDRTex2);

    SAFE_RELEASE(areaTex);
    SAFE_RELEASE(searchTex);
    SAFE_RELEASE(edgesTex);
    SAFE_RELEASE(blendTex);

    if(bForceWindowedMode)
        ForceWindowed(pPresentationParameters, pFullscreenDisplayMode);

    if(nFullScreenRefreshRateInHz)
        ForceFullScreenRefreshRateInHz(pPresentationParameters);

    if(FrameLimiter::pFPSFont)
        FrameLimiter::pFPSFont->OnLostDevice();
    if(FrameLimiter::pTimeFont)
        FrameLimiter::pTimeFont->OnLostDevice();

    auto hRet = ProxyInterface->ResetEx(pPresentationParameters, pFullscreenDisplayMode);

    if(SUCCEEDED(hRet)) {
        if(FrameLimiter::pFPSFont)
            FrameLimiter::pFPSFont->OnResetDevice();
        if(FrameLimiter::pTimeFont)
            FrameLimiter::pTimeFont->OnResetDevice();
    }
    Log::Info("Device9Ex->ResetEx();");

    CreateSmaaVertexArray();

    return hRet;
}

LRESULT WINAPI CustomWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(uMsg == WM_ACTIVATE || uMsg == WM_NCACTIVATE) {
        WndProc(hWnd, uMsg, wParam, lParam);

        switch(LOWORD(wParam)) {
            case WA_INACTIVE:
                SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
                break;
            default: // WA_ACTIVE or WA_CLICKACTIVE
                SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
                break;
        }

        return 0;
    }

    return WndProc(hWnd, uMsg, wParam, lParam);
}

// DoNotNotifyOnTaskSwitch 
ATOM __stdcall hk_RegisterClassA(WNDCLASSA* lpWndClass) {
    WndProc = lpWndClass->lpfnWndProc;
    lpWndClass->lpfnWndProc = CustomWndProc;
    return o_RegisterClassA(lpWndClass);
}
ATOM __stdcall hk_RegisterClassW(WNDCLASSW* lpWndClass) {
    WndProc = lpWndClass->lpfnWndProc;
    lpWndClass->lpfnWndProc = CustomWndProc;
    return o_RegisterClassW(lpWndClass);
}
ATOM __stdcall hk_RegisterClassExA(WNDCLASSEXA* lpWndClass) {
    WndProc = lpWndClass->lpfnWndProc;
    lpWndClass->lpfnWndProc = CustomWndProc;
    return o_RegisterClassExA(lpWndClass);
}
ATOM __stdcall hk_RegisterClassExW(WNDCLASSEXW* lpWndClass) {
    WndProc = lpWndClass->lpfnWndProc;
    lpWndClass->lpfnWndProc = CustomWndProc;
    return o_RegisterClassExW(lpWndClass);
}

// appdata to game folder
HRESULT __stdcall hk_SHGetFolderPathA(_Reserved_ HWND hwnd, _In_ int csidl, _In_opt_ HANDLE hToken, _In_ DWORD dwFlags, _Out_writes_(MAX_PATH) LPSTR pszPath) {
    char path[MAX_PATH] = { 0 };
    HMODULE hm = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR) &Direct3DCreate9, &hm);
    GetModuleFileNameA(hm, path, MAX_PATH);
    *strrchr(path, '\\') = '\0';
    strcat_s(path, "\\profile\\");
    HRESULT hr = o_SHGetFolderPathA(hwnd, csidl, hToken, dwFlags, pszPath);
    switch(csidl) {
        case CSIDL_APPDATA:
        case CSIDL_COMMON_APPDATA:
        case CSIDL_COMMON_DOCUMENTS:
        case CSIDL_LOCAL_APPDATA:
        case CSIDL_PERSONAL:
            strcpy(pszPath, path);
            break;
        default:
            break;
    }
    return hr;
}
HRESULT __stdcall hk_SHGetFolderPathW(_Reserved_ HWND hwnd, _In_ int csidl, _In_opt_ HANDLE hToken, _In_ DWORD dwFlags, _Out_writes_(MAX_PATH) LPWSTR pszPath) {
    wchar_t path[MAX_PATH] = { 0 };
    HMODULE hm = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR) &Direct3DCreate9, &hm);
    GetModuleFileNameW(hm, path, MAX_PATH);
    *wcsrchr(path, '\\') = '\0';
    wcscat(path, L"\\profile\\");
    HRESULT hr = o_SHGetFolderPathW(hwnd, csidl, hToken, dwFlags, pszPath);
    switch(csidl) {
        case CSIDL_APPDATA:
        case CSIDL_COMMON_APPDATA:
        case CSIDL_COMMON_DOCUMENTS:
        case CSIDL_LOCAL_APPDATA:
        case CSIDL_PERSONAL:
            wcscpy(pszPath, path);
            break;
        default:
            break;
    }
    return hr;
}

// module and function list
HMODULE WINAPI hk_LoadLibraryA  (LPCSTR lpLibFileName) {
    HMODULE hr = o_LoadLibraryA(lpLibFileName);

    if(hr)
        hmoduleList.push_back(stModule(lpLibFileName, hr));
    return hr;
}
HMODULE WINAPI hk_LoadLibraryW  (LPCWSTR lpLibFileName) {
    HMODULE hr = o_LoadLibraryW(lpLibFileName);
    if(hr)
        hmoduleList.push_back(stModule(std::string(pcwstr_to_pcstr(lpLibFileName)), hr));
    return hr;
}
HMODULE WINAPI hk_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
    HMODULE hr = o_LoadLibraryExW(lpLibFileName, hFile, dwFlags);
    if(hr)
        hmoduleList.push_back(stModule(std::string(pcwstr_to_pcstr(lpLibFileName)), hr));
    return hr;
}
HMODULE WINAPI hk_LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
    HMODULE hr = o_LoadLibraryExA(lpLibFileName, hFile, dwFlags);
    if(hr)
        hmoduleList.push_back(stModule(std::string(lpLibFileName), hr));
    return hr;
}
FARPROC WINAPI hk_GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    FARPROC hr = o_GetProcAddress(hModule, lpProcName);
    if(hr)
        addFunction(lpProcName, hModule);
    return hr;
}

 //IDirectInputA *diA = nullptr;
 //IDirectInputW *diW = nullptr;

HRESULT WINAPI hk_DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter) {
    HRESULT hr = S_FALSE;
    if(o_DirectInput8Create) {
        hr = o_DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
        //if(hr == S_OK) {
        //    diA = (IDirectInputA*) ppvOut;
        //    diW = (IDirectInputW*) ppvOut;
        //}
    }
    return hr;
}

BOOL WINAPI hk_ClipCursor(const RECT* lpRect) {
    lpRect = nullptr;
    if(o_ClipCursor)
        return o_ClipCursor(lpRect);
    return ClipCursor(lpRect);
}


LRESULT CALLBACK WndProcH(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(gShadowResFix->OnWndProc(hWnd, uMsg, wParam, lParam))
        return true;
    //if(gShadowResFix->mShowWindow) {
    //    return true;
    //}
    return CallWindowProc(o_WndProc, hWnd, uMsg, wParam, lParam);
}

// maybe not the best way to do this but it works, its simple and doesn't cause any issues with the GtaIV
// but it doesn't work for other games
HRESULT __stdcall DInput8DeviceGetDeviceStateH(IDirectInputDevice8* This, DWORD cbData, LPVOID lpvData) {
    HRESULT hr = o_DInput8DeviceGetDeviceState(This, cbData, lpvData);

    if(gShadowResFix->mDisableMouseControl) {
        if(cbData == sizeof(DIMOUSESTATE) || cbData == sizeof(DIMOUSESTATE2)) {
            This->Unacquire();
        }
    }

    return hr;
}

HRESULT __stdcall DInput8DeviceAcquireH(IDirectInputDevice8* This) {
    if(gShadowResFix->mDisableMouseControl) {
        return DI_OK;
    }

    return o_DInput8DeviceAcquire(This);
}

int hookWait = 10;

bool Initialize() {
    if(!gWindowWidth || !gWindowHeight)
        return false;
   
    std::stringstream logStream;
    MH_STATUS mhStatus;
    baseAddress = (uint8_t*) GetModuleHandle(NULL);
    gShadowResFix->Initialize(baseAddress);
    //gShadowResFix.LoadSettings();

    if(!*gDinpu8Device_vtbl) {
        if(!Utils::GetDInput8DeviceVTable(gDinpu8Device_vtbl)) {
            return false;
        }
    }
    if(*gDinpu8Device_vtbl) {
        if(!o_DInput8DeviceGetDeviceState) {
            mhStatus = MH_CreateHook(gDinpu8Device_vtbl[9], &DInput8DeviceGetDeviceStateH, (void**) &o_DInput8DeviceGetDeviceState);
            if(mhStatus != MH_OK) {
                logStream << "IDirectInputDevice8::GetDeviceState hook could not be created - " << MH_StatusToString(mhStatus);
                Log::Error(logStream.str());

                return false;
            }

            Log::Info("Created IDirectInputDevice8::GetDeviceState hook");
        }

        if(!o_DInput8DeviceAcquire) {
            mhStatus = MH_CreateHook(gDinpu8Device_vtbl[7], &DInput8DeviceAcquireH, (void**) &o_DInput8DeviceAcquire);
            if(mhStatus != MH_OK) {
                logStream << "IDirectInputDevice8::Acquire hook could not be created - " << MH_StatusToString(mhStatus);
                Log::Error(logStream.str());

                return false;
            }

            Log::Info("Created IDirectInputDevice8::Acquire hook");
        }
    }

    EnumWindows(EnumWindowsProc, GetCurrentProcessId());
    if(windows.empty()) {
        /*MessageBox(0, "Unable to enumerate windows", "Shader Editor", MB_ICONWARNING);
        TerminateProcess(GetCurrentProcess(), 0);*/
        return false;
    }

    if(!o_WndProc) {
        o_WndProc = (WNDPROC) SetWindowLongPtr(windows[windows.size()-1] /*FindWindowA("grcWindow", "GTAIV")*/, GWL_WNDPROC, (LONG_PTR) WndProcH);

        if(!o_WndProc) {
            return false;
        }
        Log::Info("WndProc Hooked");
    }

    mhStatus = MH_EnableHook(MH_ALL_HOOKS);
    if(mhStatus != MH_OK) {
        logStream << "Failed to enable hooks - " << MH_StatusToString(mhStatus);
        Log::Error(logStream.str());

        return false;
    }
    Log::Info("Hooked finished");
    return true;
}

void MainLoop() {
    static bool initialized = false;

    while(!initialized) {
        Sleep(hookWait);

        initialized = Initialize();
    }

    DWORD exitCode;
    GetExitCodeThread(gMainThreadHandle, &exitCode);
    TerminateThread(gMainThreadHandle, exitCode);
}



IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion) {
    if(!o_pDirect3DCreate9) {
        return nullptr;
    }

    IDirect3D9* pDirect3D9 = o_pDirect3DCreate9(SDKVersion);

    if(pDirect3D9) {
        m_IDirect3D9Ex* pDirect3D9Ex = new m_IDirect3D9Ex((IDirect3D9Ex*) pDirect3D9, IID_IDirect3D9);
        if(pDirect3D9Ex)
            Log::Info("Direct3DCreate9(): " + std::to_string(SDKVersion));
        return pDirect3D9Ex;
    }

    return nullptr;
}

HRESULT WINAPI Direct3DCreate9Ex(UINT SDKVersion, IDirect3D9Ex** ppD3D) {
    if(!o_pDirect3DCreate9Ex) {
        return E_FAIL;
    }
    HRESULT hr = o_pDirect3DCreate9Ex(SDKVersion, ppD3D);

    if(SUCCEEDED(hr) && ppD3D) {
        *ppD3D = new m_IDirect3D9Ex(*ppD3D, IID_IDirect3D9Ex);
        Log::Info("Direct3DCreate9Ex(): " + std::to_string(SDKVersion));
    }

    return hr;
}


int WINAPI Direct3D9EnableMaximizedWindowedModeShim(BOOL mEnable) {
    if(!o_pDirect3D9EnableMaximizedWindowedModeShim) {
        return NULL;
    }

    return o_pDirect3D9EnableMaximizedWindowedModeShim(mEnable);
}

HRESULT WINAPI Direct3DShaderValidatorCreate9() {
    if(!o_pDirect3DShaderValidatorCreate9) {
        return E_FAIL;
    }

    return o_pDirect3DShaderValidatorCreate9();
}

HRESULT WINAPI PSGPSampleTexture() {
    if(!o_pPSGPSampleTexture) {
        return E_FAIL;
    }

    return o_pPSGPSampleTexture();
}

HRESULT WINAPI DebugSetLevel(DWORD dw1) {
    if(!o_pDebugSetLevel) {
        return E_FAIL;
    }

    return o_pDebugSetLevel(dw1);
}

void WINAPI DebugSetMute() {
    if(!o_pDebugSetMute) {
        return;
    }

    return o_pDebugSetMute();
}

HRESULT WINAPI PSGPError() {
    if(!o_pPSGPError) {
        return E_FAIL;
    }

    return o_pPSGPError();
}

int WINAPI D3DPERF_BeginEvent(D3DCOLOR col, LPCWSTR wszName) {
    if(!o_pD3DPERF_BeginEvent) {
        return NULL;
    }

    return o_pD3DPERF_BeginEvent(col, wszName);
}

int WINAPI D3DPERF_EndEvent() {
    if(!o_pD3DPERF_EndEvent) {
        return NULL;
    }

    return o_pD3DPERF_EndEvent();
}

DWORD WINAPI D3DPERF_GetStatus() {
    if(!o_pD3DPERF_GetStatus) {
        return NULL;
    }

    return o_pD3DPERF_GetStatus();
}

BOOL WINAPI D3DPERF_QueryRepeatFrame() {
    if(!o_pD3DPERF_QueryRepeatFrame) {
        return FALSE;
    }

    return o_pD3DPERF_QueryRepeatFrame();
}

void WINAPI D3DPERF_SetMarker(D3DCOLOR col, LPCWSTR wszName) {
    if(!o_pD3DPERF_SetMarker) {
        return;
    }

    return o_pD3DPERF_SetMarker(col, wszName);
}

void WINAPI D3DPERF_SetOptions(DWORD dwOptions) {
    if(!o_pD3DPERF_SetOptions) {
        return;
    }

    return o_pD3DPERF_SetOptions(dwOptions);
}

void WINAPI D3DPERF_SetRegion(D3DCOLOR col, LPCWSTR wszName) {
    if(!o_pD3DPERF_SetRegion) {
        return;
    }

    return o_pD3DPERF_SetRegion(col, wszName);
}


bool WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    static HMODULE d3d9dll = nullptr;
    static HMODULE SHELL32_dll = nullptr;
    static HMODULE kernel32_dll = nullptr;
    static HMODULE dinput8_dll = nullptr;
    static HMODULE user32_dll = nullptr;

    //bool isAsi = false;
    switch(dwReason) {
        case DLL_PROCESS_ATTACH:
        {
            ps.resize(shader_names_ps.size());
            vs.resize(shader_names_vs.size());
            fx_ps.resize(shader_names_fxc.size());
            fx_vs.resize(shader_names_fxc.size());

            gShadowResFix = new ShadowResFix;
            // Load dll
            char path[MAX_PATH];
            // ini
            HMODULE hm = NULL;

            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR) &Direct3DCreate9, &hm);
            GetModuleFileNameA(hm, path, sizeof(path));
            std::string ext = path;
            //ext = ext.substr(ext.find_last_of(".") + 1);
            //if(ext == "asi") {
            //    isAsi = true;
            //}

            *strrchr(path, '\\') = '\0';
            strcat_s(path, "\\shadowresfix.ini");
            int rescale = 1;
            // read ini
            {
                bForceWindowedMode = GetPrivateProfileInt("MAIN", "ForceWindowedMode", 0, path) != 0;
                fFPSLimit = static_cast<float>(GetPrivateProfileInt("MAIN", "FPSLimit", 0, path));
                nFullScreenRefreshRateInHz = GetPrivateProfileInt("MAIN", "FullScreenRefreshRateInHz", 0, path);
                bDisplayFPSCounter = GetPrivateProfileInt("MAIN", "DisplayFPSCounter", 0, path);

                bUsePrimaryMonitor = GetPrivateProfileInt("FORCEWINDOWED", "UsePrimaryMonitor", 0, path) != 0;
                bCenterWindow = GetPrivateProfileInt("FORCEWINDOWED", "CenterWindow", 1, path) != 0;
                bBorderlessFullscreen = GetPrivateProfileInt("FORCEWINDOWED", "BorderlessFullscreen", 0, path) != 0;
                bAlwaysOnTop = GetPrivateProfileInt("FORCEWINDOWED", "AlwaysOnTop", 0, path) != 0;
                bDoNotNotifyOnTaskSwitch = GetPrivateProfileInt("FORCEWINDOWED", "DoNotNotifyOnTaskSwitch", 0, path) != 0;

                gEnableProxyLibrary = GetPrivateProfileInt("PROXY", "EnableProxyLibrary", 0, path) != 0;
                GetPrivateProfileString("PROXY", "ProxyLibrary", "d3d9.dll", ProxyLibrary, MAX_PATH, path);

                bHookDirect3DCreate9 = GetPrivateProfileInt("SHADOWRESFIX", "HookDirect3DCreate9", 0, path) != 0;
                gTreeLeavesSwayInTheWind = GetPrivateProfileInt("SHADOWRESFIX", "TreeLeavesSwayInTheWind", 0, path) != 0;
                gFixCascadedShadowMapResolution = GetPrivateProfileInt("SHADOWRESFIX", "FixCascadedShadowMapResolution", 0, path) != 0;
                gFixRainDrops = GetPrivateProfileInt("SHADOWRESFIX", "FixRainDrops", 0, path) != 0;
                gFixWashedMirror = GetPrivateProfileInt("SHADOWRESFIX", "FixWashedMirror", 0, path) != 0;
                gWindowDivisor = GetPrivateProfileInt("SHADOWRESFIX", "RainDropsBlur", 4, path);
                gNearFarPlane = GetPrivateProfileInt("SHADOWRESFIX", "FixNearFarPlane", 0, path) != 0;
                gReflectionResMult = GetPrivateProfileInt("SHADOWRESFIX", "ReflectionResMult", 2, path);
                bHook_SHGetFolderPath = GetPrivateProfileInt("SHADOWRESFIX", "ProfileOnGameFolder", 0, path) != 0;
                gLightResMult = GetPrivateProfileInt("SHADOWRESFIX", "LightResolutionMult", 1, path);
                bSaveSettingsOnExit = GetPrivateProfileInt("SHADOWRESFIX", "SaveSettingsOnExit", 0, path) != 0;

                DisableADAPTIVETESS_X = GetPrivateProfileInt("SHADOWRESFIX", "DisableADAPTIVETESS_X", 0, path) != 0;
                EnableDepthOverwrite = GetPrivateProfileInt("SHADOWRESFIX", "EnableDepthOverwrite", 0, path) != 0;
                UsePresentToRenderUI = GetPrivateProfileInt("SHADOWRESFIX", "UsePresentToRenderUI", 0, path) != 0;
                AlowPauseGame = GetPrivateProfileInt("SHADOWRESFIX", "AlowPauseGame", 0, path) != 0;
                UseSSAA = GetPrivateProfileInt("SHADOWRESFIX", "UseSSAA", 0, path) != 0;

                rescale = GetPrivateProfileInt("SHADOWRESFIX", "ResolutionScale", 1, path) ;

                hookWait = GetPrivateProfileInt("SHADOWRESFIX", "hookDelayMS", 0, path);

                if(fFPSLimit > 0.0f) {
                    FrameLimiter::FPSLimitMode mode = (GetPrivateProfileInt("MAIN", "FPSLimitMode", 1, path) == 2) ? FrameLimiter::FPSLimitMode::FPS_ACCURATE : FrameLimiter::FPSLimitMode::FPS_REALTIME;
                    if(mode == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
                        timeBeginPeriod(1);

                    FrameLimiter::Init(mode);
                    mFPSLimitMode = mode;
                }
                else {
                    mFPSLimitMode = FrameLimiter::FPSLimitMode::FPS_NONE;
                }
            }

            ResSSAA = rescale;
            if(rescale == 0)
                ResSSAA = 1;
            if(rescale < 0)
                ResSSAA = 1.f/ abs(rescale);

            // validate values
            switch(gReflectionResMult) {
                case  2:
                case  4:
                case  8:
                case  16:
                {
                    break;
                }
                default:
                {
                    gReflectionResMult = 1;
                    break;
                }
            }

            switch(gLightResMult) {
                case  2:
                case  4:
                case  8:
                {
                    break;
                }
                default:
                {
                    gLightResMult = 1;
                    break;
                }
            }

            switch(gWindowDivisor) {
                case  1:
                case  2:
                case  4:
                {
                    break;
                }
                default:
                {
                    gWindowDivisor = 4;
                    break;
                }
            }

            // profile on game folder
            if(bHook_SHGetFolderPath) {
                //SHELL32_dll = LoadLibraryA("SHELL32.dll");
                SHELL32_dll = GetModuleHandleA("SHELL32.dll");
                if(SHELL32_dll) {
                    o_SHGetFolderPathA = (SHGetFolderPathA_Ptr) GetProcAddress(SHELL32_dll, "SHGetFolderPathA");
                    o_SHGetFolderPathW = (SHGetFolderPathW_Ptr) GetProcAddress(SHELL32_dll, "SHGetFolderPathW");
                    
                    /*o_SHGetFolderPathA = (SHGetFolderPathA_Ptr)*/ Iat_hook::detour_iat_ptr("SHGetFolderPathA", (void*) hk_SHGetFolderPathA);
                    /*o_SHGetFolderPathW = (SHGetFolderPathW_Ptr)*/ Iat_hook::detour_iat_ptr("SHGetFolderPathW", (void*) hk_SHGetFolderPathW);
                }
            }
            {
                //dinput8_dll = GetModuleHandleA("DINPUT8.dll");
                dinput8_dll = LoadLibraryA("DINPUT8.dll");
                if(dinput8_dll) {
                    o_DirectInput8Create = (DirectInput8Create_ptr) GetProcAddress(dinput8_dll, "DirectInput8Create");
                    if(o_DirectInput8Create) {
                        Iat_hook::detour_iat_ptr("DirectInput8Create", (void*) hk_DirectInput8Create, 0);
                    }
                }
            }
             //window aways on top
            if(bDoNotNotifyOnTaskSwitch) {
                HMODULE user32 = GetModuleHandleA("user32.dll");
                //if(!user32) {
                //    user32 = LoadLibraryA("user32.dll");
                //}
                if(user32) {
                    o_RegisterClassA   = (RegisterClassA_fn)   GetProcAddress(user32, "RegisterClassA"  );
                    o_RegisterClassW   = (RegisterClassW_fn)   GetProcAddress(user32, "RegisterClassW"  );
                    o_RegisterClassExA = (RegisterClassExA_fn) GetProcAddress(user32, "RegisterClassExA");
                    o_RegisterClassExW = (RegisterClassExW_fn) GetProcAddress(user32, "RegisterClassExW");

                    /*o_RegisterClassA   = (RegisterClassA_fn)   */ Iat_hook::detour_iat_ptr("RegisterClassA"  , (void*) hk_RegisterClassA  );
                    /*o_RegisterClassW   = (RegisterClassW_fn)   */ Iat_hook::detour_iat_ptr("RegisterClassW"  , (void*) hk_RegisterClassW  );
                    /*o_RegisterClassExA = (RegisterClassExA_fn) */ Iat_hook::detour_iat_ptr("RegisterClassExA", (void*) hk_RegisterClassExA);
                    /*o_RegisterClassExW = (RegisterClassExW_fn) */ Iat_hook::detour_iat_ptr("RegisterClassExW", (void*) hk_RegisterClassExW);
                }
            }

            // function hook for a specific dll such as dxvk
            if(gEnableProxyLibrary) {
                d3d9dll = LoadLibraryA(ProxyLibrary);
                if(!d3d9dll) {
                    char buff[1000] = { 0 };
                    sprintf(buff, "Unable to open:\n\"%s\"", ProxyLibrary);
                    MessageBox(0, TEXT(buff), TEXT("Shader Editor"), MB_ICONWARNING);
                }
            }
            else if(bHookDirect3DCreate9) {
                if(false) {
                    char dllname[100] = { 0 };
                    for(int version = 43; version > 23; version--) {
                        sprintf(dllname, "d3dx9_%i.dll", version);
                        d3d9dll = LoadLibraryA(dllname);
                        if(d3d9dll) {
                            Direct3DCreate9ExProc proc1 = (Direct3DCreate9ExProc) GetProcAddress(d3d9dll, "Direct3DCreate9Ex");
                            Direct3DCreate9Proc proc2 = (Direct3DCreate9Proc) GetProcAddress(d3d9dll, "Direct3DCreate9");
                            if(proc1 && proc2) {
                                Log::Info(std::string("DirectX dll found = ") + dllname);
                                break;
                            }
                            else {
                                FreeLibrary(d3d9dll);
                                d3d9dll = nullptr;
                            }
                        }
                    }
                }
                if(!d3d9dll) {
                    // system d3d9
                    GetSystemDirectoryA(path, MAX_PATH);
                    strcat_s(path, "\\d3d9.dll");
                    d3d9dll = LoadLibraryA(path);
                    if(!d3d9dll) {
                        char buff[1000] = { 0 };
                        sprintf(buff, "Unable to open:\n\"%s\"", path);
                        MessageBox(0, TEXT(buff), TEXT("Shader Editor"), MB_ICONWARNING);
                    }
                }
            }
            
            {
                //user32_dll = GetModuleHandleA("user32.dll");
                //if(user32_dll) {
                //    o_ClipCursor = (ClipCursor_Ptr) GetProcAddress(user32_dll, "ClipCursor" );
                //    Iat_hook::detour_iat_ptr("ClipCursor" ,  (void*) hk_ClipCursor);
                //}
            }

            // hook d3d9
            if(d3d9dll){
                o_pDirect3DCreate9Ex                        = (Direct3DCreate9ExProc)                           GetProcAddress(d3d9dll, "Direct3DCreate9Ex"                         );
                o_pDirect3DCreate9                          = (Direct3DCreate9Proc)                             GetProcAddress(d3d9dll, "Direct3DCreate9"                           );
                
                o_pDirect3D9EnableMaximizedWindowedModeShim = (Direct3D9EnableMaximizedWindowedModeShimProc)    GetProcAddress(d3d9dll, "Direct3D9EnableMaximizedWindowedModeShim"  );
                o_pDirect3DShaderValidatorCreate9           = (Direct3DShaderValidatorCreate9Proc)              GetProcAddress(d3d9dll, "Direct3DShaderValidatorCreate9"            );
                o_pD3DPERF_QueryRepeatFrame                 = (D3DPERF_QueryRepeatFrameProc)                    GetProcAddress(d3d9dll, "D3DPERF_QueryRepeatFrame"                  );
                o_pD3DPERF_BeginEvent                       = (D3DPERF_BeginEventProc)                          GetProcAddress(d3d9dll, "D3DPERF_BeginEvent"                        );
                o_pD3DPERF_SetOptions                       = (D3DPERF_SetOptionsProc)                          GetProcAddress(d3d9dll, "D3DPERF_SetOptions"                        );
                o_pPSGPSampleTexture                        = (PSGPSampleTextureProc)                           GetProcAddress(d3d9dll, "PSGPSampleTexture"                         );
                o_pD3DPERF_GetStatus                        = (D3DPERF_GetStatusProc)                           GetProcAddress(d3d9dll, "D3DPERF_GetStatus"                         );
                o_pD3DPERF_SetMarker                        = (D3DPERF_SetMarkerProc)                           GetProcAddress(d3d9dll, "D3DPERF_SetMarker"                         );
                o_pD3DPERF_SetRegion                        = (D3DPERF_SetRegionProc)                           GetProcAddress(d3d9dll, "D3DPERF_SetRegion"                         );
                o_pD3DPERF_EndEvent                         = (D3DPERF_EndEventProc)                            GetProcAddress(d3d9dll, "D3DPERF_EndEvent"                          );
                o_pDebugSetLevel                            = (DebugSetLevelProc)                               GetProcAddress(d3d9dll, "DebugSetLevel"                             );
                o_pDebugSetMute                             = (DebugSetMuteProc)                                GetProcAddress(d3d9dll, "DebugSetMute"                              );
                o_pPSGPError                                = (PSGPErrorProc)                                   GetProcAddress(d3d9dll, "PSGPError"                                 );

                Iat_hook::detour_iat_ptr("Direct3DCreate9Ex"                         , (void*) Direct3DCreate9Ex                         );
                Iat_hook::detour_iat_ptr("Direct3DCreate9"                           , (void*) Direct3DCreate9                           );

                Iat_hook::detour_iat_ptr("Direct3D9EnableMaximizedWindowedModeShim"  , (void*) Direct3D9EnableMaximizedWindowedModeShim  );
                Iat_hook::detour_iat_ptr("Direct3DShaderValidatorCreate9"            , (void*) Direct3DShaderValidatorCreate9            );
                Iat_hook::detour_iat_ptr("D3DPERF_QueryRepeatFrame"                  , (void*) D3DPERF_QueryRepeatFrame                  );
                Iat_hook::detour_iat_ptr("D3DPERF_BeginEvent"                        , (void*) D3DPERF_BeginEvent                        );
                Iat_hook::detour_iat_ptr("D3DPERF_SetOptions"                        , (void*) D3DPERF_SetOptions                        );
                Iat_hook::detour_iat_ptr("PSGPSampleTexture"                         , (void*) PSGPSampleTexture                         );
                Iat_hook::detour_iat_ptr("D3DPERF_GetStatus"                         , (void*) D3DPERF_GetStatus                         );
                Iat_hook::detour_iat_ptr("D3DPERF_SetMarker"                         , (void*) D3DPERF_SetMarker                         );
                Iat_hook::detour_iat_ptr("D3DPERF_SetRegion"                         , (void*) D3DPERF_SetRegion                         );
                Iat_hook::detour_iat_ptr("D3DPERF_EndEvent"                          , (void*) D3DPERF_EndEvent                          );
                Iat_hook::detour_iat_ptr("DebugSetLevel"                             , (void*) DebugSetLevel                             );
                Iat_hook::detour_iat_ptr("DebugSetMute"                              , (void*) DebugSetMute                              );
                Iat_hook::detour_iat_ptr("PSGPError"                                 , (void*) PSGPError                                 );
                
            }

            // create module list
            if(false) {
                HMODULE kernel32_dll = GetModuleHandleA("kernel32.dll");
                if(kernel32_dll) {
                    o_LoadLibraryA   = (LoadLibraryA_Ptr  ) GetProcAddress(kernel32_dll, "o_LoadLibraryA"  );
                    o_LoadLibraryW   = (LoadLibraryW_Ptr  ) GetProcAddress(kernel32_dll, "o_LoadLibraryW"  );
                    o_LoadLibraryExW = (LoadLibraryExW_Ptr) GetProcAddress(kernel32_dll, "o_LoadLibraryExW");
                    o_LoadLibraryExA = (LoadLibraryExA_Ptr) GetProcAddress(kernel32_dll, "o_LoadLibraryExA");
                    o_GetProcAddress = (GetProcAddress_Ptr) GetProcAddress(kernel32_dll, "o_GetProcAddress");

                    Iat_hook::detour_iat_ptr("o_LoadLibraryA"  , (void*) hk_LoadLibraryA   );
                    Iat_hook::detour_iat_ptr("o_LoadLibraryW"  , (void*) hk_LoadLibraryW   );
                    Iat_hook::detour_iat_ptr("o_LoadLibraryExW", (void*) hk_LoadLibraryExW );
                    Iat_hook::detour_iat_ptr("o_LoadLibraryExA", (void*) hk_LoadLibraryExA );
                    Iat_hook::detour_iat_ptr("o_GetProcAddress", (void*) hk_GetProcAddress );
                }
            }

            // mhook and log
            {
                if(!Log::Initialize()) {
                    return false;
                }

                std::stringstream logStream;
                int32_t gameVersion = 0;

                if(!Utils::GetGameVersion(gameVersion)) {
                    logStream << "Shader Editor only fully supports GTAIV 1.2.00 Complete Edition with FusionFix shaders." << "\n";
                }

                logStream << "Game Version: " << gameVersion << "\n";

                MH_STATUS mhStatus = MH_Initialize();
                if(mhStatus != MH_OK) {
                    logStream << "MinHook could not be initialized - " << MH_StatusToString(mhStatus) << "\n";
                    return false;
                }

                Log::Info(logStream.str());
                Log::Info("MinHook initialized");

                gMainThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) MainLoop, NULL, 0, NULL);
            }
        }
        break;
        case DLL_PROCESS_DETACH:
        {
            if(bSaveSettingsOnExit)
                gShadowResFix->SaveSettings();

            if(mFPSLimitMode == FrameLimiter::FPSLimitMode::FPS_ACCURATE)
                timeEndPeriod(1);
        }
        break;
    }

    return true;
}







