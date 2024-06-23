#include "ShadowResFix.h"
#include "d3dcompiler.h"
#include <iostream>
#include <ostream>
#include <istream>
#include <stdlib.h>

#include "var2.hpp"

#pragma comment( lib, "d3dcompiler.lib" )

extern m_IDirect3DTexture9* NormalTex;
extern m_IDirect3DTexture9* DiffuseTex;
extern m_IDirect3DTexture9* SpecularTex;
extern m_IDirect3DTexture9* DepthTex;
extern m_IDirect3DTexture9* StencilTex;
extern m_IDirect3DTexture9* BloomTex;

extern IDirect3DTexture9* pRainDropsRefractionHDRTex;
extern std::list<m_IDirect3DTexture9*> textureList;


extern IDirect3DPixelShader9* SMAA_EdgeDetection ;
extern IDirect3DPixelShader9* SMAA_BlendingWeightsCalculation ;
extern IDirect3DPixelShader9* SMAA_NeighborhoodBlending ;

extern IDirect3DVertexShader9* SMAA_EdgeDetectionVS ;
extern IDirect3DVertexShader9* SMAA_BlendingWeightsCalculationVS ;
extern IDirect3DVertexShader9* SMAA_NeighborhoodBlendingVS ;

extern IDirect3DPixelShader9* SSAO_ps;
extern IDirect3DVertexShader9* SSAO_vs;

extern bool UseSSAO;


extern bool bDisplayFPSCounter;
extern bool gFixRainDrops;
extern bool gTreeLeavesSwayInTheWind;
extern bool gNearFarPlane;

extern UINT gWindowWidth;
extern UINT gWindowHeight;

bool fixCoronaDepth = 1;
bool useMotionBlur = 0;
int UseDebugTextures = 0;
int UsePostFxAA[4] = { 1 }; // none, fxaa pre, fxaa pos, smaa, blend, edge

float PostFxFog[4] = { 1,1,(-1.f/gWindowWidth)*0.5,(-1.f/gWindowHeight)*0.5 }; 

long long g_id = 0;

extern float shaderReturnColor[4] = { 0.f };

extern float AoDistance;
extern float FocusPoint[4] ;
extern float FocusScale[4] ;

extern float SunCentre[4] ;
extern float SunColor [4] ;
extern float SunDirection[4] ;

extern int DofSamples[4];

extern float DEPTHBIAS;
extern float SLOPESCALEDEPTHBIAS;

const char* TexNames[] = {
    "hdr",
    "edgesTex",
    "areaTex",
    "searchTex",
    "blendTex",
    ""
};

// name, src, editor

extern IDirect3DPixelShader9* CompilePixelShaderFromFile(const char* filename, const char* mainFunction, const char* name, m_IDirect3DDevice9Ex* m_pDeviceEx, bool isAsm, bool addToList);
extern IDirect3DVertexShader9* CompileVertexShaderFromFile(const char* filename, const char* mainFunction, const char* name, m_IDirect3DDevice9Ex* m_pDeviceEx, bool isAsm, bool addToList);

struct stShaderEditor {
    bool show;
    ShaderType shaderType;
    std::string name;
    TextEditor* editor;
    basicShader* bs;
    stShaderEditor(std::string _name, ShaderType _shaderType, TextEditor* _editor, basicShader* _bs) :show(true), name(_name), shaderType(_shaderType), editor(_editor), bs(_bs) {}
};
std::list<stShaderEditor*> lst;

extern uint8_t* baseAddress;

bool* IsGamePaused;
bool* IsGameOnMenu;
bool* IsInInterior;

extern bool DisableADAPTIVETESS_X;
extern bool EnableDepthOverwrite;
bool AlowPauseGame = 1;
bool usePrimitiveUp = false;

extern bool useDof;
extern int UseSunShafts;
extern int useDepthOfField ;
//extern bool useDepthOfField2;
extern bool useStippleFilter;
extern bool useNewShadowAtlas;

extern bool fixDistantOutlineUsingDXVK;


extern LPDIRECT3DPIXELSHADER9       ImGuiPS;
extern LPDIRECT3DVERTEXSHADER9      ImGuiVS;

ShadowResFix::ShadowResFix() :
    mDisableMouseControl(false),
    mIsImGuiInitialized(false),
    mSettingsFileMajorVersion(1),
    mSettingsFileMinorVersion(1),
    mShowWindow(false),
    mShowEditor(false),
    showEditorWindow(false),
    mShowLogWindow(true),
    bShowLogWindow(true),
    mShowSettingsWindow(false),
    showDemoWindow(false),
    pauseGame(false),
    GameVersion(0),
    mOpenWindowKey(ImGuiKey_F10),
    mCompileShader(ImGuiKey_F6),
    mWindowPos(ImVec2(5.f, 5.f)),
    mWindowSize(ImVec2(440.f, 650.f)),
    mEditorPos(ImVec2(0.f, 0.f)),
    mEditorSize(ImVec2(0.f, 0.f)),
    mLogPos(ImVec2(0.f, 0.f)),
    mLogSize(ImVec2(0.f, 0.f)),
    mItemInnerSpacing(4.f),
    mFontScale(0.9f),
    mToggleCameraControlKey(ImGuiKey_None) {
};
ShadowResFix::~ShadowResFix() {};

void ShadowResFix::Initialize(const uint8_t* baseAddress) {
    static bool def = false;
    Utils::GetGameVersion(GameVersion);
    if(GameVersion == 1200) {
        IsGamePaused = (bool*) (baseAddress + 0xD73590);
        IsGameOnMenu = (bool*) (baseAddress + 0xC30B7C);
        IsInInterior = (bool*) (baseAddress + 0x1320FA8);
    }
    else {
        IsGamePaused = IsGameOnMenu = IsInInterior = &def;
    }

    //int32_t gameVersion;
    //Utils::GetGameVersion(gameVersion);
    //
    //switch(gameVersion)
    //{
    //	case 1040:
    //		mTimeCycle = (Timecycle*)(baseAddress + 0xCF46F0);
    //		mHour = (int32_t*)(baseAddress + 0xC7AD84);
    //		mMinutes = (int32_t*)(baseAddress + 0xC7AD80);
    //		mTimerLength = (uint32_t*)(baseAddress + 0xC7AD88);
    //		FORCE_WEATHER_NOW = (FORCE_WEATHER_NOWT*)(baseAddress + 0x446930);
    //		RELEASE_WEATHER = (RELEASE_WEATHERT*)(baseAddress + 0x4469A0);
    //		SET_TIME_ONE_DAY_FORWARD = (SET_TIME_ONE_DAY_FORWARDT*)(baseAddress + 0x5CB090);
    //		SET_TIME_ONE_DAY_BACK = (SET_TIME_ONE_DAY_BACKT*)(baseAddress + 0x5CB0D0);
    //	break;
    //
    //	case 1080:
    //		mTimeCycle = (Timecycle*)(baseAddress + 0xFF1150);
    //		mHour = (int32_t*)(baseAddress + 0xD51694);
    //		mMinutes = (int32_t*)(baseAddress + 0xD51690);
    //		mTimerLength = (uint32_t*)(baseAddress + 0xD51698);
    //		FORCE_WEATHER_NOW = (FORCE_WEATHER_NOWT*)(baseAddress + 0x5A0910);
    //		RELEASE_WEATHER = (RELEASE_WEATHERT*)(baseAddress + 0x5A0980);
    //		SET_TIME_ONE_DAY_FORWARD = (SET_TIME_ONE_DAY_FORWARDT*)(baseAddress + 0x711E50);
    //		SET_TIME_ONE_DAY_BACK = (SET_TIME_ONE_DAY_BACKT*)(baseAddress + 0x711E90);
    //	break;
    //	case 1200:
    //		mTimeCycle = new Timecycle;
    //		mHour = new int32_t;
    //		mMinutes = new int32_t;
    //		mTimerLength = new uint32_t;
    //		FORCE_WEATHER_NOW = 0;
    //		RELEASE_WEATHER = 0;
    //		SET_TIME_ONE_DAY_FORWARD = 0;
    //		SET_TIME_ONE_DAY_BACK = 0;
    //		break;
    //	default:
    //	{
    //		mTimeCycle = new Timecycle;
    //		mHour = new int32_t;
    //		mMinutes = new int32_t;
    //		mTimerLength = new uint32_t;
    //		FORCE_WEATHER_NOW = 0;
    //		RELEASE_WEATHER = 0;
    //		SET_TIME_ONE_DAY_FORWARD = 0;
    //		SET_TIME_ONE_DAY_BACK = 0;
    //
    //		break;
    //	}
    //}

    LoadSettings();
    //mTimeCycle->Load("pc/data/timecyc.dat", NULL, 0);
    //InitializeColors();
}

void ShadowResFix::InitializeImGui(IDirect3DDevice9* d3d9Device) {
    if(!mIsImGuiInitialized) {
        D3DDEVICE_CREATION_PARAMETERS creationParams;
        d3d9Device->GetCreationParameters(&creationParams);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void) io;

        io.IniFilename = NULL;

        //ImGui Style
        {
            ImGuiStyle* style = &ImGui::GetStyle();
            style->FrameRounding = 1;
            style->WindowPadding.x = 20;
            style->WindowPadding.y = 10;
            style->FramePadding.x = 1;
            style->FramePadding.y = 3;
            style->ItemSpacing.x = 10;
            style->ItemSpacing.x = 5;
            style->ScrollbarSize = 20;
            style->ScrollbarRounding = 1;
            style->GrabMinSize = 15;

            style->WindowBorderSize = 0;
            style->WindowRounding = 1;

            style->WindowTitleAlign.x = 0.5;
            style->WindowTitleAlign.y = 0.5;
            style->WindowMenuButtonPosition = 0;

            style->SeparatorTextBorderSize = 3;
        }
        //ImGui Color
        {
            ImVec4* colors = ImGui::GetStyle().Colors;
            colors[ImGuiCol_Text] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
            colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
            colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
            colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
            colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
            colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
            colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colors[ImGuiCol_CheckMark] = ImVec4(0.84f, 0.84f, 0.84f, 1.00f);
            colors[ImGuiCol_SliderGrab] = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
            colors[ImGuiCol_Button] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
            colors[ImGuiCol_Header] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
            colors[ImGuiCol_Separator] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colors[ImGuiCol_SeparatorHovered] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
            colors[ImGuiCol_SeparatorActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
            colors[ImGuiCol_ResizeGrip] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.72f, 0.72f, 0.72f, 1.00f);
            colors[ImGuiCol_ResizeGripActive] = ImVec4(0.77f, 0.77f, 0.77f, 1.00f);
            colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
            colors[ImGuiCol_TabActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
            colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
            colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
            colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
            colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
            colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
            colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
            colors[ImGuiCol_TextSelectedBg] = ImVec4(0.43f, 0.43f, 0.43f, 0.98f);
            colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        }

        ImGui_ImplWin32_Init(creationParams.hFocusWindow);
        ImGui_ImplDX9_Init(d3d9Device);

        ImFontConfig conf = {};
        conf.SizePixels = 13;
        conf.OversampleH = 2;
        conf.OversampleV = 2;

        static const ImWchar ranges[] =
        {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0,
        };
        const ImWchar* glyph_ranges = ranges;

        io.Fonts->AddFontFromMemoryCompressedBase85TTF(gCousineRegularCompressedDataBase85, conf.SizePixels, &conf, glyph_ranges);
        io.FontGlobalScale = mFontScale;

        mIsImGuiInitialized = true;

        // Text Editor
        {
            auto lang = TextEditor::LanguageDefinition::HLSL();

            // set your own known preprocessor symbols...
            static const char* ppnames[] = { "NULL", "PM_REMOVE",
                "ZeroMemory", "DXGI_SWAP_EFFECT_DISCARD", "D3D_FEATURE_LEVEL", "D3D_DRIVER_TYPE_HARDWARE", "WINAPI","D3D11_SDK_VERSION", "assert" };
            // ... and their corresponding values
            static const char* ppvalues[] = {
                "#define NULL ((void*)0)",
                "#define PM_REMOVE (0x0001)",
                "Microsoft's own memory zapper function\n(which is a macro actually)\nvoid ZeroMemory(\n\t[in] PVOID  Destination,\n\t[in] SIZE_T Length\n); ",
                "enum DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD = 0",
                "enum D3D_FEATURE_LEVEL",
                "enum D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE  = ( D3D_DRIVER_TYPE_UNKNOWN + 1 )",
                "#define WINAPI __stdcall",
                "#define D3D11_SDK_VERSION (7)",
                " #define assert(expression) (void)(                                                  \n"
                "    (!!(expression)) ||                                                              \n"
                "    (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \n"
                " )"
            };

            for(int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i) {
                TextEditor::Identifier id;
                id.mDeclaration = ppvalues[i];
                lang.mPreprocIdentifiers.insert(std::make_pair(std::string(ppnames[i]), id));
            }

            // set your own identifiers
            static const char* identifiers[] = {
                "HWND", "HRESULT", "LPRESULT","D3D11_RENDER_TARGET_VIEW_DESC", "DXGI_SWAP_CHAIN_DESC","MSG","LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
                "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
                "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
                "IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "TextEditor" };
            static const char* idecls[] =
            {
                "typedef HWND_* HWND", "typedef long HRESULT", "typedef long* LPRESULT", "struct D3D11_RENDER_TARGET_VIEW_DESC", "struct DXGI_SWAP_CHAIN_DESC",
                "typedef tagMSG MSG\n * Message structure","typedef LONG_PTR LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
                "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
                "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
                "IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "class TextEditor" };
            for(int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i) {
                TextEditor::Identifier id;
                id.mDeclaration = std::string(idecls[i]);
                lang.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
            }
            //editor.SetLanguageDefinition(lang);
            //editor.SetPalette(TextEditor::GetLightPalette());

            // error markers
            TextEditor::ErrorMarkers markers;
            markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
            markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
            //editor.SetErrorMarkers(markers);
        }
    }
}

void ShadowResFix::InitializeColors() {
}

void ShadowResFix::SaveSettings() {
    std::ofstream file("LiveShaderEditorSettings.bin", std::ios::binary);

    if(!file.good()) {
        return;
    }

    file.write((char*) &mSettingsFileMajorVersion, sizeof(char));
    file.write((char*) &mSettingsFileMinorVersion, sizeof(char));
    file.write((char*) &mWindowPos.x, sizeof(float));
    file.write((char*) &mWindowPos.y, sizeof(float));
    file.write((char*) &mWindowSize.x, sizeof(float));
    file.write((char*) &mWindowSize.y, sizeof(float));
    file.write((char*) &mFontScale, sizeof(float));
    file.write((char*) &mOpenWindowKey, sizeof(ImGuiKey));
    file.write((char*) &mToggleCameraControlKey, sizeof(ImGuiKey));
    file.write((char*) &mItemInnerSpacing, sizeof(float));

    file.write((char*) &mEditorPos.x, sizeof(float));
    file.write((char*) &mEditorPos.y, sizeof(float));
    file.write((char*) &mEditorSize.x, sizeof(float));
    file.write((char*) &mEditorSize.y, sizeof(float));

    file.write((char*) &mLogPos.x, sizeof(float));
    file.write((char*) &mLogPos.y, sizeof(float));
    file.write((char*) &mLogSize.x, sizeof(float));
    file.write((char*) &mLogSize.y, sizeof(float));

    file.write((char*) &mShowEditor, sizeof(mShowEditor));
    file.write((char*) &bShowLogWindow, sizeof(bShowLogWindow));
}

void ShadowResFix::LoadSettings() {
    std::ifstream file("LiveShaderEditorSettings.bin", std::ios::binary);

    if(!file.good()) {
        return;
    }

    file.seekg(2);
    file.read((char*) &mWindowPos.x, sizeof(float));
    file.read((char*) &mWindowPos.y, sizeof(float));
    file.read((char*) &mWindowSize.x, sizeof(float));
    file.read((char*) &mWindowSize.y, sizeof(float));
    file.read((char*) &mFontScale, sizeof(float));
    file.read((char*) &mOpenWindowKey, sizeof(ImGuiKey));
    file.read((char*) &mToggleCameraControlKey, sizeof(ImGuiKey));
    file.read((char*) &mItemInnerSpacing, sizeof(float));

    file.read((char*) &mEditorPos.x, sizeof(float));
    file.read((char*) &mEditorPos.y, sizeof(float));
    file.read((char*) &mEditorSize.x, sizeof(float));
    file.read((char*) &mEditorSize.y, sizeof(float));

    file.read((char*) &mLogPos.x, sizeof(float));
    file.read((char*) &mLogPos.y, sizeof(float));
    file.read((char*) &mLogSize.x, sizeof(float));
    file.read((char*) &mLogSize.y, sizeof(float));

    file.read((char*) &mShowEditor, sizeof(mShowEditor));
    file.read((char*) &bShowLogWindow, sizeof(bShowLogWindow));

    if(mIsImGuiInitialized) {
        ImGui::GetIO().FontGlobalScale = mFontScale;
        ImGui::GetStyle().ItemInnerSpacing.x = mItemInnerSpacing;
    }

    HWND gameWindow = FindWindow("grcWindow", "GTAIV");
    if(!gameWindow) {
        return;
    }

    RECT gameWindowRect = {};
    GetWindowRect(gameWindow, &gameWindowRect);

    int32_t gameWindowWidth = gameWindowRect.right - gameWindowRect.left;
    int32_t gameWindowHeight = gameWindowRect.bottom - gameWindowRect.top;

    if(mWindowPos.x >= gameWindowWidth || mWindowPos.x < 0.0f) { mWindowPos.x = 0.0f; }
    if(mWindowPos.y >= gameWindowHeight || mWindowPos.y < 0.0f) { mWindowPos.y = 0.0f; }

    if(mEditorPos.x >= gameWindowWidth || mEditorPos.x < 0.0f) { mEditorPos.x = 0.0f; }
    if(mEditorPos.y >= gameWindowHeight || mEditorPos.y < 0.0f) { mEditorPos.y = 0.0f; }

    if(mLogPos.x >= gameWindowWidth || mLogPos.x < 0.0f) { mLogPos.x = 0.0f; }
    if(mLogPos.y >= gameWindowHeight || mLogPos.y < 0.0f) { mLogPos.y = 0.0f; }
}

bool ShadowResFix::OnWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if(mIsImGuiInitialized) {
        if(ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
            return true;
        }
    }

    return false;
}

void ShadowResFix::Update() {
    static bool prevShowWindow = 0;
    prevShowWindow = mShowWindow;
    ImGuiIO& io = ImGui::GetIO();

    if(ImGui::IsKeyPressed(mOpenWindowKey)) {
        //mShowWindow = !mShowWindow;

        //if(mShowWindow) {
        //    LoadSettings();
        //    ImGui::GetIO().FontGlobalScale = mFontScale;

        //    mDisableMouseControl = true;
        //    ImGui::GetIO().MouseDrawCursor = 1;
        //}
        //else {
        //    mDisableMouseControl = false;
        //    ImGui::GetIO().MouseDrawCursor = 0;
        //}
    }

    //if(ImGui::IsKeyPressed(ImGuiKey_F5)) {
    //    pauseGame = !pauseGame;
    //}

    if(mShowWindow) {
        if(ImGui::IsKeyPressed(mToggleCameraControlKey)) {
            mDisableMouseControl = !mDisableMouseControl;
        }
    }

    if(GameVersion == 1200 && baseAddress && IsGameOnMenu && IsGamePaused && AlowPauseGame) {
        try {
            if(/**IsGameOnMenu ||*/ pauseGame || io.WantCaptureKeyboard) {
                *IsGamePaused = true;
            }
            else {
                if(!*IsGameOnMenu) {
                    *IsGamePaused = false;
                }
            }
        }
        catch(const std::exception&) {
        }
    }
    //if(io.WantCaptureKeyboard || pauseGame) {
    //    *IsGamePaused = true;
    //}
    //else {
    //    if(*IsGameOnMenu) {
    //        *IsGamePaused = true;
    //    }
    //    else {
    //        *IsGamePaused = false;
    //    }
    //}

    bool windowWasJustClosed = prevShowWindow && !mShowWindow;
    if(windowWasJustClosed) {
    }

    bool windowWasJustOpened = !prevShowWindow && mShowWindow;
    if(windowWasJustOpened) {
    }
}

void ShadowResFix::OnBeforeD3D9DeviceReset(IDirect3DDevice9* d3d9Device) {
    textureList.clear();
    InitializeImGui(d3d9Device);

    ImGui_ImplDX9_InvalidateDeviceObjects();
}

void ShadowResFix::OnAfterD3D9DeviceReset() {
    ImGui_ImplDX9_CreateDeviceObjects();
}

void ShadowResFix::OnBeforeD3D9DeviceEndScene(IDirect3DDevice9* d3d9Device) {
    InitializeImGui(d3d9Device);

    Update();

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if(mShowWindow) {
        DrawMainWindow();
        DrawSettingsWindow();
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if(ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

extern std::vector<HWND> windows;
extern std::vector<float> mtx;


extern bool downsampleStencil;

extern float parametersAA[4];
extern float NoiseSale[4];
extern float SS_params[4];
extern float SS_params2[4];
extern int SS_NumSamples[4];
extern float BilateralDepthTreshold[4];

extern int useDefferedShadows;

//struct crc_name {
//    uint32_t crc32;
//    std::string name;
//    crc_name(std::string _name, uint32_t _crc32) {
//        name = _name;
//        crc32 = _crc32;
//    }
//};

extern std::vector<crc_name*> crclist_ps;
extern std::vector<crc_name*> crclist_vs;

struct source_name {
    std::string name;
    std::string source;
    int id;
    source_name(std::string _name, std::string _source, int _id) {
        name = _name;
        source = _source;
        id = _id;
    }
};

int GenShaderList_ps(int version) {
    //std::vector<crc_name*> crclist_ps;
    std::vector<source_name*> shadersSources;
    int cnt = 0;
    for(int i = 0; i < (int) shader_names_ps.size(); i++) {
        std::string fname = std::string("common/shaders/win32_30_atidx10/") + shader_names_ps[i];
        FILE* f = fopen(fname.c_str(), "r");
        if(f) {
            fseek(f, 0, SEEK_END);
            uint32_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            char* c = new char[size + 1];
            fread(c, 1, size, f);
            shadersSources.push_back(new source_name(shader_names_ps[i], c, -1));
            fclose(f);
        }
    }
    FILE* fo = 0;
    if(version >= 1060)
        fo = fopen("crc_name_1070_PS.txt", "w+b");
    else
        fo = fopen("crc_name_1040_PS.txt", "w+b");
    if(fo == NULL) {
        printf("erro");
        return 0;
    }
    for(int i = 0; i < (int) ps_4.size(); i++) {
        if(ps_4[i] && ps_4[i] != ImGuiPS) {
            uint32_t crc = ps_4[i]->crc32;
            std::string shaderasm = ps_4[i]->GetAsm();
            for(int sh = 0; sh < (int) shadersSources.size(); sh++) {
                if(shaderasm.length() == shadersSources[sh]->source.length()) {
                    if(shaderasm == shadersSources[sh]->source) {
                        crclist_ps.push_back(new crc_name(shadersSources[sh]->name, crc, -1));
                        fprintf(fo, "%i, %i, %.8x, %s\n",i , ps_4[i]->id, crc, shadersSources[sh]->name.c_str());
                        cnt++;
                        break;
                    }
                }
            }
        }
    }
    if(fo)
        fclose(fo);
    return cnt;
}
int read_crc_name_ps(int version) {
    int cnt = 0;
    FILE* file = 0;
    if(version >= 1060)
        file = fopen("crc_name_1070_PS.txt", "r");
    else
        file = fopen("crc_name_1040_PS.txt", "r");
    if(file != NULL) {
        char line[128];
        while(fgets(line, sizeof line, file) != NULL) {
            UINT crc = 0;
            char str[128] = { 0 };
            int id = -1;
            int any = 0;
            int c = sscanf(line, "%i, %i, %x, %s",&any, &id, &crc, str);
            crclist_ps.push_back(new crc_name(str, crc, id));
            cnt++;
        }
        fclose(file);
    }
    return cnt;
}
int GenShaderList_vs(int version) {
    //std::vector<crc_name*> crclist_vs;
    std::vector<source_name*> shadersSources;
    int cnt = 0;
    for(int i = 0; i < (int) shader_names_vs.size(); i++) {
        std::string fname = std::string("common/shaders/win32_30_atidx10/") + shader_names_vs[i];
        FILE* f = fopen(fname.c_str(), "r");
        if(f) {
            fseek(f, 0, SEEK_END);
            uint32_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            char* c = new char[size + 1];
            fread(c, 1, size, f);
            shadersSources.push_back(new source_name(shader_names_vs[i], c, -1));
            fclose(f);
        }
    }
    FILE* fo = 0;
    if(version >= 1060)
        fo = fopen("crc_name_1070_VS.txt", "w+b");
    else
        fo = fopen("crc_name_1040_VS.txt", "w+b");
    if(fo == NULL) {
        printf("erro");
        return 0;
    }
    for(int i = 0; i < (int) vs_4.size(); i++) {
        if(vs_4[i] && vs_4[i] != ImGuiVS) {
            uint32_t crc = vs_4[i]->crc32;
            std::string shaderasm = vs_4[i]->GetAsm();
            for(int sh = 0; sh < (int) shadersSources.size(); sh++) {
                if(shaderasm.length() == shadersSources[sh]->source.length()) {
                    if(shaderasm == shadersSources[sh]->source) {
                        crclist_vs.push_back(new crc_name(shadersSources[sh]->name, crc, shadersSources[sh]->id));
                        fprintf(fo, "%i, %i, %.8x, %s\n", i, vs_4[i]->id, crc, shadersSources[sh]->name.c_str());
                        cnt++;
                        break;
                    }
                }
            }
        }
    }
    if(fo)
        fclose(fo);
    return cnt;
}
int read_crc_name_vs(int version) {
    int cnt = 0;
    FILE* file = 0;
    if(version >= 1060)
        file = fopen("crc_name_1070_VS.txt", "r");
    else
        file = fopen("crc_name_1040_VS.txt", "r");
    if(file != NULL) {
        char line[128];
        while(fgets(line, sizeof line, file) != NULL) {
            UINT crc = 0;
            char str[128] = { 0 };
            int id = -1;
            int loadN = 0;
            int c = sscanf(line, "%i, %i, %x, %s", &loadN, &id, &crc, str);
            crclist_vs.push_back(new crc_name(str, crc, id));
            cnt++;
        }
        fclose(file);
    }
    return cnt;
}

int GenShaderList_ps2(int version) {
    //std::vector<crc_name*> crclist_ps;
    std::vector<source_name*> shadersSources;
    int cnt = 0;
    for(int i = 0; i < (int) shader_names_ps.size(); i++) {
        std::string fname = std::string("common/shaders/win32_30_atidx10/") + shader_names_ps[i];
        FILE* f = fopen(fname.c_str(), "r");
        if(f) {
            fseek(f, 0, SEEK_END);
            uint32_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            char* c = new char[size + 1];
            fread(c, 1, size, f);
            shadersSources.push_back(new source_name(shader_names_ps[i], c, -1));
            fclose(f);
        }
    }
    FILE* fo = 0;
    if(version >= 1060)
        fo = fopen("crc_name_1070_PS.txt", "w+b");
    else
        fo = fopen("crc_name_1040_PS.txt", "w+b");
    if(fo == NULL) {
        printf("erro");
        return 0;
    }
    for(int i = 0; i < (int) ps_4.size(); i++) {
        if(ps_4[i] && ps_4[i] != ImGuiPS) {
            uint32_t crc = ps_4[i]->crc32;
            std::string shaderasm = ps_4[i]->GetAsm();
            //for(int sh = 0; sh < (int) shadersSources.size(); sh++) {
            //    if(shaderasm.length() == shadersSources[sh]->source.length()) {
            //        if(shaderasm == shadersSources[sh]->source) {
            //            crclist_ps.push_back(new crc_name(shadersSources[sh]->name, crc, -1));
            fprintf(fo, "%i, %i, %.8x, %s/%s\n", i, ps_4[i]->id, crc, ps_4[i]->fxName.c_str(), ps_4[i]->oName.c_str());
            //            cnt++;
            //            break;
            //        }
            //    }
            //}
        }
    }
    if(fo)
        fclose(fo);
    return cnt;
}
int read_crc_name_ps2(int version) {
    int cnt = 0;
    FILE* file = 0;
    if(version >= 1060)
        file = fopen("crc_name_1070_PS.txt", "r");
    else
        file = fopen("crc_name_1040_PS.txt", "r");
    if(file != NULL) {
        char line[128];
        while(fgets(line, sizeof line, file) != NULL) {
            UINT crc = 0;
            char str[128] = { 0 };
            int id = -1;
            int any = 0;
            int c = sscanf(line, "%i, %i, %x, %s", &any, &id, &crc, str);
            crclist_ps.push_back(new crc_name(str, crc, id));
            cnt++;
        }
        fclose(file);
    }
    return cnt;
}
int GenShaderList_vs2(int version) {
    //std::vector<crc_name*> crclist_vs;
    std::vector<source_name*> shadersSources;
    int cnt = 0;
    for(int i = 0; i < (int) shader_names_vs.size(); i++) {
        std::string fname = std::string("common/shaders/win32_30_atidx10/") + shader_names_vs[i];
        FILE* f = fopen(fname.c_str(), "r");
        if(f) {
            fseek(f, 0, SEEK_END);
            uint32_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            char* c = new char[size + 1];
            fread(c, 1, size, f);
            shadersSources.push_back(new source_name(shader_names_vs[i], c, -1));
            fclose(f);
        }
    }
    FILE* fo = 0;
    if(version >= 1060)
        fo = fopen("crc_name_1070_VS.txt", "w+b");
    else
        fo = fopen("crc_name_1040_VS.txt", "w+b");
    if(fo == NULL) {
        printf("erro");
        return 0;
    }
    for(int i = 0; i < (int) vs_4.size(); i++) {
        if(vs_4[i] && vs_4[i] != ImGuiVS) {
            uint32_t crc = vs_4[i]->crc32;
            std::string shaderasm = vs_4[i]->GetAsm();
            //for(int sh = 0; sh < (int) shadersSources.size(); sh++) {
            //    if(shaderasm.length() == shadersSources[sh]->source.length()) {
            //        if(shaderasm == shadersSources[sh]->source) {
            //            crclist_vs.push_back(new crc_name(shadersSources[sh]->name, crc, shadersSources[sh]->id));
            fprintf(fo, "%i, %i, %.8x, %s/%s\n", i, vs_4[i]->id, crc, vs_4[i]->fxName.c_str(), vs_4[i]->oName.c_str());
            //            cnt++;
            //            break;
            //        }
            //    }
            //}
        }
    }
    if(fo)
        fclose(fo);
    return cnt;
}
int read_crc_name_vs2(int version) {
    int cnt = 0;
    FILE* file = 0;
    if(version >= 1060)
        file = fopen("crc_name_1070_VS.txt", "r");
    else
        file = fopen("crc_name_1040_VS.txt", "r");
    if(file != NULL) {
        char line[128];
        while(fgets(line, sizeof line, file) != NULL) {
            UINT crc = 0;
            char str[128] = { 0 };
            int id = -1;
            int any = 0;
            int c = sscanf(line, "%i, %i, %x, %s", &any, &id, &crc, str);
            crclist_vs.push_back(new crc_name(str, crc, id));
            cnt++;
        }
        fclose(file);
    }
    return cnt;
}

extern IDirect3DTexture9* pHDRDownsampleTex; // main downsampled texture


extern bool useLinear;
void ShadowResFix::DrawMainWindow() {
    static float shaderColor[4] = { 0 };
    static ImVec4 bgColor(0.0549f, 0.047f, 0.0274509f, 0.333f);
    static ImVec4 textureColor(0.5f, 0.5f, 0.5f, 1.0f);
    ImGuiIO& io = ImGui::GetIO();

    //RECT rc = { 0 };
    //GetClientRect(windows[0], &rc);

    static char buf100[200] = { 0 };
    static bool showUnused = false;
    static bool showUnusedTex = true;
    static bool hideEmptyFXC = true;

    ImGui::Begin("RaGeFX 1.0.0.8", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // semi transparent background
    if(pRainDropsRefractionHDRTex) {
        ImVec2 wsize = ImVec2(gWindowWidth, gWindowHeight);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(0, 0));
        //if(pHDRDownsampleTex && (useDepthOfField2 || UseSunShafts == 1))
        //    ImGui::Image(pHDRDownsampleTex, wsize, ImVec2(0, 0), ImVec2(1, 1), bgColor);
        //else
            ImGui::Image(pRainDropsRefractionHDRTex, wsize, ImVec2(0, 0), ImVec2(1, 1), bgColor);
        ImGui::SetCursorScreenPos(pos);
    }

    if(mWindowPos.x == 0.f)
        mWindowPos.x = 600.f;
    if(mWindowSize.x == 0.f || mWindowSize.y == 0.f)
        mWindowSize = ImVec2(440.f, 650.f);

    ImGui::SetWindowPos(mWindowPos, ImGuiCond_FirstUseEver);
    ImGui::SetWindowSize(mWindowSize, ImGuiCond_FirstUseEver);
    mWindowPos = ImGui::GetWindowPos();
    ImVec2 WindowSize = ImGui::GetWindowSize();
    if(WindowSize.x > 100 && WindowSize.y > 50)
        mWindowSize = WindowSize;

    if(ImGui::BeginMenuBar()) {
        if(ImGui::BeginMenu("Settings")) {
            mShowSettingsWindow = true;
            ImGui::EndMenu();
        }
        //ImGui::SetNextItemWidth(140.f);
        ImGui::ColorEdit4("ShadCol", shaderReturnColor, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoInputs);
        //ImGui::SetNextItemWidth(140.f);
        ImGui::ColorEdit4("textureColor", &textureColor.x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoInputs);
        ImGui::Checkbox("Pause Game F5", &pauseGame);
        ImGui::EndMenuBar();
    }
    //ImGui::Text("%i %i %i %i", rc.left, rc.right, rc.top, rc.bottom);
    ImGui::Checkbox("Display FPS Counter", &bDisplayFPSCounter);
    ImGui::Checkbox("FixTreeLeavesSwayInTheWind", &gTreeLeavesSwayInTheWind);
    ImGui::Checkbox("FixRainDrops", &gFixRainDrops);
    ImGui::Checkbox("FixAdaptiveTess", &DisableADAPTIVETESS_X);
    ImGui::Checkbox("FixCoronaDepth", &fixCoronaDepth);
    ImGui::Checkbox("FixDistantOutlineUsingDXVK", &fixDistantOutlineUsingDXVK);
    ImGui::Checkbox("NearFarPlane", &gNearFarPlane);
    ImGui::Checkbox("Show Log Window", &mShowLogWindow);
    if(ImGui::Button("Clear Log")) {
        Log::clear();
    }
    ImGui::Checkbox("Show Editor Window", &showEditorWindow);


    if(ImGui::CollapsingHeader("Main Post Process")) {
        ImGui::Checkbox("downsampleStencil", &downsampleStencil);
        ImGui::Checkbox("UseSSAO", &UseSSAO);
        ImGui::DragFloat("AoDistance", &AoDistance, 1, 0, 1500, "%.3f", ImGuiSliderFlags_Logarithmic);
        static const char* PostFx[] = {
            "None", "FXAA", "SMAA", "SMAA blend", "SMAA edges"
        };
        static const char* DebugTextures[] = {
            "none",
            "Difuse",
            "normal",
            "bloom",
            "specular",
            "depth",
            "stencil"
        };
        ImGui::SliderInt("DebugTextures", &UseDebugTextures, 0, 6, DebugTextures[UseDebugTextures]);
        ImGui::Checkbox("useMotionBlur", &useMotionBlur);
        ImGui::Checkbox("useStippleFilter", &useStippleFilter);
        ImGui::DragFloat4("NoiseSale", NoiseSale, 0.001, -1, 1, "%.3f", ImGuiSliderFlags_Logarithmic);

        //if(ImGui::Checkbox("useDepthOfField", &useDepthOfField) && useDepthOfField) {
        //    useDepthOfField2 = false;
        //};
        //if(ImGui::Checkbox("useDepthOfField 2", &useDepthOfField2) && useDepthOfField2) {
        //    useDepthOfField = false;
        //};
        //ImGui::Checkbox("UseDof 2", &useDof);

        static const char* DofTypes[] = {
            "none",
            "Dof1",
            "Dof2",
            "Dof3",
            "Dof4",
        };
        ImGui::SliderInt("DofType", &useDepthOfField, 0, 4, DofTypes[useDepthOfField]);

        ImGui::DragFloat4("FocusPoint", FocusPoint, 1, 0, 1500, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::DragFloat4("FocusScale", FocusScale, 1, 0, 1500, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::DragInt4("DofSamples", DofSamples, 1, 0, 200);

        ImGui::SliderInt("PostFx AA", UsePostFxAA, 0, 4, PostFx[UsePostFxAA[0]]);
        if(UsePostFxAA[0] == 1) {
            ImGui::DragFloat("FXAA Subpix", &parametersAA[0], 0.001, 0, 1, "%.3f");
            ImGui::DragFloat("FXAA EdgeThreshold", &parametersAA[1], 0.001, 0, 1, "%.3f");
            ImGui::DragFloat("FXAA EdgeThresholdMin", &parametersAA[2], 0.001, 0, 1, "%.3f");
        }

        //if(ImGui::Button("Reload SMAA##1")) {
        //    SAFE_RELEASE(SMAA_EdgeDetection);
        //    SAFE_RELEASE(SMAA_BlendingWeightsCalculation);
        //    SAFE_RELEASE(SMAA_NeighborhoodBlending);
        //    SAFE_RELEASE(SMAA_EdgeDetectionVS);
        //    SAFE_RELEASE(SMAA_BlendingWeightsCalculationVS);
        //    SAFE_RELEASE(SMAA_NeighborhoodBlendingVS);
        //}


        //ImGui::Checkbox("SunShafts", &UseSunShafts);

        static const char* DFTypes[] = {
            "None", "Horizontal", "Vertical", "Horizontal & Vertical", "Omini"
        };
        ImGui::Checkbox("useNewShadowAtlas", &useNewShadowAtlas);
        ImGui::SliderInt("Blur Shadows Direction", &useDefferedShadows, 0, 4, DFTypes[useDefferedShadows]);
        ImGui::DragFloat4("BilateralDepthTreshold", BilateralDepthTreshold, 0.01f, 0, 1, "%f", ImGuiSliderFlags_Logarithmic);

        static const char* SSTypes[] = {
            "None", "HalfRes one pass", "HalfRes Two passes WIP", "Full screen one pass", "Full screen Two passes"
        };
        ImGui::SliderInt("God Rays Type", &UseSunShafts, 0, 4, SSTypes[UseSunShafts]);


        ImGui::DragFloat4("Weight Density, Exposure, Decay", SS_params, 0.001, -2, 8, "%.3f");
        ImGui::DragFloat4("SunSize, pow, depth, power", SS_params2, 0.01, -0.01, 16, "%.3f");
        ImGui::DragInt4("NumSamples", SS_NumSamples, 1, 1, 256, "%d");

        ImGui::Text("SunDirection = %f, %f, %f, %f", SunDirection[0], SunDirection[1], SunDirection[2], SunDirection[3]);
        ImGui::Text("SunCentre = %f, %f, %f, %f", SunCentre[0], SunCentre[1], SunCentre[2], SunCentre[3]);
        ImGui::Text("SunColor  = %f, %f, %f, %f", SunColor [0], SunColor [1], SunColor [2], SunColor [3]);

        if(ImGui::TreeNode("MTX")) {
            for(int i = 0; i < (int) mtx.size(); i+=4) {
                ImGui::Text("%i = %.5f, %.5f, %.5f, %.5f", i, mtx[i+0], mtx[i+1], mtx[i+2], mtx[i+3]);

            }

            ImGui::TreePop();
        }
    }


    ImGui::Checkbox("EnableDepthOverwrite", &EnableDepthOverwrite);
    ImGui::SameLine();
    HelpMarker("Activating the depth override causes the rain bug\nwhere the rain on the floor does not appear and the rain passes through the ceilings.");
    
    ImGui::Separator();
    if(ImGui::Button("Set all shaders to use game fxc")) {
        for(auto& l : fx_ps) {
            for(auto s : l) {
                if(s) {
                    s->setCompiledShaderToUse(SU_FXC);
                }
            }
        }
        for(auto& l : fx_vs) {
            for(auto s : l) {
                if(s) {
                    s->setCompiledShaderToUse(SU_FXC);
                }
            }
        }
    }
    if(ImGui::Button("Set all shaders to use loaded asm")) {
        for(auto& l : fx_ps) {
            for(auto s : l) {
                if(s) {
                    s->setCompiledShaderToUse(SU_LASM);
                }
            }
        }
        for(auto& l : fx_vs) {
            for(auto s : l) {
                if(s) {
                    s->setCompiledShaderToUse(SU_LASM);
                }
            }
        }
    }
    if(ImGui::Button("Set all shaders to use edited asm")) {
        for(auto& l : fx_ps) {
            for(auto s : l) {
                if(s) {
                    s->setCompiledShaderToUse(SU_EASM);
                }
            }
        }
        for(auto& l : fx_vs) {
            for(auto s : l) {
                if(s) {
                    s->setCompiledShaderToUse(SU_EASM);
                }
            }
        }
    }
    if(ImGui::Button("Set all shaders to use loaded hlsl")) {
        for(auto& l : fx_ps) {
            for(auto s : l) {
                if(s) {
                    s->setCompiledShaderToUse(SU_LFX);
                }
            }
        }
        for(auto& l : fx_vs) {
            for(auto s : l) {
                if(s) {
                    s->setCompiledShaderToUse(SU_LFX);
                }
            }
        }
    }
    if(ImGui::Button("Set all shaders to use edited hlsl")) {
        for(auto& l : fx_ps) {
            for(auto s : l) {
                if(s) {
                    s->setCompiledShaderToUse(SU_EFX);
                }
            }
        }
        for(auto& l : fx_vs) {
            for(auto s : l) {
                if(s) {
                    s->setCompiledShaderToUse(SU_EFX);
                }
            }
        }
    }

    ImGui::Separator();

    static const std::list <std::string> nameList = {
        "Any R float 16",
        "Any R float 32",
        "Any R",
        "Any RG LA",
        "Any RG float",
        "Any depth stencil",
        "Any rgb",
        "Any rgba float",
        "Any rgba",
        "MainPostfx",
        "Screen / 4",
        "Screen / 2",
        "Screen Size2",
        "Screen Size",
        "Screen",
        "gBuffer 2?",
        "gBuffer",
        "Mirror Tex",
        "reflex",
        "water reflex",
        "light atlas",
        "shadow atlas",
        "shadow cascade",
        "Loaded"
    };
    
    if(ImGui::CollapsingHeader("Textures")) {
        ImGui::Checkbox("Show Unused tex", &showUnusedTex);
        if(ImGui::Button("Reset Use##tex")) {
            for(auto tex : textureList) {
                if(tex) {
                    tex->useCounter = 0;
                }
            }
        }

        ImGui::BeginChild("ChildTex", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 2.f / 3.f), false, ImGuiWindowFlags_HorizontalScrollbar);
        int i = 0;
        for(auto& tname : nameList) {
            if(ImGui::TreeNode(tname.c_str())) {
                for(auto tex : textureList) {
                    if(tex && tex->name == tname && ((showUnusedTex == false && tex->useCounter > 0) || showUnusedTex == true)) {
                        if(tname == "Screen") {
                            printf("");
                        }
                        i++;
                        ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
                        ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
                        ImVec4 tint_col = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);   // No tint
                        ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        float pp = tex->Width / float(tex->Height);
                        //float my_tex_w = (float) tex->Width;
                        //float my_tex_h = (float) tex->Height;
                        float my_tex_w = 200.f;
                        float my_tex_h = 200.f / pp;
                        ImVec2 pos = ImGui::GetCursorScreenPos();

                        {
                            //ImGui::Image(tex, ImVec2(200.f, 200.f/pp), uv_min, uv_max, tint_col, border_col);
                            ImGui::Image(tex, ImVec2(my_tex_w, my_tex_h), uv_min, uv_max, textureColor, border_col);
                            if(ImGui::BeginItemTooltip()) {
                                float region_sz = 32.0f;
                                float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
                                float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
                                float zoom = 4.0f + tex->Width / 200.f;
                                if(region_x < 0.0f) { region_x = 0.0f; }
                                else if(region_x > my_tex_w - region_sz) { region_x = my_tex_w - region_sz; }
                                if(region_y < 0.0f) { region_y = 0.0f; }
                                else if(region_y > my_tex_h - region_sz) { region_y = my_tex_h - region_sz; }
                                //ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
                                //ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
                                ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
                                ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
                                ImGui::Image(tex, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, textureColor, border_col);
                                ImGui::EndTooltip();
                            }
                        }
                        ImGui::SameLine();

                        ImGui::Text("Name: %s\nFormat: %s\nWidth: %i\nHeight: %i\nLevels: %i\nUsage: %i\nPool: %i\nUseCnt: %ui",
                                    tex->name.c_str(), tex->FormatName, tex->Width, tex->Height, tex->Levels, (UINT) tex->Usage, (UINT) tex->Pool, tex->useCounter);

                        //ImGui::SameLine();
                        const char* formats[] = { "BMP", "JPG", "TGA", "PNG", "DDS",
                            "PPM", "DIB", "HDR", "PFM", "\0\0" };
                            
                        static _D3DXIMAGE_FILEFORMAT fmt = D3DXIFF_PNG;

                        sprintf(buf100, "SaveTextureToFile##%s##ps%i %i %i %i", tex->name.c_str(), tex->Format, tex->Height, tex->Width, i);
                        if(ImGui::Button(buf100)) {
                            SYSTEMTIME lt;
                            GetLocalTime(&lt);
                            std::string t = "";
                            for(int i = 0; i < tex->name.length(); i++) {
                                if((tex->name[i] >= '0' && tex->name[i] <= '9') || (tex->name[i] >= 'a' && tex->name[i] <= 'z') || (tex->name[i] >= 'A' && tex->name[i] <= 'Z') || tex->name[i] == '_'|| tex->name[i] == ' ') {
                                    t.push_back(tex->name[i]);
                                }
                            }
                            auto r = CreateDirectoryA("SavedTextures", 0);
                            sprintf(buf100, "SavedTextures/%s_%i%i%i%i%i%i.%s", t.c_str(), (int) lt.wYear, (int) lt.wMonth, (int) lt.wHour, (int) lt.wMinute, (int) lt.wSecond, (int) lt.wMilliseconds, formats[fmt]);
                            D3DXSaveTextureToFileA(buf100, fmt, tex->GetProxyInterface(), 0);
                        }

                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(80);
                        sprintf(buf100, "Format##%s##ps%i %i %i %i", tex->name.c_str(), tex->Format, tex->Height, tex->Width, i);
                        ImGui::Combo(buf100, (int*) &fmt,
                                     formats[0]);
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();
    }

    if(ImGui::CollapsingHeader("Pixel shaders")) {
        ImGui::Checkbox("Show Unused", &showUnused);
        ImGui::Checkbox("Hide Empty FXC##ps", &hideEmptyFXC);

        if(ImGui::Button("Reset Use##ps")) {
            for(int i = 0; i < (int) fx_ps.size(); i++) {
                for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                    if(fx_ps[i][j]) {
                        fx_ps[i][j]->used = 0;
                    }
                }
            }
        }
        if(ImGui::Button("Enable all ps")) {
            for(int i = 0; i < (int) fx_ps.size(); i++) {
                for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                    if(fx_ps[i][j]) {
                        fx_ps[i][j]->disable = false;
                        fx_ps[i][j]->useNewShader = false;
                    }
                }
            }
        }
        ImGui::BeginChild("ChildPS", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 2.f / 3.f), false, ImGuiWindowFlags_HorizontalScrollbar);

        for(int i = 0; i < (int) fx_ps.size(); i++) {
            if(fx_ps[i].size() == 0 && hideEmptyFXC)
                continue;
            sprintf(buf100, "%s##ps%i", shader_names_fxc[i], i);
            if(ImGui::TreeNode(buf100)) {
                sprintf(buf100, "Disable all##ps%i", i);
                if(ImGui::Button(buf100)) {
                    for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                        if(fx_ps[i][j]) {
                            fx_ps[i][j]->disable = true;
                            fx_ps[i][j]->useNewShader = false;
                        }
                    }
                }
                sprintf(buf100, "Enable all##ps%i", i);
                ImGui::SameLine();
                if(ImGui::Button(buf100)) {
                    for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                        if(fx_ps[i][j]) {
                            fx_ps[i][j]->disable = false;
                            fx_ps[i][j]->useNewShader = false;
                        }
                    }
                }
                for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                    if(fx_ps[i][j] && ((showUnused == false && fx_ps[i][j]->used > 0) || showUnused == true)) {
                        sprintf(buf100, "##Disable PS Shader %i_%i_%i", i, j, fx_ps[i][j]->id);
                        ImGui::Checkbox(buf100, &fx_ps[i][j]->disable);
                        sprintf(buf100, "%s #ps%i_%i_%i", fx_ps[i][j]->oName.c_str(), i, j, fx_ps[i][j]->id);
                        ImGui::SameLine();
                        if(ImGui::TreeNode(buf100)) {


                            std::string namefx = fx_ps[i][j]->oName.substr(0, fx_ps[i][j]->oName.find_last_of(".")) + std::string(".fx");
                            std::string nameasm = fx_ps[i][j]->oName;

                            //ImGui::Text("PS ID %i ##%i_%i_%i", fx_ps[i][j]->id, i, j, fx_ps[i][j]->id);
                            ImGui::Text("CRC_32:  %x ##vs%i_%i_%i", fx_ps[i][j]->crc32, i, j, fx_ps[i][j]->id);
                            ImGui::Text("Use Counter: %i ##%i_%i_%i", fx_ps[i][j]->used, i, j, fx_ps[i][j]->id);

                            sprintf(buf100, "Compile PS asm ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->editedAsm.length() > 3 && ImGui::Button(buf100)) {
                                for(auto t : lst) {
                                    if(t->bs && t->bs == fx_ps[i][j] && t->name == nameasm) {
                                        t->bs->editedAsm = t->editor->GetText();;
                                        if(t->bs->compileNewASM() == S_OK && fx_ps[i][j]->newShader) {
                                            t->bs->useNewShader = true;
                                        }
                                    }
                                }
                            }
                            sprintf(buf100, "Compile PS hlsl ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->editedFx.length() > 3 && ImGui::Button(buf100)) {
                                for(auto t : lst) {
                                    if(t->bs && t->bs == fx_ps[i][j] && t->name == namefx) {
                                        t->bs->editedFx = t->editor->GetText();;
                                        if(t->bs->compileNewFx() == S_OK && fx_ps[i][j]->newShader) {
                                            IDirect3DPixelShader9* pShader = fx_ps[i][j]->newShader;
                                            static std::vector<uint8_t> pbFunc;
                                            UINT len;
                                            pShader->GetFunction(nullptr, &len);
                                            if(pbFunc.size() < len) {
                                                pbFunc.resize(len + len % 4);
                                            }
                                            pShader->GetFunction(pbFunc.data(), &len);

                                            ID3DXBuffer* pShaderAsm = NULL;
                                            HRESULT hr = D3DXDisassembleShader((DWORD*) pbFunc.data(), FALSE, NULL, &pShaderAsm);

                                            if(SUCCEEDED(hr) && pShaderAsm) {
                                                t->bs->editedAsm = (char*) pShaderAsm->GetBufferPointer();
                                            }

                                            if(0) {
                                                ID3DBlob* disShader;
                                                hr = D3DDisassemble(pbFunc.data(), pbFunc.size(),
                                                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS,
                                                                    nullptr,
                                                                    &disShader
                                                );

                                                if(SUCCEEDED(hr) && disShader) {
                                                    t->bs->editedAsm = (char*) disShader->GetBufferPointer();
                                                }
                                            }

                                            t->bs->useNewShader = true;
                                        }
                                    }
                                }
                            }
                            sprintf(buf100, "Reset PS ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(ImGui::Button(buf100)) {
                                for(auto t : lst) {
                                    if(t->bs && t->bs == fx_ps[i][j]) {
                                        t->bs->editedAsm = "";
                                        t->bs->editedFx = "";
                                        if(fx_ps[i][j]->newShader)
                                            fx_ps[i][j]->newShader->Release();
                                        fx_ps[i][j]->newShader = 0;
                                        lst.remove(t);
                                        break;
                                    }
                                }
                                for(auto t : lst) {
                                    if(t->bs && t->bs == fx_ps[i][j]) {
                                        t->bs->editedAsm = "";
                                        t->bs->editedFx = "";
                                        if(fx_ps[i][j]->newShader)
                                            fx_ps[i][j]->newShader->Release();
                                        fx_ps[i][j]->newShader = 0;
                                        lst.remove(t);
                                        break;
                                    }
                                }
                            }

                            sprintf(buf100, "Edit PS FXC asm ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(ImGui::Button(buf100)) {
                                bool isInEditor = false;
                                sprintf(buf100, "%s#FXCASM##%i_%i_%i", nameasm.c_str(), i, j, fx_ps[i][j]->id);
                                std::string name = buf100;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_ps[i][j]);
                                    if(bs && t->bs == bs && t->name == name) {
                                        isInEditor = true;
                                        t->show = true;
                                        showEditorWindow = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    //if(fx_ps[i][j]->editedAsm.length() < 3) {
                                    //	fx_ps[i][j]->editedAsm = fx_ps[i][j]->GetAsm();
                                    //}
                                    stShaderEditor* s = new stShaderEditor(name, PS_ASM, new TextEditor(), fx_ps[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->GetAsm());
                                    lst.push_back(s);
                                    showEditorWindow = true;
                                }
                            }
                            sprintf(buf100, "Edit PS Loaded asm ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->loadedAsm.length() > 0 && ImGui::Button(buf100)) {
                                bool isInEditor = false;
                                sprintf(buf100, "%s#LASM##%i_%i_%i", nameasm.c_str(), i, j, fx_ps[i][j]->id);
                                std::string name = buf100;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_ps[i][j]);
                                    if(bs && t->bs == bs && t->name == name) {
                                        isInEditor = true;
                                        t->show = true;
                                        showEditorWindow = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    //if(fx_ps[i][j]->editedAsm.length() < 3) {
                                    //	fx_ps[i][j]->editedAsm = fx_ps[i][j]->loadedAsm;
                                    //}
                                    stShaderEditor* s = new stShaderEditor(name, PS_ASM, new TextEditor(), fx_ps[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->loadedAsm);
                                    lst.push_back(s);
                                    showEditorWindow = true;
                                }
                            }
                            sprintf(buf100, "Edit PS Loaded hlsl ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->loadedFx.length() > 0 && ImGui::Button(buf100)) {
                                bool isInEditor = false;
                                sprintf(buf100, "%s#LFX##%i_%i_%i", nameasm.c_str(), i, j, fx_ps[i][j]->id);
                                std::string name = buf100;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_ps[i][j]);
                                    if(bs && t->bs == bs && t->name == name) {
                                        isInEditor = true;
                                        t->show = true;
                                        showEditorWindow = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    //if(fx_ps[i][j]->editedFx.length() < 3) {
                                    //	fx_ps[i][j]->editedFx = fx_ps[i][j]->loadedFx;
                                    //}
                                    stShaderEditor* s = new stShaderEditor(name, PS_FX, new TextEditor(), fx_ps[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->loadedFx);
                                    lst.push_back(s);
                                    showEditorWindow = true;
                                }
                            }
                            sprintf(buf100, "Edit PS Edited hlsl ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->editedFx.length() > 0 && ImGui::Button(buf100)) {
                                bool isInEditor = false;
                                sprintf(buf100, "%s#EFX##%i_%i_%i", nameasm.c_str(), i, j, fx_ps[i][j]->id);
                                std::string name = buf100;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_ps[i][j]);
                                    if(bs && t->bs == bs && t->name == name) {
                                        isInEditor = true;
                                        t->show = true;
                                        showEditorWindow = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    //if(fx_ps[i][j]->editedFx.length() < 3) {
                                    //	fx_ps[i][j]->editedFx = fx_ps[i][j]->loadedFx;
                                    //}
                                    stShaderEditor* s = new stShaderEditor(name, PS_FX, new TextEditor(), fx_ps[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->editedFx);
                                    lst.push_back(s);
                                    showEditorWindow = true;
                                }
                            }

                            if(!fx_ps[i][j]->newShader)
                                fx_ps[i][j]->useNewShader = false;
                            else {
                                sprintf(buf100, "Use New PS Shader ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                                ImGui::Checkbox(buf100, &fx_ps[i][j]->useNewShader);
                            }
                            static int e = 0;
                            ImGui::RadioButton("FXC", (int*) &fx_ps[i][j]->usingShader, 0); ImGui::SameLine();
                            if(fx_ps[i][j]->compiledShaders[SU_LASM]) { ImGui::RadioButton("Loaded ASM", (int*) &fx_ps[i][j]->usingShader, 1); ImGui::SameLine(); }
                            if(fx_ps[i][j]->compiledShaders[SU_EASM]) { ImGui::RadioButton("Edited ASM", (int*) &fx_ps[i][j]->usingShader, 2); }
                            if(fx_ps[i][j]->compiledShaders[SU_LFX]) { ImGui::RadioButton("Loaded HLSL", (int*) &fx_ps[i][j]->usingShader, 3); ImGui::SameLine(); }
                            if(fx_ps[i][j]->compiledShaders[SU_EFX]) { ImGui::RadioButton("Edited HLSL", (int*) &fx_ps[i][j]->usingShader, 4); ImGui::SameLine(); }

                            ImGui::Text("");

                            sprintf(buf100, "Overwrite Depth ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            ImGui::Checkbox(buf100, &fx_ps[i][j]->overwriteDepth);
                            sprintf(buf100, "Depth write ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            ImGui::Checkbox(buf100, &fx_ps[i][j]->depthWrite);

                            sprintf(buf100, "Original PS ASM ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->fxcAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "PS fxcAsm ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                                ImGui::TextUnformatted(fx_ps[i][j]->fxcAsm.c_str());
                                ImGui::EndChild();
                                ImGui::TreePop();
                            }
                            sprintf(buf100, "Loaded PS ASM ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->loadedAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "PS loadedAsm ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                                ImGui::TextUnformatted(fx_ps[i][j]->loadedAsm.c_str());
                                ImGui::EndChild();
                                ImGui::TreePop();
                            }
                            sprintf(buf100, "Edited PS ASM ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->editedAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "PS editedAsm ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                                ImGui::TextUnformatted(fx_ps[i][j]->editedAsm.c_str());
                                ImGui::EndChild();
                                ImGui::TreePop();
                            }
                            sprintf(buf100, "Loaded PS HLSL ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->loadedFx.length() > 1 && ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "PS loadedFx ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                                ImGui::TextUnformatted(fx_ps[i][j]->loadedFx.c_str());
                                ImGui::EndChild();
                                ImGui::TreePop();
                            }
                            sprintf(buf100, "Edited PS HLSL ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "PS editedFx ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                                ImGui::TextUnformatted(fx_ps[i][j]->editedFx.c_str());
                                ImGui::EndChild();
                                ImGui::TreePop();
                            }

                            sprintf(buf100, "Last PS Constants ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "PS Constants ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);

                                auto& cntnt = (fx_ps[i][j]->usingShader != SU_FXC && fx_ps[i][j]->compiledShaders[fx_ps[i][j]->usingShader] != nullptr) ? static_cast<m_IDirect3DPixelShader9*>(fx_ps[i][j]->compiledShaders[fx_ps[i][j]->usingShader])->constants : fx_ps[i][j]->constants;

                                auto shad = (fx_ps[i][j]->usingShader != SU_FXC && fx_ps[i][j]->compiledShaders[fx_ps[i][j]->usingShader] != nullptr) ? static_cast<m_IDirect3DPixelShader9*>(fx_ps[i][j]->compiledShaders[fx_ps[i][j]->usingShader]) : fx_ps[i][j];

                                if(shad) {
                                    for(int c = 0; c < 255; c++) {
                                        if(shad->constantIndex[c]) {
                                            sprintf(buf100, "c%i##psb%i_%i_%i_%i", c, c, i, j, fx_ps[i][j]->id);
                                        }
                                        else {
                                            sprintf(buf100, "##psb%i_%i_%i_%i", c, i, j, fx_ps[i][j]->id);
                                        }
                                        if(ImGui::Checkbox(buf100, &shad->constantIndex[c])) {
                                            if(shad->constantIndex[c]) {
                                                shad->constantReplace[c] = cntnt[c];
                                            }
                                            else {
                                                shad->constantReplace.erase(c);
                                            }
                                        }
                                        ImGui::SameLine();
                                        if(shad->constantIndex[c]) {
                                            sprintf(buf100, "##psv%i_%i_%i_%i", c, i, j, fx_ps[i][j]->id);
                                            ImGui::SetNextItemWidth(-1.f);
                                            ImGui::DragFloat4(buf100, (shad->constantReplace[c]), 1.0f, -2000, 2000, "%f", ImGuiSliderFlags_Logarithmic);
                                        }
                                        else {
                                            ImGui::Text("c%i, %f, %f, %f, %f", c, cntnt[c][0], cntnt[c][1], cntnt[c][2], cntnt[c][3]);
                                        }
                                    }
                                }

                                ImGui::EndChild();

                                ImGui::TreePop();
                            }

                            sprintf(buf100, "Edited Constants ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "Edited Constants ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);

                                auto shad = (fx_ps[i][j]->usingShader != SU_FXC && fx_ps[i][j]->compiledShaders[fx_ps[i][j]->usingShader] != nullptr) ? static_cast<m_IDirect3DPixelShader9*>(fx_ps[i][j]->compiledShaders[fx_ps[i][j]->usingShader]) : fx_ps[i][j];

                                if(shad && !shad->constantReplace.empty()) {
                                    for(auto& [key, value] : shad->constantReplace) {
                                        sprintf(buf100, "##pse%i_%i_%i_%i", key, i, j, fx_ps[i][j]->id);
                                        ImGui::Text("c%i", key);
                                        ImGui::SameLine();

                                        ImGui::SetNextItemWidth(-40.f);
                                        ImGui::DragFloat4(buf100, (float*)value, 1.0f, -2000, 2000, "%f", ImGuiSliderFlags_Logarithmic);
                                    }
                                }

                                ImGui::EndChild();

                                ImGui::TreePop();
                            }

                            ImGui::TreePop();
                        }
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();
    }

    if(ImGui::CollapsingHeader("Vertex shaders")) {
        ImGui::Checkbox("Show Unused", &showUnused);
        ImGui::Checkbox("Hide Empty FXC##vs", &hideEmptyFXC);
        if(ImGui::Button("Reset Use##vs")) {
            for(int i = 0; i < (int) fx_vs.size(); i++) {
                for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                    if(fx_vs[i][j]) {
                        fx_vs[i][j]->used = 0;
                    }
                }
            }
        }
        if(ImGui::Button("Enable all vs")) {
            for(int i = 0; i < (int) fx_vs.size(); i++) {
                for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                    if(fx_vs[i][j]) {
                        fx_vs[i][j]->disable = false;
                        fx_vs[i][j]->useNewShader = false;
                    }
                }
            }
        }
        ImGui::BeginChild("ChildVS", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 2.f / 3.f), false, ImGuiWindowFlags_HorizontalScrollbar);

        for(int i = 0; i < (int) fx_vs.size(); i++) {
            if(fx_vs[i].size() == 0 && hideEmptyFXC)
                continue;
            sprintf(buf100, "%s##vs%i", shader_names_fxc[i], i);
            if(ImGui::TreeNode(buf100)) {
                sprintf(buf100, "Disable all##vs%i", i);
                if(ImGui::Button(buf100)) {
                    for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                        if(fx_vs[i][j]) {
                            fx_vs[i][j]->disable = true;
                            fx_vs[i][j]->useNewShader = false;
                        }
                    }
                }
                sprintf(buf100, "Enable all##vs%i", i);
                ImGui::SameLine();
                if(ImGui::Button(buf100)) {
                    for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                        if(fx_vs[i][j]) {
                            fx_vs[i][j]->disable = false;
                            fx_vs[i][j]->useNewShader = false;
                        }
                    }
                }
                for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                    if(fx_vs[i][j] && ((showUnused == false && fx_vs[i][j]->used > 0) || showUnused == true)) {
                        sprintf(buf100, "##Disable VS Shader %i_%i_%i", i, j, fx_vs[i][j]->id);
                        ImGui::Checkbox(buf100, &fx_vs[i][j]->disable);
                        ImGui::SameLine();
                        sprintf(buf100, "%s #vs%i_%i_%i", fx_vs[i][j]->oName.c_str(), i, j, fx_vs[i][j]->id);
                        if(ImGui::TreeNode(buf100)) {
                            std::string namefx = fx_vs[i][j]->oName.substr(0, fx_vs[i][j]->oName.find_last_of(".")) + std::string(".fx");
                            std::string nameasm = fx_vs[i][j]->oName;

                            //ImGui::Text("VS ID %i ##%i_%i_%i", fx_vs[i][j]->id, i, j, fx_vs[i][j]->id);
                            ImGui::Text("CRC_32:   %x ##vs%i_%i_%i", fx_vs[i][j]->crc32, i, j, fx_vs[i][j]->id);
                            ImGui::Text("Use Counter: %i ##%i_%i_%i", fx_vs[i][j]->used, i, j, fx_vs[i][j]->id);

                            sprintf(buf100, "Compile VS asm ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->editedAsm.length() > 3 && ImGui::Button(buf100)) {
                                for(auto t : lst) {
                                    if(t->bs && t->bs == fx_vs[i][j] && t->name == nameasm) {
                                        t->bs->editedAsm = t->editor->GetText();;
                                        if(t->bs->compileNewASM() == S_OK && fx_vs[i][j]->newShader) {
                                            t->bs->useNewShader = true;
                                        }
                                    }
                                }
                            }
                            sprintf(buf100, "Compile VS hlsl ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->editedFx.length() > 3 && ImGui::Button(buf100)) {
                                for(auto t : lst) {
                                    if(t->bs && t->bs == fx_vs[i][j] && t->name == namefx) {
                                        t->bs->editedFx = t->editor->GetText();;
                                        if(t->bs->compileNewFx() == S_OK && fx_vs[i][j]->newShader) {
                                            IDirect3DVertexShader9* pShader = fx_vs[i][j]->newShader;
                                            static std::vector<uint8_t> pbFunc;
                                            UINT len;
                                            pShader->GetFunction(nullptr, &len);
                                            if(pbFunc.size() < len) {
                                                pbFunc.resize(len + len % 4);
                                            }
                                            pShader->GetFunction(pbFunc.data(), &len);

                                            ID3DXBuffer* pShaderAsm = NULL;
                                            HRESULT hr = D3DXDisassembleShader((DWORD*) pbFunc.data(), FALSE, NULL, &pShaderAsm);
                                            if(SUCCEEDED(hr) && pShaderAsm) {
                                                t->bs->editedAsm = (char*) pShaderAsm->GetBufferPointer();
                                            }

                                            t->bs->useNewShader = true;
                                        }
                                    }
                                }
                            }
                            sprintf(buf100, "Reset VS ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(ImGui::Button(buf100)) {
                                for(auto t : lst) {
                                    if(t->bs && t->bs == fx_vs[i][j]) {
                                        t->bs->editedAsm = "";
                                        t->bs->editedFx = "";
                                        if(fx_vs[i][j]->newShader)
                                            fx_vs[i][j]->newShader->Release();
                                        fx_vs[i][j]->newShader = 0;
                                        lst.remove(t);
                                        break;
                                    }
                                }
                                for(auto t : lst) {
                                    if(t->bs && t->bs == fx_vs[i][j]) {
                                        t->bs->editedAsm = "";
                                        t->bs->editedFx = "";
                                        if(fx_vs[i][j]->newShader)
                                            fx_vs[i][j]->newShader->Release();
                                        fx_vs[i][j]->newShader = 0;
                                        lst.remove(t);
                                        break;
                                    }
                                }
                            }

                            sprintf(buf100, "Edit VS FXC asm ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(ImGui::Button(buf100)) {
                                bool isInEditor = false;
                                sprintf(buf100, "%s#FXCASM##%i_%i_%i", nameasm.c_str(), i, j, fx_vs[i][j]->id);
                                std::string name = buf100;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_vs[i][j]);
                                    if(bs && t->bs == bs && t->name == name) {
                                        isInEditor = true;
                                        t->show = true;
                                        showEditorWindow = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    //if(fx_vs[i][j]->editedAsm.length() < 3) {
                                    //	fx_vs[i][j]->editedAsm = fx_vs[i][j]->GetAsm();
                                    //}
                                    stShaderEditor* s = new stShaderEditor(name, VS_ASM, new TextEditor(), fx_vs[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->GetAsm());
                                    lst.push_back(s);
                                    showEditorWindow = true;
                                }
                            }
                            sprintf(buf100, "Edit VS Loaded asm ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->loadedAsm.length() > 0 && ImGui::Button(buf100)) {
                                bool isInEditor = false;
                                sprintf(buf100, "%s#LASM##%i_%i_%i", nameasm.c_str(), i, j, fx_vs[i][j]->id);
                                std::string name = buf100;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_vs[i][j]);
                                    if(bs && t->bs == bs && t->name == name) {
                                        isInEditor = true;
                                        t->show = true;
                                        showEditorWindow = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    //if(fx_vs[i][j]->editedAsm.length() < 3) {
                                    //	fx_vs[i][j]->editedAsm = fx_vs[i][j]->loadedAsm;
                                    //}
                                    stShaderEditor* s = new stShaderEditor(name, VS_ASM, new TextEditor(), fx_vs[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->loadedAsm);
                                    lst.push_back(s);
                                    showEditorWindow = true;
                                }
                            }
                            sprintf(buf100, "Edit VS Loaded hlsl ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->loadedFx.length() > 0 && ImGui::Button(buf100)) {
                                bool isInEditor = false;
                                sprintf(buf100, "%s#LFX##%i_%i_%i", nameasm.c_str(), i, j, fx_vs[i][j]->id);
                                std::string name = buf100;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_vs[i][j]);
                                    if(bs && t->bs == bs && t->name == name) {
                                        isInEditor = true;
                                        t->show = true;
                                        showEditorWindow = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    //if(fx_vs[i][j]->editedFx.length() < 3) {
                                    //	fx_vs[i][j]->editedFx = fx_vs[i][j]->loadedFx;
                                    //}
                                    stShaderEditor* s = new stShaderEditor(name, VS_FX, new TextEditor(), fx_vs[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->loadedFx);
                                    lst.push_back(s);
                                    showEditorWindow = true;
                                }
                            }
                            sprintf(buf100, "Edit VS Edited hlsl ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->editedFx.length() > 0 && ImGui::Button(buf100)) {
                                bool isInEditor = false;
                                sprintf(buf100, "%s#EFX##%i_%i_%i", nameasm.c_str(), i, j, fx_vs[i][j]->id);
                                std::string name = buf100;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_vs[i][j]);
                                    if(bs && t->bs == bs && t->name == name) {
                                        isInEditor = true;
                                        t->show = true;
                                        showEditorWindow = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    //if(fx_vs[i][j]->editedFx.length() < 3) {
                                    //	fx_vs[i][j]->editedFx = fx_vs[i][j]->loadedFx;
                                    //}
                                    stShaderEditor* s = new stShaderEditor(name, VS_FX, new TextEditor(), fx_vs[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->editedFx);
                                    lst.push_back(s);
                                    showEditorWindow = true;
                                }
                            }

                            if(!fx_vs[i][j]->newShader)
                                fx_vs[i][j]->useNewShader = false;
                            else {
                                sprintf(buf100, "Use New VS Shader ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                                ImGui::Checkbox(buf100, &fx_vs[i][j]->useNewShader);
                            }
                            static int e = 0;
                            ImGui::RadioButton("FXC", (int*) &fx_vs[i][j]->usingShader, 0); ImGui::SameLine();
                            if(fx_vs[i][j]->compiledShaders[SU_LASM]) { ImGui::RadioButton("Loaded ASM", (int*) &fx_vs[i][j]->usingShader, 1); ImGui::SameLine(); }
                            if(fx_vs[i][j]->compiledShaders[SU_EASM]) { ImGui::RadioButton("Edited ASM", (int*) &fx_vs[i][j]->usingShader, 2); }
                            if(fx_vs[i][j]->compiledShaders[SU_LFX]) { ImGui::RadioButton("Loaded HLSL", (int*) &fx_vs[i][j]->usingShader, 3); ImGui::SameLine(); }
                            if(fx_vs[i][j]->compiledShaders[SU_EFX]) { ImGui::RadioButton("Edited HLSL", (int*) &fx_vs[i][j]->usingShader, 4); ImGui::SameLine(); }

                            ImGui::Text("");

                            //sprintf(buf100, "Overwrite Depth ##vs%i_%i_%i", i, j, fx_vs[i][j]->id);
                            //ImGui::Checkbox(buf100, &fx_vs[i][j]->overwriteDepth);
                            //sprintf(buf100, "Depth write ##vs%i_%i_%i", i, j, fx_vs[i][j]->id);
                            //ImGui::Checkbox(buf100, &fx_vs[i][j]->depthWrite);

                            //sprintf(buf100, "Disable depth write ##vs%i_%i_%i", i, j, fx_vs[i][j]->id);
                            //ImGui::Checkbox(buf100, &fx_vs[i][j]->disableDepthWrite);

                            sprintf(buf100, "Original VS ASM ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->fxcAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "VS fxcAsm ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                                ImGui::TextUnformatted(fx_vs[i][j]->fxcAsm.c_str());
                                ImGui::EndChild();
                                ImGui::TreePop();
                            }
                            sprintf(buf100, "Loaded VS ASM ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->loadedAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "VS loadedAsm ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                                ImGui::TextUnformatted(fx_vs[i][j]->loadedAsm.c_str());
                                ImGui::EndChild();
                                ImGui::TreePop();
                            }
                            sprintf(buf100, "Edited VS ASM ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->editedAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "VS editedAsm ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                                ImGui::TextUnformatted(fx_vs[i][j]->editedAsm.c_str());
                                ImGui::EndChild();
                                ImGui::TreePop();
                            }
                            sprintf(buf100, "Loaded VS HLSL ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->loadedFx.length() > 1 && ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "VS loadedFx ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                                ImGui::TextUnformatted(fx_vs[i][j]->loadedFx.c_str());
                                ImGui::EndChild();
                                ImGui::TreePop();
                            }
                            sprintf(buf100, "Edited VS HLSL ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "VS editedFx ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                                ImGui::TextUnformatted(fx_vs[i][j]->editedFx.c_str());
                                ImGui::EndChild();
                                ImGui::TreePop();
                            }

                            sprintf(buf100, "Last VS Constants ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(ImGui::TreeNode(buf100)) {
                                sprintf(buf100, "Last VS Constants ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                                ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);

                                //for(int c = 0; c < 255; c++) {
                                //    ImGui::Text("c%i, %f, %f, %f, %f", c, fx_vs[i][j]->constants[c][0], fx_vs[i][j]->constants[c][1], fx_vs[i][j]->constants[c][2], fx_vs[i][j]->constants[c][3]);
                                //}
                                auto& cntnt = (fx_vs[i][j]->usingShader != SU_FXC && fx_vs[i][j]->compiledShaders[fx_vs[i][j]->usingShader] != nullptr) ? static_cast<m_IDirect3DVertexShader9*>(fx_vs[i][j]->compiledShaders[fx_vs[i][j]->usingShader])->constants : fx_vs[i][j]->constants;
                                
                                auto shad = (fx_vs[i][j]->usingShader != SU_FXC && fx_vs[i][j]->compiledShaders[fx_vs[i][j]->usingShader] != nullptr) ? static_cast<m_IDirect3DVertexShader9*>(fx_vs[i][j]->compiledShaders[fx_vs[i][j]->usingShader]) : fx_vs[i][j];

                                if(shad) {
                                    for(int c = 0; c < 255; c++) {
                                        if(shad->constantIndex[c]) {
                                            sprintf(buf100, "c%i##vsb%i_%i_%i_%i", c, c, i, j, fx_vs[i][j]->id);
                                        }
                                        else {
                                            sprintf(buf100, "##vsb%i_%i_%i_%i", c, i, j, fx_vs[i][j]->id);
                                        }
                                        if(ImGui::Checkbox(buf100, &shad->constantIndex[c])) {
                                            if(shad->constantIndex[c]) {
                                                shad->constantReplace[c] = cntnt[c];
                                            }
                                            else {
                                                shad->constantReplace.erase(c);
                                            }
                                        }
                                        ImGui::SameLine();
                                        if(shad->constantIndex[c]) {
                                            sprintf(buf100, "##vsv%i_%i_%i_%i", c, i, j, fx_vs[i][j]->id);
                                            ImGui::SetNextItemWidth(-1.f);
                                            ImGui::DragFloat4(buf100, (shad->constantReplace[c]), 0.1, -2000, 2000, "%f", ImGuiSliderFlags_Logarithmic);
                                        }
                                        else {
                                            ImGui::Text("c%i, %f, %f, %f, %f", c, cntnt[c][0], cntnt[c][1], cntnt[c][2], cntnt[c][3]);
                                        }
                                    }
                                }
                                
                                //for(int c = 0; c < 255; c++) {
                                //    ImGui::Text("c%i, %f, %f, %f, %f", c, cntnt[c][0], cntnt[c][1], cntnt[c][2], cntnt[c][3]);
                                //}

                                ImGui::EndChild();
                                ImGui::TreePop();
                            }
                            ImGui::TreePop();
                        }
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();
    }

    if(ImGui::CollapsingHeader("Pixel shaders 2")) {
        ImGui::BeginChild("ChildPS2", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 2.f / 3.f), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Checkbox("Show Unused##ps2", &showUnused);
        if(ImGui::Button("Reset Use##ps2")) {
            for(int i = 0; i < (int) ps_2.size(); i++) {
                if(ps_2[i]) {
                    ps_2[i]->used = 0;
                }
            }
        }
        if(ImGui::Button("Enable all ps2")) {
            for(int i = 0; i < (int) ps_2.size(); i++) {
                if(ps_2[i]) {
                    ps_2[i]->disable = false;
                    ps_2[i]->useNewShader = false;
                }
            }
        }

        static char filter[50] = { 0 };
        ImGui::InputText("Filter PS2", filter, 49);
        for(int i = 0; i < (int) ps_2.size(); i++) {
            m_IDirect3DPixelShader9* pShader2 = static_cast<m_IDirect3DPixelShader9*>(ImGuiPS);
            if(ps_2[i] /*&& ps_2[i] != m_IDirect3DPixelShader9::dummyShader && ps_2[i] != pShader2*/ && ((showUnused == false && ps_2[i]->used > 0) || showUnused == true)) {
                //sprintf(buf100, "Pixel Shader2 %i", i);
                //if(ImGui::TreeNode(buf100)) {
                //	//m_IDirect3DPixelShader9* pShader2 = static_cast<m_IDirect3DPixelShader9*>(ImGuiPS);
                //	//if(ps_2[i] && ps_2[i]->GetProxyInterface() != m_IDirect3DPixelShader9::dummyShader && ps_2[i] != pShader2) {
                //	ImGui::Text("PS %i", ps_2[i]->id);
                //	if(ps_2[i] != ps_2[i]->dummyShader) {
                //		sprintf(buf100, "Disable PS Shader2 ##%i_%i", i, ps_2[i]->id);
                //		ImGui::Checkbox(buf100, &ps_2[i]->disable);
                //	}
                //	sprintf(buf100, "PS Assembly2 %i", i);
                //	if(ImGui::TreeNode(buf100)) {
                //		//char* as = (char*) ps[i]->GetAsm();
                //		ImGui::Text("%s\n", ps_2[i]->fxcAsm.c_str());
                //		ImGui::TreePop();
                //	}
                //	sprintf(buf100, "Last PS2 Constants ##%i", i);
                //	if(ImGui::TreeNode(buf100)) {
                //		for(int c = 0; c < 255; c++) {
                //			ImGui::Text("c%i, %f, %f, %f, %f", c, ps_2[i]->constants[c][0], ps_2[i]->constants[c][1], ps_2[i]->constants[c][2], ps_2[i]->constants[c][3]);
                //		}
                //		ImGui::TreePop();
                //	}
                //	ImGui::TreePop();
                //}
                if((strlen(filter) >0 && ps_2[i]->oName.find(filter) != ps_2[i]->oName.npos) || strlen(filter) == 0) {
                    //continue;
                
                    sprintf(buf100, "##Disable PS Shader %i_%i", i, ps_2[i]->id);
                    ImGui::Checkbox(buf100, &ps_2[i]->disable);
                    ImGui::SameLine();

                    sprintf(buf100, "%s #ps2%i %i", ps_2[i]->oName.c_str(), i, ps_2[i]->id);
                    if(ImGui::TreeNode(buf100)) {
                        std::string fname = ps_2[i]->fileName;
                        if(fname.length() < 2)
                            fname = ps_2[i]->oName;
                        fname = fname.substr(0, fname.find_last_of("."));
                        std::string namefx;
                        std::string nameasm;

                        if(ps_2[i]->entryFunction.length() > 2)
                            fname = fname + "_" + ps_2[i]->entryFunction;


                        sprintf(buf100, "%s.fx", fname.c_str());
                        namefx = buf100;
                        sprintf(buf100, "%s.asm", fname.c_str());
                        nameasm = buf100;

                        //if(fname != nameasm && fname != namefx) {
                        //    if(fname.find_last_of(".fx") == fname.npos) {
                        //        sprintf(buf100, "%s.fx", fname.c_str());
                        //        namefx = buf100;
                        //    }
                        //    else
                        //        namefx = fname.c_str();;

                        //    if(fname.find_last_of(".asm") == fname.npos) {
                        //        sprintf(buf100, "%s.asm", fname.c_str());
                        //        nameasm = buf100;
                        //    }
                        //    else
                        //        nameasm = fname.c_str();;
                        //}

                        //fname;

                        //ImGui::Text("PS ID %i ##%i", ps_2[i]->id, i);
                        ImGui::Text("Use Counter: %i ##%i_%i", ps_2[i]->used, i, ps_2[i]->id);
                        if(ps_2[i]->fileName.length() > 1)
                            ImGui::Text("Filename: %s ##%i_%i", ps_2[i]->fileName.c_str(), i, ps_2[i]->id);
                        if(ps_2[i]->entryFunction.length() > 1)
                            ImGui::Text("Function: %s ##%i_%i", ps_2[i]->entryFunction.c_str(), i, ps_2[i]->id);

                        sprintf(buf100, "Compile PS asm ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->editedAsm.length() > 3 && ImGui::Button(buf100)) {
                            for(auto t : lst) {
                                if(t->bs && t->bs == ps_2[i] && t->name == nameasm) {
                                    t->bs->editedAsm = t->editor->GetText();;
                                    if(t->bs->compileNewASM() == S_OK && ps_2[i]->newShader) {
                                        t->bs->useNewShader = true;
                                    }
                                }
                            }
                        }
                        sprintf(buf100, "Compile PS hlsl ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->editedFx.length() > 3 && ImGui::Button(buf100)) {
                            for(auto t : lst) {
                                if(t->bs && t->bs == ps_2[i] && t->name == namefx) {
                                    t->bs->editedFx = t->editor->GetText();;
                                    if(t->bs->compileNewFx() == S_OK && ps_2[i]->newShader) {
                                        IDirect3DPixelShader9* pShader = ps_2[i]->newShader;
                                        static std::vector<uint8_t> pbFunc;
                                        UINT len;
                                        pShader->GetFunction(nullptr, &len);
                                        if(pbFunc.size() < len) {
                                            pbFunc.resize(len + len % 4);
                                        }
                                        pShader->GetFunction(pbFunc.data(), &len);

                                        ID3DXBuffer* pShaderAsm = NULL;
                                        HRESULT hr = D3DXDisassembleShader((DWORD*) pbFunc.data(), FALSE, NULL, &pShaderAsm);
                                        if(SUCCEEDED(hr) && pShaderAsm) {
                                            t->bs->editedAsm = (char*) pShaderAsm->GetBufferPointer();
                                        }

                                        t->bs->useNewShader = true;
                                    }
                                }
                            }
                        }
                        sprintf(buf100, "Reset PS ##%i_%i", i, ps_2[i]->id);
                        if(ImGui::Button(buf100)) {
                            for(auto t : lst) {
                                if(t->bs && t->bs == ps_2[i]) {
                                    t->bs->editedAsm = "";
                                    t->bs->editedFx = "";
                                    if(ps_2[i]->newShader)
                                        ps_2[i]->newShader->Release();
                                    ps_2[i]->newShader = 0;
                                    lst.remove(t);
                                    break;
                                }
                            }
                            for(auto t : lst) {
                                if(t->bs && t->bs == ps_2[i]) {
                                    t->bs->editedAsm = "";
                                    t->bs->editedFx = "";
                                    if(ps_2[i]->newShader)
                                        ps_2[i]->newShader->Release();
                                    ps_2[i]->newShader = 0;
                                    lst.remove(t);
                                    break;
                                }
                            }
                        }

                        sprintf(buf100, "Edit PS FXC asm ##%i_%i", i, ps_2[i]->id);
                        if(ImGui::Button(buf100)) {
                            bool isInEditor = false;
                            sprintf(buf100, "%s#FXCASM##%i_%i", nameasm.c_str(), i, ps_2[i]->id);
                            std::string name = buf100;
                            for(auto t : lst) {
                                basicShader* bs = static_cast<basicShader*> (ps_2[i]);
                                if(bs && t->bs == bs && t->name == name) {
                                    isInEditor = true;
                                    t->show = true;
                                    showEditorWindow = true;
                                }
                            }
                            if(isInEditor == false) {
                                //if(ps_2[i]->editedAsm.length() < 3) {
                                //	ps_2[i]->editedAsm = ps_2[i]->GetAsm();
                                //}
                                stShaderEditor* s = new stShaderEditor(name, PS_ASM, new TextEditor(), ps_2[i]);
                                s->editor->SetShowWhitespaces(0);
                                if(s->bs->fxcAsm.length() > 1)
                                    s->editor->SetText(s->bs->fxcAsm);
                                else
                                    s->editor->SetText(s->bs->GetAsm());
                                lst.push_back(s);
                                showEditorWindow = true;
                            }
                        }
                        sprintf(buf100, "Edit PS Loaded asm ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->loadedAsm.length() > 0 && ImGui::Button(buf100)) {
                            bool isInEditor = false;
                            sprintf(buf100, "%s#LASM##%i_%i", nameasm.c_str(), i, ps_2[i]->id);
                            std::string name = buf100;
                            for(auto t : lst) {
                                basicShader* bs = static_cast<basicShader*> (ps_2[i]);
                                if(bs && t->bs == bs && t->name == name) {
                                    isInEditor = true;
                                    t->show = true;
                                    showEditorWindow = true;
                                }
                            }
                            if(isInEditor == false) {
                                //if(ps_2[i]->editedAsm.length() < 3) {
                                //	ps_2[i]->editedAsm = ps_2[i]->loadedAsm;
                                //}
                                stShaderEditor* s = new stShaderEditor(name, PS_ASM, new TextEditor(), ps_2[i]);
                                s->editor->SetShowWhitespaces(0);
                                s->editor->SetText(s->bs->loadedAsm);
                                lst.push_back(s);
                                showEditorWindow = true;
                            }
                        }
                        sprintf(buf100, "Edit PS Loaded hlsl ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->loadedFx.length() > 0 && ImGui::Button(buf100)) {
                            bool isInEditor = false;
                            sprintf(buf100, "%s#LFX##%i_%i", namefx.c_str(), i, ps_2[i]->id);
                            std::string name = buf100;
                            for(auto t : lst) {
                                basicShader* bs = static_cast<basicShader*> (ps_2[i]);
                                if(bs && t->bs == bs && t->name == name) {
                                    isInEditor = true;
                                    t->show = true;
                                    showEditorWindow = true;
                                }
                            }
                            if(isInEditor == false) {
                                //if(ps_2[i]->editedFx.length() < 3) {
                                //	ps_2[i]->editedFx = ps_2[i]->loadedFx;
                                //}
                                stShaderEditor* s = new stShaderEditor(name, PS_FX, new TextEditor(), ps_2[i]);
                                s->editor->SetShowWhitespaces(0);
                                s->editor->SetText(s->bs->loadedFx);
                                lst.push_back(s);
                                showEditorWindow = true;
                            }
                        }
                        sprintf(buf100, "Edit PS Edited asm ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->editedAsm.length() > 0 && ImGui::Button(buf100)) {
                            bool isInEditor = false;
                            sprintf(buf100, "%s#EASM##%i_%i", nameasm.c_str(), i, ps_2[i]->id);
                            std::string name = buf100;
                            for(auto t : lst) {
                                basicShader* bs = static_cast<basicShader*> (ps_2[i]);
                                if(bs && t->bs == bs && t->name == name) {
                                    isInEditor = true;
                                    t->show = true;
                                    showEditorWindow = true;
                                }
                            }
                            if(isInEditor == false) {
                                //if(ps_2[i]->editedAsm.length() < 3) {
                                //	ps_2[i]->editedAsm = ps_2[i]->loadedAsm;
                                //}
                                stShaderEditor* s = new stShaderEditor(name, PS_ASM, new TextEditor(), ps_2[i]);
                                s->editor->SetShowWhitespaces(0);
                                s->editor->SetText(s->bs->editedAsm);
                                lst.push_back(s);
                                showEditorWindow = true;
                            }
                        }
                        sprintf(buf100, "Edit PS Edited hlsl ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->editedFx.length() > 0 && ImGui::Button(buf100)) {
                            bool isInEditor = false;
                            sprintf(buf100, "%s#EFX##%i_%i", namefx.c_str(), i, ps_2[i]->id);
                            std::string name = buf100;
                            for(auto t : lst) {
                                basicShader* bs = static_cast<basicShader*> (ps_2[i]);
                                if(bs && t->bs == bs && t->name == name) {
                                    isInEditor = true;
                                    t->show = true;
                                    showEditorWindow = true;
                                }
                            }
                            if(isInEditor == false) {
                                //if(ps_2[i]->editedFx.length() < 3) {
                                //	ps_2[i]->editedFx = ps_2[i]->loadedFx;
                                //}
                                stShaderEditor* s = new stShaderEditor(name, PS_FX, new TextEditor(), ps_2[i]);
                                s->editor->SetShowWhitespaces(0);
                                s->editor->SetText(s->bs->editedFx);
                                lst.push_back(s);
                                showEditorWindow = true;
                            }
                        }

                        sprintf(buf100, "Load Shader fx ##%i_%i", i, ps_2[i]->id);
                        if(ImGui::Button(buf100)) {
                            ps_2[i]->editedFx = LoadFX(ps_2[i]->fxName, fname);
                            //std::string src = LoadFX(ps_2[i]->fxName, ps_2[i]->fileName);
                            //std::string src2 = LoadASM(ps_2[i]->fxName, ps_2[i]->fileName);
                            //std::string src3 = LoadFX(ps_2[i]->fxName, fname);
                            //std::string src4 = LoadASM(ps_2[i]->fxName, fname);
                            //printf("%s %s %s %s", src.c_str(), src2.c_str(), src3.c_str(), src4.c_str());
                        }

                        sprintf(buf100, "Load Shader asm ##%i_%i", i, ps_2[i]->id);
                        if(ImGui::Button(buf100)) {
                            //std::string src = LoadFX(ps_2[i]->fxName, ps_2[i]->fileName);
                            //std::string src2 = LoadASM(ps_2[i]->fxName, ps_2[i]->fileName);
                            //std::string src3 = LoadFX(ps_2[i]->fxName, fname);
                            ps_2[i]->editedAsm = LoadASM(ps_2[i]->fxName, fname);
                            //printf("%s %s %s %s", src.c_str(), src2.c_str(), src3.c_str(), src4.c_str());
                        }

                        sprintf(buf100, "Reload Shader ##%i_%i", i, ps_2[i]->id);
                        if(ImGui::Button(buf100)) {
                            m_IDirect3DPixelShader9* reloadedShader = nullptr;
                            if(ps_2[i]->fileName.length() > 1 && ps_2[i]->entryFunction.length() > 1) {
                                reloadedShader = static_cast<m_IDirect3DPixelShader9*>(CompilePixelShaderFromFile(ps_2[i]->fileName.c_str(), ps_2[i]->entryFunction.c_str(), ps_2[i]->oName.c_str(), ps_2[i]->m_pDeviceEx, false, false));
                            }
                            else {
                                reloadedShader = static_cast<m_IDirect3DPixelShader9*>(CompilePixelShaderFromFile(ps_2[i]->fileName.c_str(), ps_2[i]->entryFunction.c_str(), ps_2[i]->oName.c_str(), ps_2[i]->m_pDeviceEx, true, false));
                            }
                            if(reloadedShader) {
                                for(int k = 0; k < 5; k++) {
                                    if(reloadedShader->compiledShaders[k]) {
                                        SAFE_RELEASE(ps_2[k]->compiledShaders[k]);
                                        ps_2[k]->compiledShaders[k] = reloadedShader->compiledShaders[k];
                                    }
                                }

                                if(reloadedShader->newShader) {
                                    SAFE_RELEASE(ps_2[i]->newShader);
                                    ps_2[i]->newShader = reloadedShader->newShader;
                                }

                                //memcpy(ps_2[i]->compiledShaders, reloadedShader->compiledShaders, sizeof(reloadedShader->compiledShaders));
                                //memcpy(ps_2[i]->globalConstants, reloadedShader->globalConstants, sizeof(reloadedShader->globalConstants)); // constant table, set with Set*ShaderConstantF
                                //memcpy(ps_2[i]->constants, reloadedShader->constants, sizeof(reloadedShader->constants)); // constant table, set with Set*ShaderConstantF

                                //ps_2[i]->id = reloadedShader->id;
                                //ps_2[i]->fxid = reloadedShader->fxid;
                                //ps_2[i]->gid = reloadedShader->gid;
                                ps_2[i]->usingShader = reloadedShader->usingShader;
                                ps_2[i]->oName = reloadedShader->oName;
                                ps_2[i]->fxName = reloadedShader->fxName;
                                ps_2[i]->fxcAsm = reloadedShader->fxcAsm;
                                ps_2[i]->loadedAsm = reloadedShader->loadedAsm;
                                ps_2[i]->editedAsm = reloadedShader->editedAsm;
                                ps_2[i]->loadedFx = reloadedShader->loadedFx;
                                ps_2[i]->editedFx = reloadedShader->editedFx;
                                //ps_2[i]->entryFunction = reloadedShader->entryFunction;
                                //ps_2[i]->fileName = reloadedShader->fileName;
                                ps_2[i]->crc32 = reloadedShader->crc32;
                                ps_2[i]->useNewShader = reloadedShader->useNewShader;
                                //ps_2[i]->disable           = reloadedShader->disable             ;
                                ps_2[i]->dirt = reloadedShader->dirt;
                                ps_2[i]->pixel = reloadedShader->pixel;
                                ps_2[i]->useBias = reloadedShader->useBias;
                                ps_2[i]->overwriteDepth = reloadedShader->overwriteDepth;
                                ps_2[i]->depthWrite = reloadedShader->depthWrite;
                            }
                        }

                        if(!ps_2[i]->newShader)
                            ps_2[i]->useNewShader = false;
                        else {
                            sprintf(buf100, "Use New PS Shader ##%i_%i", i, ps_2[i]->id);
                            ImGui::Checkbox(buf100, &ps_2[i]->useNewShader);
                        }
                        static int e = 0;
                        ImGui::RadioButton("FXC", (int*) &ps_2[i]->usingShader, 0); ImGui::SameLine();
                        if(ps_2[i]->compiledShaders[SU_LASM]) { ImGui::RadioButton("Loaded ASM", (int*) &ps_2[i]->usingShader, 1); ImGui::SameLine(); }
                        if(ps_2[i]->compiledShaders[SU_LFX]) { ImGui::RadioButton("Loaded HLSL", (int*) &ps_2[i]->usingShader, 2); }
                        if(ps_2[i]->compiledShaders[SU_EASM]) { ImGui::RadioButton("Edited ASM", (int*) &ps_2[i]->usingShader, 3); ImGui::SameLine(); }
                        if(ps_2[i]->compiledShaders[SU_EFX]) { ImGui::RadioButton("Edited HLSL", (int*) &ps_2[i]->usingShader, 4); ImGui::SameLine(); }

                        ImGui::Text("");

                        sprintf(buf100, "Overwrite Depth ##%i_%i", i, ps_2[i]->id);
                        ImGui::Checkbox(buf100, &ps_2[i]->overwriteDepth);
                        sprintf(buf100, "Depth write ##%i_%i", i, ps_2[i]->id);
                        ImGui::Checkbox(buf100, &ps_2[i]->depthWrite);

                        sprintf(buf100, "Original PS ASM ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->fxcAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                            sprintf(buf100, "PS2 fxcAsm ##%i_%i", i, ps_2[i]->id);
                            ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                            ImGui::TextUnformatted(ps_2[i]->fxcAsm.c_str());
                            ImGui::EndChild();
                            ImGui::TreePop();
                        }
                        sprintf(buf100, "Loaded PS ASM ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->loadedAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                            sprintf(buf100, "PS2 loadedAsm ##%i_%i", i, ps_2[i]->id);
                            ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                            ImGui::TextUnformatted(ps_2[i]->loadedAsm.c_str());
                            ImGui::EndChild();
                            ImGui::TreePop();
                        }
                        sprintf(buf100, "Loaded PS HLSL ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->loadedFx.length() > 1 && ImGui::TreeNode(buf100)) {
                            sprintf(buf100, "PS2 loadedFx ##%i_%i", i, ps_2[i]->id);
                            ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                            ImGui::TextUnformatted(ps_2[i]->loadedFx.c_str());
                            ImGui::EndChild();
                            ImGui::TreePop();
                        }
                        sprintf(buf100, "Edited PS ASM ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->editedAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                            sprintf(buf100, "PS2 editedAsm ##%i_%i", i, ps_2[i]->id);
                            ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                            ImGui::TextUnformatted(ps_2[i]->editedAsm.c_str());
                            ImGui::EndChild();
                            ImGui::TreePop();
                        }
                        sprintf(buf100, "Edited PS HLSL ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) {
                            sprintf(buf100, "PS2 editedFx ##%i_%i", i, ps_2[i]->id);
                            ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                            ImGui::TextUnformatted(ps_2[i]->editedFx.c_str());
                            ImGui::EndChild();
                            ImGui::TreePop();
                        }

                        sprintf(buf100, "Last PS Constants ##%i_%i", i, ps_2[i]->id);
                        if(ImGui::TreeNode(buf100)) {
                            sprintf(buf100, "Last PS2 Constants  ##%i_%i", i, ps_2[i]->id);
                            ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);

                            //for(int c = 0; c < 255; c++) {
                            //    ImGui::Text("c%i, %f, %f, %f, %f", c, ps_2[i]->constants[c][0], ps_2[i]->constants[c][1], ps_2[i]->constants[c][2], ps_2[i]->constants[c][3]);
                            //}
                            auto& cntnt = (ps_2[i]->usingShader != SU_FXC && ps_2[i]->compiledShaders[ps_2[i]->usingShader] != nullptr) ? static_cast<m_IDirect3DPixelShader9*>(ps_2[i]->compiledShaders[ps_2[i]->usingShader])->constants : ps_2[i]->constants;
                            for(int c = 0; c < 255; c++) {
                                ImGui::Text("c%i, %f, %f, %f, %f", c, cntnt[c][0], cntnt[c][1], cntnt[c][2], cntnt[c][3]);
                            }

                            ImGui::EndChild();
                            ImGui::TreePop();
                        }
                        ImGui::TreePop();
                    }
                }
            }
        }
        ImGui::EndChild();
    }

    if(ImGui::CollapsingHeader("Vertex shaders 2")) {
        ImGui::BeginChild("ChildVS2", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 2.f / 3.f), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Checkbox("Show Unused##vs2", &showUnused);
        if(ImGui::Button("Reset Use##vs2")) {
            for(int i = 0; i < (int) vs_2.size(); i++) {
                if(vs_2[i]) {
                    vs_2[i]->used = 0;
                }
            }
        }
        if(ImGui::Button("Enable all vs2")) {
            for(int i = 0; i < (int) vs_2.size(); i++) {
                if(vs_2[i]) {
                    vs_2[i]->disable = false;
                    vs_2[i]->useNewShader = false;
                }
            }
        }

        static char filter[50] = { 0 };
        ImGui::InputText("Filter VS2", filter, 49);

        for(int i = 0; i < (int) vs_2.size(); i++) {
            m_IDirect3DVertexShader9* pShader2 = static_cast<m_IDirect3DVertexShader9*>(ImGuiVS);
            if(vs_2[i] /*&& vs_2[i] != m_IDirect3DVertexShader9::dummyShader && vs_2[i] != pShader2*/ && ((showUnused == false && vs_2[i]->used > 0) || showUnused == true)) {
                //sprintf(buf100, "Vertex Shader2 %i", i);
                //if(ImGui::TreeNode(buf100)) {
                //	//m_IDirect3DVertexShader9* pShader2 = static_cast<m_IDirect3DVertexShader9*>(ImGuiVS);
                //	//if(vs_2[i] && vs_2[i]->GetProxyInterface() != m_IDirect3DVertexShader9::dummyShader && vs_2[i] != pShader2) {
                //	ImGui::Text("VS %i", vs_2[i]->id);
                //	if(vs_2[i] != vs_2[i]->dummyShader) {
                //		sprintf(buf100, "Disable VS Shader2 ##%i_%i", i, vs_2[i]->id);
                //		ImGui::Checkbox(buf100, &vs_2[i]->disable);
                //	}
                //	sprintf(buf100, "VS Assembly2 %i", i);
                //	if(ImGui::TreeNode(buf100)) {
                //		//char* as = (char*) ps[i]->GetAsm();
                //		ImGui::Text("%s\n", vs_2[i]->fxcAsm.c_str());
                //		ImGui::TreePop();
                //	}
                //	sprintf(buf100, "Last VS2 Constants ##%i", i);
                //	if(ImGui::TreeNode(buf100)) {
                //		for(int c = 0; c < 255; c++) {
                //			ImGui::Text("c%i, %f, %f, %f, %f", c, vs_2[i]->constants[c][0], vs_2[i]->constants[c][1], vs_2[i]->constants[c][2], vs_2[i]->constants[c][3]);
                //		}
                //		ImGui::TreePop();
                //	}
                //	ImGui::TreePop();
                //}
                if(strnlen(filter, 50) >0 && !(vs_2[i]->oName.find(filter) != vs_2[i]->oName.npos)) {
                    continue;
                }

                sprintf(buf100, "##Disable VS Shader %i_%i", i, vs_2[i]->id);
                ImGui::Checkbox(buf100, &vs_2[i]->disable);

                ImGui::SameLine();

                sprintf(buf100, "%s #vs2%i %i", vs_2[i]->oName.c_str(), i, vs_2[i]->id);
                if(ImGui::TreeNode(buf100)) {
                    sprintf(buf100, "vs_%x.fx", vs_2[i]->crc32);
                    std::string namefx = buf100;
                    sprintf(buf100, "vs_%x.asm", vs_2[i]->crc32);
                    std::string nameasm = buf100;
                    //vs_2[i]->oName;

                    //ImGui::Text("VS ID %i ##%i", vs_2[i]->id, i);
                    ImGui::Text("Use Counter: %i ##%i_%i", vs_2[i]->used, i, vs_2[i]->id);

                    sprintf(buf100, "Compile VS asm ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->editedAsm.length() > 3 && ImGui::Button(buf100)) {
                        for(auto t : lst) {
                            if(t->bs && t->bs == vs_2[i] && t->name == nameasm) {
                                t->bs->editedAsm = t->editor->GetText();;
                                if(t->bs->compileNewASM() == S_OK && vs_2[i]->newShader) {
                                    t->bs->useNewShader = true;
                                }
                            }
                        }
                    }
                    sprintf(buf100, "Compile VS hlsl ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->editedFx.length() > 3 && ImGui::Button(buf100)) {
                        for(auto t : lst) {
                            if(t->bs && t->bs == vs_2[i] && t->name == namefx) {
                                t->bs->editedFx = t->editor->GetText();;
                                if(t->bs->compileNewFx() == S_OK && vs_2[i]->newShader) {
                                    IDirect3DVertexShader9* pShader = vs_2[i]->newShader;
                                    static std::vector<uint8_t> pbFunc;
                                    UINT len;
                                    pShader->GetFunction(nullptr, &len);
                                    if(pbFunc.size() < len) {
                                        pbFunc.resize(len + len % 4);
                                    }
                                    pShader->GetFunction(pbFunc.data(), &len);

                                    ID3DXBuffer* pShaderAsm = NULL;
                                    HRESULT hr = D3DXDisassembleShader((DWORD*) pbFunc.data(), FALSE, NULL, &pShaderAsm);
                                    if(SUCCEEDED(hr) && pShaderAsm) {
                                        t->bs->editedAsm = (char*) pShaderAsm->GetBufferPointer();
                                    }

                                    t->bs->useNewShader = true;
                                }
                            }
                        }
                    }
                    sprintf(buf100, "Reset VS ##%i_%i", i, vs_2[i]->id);
                    if(ImGui::Button(buf100)) {
                        for(auto t : lst) {
                            if(t->bs && t->bs == vs_2[i]) {
                                t->bs->editedAsm = "";
                                t->bs->editedFx = "";
                                if(vs_2[i]->newShader)
                                    vs_2[i]->newShader->Release();
                                vs_2[i]->newShader = 0;
                                lst.remove(t);
                                break;
                            }
                        }
                        for(auto t : lst) {
                            if(t->bs && t->bs == vs_2[i]) {
                                t->bs->editedAsm = "";
                                t->bs->editedFx = "";
                                if(vs_2[i]->newShader)
                                    vs_2[i]->newShader->Release();
                                vs_2[i]->newShader = 0;
                                lst.remove(t);
                                break;
                            }
                        }
                    }

                    sprintf(buf100, "Edit VS FXC asm ##%i_%i", i, vs_2[i]->id);
                    if(ImGui::Button(buf100)) {
                        bool isInEditor = false;
                        sprintf(buf100, "%s#FXCASM##%i_%i", nameasm.c_str(), i, vs_2[i]->id);
                        std::string name = buf100;
                        for(auto t : lst) {
                            basicShader* bs = static_cast<basicShader*> (vs_2[i]);
                            if(bs && t->bs == bs && t->name == name) {
                                isInEditor = true;
                                t->show = true;
                                showEditorWindow = true;
                            }
                        }
                        if(isInEditor == false) {
                            //if(vs_2[i]->editedAsm.length() < 3) {
                            //	vs_2[i]->editedAsm = vs_2[i]->GetAsm();
                            //}
                            stShaderEditor* s = new stShaderEditor(name, VS_ASM, new TextEditor(), vs_2[i]);
                            s->editor->SetShowWhitespaces(0);
                            s->editor->SetText(s->bs->GetAsm());
                            lst.push_back(s);
                            showEditorWindow = true;
                        }
                    }
                    sprintf(buf100, "Edit VS Loaded asm ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->loadedAsm.length() > 0 && ImGui::Button(buf100)) {
                        bool isInEditor = false;
                        sprintf(buf100, "%s#LASM##%i_%i", nameasm.c_str(), i, vs_2[i]->id);
                        std::string name = buf100;
                        for(auto t : lst) {
                            basicShader* bs = static_cast<basicShader*> (vs_2[i]);
                            if(bs && t->bs == bs && t->name == name) {
                                isInEditor = true;
                                t->show = true;
                                showEditorWindow = true;
                            }
                        }
                        if(isInEditor == false) {
                            //if(vs_2[i]->editedAsm.length() < 3) {
                            //	vs_2[i]->editedAsm = vs_2[i]->loadedAsm;
                            //}
                            stShaderEditor* s = new stShaderEditor(name, VS_ASM, new TextEditor(), vs_2[i]);
                            s->editor->SetShowWhitespaces(0);
                            s->editor->SetText(s->bs->loadedAsm);
                            lst.push_back(s);
                            showEditorWindow = true;
                        }
                    }
                    sprintf(buf100, "Edit VS Loaded hlsl ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->loadedFx.length() > 0 && ImGui::Button(buf100)) {
                        bool isInEditor = false;
                        sprintf(buf100, "%s#LFX##%i_%i", nameasm.c_str(), i, vs_2[i]->id);
                        std::string name = buf100;
                        for(auto t : lst) {
                            basicShader* bs = static_cast<basicShader*> (vs_2[i]);
                            if(bs && t->bs == bs && t->name == name) {
                                isInEditor = true;
                                t->show = true;
                                showEditorWindow = true;
                            }
                        }
                        if(isInEditor == false) {
                            //if(vs_2[i]->editedFx.length() < 3) {
                            //	vs_2[i]->editedFx = vs_2[i]->loadedFx;
                            //}
                            stShaderEditor* s = new stShaderEditor(name, VS_FX, new TextEditor(), vs_2[i]);
                            s->editor->SetShowWhitespaces(0);
                            s->editor->SetText(s->bs->loadedFx);
                            lst.push_back(s);
                            showEditorWindow = true;
                        }
                    }
                    sprintf(buf100, "Edit VS Edited hlsl ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->editedFx.length() > 0 && ImGui::Button(buf100)) {
                        bool isInEditor = false;
                        sprintf(buf100, "%s#EFX##%i_%i", nameasm.c_str(), i, vs_2[i]->id);
                        std::string name = buf100;
                        for(auto t : lst) {
                            basicShader* bs = static_cast<basicShader*> (vs_2[i]);
                            if(bs && t->bs == bs && t->name == name) {
                                isInEditor = true;
                                t->show = true;
                                showEditorWindow = true;
                            }
                        }
                        if(isInEditor == false) {
                            //if(vs_2[i]->editedFx.length() < 3) {
                            //	vs_2[i]->editedFx = vs_2[i]->loadedFx;
                            //}
                            stShaderEditor* s = new stShaderEditor(name, VS_FX, new TextEditor(), vs_2[i]);
                            s->editor->SetShowWhitespaces(0);
                            s->editor->SetText(s->bs->editedFx);
                            lst.push_back(s);
                            showEditorWindow = true;
                        }
                    }

                    if(!vs_2[i]->newShader)
                        vs_2[i]->useNewShader = false;
                    else {
                        sprintf(buf100, "Use New VS Shader ##%i_%i", i, vs_2[i]->id);
                        ImGui::Checkbox(buf100, &vs_2[i]->useNewShader);
                    }
                    static int e = 0;
                    ImGui::RadioButton("FXC", (int*) &vs_2[i]->usingShader, 0); ImGui::SameLine();
                    if(vs_2[i]->compiledShaders[SU_LASM]) { ImGui::RadioButton("Loaded ASM", (int*) &vs_2[i]->usingShader, 1); ImGui::SameLine(); }
                    if(vs_2[i]->compiledShaders[SU_EASM]) { ImGui::RadioButton("Edited ASM", (int*) &vs_2[i]->usingShader, 2); }
                    if(vs_2[i]->compiledShaders[SU_LFX]) { ImGui::RadioButton("Loaded HLSL", (int*) &vs_2[i]->usingShader, 3); ImGui::SameLine(); }
                    if(vs_2[i]->compiledShaders[SU_EFX]) { ImGui::RadioButton("Edited HLSL", (int*) &vs_2[i]->usingShader, 4); ImGui::SameLine(); }

                    ImGui::Text("");

                    //sprintf(buf100, "Overwrite Depth ##%i_%i", i, vs_2[i]->id);
                    //ImGui::Checkbox(buf100, &vs_2[i]->overwriteDepth);
                    //sprintf(buf100, "Depth write ##%i_%i", i, vs_2[i]->id);
                    //ImGui::Checkbox(buf100, &vs_2[i]->depthWrite);

                    sprintf(buf100, "Original VS ASM ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->fxcAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                        sprintf(buf100, "VS2 fxcAsm ##%i_%i", i, vs_2[i]->id);
                        ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                        ImGui::TextUnformatted(vs_2[i]->fxcAsm.c_str());
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    sprintf(buf100, "Loaded VS ASM ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->loadedAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                        sprintf(buf100, "VS2 loadedAsm ##%i_%i", i, vs_2[i]->id);
                        ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                        ImGui::TextUnformatted(vs_2[i]->loadedAsm.c_str());
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    sprintf(buf100, "Edited VS ASM ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->editedAsm.length() > 1 && ImGui::TreeNode(buf100)) {
                        sprintf(buf100, "VS2 editedAsm ##%i_%i", i, vs_2[i]->id);
                        ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                        ImGui::TextUnformatted(vs_2[i]->editedAsm.c_str());
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    sprintf(buf100, "Loaded VS HLSL ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->loadedFx.length() > 1 && ImGui::TreeNode(buf100)) {
                        sprintf(buf100, "VS2 loadedFx ##%i_%i", i, vs_2[i]->id);
                        ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                        ImGui::TextUnformatted(vs_2[i]->loadedFx.c_str());
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    sprintf(buf100, "Edited VS HLSL ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) {
                        sprintf(buf100, "VS2 editedFx ##%i_%i", i, vs_2[i]->id);
                        ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);
                        ImGui::TextUnformatted(vs_2[i]->editedFx.c_str());
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }

                    sprintf(buf100, "Last VS Constants ##%i_%i", i, vs_2[i]->id);
                    if(ImGui::TreeNode(buf100)) {
                        sprintf(buf100, "Last VS2 Constants ##%i_%i", i, vs_2[i]->id);
                        ImGui::BeginChild(buf100, ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 0.5f), false, ImGuiWindowFlags_HorizontalScrollbar);

                        //for(int c = 0; c < 255; c++) {
                        //    ImGui::Text("c%i, %f, %f, %f, %f", c, vs_2[i]->constants[c][0], vs_2[i]->constants[c][1], vs_2[i]->constants[c][2], vs_2[i]->constants[c][3]);
                        //}
                        auto& cntnt = (vs_2[i]->usingShader != SU_FXC && vs_2[i]->compiledShaders[vs_2[i]->usingShader] != nullptr) ? static_cast<m_IDirect3DVertexShader9*>(vs_2[i]->compiledShaders[vs_2[i]->usingShader])->constants : vs_2[i]->constants;
                        for(int c = 0; c < 255; c++) {
                            ImGui::Text("c%i, %f, %f, %f, %f", c, cntnt[c][0], cntnt[c][1], cntnt[c][2], cntnt[c][3]);
                        }

                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndChild();
    }

    if(ImGui::CollapsingHeader("Debug editor")) {
        ImGui::ColorEdit4("BgColor", (float*)&bgColor);

        ImGui::BeginChild("ChildDE", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 2.f / 3.f), false, ImGuiWindowFlags_HorizontalScrollbar);
        int i = 0;
        for(auto t : lst) {
            if(t) {
                //t->bs->editedAsm = t->editor->GetText();
                char c[] = "Pixel Shader2 000000000000";
                sprintf(c, "Pixel Shader2 %i", i);
                if(ImGui::TreeNode(c)) {
                    char c4[] = "show 2222";
                    sprintf(c4, "Show %i", i);
                    ImGui::Text("PS2 %i", i);
                    ImGui::Checkbox(c4, &t->show);
                    if(ImGui::TreeNode(t->name.c_str())) {
                        ImGui::TextUnformatted(t->editor->GetText().c_str());
                        ImGui::TreePop();
                    }
                    static char c1[200] = { 0 };
                    sprintf(c1, "New ASM %s##%i", t->name.c_str(), t->bs->id);
                    if(ImGui::TreeNode(c1)) {
                        ImGui::TextUnformatted(t->bs->editedAsm.c_str());
                        ImGui::TreePop();
                    }
                    sprintf(c1, "New FX %s##%i", t->name.c_str(), t->bs->id);
                    if(ImGui::TreeNode(c1)) {
                        ImGui::TextUnformatted(t->bs->editedFx.c_str());
                        ImGui::TreePop();
                    }
                    //for(int c = 0; c < 255; c++) {
                    //	ImGui::Text("c%i, %f, %f, %f, %f", c, ps_2[i]->constants[c][0], ps_2[i]->constants[c][1], ps_2[i]->constants[c][2], ps_2[i]->constants[c][3]);
                    //}
                    ImGui::TreePop();
                }
            }
            i++;
        }
        ImGui::EndChild();
    }

    if(bDisplayFPSCounter && ImGui::CollapsingHeader("address debug")) {
        ImGui::Checkbox("Show Demo Window", &showDemoWindow);

        if(ImGui::Button("exportShaderConstants##ps")) {
            {
                FILE* f = 0;
                f = fopen("gViewInverse.txt", "w");
                if(f) {
                    for(int i = 0; i < (int) fx_ps.size(); i++) {
                        for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                            if(fx_ps[i][j] && fx_ps[i][j]->used > 100) {
                                for(int k = 0; k < gViewInverse.size(); k++) {
                                    if(gViewInverse[k].id == fx_ps[i][j]->id && gViewInverse[k].vs == 0) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "0, %i, %i, %s\n", gViewInverse[k].id, gViewInverse[k].constant, gViewInverse[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    for(int i = 0; i < (int) fx_vs.size(); i++) {
                        for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                            if(fx_vs[i][j] && fx_vs[i][j]->used > 100) {
                                for(int k = 0; k < gViewInverse.size(); k++) {
                                    if(gViewInverse[k].id == fx_vs[i][j]->id && gViewInverse[k].vs == 1) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "1, %i, %i, %s\n", gViewInverse[k].id, gViewInverse[k].constant, gViewInverse[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    fclose(f);
                }
            }
            {
                FILE* f = 0;
                f = fopen("gWorld.txt", "w");
                if(f) {
                    for(int i = 0; i < (int) fx_ps.size(); i++) {
                        for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                            if(fx_ps[i][j] && fx_ps[i][j]->used > 100) {
                                for(int k = 0; k < gWorld.size(); k++) {
                                    if(gWorld[k].id == fx_ps[i][j]->id && gWorld[k].vs == 0) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "0, %i, %i, %s\n", gWorld[k].id, gWorld[k].constant, gWorld[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    for(int i = 0; i < (int) fx_vs.size(); i++) {
                        for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                            if(fx_vs[i][j] && fx_vs[i][j]->used > 100) {
                                for(int k = 0; k < gWorld.size(); k++) {
                                    if(gWorld[k].id == fx_vs[i][j]->id && gWorld[k].vs == 1) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "1, %i, %i, %s\n", gWorld[k].id, gWorld[k].constant, gWorld[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    fclose(f);
                }
            }
            {
                FILE* f = 0;
                f = fopen("gWorldView.txt", "w");
                if(f) {
                    for(int i = 0; i < (int) fx_ps.size(); i++) {
                        for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                            if(fx_ps[i][j] && fx_ps[i][j]->used > 100) {
                                for(int k = 0; k < gWorldView.size(); k++) {
                                    if(gWorldView[k].id == fx_ps[i][j]->id && gWorldView[k].vs == 0) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "0, %i, %i, %s\n", gWorldView[k].id, gWorldView[k].constant, gWorldView[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    for(int i = 0; i < (int) fx_vs.size(); i++) {
                        for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                            if(fx_vs[i][j] && fx_vs[i][j]->used > 100) {
                                for(int k = 0; k < gWorldView.size(); k++) {
                                    if(gWorldView[k].id == fx_vs[i][j]->id && gWorldView[k].vs == 1) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "1, %i, %i, %s\n", gWorldView[k].id, gWorldView[k].constant, gWorldView[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    fclose(f);
                }
            }
            {
                FILE* f = 0;
                f = fopen("gWorldViewProj.txt", "w");
                if(f) {
                    for(int i = 0; i < (int) fx_ps.size(); i++) {
                        for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                            if(fx_ps[i][j] && fx_ps[i][j]->used > 100) {
                                for(int k = 0; k < gWorldViewProj.size(); k++) {
                                    if(gWorldViewProj[k].id == fx_ps[i][j]->id && gWorldViewProj[k].vs == 0) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "0, %i, %i, %s\n", gWorldViewProj[k].id, gWorldViewProj[k].constant, gWorldViewProj[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    for(int i = 0; i < (int) fx_vs.size(); i++) {
                        for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                            if(fx_vs[i][j] && fx_vs[i][j]->used > 100) {
                                for(int k = 0; k < gWorldViewProj.size(); k++) {
                                    if(gWorldViewProj[k].id == fx_vs[i][j]->id && gWorldViewProj[k].vs == 1) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "1, %i, %i, %s\n", gWorldViewProj[k].id, gWorldViewProj[k].constant, gWorldViewProj[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    fclose(f);
                }
            }
            {
                FILE* f = 0;
                f = fopen("gShadowMatrix.txt", "w");
                if(f) {
                    for(int i = 0; i < (int) fx_ps.size(); i++) {
                        for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                            if(fx_ps[i][j] && fx_ps[i][j]->used > 100) {
                                for(int k = 0; k < gShadowMatrix.size(); k++) {
                                    if(gShadowMatrix[k].id == fx_ps[i][j]->id && gShadowMatrix[k].vs == 0) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "0, %i, %i, %s\n", gShadowMatrix[k].id, gShadowMatrix[k].constant, gShadowMatrix[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    for(int i = 0; i < (int) fx_vs.size(); i++) {
                        for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                            if(fx_vs[i][j] && fx_vs[i][j]->used > 100) {
                                for(int k = 0; k < gShadowMatrix.size(); k++) {
                                    if(gShadowMatrix[k].id == fx_vs[i][j]->id && gShadowMatrix[k].vs == 1) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "1, %i, %i, %s\n", gShadowMatrix[k].id, gShadowMatrix[k].constant, gShadowMatrix[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    fclose(f);
                }
            }
            {
                FILE* f = 0;
                f = fopen("sunDirection.txt", "w");
                if(f) {
                    for(int i = 0; i < (int) fx_ps.size(); i++) {
                        for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                            if(fx_ps[i][j] && fx_ps[i][j]->used > 100) {
                                for(int k = 0; k < sunDirection.size(); k++) {
                                    if(sunDirection[k].id == fx_ps[i][j]->id && sunDirection[k].vs == 0) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "0, %i, %i, %s\n", sunDirection[k].id, sunDirection[k].constant, sunDirection[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    for(int i = 0; i < (int) fx_vs.size(); i++) {
                        for(int j = 0; j < (int) fx_vs[i].size(); j++) {
                            if(fx_vs[i][j] && fx_vs[i][j]->used > 100) {
                                for(int k = 0; k < sunDirection.size(); k++) {
                                    if(sunDirection[k].id == fx_vs[i][j]->id && sunDirection[k].vs == 1) {
                                        char buff[1024] = { 0 };
                                        sprintf(buff, "1, %i, %i, %s\n", sunDirection[k].id, sunDirection[k].constant, sunDirection[k].file.c_str());
                                        fwrite(buff, 1, strlen(buff), f);
                                    }
                                }
                            }
                        }
                    }
                    fclose(f);
                }
            }
        }

        if(ImGui::Button("exportShaderBin##ps")) {
            for(int i = 0; i < (int) fx_ps.size(); i++) {
                for(int j = 0; j < (int) fx_ps[i].size(); j++) {
                    if(fx_ps[i][j]) {
                        std::vector<BYTE*> data;
                        UINT len = 0;
                        fx_ps[i][j]->GetFunction(0, &len);
                        if(len > 0) {
                            data.resize(len);
                            fx_ps[i][j]->GetFunction(&data[0], &len);
                            char filename[1024] = { 0 };
                            char dir[1024] = { 0 };

                            sprintf(dir, "mkdir shaderBin\\%s\\", fx_ps[i][j]->fxName.c_str());
                            sprintf(filename, "shaderBin\\%s\\%s.bin", fx_ps[i][j]->fxName.c_str(), fx_ps[i][j]->oName.c_str());
                            //system(dir);
                            //CreateDirectoryA(dir, 0);
                            FILE* f = 0;
                            f = fopen(filename, "wb");
                            if(f) {
                                fwrite(&data[0], 1, len, f);
                                fclose(f);
                            }
                        }
                    }
                }
            }
            for(int i = 0; i < (int) ps_2.size(); i++) {
                if(ps_2[i]) {
                    std::vector<BYTE*> data;
                    UINT len = 0;
                    ps_2[i]->GetFunction(0, &len);
                    if(len > 0) {
                        data.resize(len);
                        ps_2[i]->GetFunction(&data[0], &len);
                        char filename[1024] = { 0 };
                        char dir[1024] = { 0 };

                        sprintf(dir, "mkdir shaderBin\\%s\\", ps_2[i]->fxName.c_str());
                        sprintf(filename, "shaderBin\\%s\\%s.bin", ps_2[i]->fxName.c_str(), ps_2[i]->oName.c_str());
                        //system(dir);
                        //CreateDirectoryA(dir, 0);
                        FILE* f = 0;
                        f = fopen(filename, "wb");
                        if(f) {
                            fwrite(&data[0], 1, len, f);
                            fclose(f);
                        }
                    }
                }

            }

        }

        static int cnt1 = 0;
        static int cnt2 = 0;
        if(ImGui::Button("Gen Shader crc")) {
            int version = 0;
            Utils::GetGameVersion(version);

            cnt1 = GenShaderList_ps(version);
            cnt2 = GenShaderList_vs(version);
        }
        ImGui::Text("shader cont = %i, %i", cnt1, cnt2);

        static int cnt3 = 0;
        static int cnt4 = 0;
        if(ImGui::Button("Load Shader crc")) {
            int version = 0;
            Utils::GetGameVersion(version);

            cnt3 = read_crc_name_ps(version);
            cnt4 = read_crc_name_vs(version);
        }
        ImGui::Text("shader cont = %i, %i", cnt3, cnt4);

        if(GameVersion == 1080 || GameVersion == 1070) {
            ImGui::SliderFloat("Shadow Distance BA + 0xB3E194 ", (float*) (baseAddress + 0xB3E194), 0, 1000);
        }
        if(GameVersion == 1200) {
            ImGui::DragFloat4("Shadow Distance L BA + 0xC36960 ", (float*) (baseAddress + (0xC36960-3*4)), 1, 0, 2500, "%.3f", ImGuiSliderFlags_Logarithmic);
            ImGui::DragFloat4("Shadow Distance M BA + 0xC36974 ", (float*) (baseAddress + (0xC36974-3*4)), 1, 0, 2500, "%.3f", ImGuiSliderFlags_Logarithmic);
            ImGui::DragFloat4("Shadow Distance H BA + 0xC36988 ", (float*) (baseAddress + (0xC36988-3*4)), 1, 0, 2500, "%.3f", ImGuiSliderFlags_Logarithmic);
            ImGui::DragFloat4("Shadow Distance VH BA + 0xC3699C", (float*) (baseAddress + (0xC3699C-3*4)), 1, 0, 2500, "%.3f", ImGuiSliderFlags_Logarithmic);
            ImGui::Checkbox("isininterior baseAddress + 0x1320FA8", IsInInterior);
            ImGui::Checkbox("inpausemenu baseAddress + C30B7C", IsGameOnMenu);
            ImGui::Checkbox("isgamepaused baseAddress + D73590", (bool*) (baseAddress + 0xD73590));
            ImGui::Checkbox(" 0019BB5C			", (bool*) (0x0019BB5C));
            ImGui::Checkbox(" 0019BFAC			", (bool*) (0x0019BFAC));
            ImGui::Checkbox(" 0019BFC0			", (bool*) (0x0019BFC0));
            ImGui::Checkbox(" 0019C108			", (bool*) (0x0019C108));
            ImGui::Checkbox(" 0019C138			", (bool*) (0x0019C138));
            ImGui::Checkbox(" 0019C13C			", (bool*) (0x0019C13C));
            ImGui::Checkbox(" 0019C164			", (bool*) (0x0019C164));
            ImGui::Checkbox(" 0019C168			", (bool*) (0x0019C168));
            ImGui::Checkbox(" 0019C1BC			", (bool*) (0x0019C1BC));
            ImGui::Checkbox(" 0019C1E0			", (bool*) (0x0019C1E0));
            ImGui::Checkbox(" baseAddress + C344B4", (bool*) (baseAddress + 0xC344B4));
            ImGui::Checkbox(" baseAddress + D60B84", (bool*) (baseAddress + 0xD60B84));
            ImGui::Checkbox(" baseAddress + D60C34", (bool*) (baseAddress + 0xD60C34));
            ImGui::Checkbox(" baseAddress + D7354C", (bool*) (baseAddress + 0xD7354C));
            ImGui::Checkbox(" baseAddress + D73554", (bool*) (baseAddress + 0xD73554));
            //ImGui::Checkbox(" baseAddress + D73610", (bool*) ( baseAddress + 0xD73610		) );
            //ImGui::Checkbox(" baseAddress + E845C8", (bool*) ( baseAddress + 0xE845C8		) );
            //ImGui::Checkbox(" 0B3D41C4			", (bool*) ( 0x0B3D41C4			) );
            //ImGui::Checkbox(" 0BC3ED00			", (bool*) ( 0x0BC3ED00			) );
            //
            // baseAddress + 0xD60EB0
            // baseAddress + 0x14B6DEC


            ImGui::Checkbox("game baseAddress + 0x0C6B420", (bool*) (baseAddress + 0x0C6B420));
            ImGui::Checkbox("game baseAddress + 0x0C6B428", (bool*) (baseAddress + 0x0C6B428));
            ImGui::Checkbox("game baseAddress + 0x13ED9C4", (bool*) (baseAddress + 0x13ED9C4));
            ImGui::Checkbox("game baseAddress + 0x13ED9E0", (bool*) (baseAddress + 0x13ED9E0));
            ImGui::Checkbox("game baseAddress + 0x13F5860", (bool*) (baseAddress + 0x13F5860));
            ImGui::Checkbox("game baseAddress + 0x13F5870", (bool*) (baseAddress + 0x13F5870));


            //
            ImGui::SliderInt("Reflex Quality BA + 0xD612BC ", (int*) (baseAddress + 0xD612BC), 0, 100);

            ImGui::SliderInt("water Quality BA + 0xD612C0 ", (int*) (baseAddress + 0xD612C0), 0, 100);

            ImGui::SliderInt("Shadow Quality BA + 0xD612B8 ", (int*) (baseAddress + 0xD612B8), 0, 100);

            ImGui::SliderInt("Night Shadow BA + 0xD612B4 ", (int*) (baseAddress + 0xD612B4), 0, 100);

            ImGui::SliderInt("Aniso Quality BA + 0xD6129C ", (int*) (baseAddress + 0xD6129C), 0, 100);

            ImGui::SliderInt("View distance BA + 0x0D612A0 ", (int*) (baseAddress + 0x0D612A0), 0, 100);
            //ImGui::SliderInt("View distance BA + 0x13ADB10 ", (int*) (baseAddress + 0x13ADB10), 0, 100);

            ImGui::SliderInt("Detail distance BA + 0xD612A4 ", (int*) (baseAddress + 0xD612A4), 0, 100);

            ImGui::SliderInt("Vehicle density BA + 0xD612A8 ", (int*) (baseAddress + 0xD612A8), 0, 100);

            ImGui::SliderInt("Reflex Quality BA + 0xD60EB0 ", (int*) (baseAddress + 0xD60EB0), 0, 100);
            ImGui::SliderInt("water Quality BA +  0xD60EB4 ", (int*) (baseAddress + 0xD60EB4), 0, 100);
            ImGui::SliderInt("Shadow Quality BA + 0xD60EAC ", (int*) (baseAddress + 0xD60EAC), 0, 100);
            ImGui::SliderInt("Night Shadow BA +   0xD60EA8 ", (int*) (baseAddress + 0xD60EA8), 0, 100);
            ImGui::SliderInt("Aniso Quality BA +  0xD60E90 ", (int*) (baseAddress + 0xD60E90), 0, 100);
            ImGui::SliderInt("View distance BA +  0xD60E94 ", (int*) (baseAddress + 0xD60E94), 0, 100);
            ImGui::SliderInt("Detail distance BA +0xD60E98 ", (int*) (baseAddress + 0xD60E98), 0, 100);
            ImGui::SliderInt("Vehicle density BA +0xD60E9C ", (int*) (baseAddress + 0xD60E9C), 0, 100);
        }
    }

    ImGui::BeginChild("##ChildDummy", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::EndChild();

    ImGui::End();

    if(showEditorWindow) {
        ImGui::SetNextWindowCollapsed(!mShowEditor);
        ImGui::Begin("Shader Editor", &showEditorWindow /*&mShowEditor*/, /*ImGuiWindowFlags_HorizontalScrollbar |*/ ImGuiWindowFlags_MenuBar);
        mShowEditor = !ImGui::IsWindowCollapsed();
        if(mEditorPos.x == 0.f)
            mEditorPos.x = 600.f;
        if(mEditorSize.x == 0.f || mEditorSize.y == 0.f)
            mEditorSize = ImVec2(600, 600);
        ImGui::SetWindowPos(mEditorPos, ImGuiCond_FirstUseEver);
        ImGui::SetWindowSize(mEditorSize, ImGuiCond_FirstUseEver);
        mEditorPos = ImGui::GetWindowPos();
        ImVec2 EditorSize = ImGui::GetWindowSize();
        if(EditorSize.x > 100 && EditorSize.y > 50)
            mEditorSize = EditorSize;
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_TabListPopupButton;
        if(ImGui::BeginTabBar("##MyTabBar", tab_bar_flags)) {
            int i = 0;
            for(auto t : lst) {
                TextEditor& editor = *t->editor;
                char c[100] = {};
                sprintf(c, "%s##%i", t->name.c_str(), i);
                if(ImGui::BeginTabItem(c, &t->show)) {
                    if(ImGui::BeginMenuBar()) {
                        if(ImGui::BeginMenu("File")) {
                            if(ImGui::MenuItem("Save edited from text editor")) {
                                switch(t->shaderType) {
                                    case	PS_ASM:
                                    case	VS_ASM:
                                        SaveASM(t->bs->fxName, t->name, t->editor->GetText(), t->shaderType);
                                        break;
                                    case	PS_FX:
                                    case	VS_FX:
                                        SaveFX(t->bs->fxName, t->name, t->editor->GetText(), t->shaderType);
                                        break;
                                    default:
                                        Log::Error("Unknow shader type to save:" + t->name);
                                        //Log::Error(t->name);
                                        break;
                                }
                                //std::string textToSave = t->editor->GetText();
                                //std::string dir1 = "shaders\\";
                                //std::string dir2 = dir1 + t->bs->fxName + "\\";
                                //std::string name = dir2 + t->name;
                                //std::string mk = std::string("mkdir ") + dir2;
                                //system(mk.c_str());
                                //FILE* file = fopen(name.c_str(), "w");
                                //if(file) {
                                //	fwrite(textToSave.data(), 1, textToSave.length(), file);
                                //	fclose(file);
                                //	Log::Info("File saved:");
                                //	Log::Info(name);
                                //}
                                //else {
                                //	Log::Error("File not saved");
                                //	Log::Error(name);
                                //}
                            }
                            if(ImGui::MenuItem("Save loaded shader")) {
                                switch(t->shaderType) {
                                    case	PS_ASM:
                                    case	VS_ASM:
                                        if(t->bs->loadedAsm.length() > 1) {
                                            SaveASM(t->bs->fxName, t->name, t->bs->loadedAsm, t->shaderType);
                                        }
                                        break;
                                    case	PS_FX:
                                    case	VS_FX:
                                        if(t->bs->loadedFx.length() > 1) {
                                            SaveFX(t->bs->fxName, t->name, t->bs->loadedFx, t->shaderType);
                                            break;
                                        }
                                    default:
                                        Log::Error("Unknow shader type to save:" + t->name);
                                        //Log::Error(t->name);
                                        break;
                                }
                            }
                            if(ImGui::MenuItem("Save original from fxc file")) {
                                switch(t->shaderType) {
                                    case	PS_ASM:
                                    case	VS_ASM:
                                        SaveASM(t->bs->fxName, t->name, t->bs->GetAsm(), t->shaderType);
                                        break;
                                    case	PS_FX:
                                    case	VS_FX:
                                        if(t->bs->loadedFx.length() > 1) {
                                            SaveFX(t->bs->fxName, t->name, t->bs->loadedFx, t->shaderType);
                                        }
                                        break;
                                    default:
                                        Log::Error("Unknow shader type to save:" + t->name);
                                        //Log::Error(t->name);
                                        break;
                                }
                                //	std::string textToSave = t->bs->GetAsm();
                                //	std::string dir1 = "shaders\\";
                                //	std::string dir2 = dir1 + t->bs->fxName + "\\";
                                //	std::string name = dir2 + t->name;
                                //	std::string mk = std::string("mkdir ") + dir2;
                                //	system(mk.c_str());
                                //	FILE* file = fopen(name.c_str(), "w");
                                //	if(file) {
                                //		fwrite(textToSave.data(), 1, textToSave.length(), file);
                                //		fclose(file);
                                //		Log::Info("File saved:");
                                //		Log::Info(name);
                                //	}
                                //	else {
                                //		Log::Error("File not saved");
                                //		Log::Error(name);
                                //	}
                            }

                            if(ImGui::MenuItem("Reload from file")) {
                                switch(t->shaderType) {
                                    case	PS_ASM:
                                    case	VS_ASM:
                                        Log::Info(t->name);
                                        //SaveASM(t->bs->fxName, t->name, t->bs->GetAsm(), t->shaderType);
                                        break;
                                    case	PS_FX:
                                    case	VS_FX:
                                        Log::Info(t->name);
                                        break;
                                    default:
                                        Log::Error("Unknow shader type to save:" + t->name);
                                        //Log::Error(t->name);
                                        break;
                                }
                                //	std::string textToSave = t->bs->GetAsm();
                                //	std::string dir1 = "shaders\\";
                                //	std::string dir2 = dir1 + t->bs->fxName + "\\";
                                //	std::string name = dir2 + t->name;
                                //	std::string mk = std::string("mkdir ") + dir2;
                                //	system(mk.c_str());
                                //	FILE* file = fopen(name.c_str(), "w");
                                //	if(file) {
                                //		fwrite(textToSave.data(), 1, textToSave.length(), file);
                                //		fclose(file);
                                //		Log::Info("File saved:");
                                //		Log::Info(name);
                                //	}
                                //	else {
                                //		Log::Error("File not saved");
                                //		Log::Error(name);
                                //	}
                            }

                            ImGui::EndMenu();
                        }
                        if(ImGui::BeginMenu("Edit")) {
                            bool ro = editor.IsReadOnly();
                            if(ImGui::MenuItem("Read-only mode", nullptr, &ro))
                                editor.SetReadOnly(ro);
                            ImGui::Separator();

                            if(ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
                                editor.Undo();
                            if(ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
                                editor.Redo();

                            ImGui::Separator();

                            if(ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
                                editor.Copy();
                            if(ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
                                editor.Cut();
                            if(ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
                                editor.Delete();
                            if(ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
                                editor.Paste();

                            ImGui::Separator();

                            if(ImGui::MenuItem("Select all", nullptr, nullptr))
                                editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

                            ImGui::EndMenu();
                        }
                        if(ImGui::BeginMenu("View")) {
                            if(ImGui::MenuItem("Dark palette"))
                                editor.SetPalette(TextEditor::GetDarkPalette());
                            if(ImGui::MenuItem("Light palette"))
                                editor.SetPalette(TextEditor::GetLightPalette());
                            if(ImGui::MenuItem("Retro blue palette"))
                                editor.SetPalette(TextEditor::GetRetroBluePalette());
                            ImGui::EndMenu();
                        }
                        if(ImGui::MenuItem("Compile", "F6", nullptr, true) || ImGui::IsKeyPressed(ImGuiKey_F6, false)) {
                            ShaderUse su = SU_EASM;
                            switch(t->shaderType) {
                                case PS_ASM:
                                case VS_ASM:
                                    t->bs->compileShaderSource(t->editor->GetText(), t->shaderType, SU_EASM);
                                    t->bs->editedAsm = t->editor->GetText();
                                    t->bs->usingShader = SU_EASM;
                                    break;
                                case PS_FX:
                                case VS_FX:
                                    t->bs->compileShaderSource(t->editor->GetText(), t->shaderType, SU_EFX);
                                    t->bs->editedFx = t->editor->GetText();
                                    t->bs->usingShader = SU_EFX;
                                    break;
                                default:
                                    Log::Error("Undefined shader type to compile!");
                                    break;
                            }
                        }
                        ImGui::EndMenuBar();
                    }
                    auto cpos = t->editor->GetCursorPosition();
                    t->editor->Render(t->name.c_str());
                    ImGui::EndTabItem();
                }
                i++;
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    if(showDemoWindow)
        ImGui::ShowDemoWindow(&showDemoWindow);

    if(mShowLogWindow) {
        if(mLogPos.y < 10.f) {
            mLogPos.y = gWindowHeight - 230.f;
        }
        if(mLogSize.y < 10.f) {
            mLogSize.y = 10.f;
        }
        if(mLogSize.x < 20.f) {
            mLogSize.x = 20.f;
        }
        if(mLogSize.x > float(gWindowWidth - 5)) {
            mLogSize.x = float(gWindowWidth - 10);
        }
        ImGui::SetNextWindowCollapsed(!bShowLogWindow);
        ImGui::Begin("##LOG Window", &mShowLogWindow, 0);
        bShowLogWindow = !ImGui::IsWindowCollapsed();
        ImGui::SetWindowPos(mLogPos, ImGuiCond_FirstUseEver);
        ImGui::SetWindowSize(mLogSize, ImGuiCond_FirstUseEver);

        ImGui::InputTextMultiline("##Log", (char*) Log::c_str(), Log::length(), ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly | ImGuiWindowFlags_HorizontalScrollbar);
        mLogPos = ImGui::GetWindowPos();
        ImVec2 LogSize = ImGui::GetWindowSize();
        if(LogSize.x > 100 && LogSize.y > 50)
            mLogSize = LogSize;

        if(Log::isDirt) {
            ImGui::SetScrollHereY(1.0f);
            ImGui::SetScrollY(1.0f);
            Log::isDirt = false;
        }
        ImGui::End();
    }
}

void ShadowResFix::DrawSettingsWindow() {
    if(mShowSettingsWindow) {
        ImGui::Begin("Settings", NULL, ImGuiWindowFlags_NoResize);
        {
            ImGui::SetWindowSize(ImVec2(330.0f, 445.0f));

            ImGui::DragFloat("Font Scale", &mFontScale, 0.001f, 0.1f, 2.0f);
            ImGui::DragFloat("Item Inner Spacing", &mItemInnerSpacing, 0.1f, 0.1f, 100.0f);
            ImGui::Text("Position");
            ImGui::DragFloat2("##Position", (float*) &mWindowPos, 1.0f, 0.0f, FLT_MAX);
            ImGui::Text("Size");
            ImGui::DragFloat2("##Size", (float*) &mWindowSize, 1.0f, 1.0f, FLT_MAX);

            ImGui::Checkbox("Show Editor", &mShowEditor);
            ImGui::Text("Position Editor");
            ImGui::DragFloat2("##PositionEditor", (float*) &mEditorPos, 1.0f, 0.0f, FLT_MAX);
            ImGui::Text("Size Editor");
            ImGui::DragFloat2("##SizeEditor", (float*) &mEditorSize, 1.0f, 1.0f, FLT_MAX);

            ImGui::Checkbox("Show Log Window", &bShowLogWindow);
            ImGui::Text("Position Log");
            ImGui::DragFloat2("##PositionLog", (float*) &mLogPos, 1.0f, 0.0f, FLT_MAX);
            ImGui::Text("Size Log");
            ImGui::DragFloat2("##SizeLog", (float*) &mLogSize, 1.0f, 1.0f, FLT_MAX);

            ImGui::GetIO().FontGlobalScale = mFontScale;
            ImGui::GetStyle().ItemInnerSpacing.x = mItemInnerSpacing;

            //open window key setting
            //ImGui::NewLine();

            static ImGuiKey newOpenWindowKey = mOpenWindowKey;
            static ImGuiKey newCompileShader = mCompileShader;

            std::string openEditorKeyStr = "Open Editor Window Key: " + std::string(ImGui::GetKeyName(newOpenWindowKey));
            ImGui::Text(openEditorKeyStr.c_str());
            static bool showChangeWindowKeyPrompt = false;
            if(ImGui::Button("Change")) {
                showChangeWindowKeyPrompt = true;
            }
            if(showChangeWindowKeyPrompt) {
                ImGui::TextWrapped("Press the Key You Want to Use to Open the Editor Window");
                for(uint32_t i = ImGuiKey_Tab; i < ImGuiKey_KeypadEqual; i++) {
                    if(ImGui::IsKeyPressed((ImGuiKey) i) && i != mToggleCameraControlKey) {
                        newOpenWindowKey = (ImGuiKey) i;
                        showChangeWindowKeyPrompt = false;
                        break;
                    }
                }
            }

            std::string CompileShaderStr = "Compile Shader Key: " + std::string(ImGui::GetKeyName(newCompileShader));
            ImGui::Text(CompileShaderStr.c_str());
            static bool showChangeCompileShaderKeyPrompt = false;
            if(ImGui::Button("Change")) {
                showChangeCompileShaderKeyPrompt = true;
            }
            if(showChangeCompileShaderKeyPrompt) {
                ImGui::TextWrapped("Press the Key You Want to Use to Compile Shader in editor");
                for(uint32_t i = ImGuiKey_Tab; i < ImGuiKey_KeypadEqual; i++) {
                    if(ImGui::IsKeyPressed((ImGuiKey) i) && i != mToggleCameraControlKey && i != mOpenWindowKey) {
                        newCompileShader = (ImGuiKey) i;
                        showChangeCompileShaderKeyPrompt = false;
                        break;
                    }
                }
            }

            //ImGui::NewLine();

            //toggle mouse control setting
            static ImGuiKey newCameraToggleKey = mToggleCameraControlKey;
            std::string toggleCameraKeyStr = "Toggle Camera Control Key: " + std::string(ImGui::GetKeyName(newCameraToggleKey));
            ImGui::Text(toggleCameraKeyStr.c_str());

            static bool showChangeCameraToggleKeyPrompt = false;
            if(ImGui::Button("Change##2")) {
                showChangeCameraToggleKeyPrompt = true;
            }

            if(showChangeCameraToggleKeyPrompt) {
                ImGui::TextWrapped("Press the Key You Want to Use to Toggle Camera Control");

                for(uint32_t i = ImGuiKey_Tab; i < ImGuiKey_KeypadEqual; i++) {
                    if(ImGui::IsKeyPressed((ImGuiKey) i) && i != mOpenWindowKey) {
                        newCameraToggleKey = (ImGuiKey) i;
                        showChangeCameraToggleKeyPrompt = false;
                        break;
                    }
                }
            }

            //ImGui::NewLine();

            if(ImGui::Button("Save")) {
                mOpenWindowKey = newOpenWindowKey;
                mToggleCameraControlKey = newCameraToggleKey;
                SaveSettings();
                mShowSettingsWindow = false;
                showChangeWindowKeyPrompt = false;
                showChangeCameraToggleKeyPrompt = false;
            }

            ImGui::SameLine();

            if(ImGui::Button("Cancel")) {
                newOpenWindowKey = mOpenWindowKey;
                newCameraToggleKey = mToggleCameraControlKey;
                LoadSettings();
                mShowSettingsWindow = false;
                showChangeWindowKeyPrompt = false;
                showChangeCameraToggleKeyPrompt = false;
            }
        }
        ImGui::End();
    }
}
