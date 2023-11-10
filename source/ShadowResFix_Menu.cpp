#include "ShadowResFix.h"

extern IDirect3DTexture9* pRainDropsRefractionHDRTex;
extern std::list<m_IDirect3DTexture9*> textureList;

extern bool bDisplayFPSCounter;
extern bool gFixRainDrops;
extern bool gTreeLeavesSwayInTheWind;
extern bool gNearFarPlane;

extern UINT gWindowWidth;
extern UINT gWindowHeight;

//bool bShowLogWindow = 1;

long long g_id = 0;

extern float shaderReturnColor[4] = { 0.f };

extern float DEPTHBIAS;
extern float SLOPESCALEDEPTHBIAS;

// name, src, editor

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

extern LPDIRECT3DPIXELSHADER9       ImGuiPS;
extern LPDIRECT3DVERTEXSHADER9      ImGuiVS;

ShadowResFix::ShadowResFix() {};
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

    if(ImGui::IsKeyPressed(mOpenWindowKey)) {
        mShowWindow = !mShowWindow;

        if(mShowWindow) {
            LoadSettings();
            ImGui::GetIO().FontGlobalScale = mFontScale;

            mDisableMouseControl = true;
            ImGui::GetIO().MouseDrawCursor = 1;
        }
        else {
            mDisableMouseControl = false;
            ImGui::GetIO().MouseDrawCursor = 0;
        }
    }

    if(ImGui::IsKeyPressed(ImGuiKey_F5)) {
        pauseGame = !pauseGame;
    }

    if(mShowWindow) {
        if(ImGui::IsKeyPressed(mToggleCameraControlKey)) {
            mDisableMouseControl = !mDisableMouseControl;
        }
    }
    else if(GameVersion == 1200 && baseAddress && IsGameOnMenu && IsGamePaused) {
        try {
            if(*IsGameOnMenu || pauseGame) {
                *IsGamePaused = true;
            }
            else {
                *IsGamePaused = false;
            }
        }
        catch(const std::exception&) {
        }
    }

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

void ShadowResFix::DrawMainWindow() {
    static float color[4] = { 0 };
    static ImVec4 texCol(0.5f, 0.5f, 0.5f, 1.0f);
    static bool checkBox = false;
    static int int4[4] = { 0 };
    ImGuiIO& io = ImGui::GetIO();

    static char buf100[100] = { 0 };
    static bool showUnused = false;
    static bool showUnusedTex = true;
    static bool hideEmptyFXC = true;
    static bool showDemoWindow = false;

    ImGui::Begin("Shader Editor 1.0.0.6", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

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
        ImGui::ColorEdit4("texCol", &texCol.x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoInputs);
        ImGui::Checkbox("Pause Game F5", &pauseGame);
        ImGui::EndMenuBar();
    }

    ImGui::Checkbox("Display FPS Counter", &bDisplayFPSCounter);
    ImGui::Checkbox("FixRainDrops", &gFixRainDrops);
    ImGui::Checkbox("TreeLeavesSwayInTheWind", &gTreeLeavesSwayInTheWind);
    ImGui::Checkbox("NearFarPlane", &gNearFarPlane);
    ImGui::Checkbox("Show Log Window", &bShowLogWindow);
    
    ImGui::Separator();
    if(ImGui::Button("Set all shaders to use original fxc")) {
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
        "Screen Size2",
        "gBuffer 2?",
        "Screen / 2",
        "Screen / 4",
        "Screen Size",
        "Screen",
        "gBuffer",
        "Mirror Tex",
        "reflex",
        "water reflex",
        "light atlas",
        "shadow atlas",
        "shadow cascade"
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
        for(auto& tname : nameList) {
            if(ImGui::TreeNode(tname.c_str())) {
                for(auto tex : textureList) {
                    if(tex && tex->name == tname && ((showUnusedTex == false && tex->useCounter > 0) || showUnusedTex == true)) {
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
                            ImGui::Image(tex, ImVec2(my_tex_w, my_tex_h), uv_min, uv_max, texCol, border_col);
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
                                ImGui::Image(tex, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, texCol, border_col);
                                ImGui::EndTooltip();
                            }
                        }
                        ImGui::SameLine();

                        ImGui::Text("Name: %s\nFormat: %s\nWidth: %i\nHeight: %i\nLevels: %i\nUsage: %i\nPool: %i\nUseCnt: %ui",
                                    tex->name.c_str(), tex->FormatName, tex->Width, tex->Height, tex->Levels, (UINT) tex->Usage, (UINT) tex->Pool, tex->useCounter);

                        //ImGui::Text("Name: %s", tex->name.c_str());
                        //ImGui::Text("Format: %s %x", tex->FormatName, (UINT) tex->Format);
                        //ImGui::Text("Width: %i", (UINT) tex->Width);
                        //ImGui::Text("Height: %i", (UINT) tex->Height);
                        //ImGui::Text("Levels: %i", (UINT) tex->Levels);
                        //ImGui::Text("Usage: %i", (UINT) tex->Usage);
                        //ImGui::Text("Pool: %i", (UINT) tex->Pool);
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();
        //}
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
                        sprintf(buf100, "%s #ps%i_%i_%i", fx_ps[i][j]->oName.c_str(), i, j, fx_ps[i][j]->id);
                        if(ImGui::TreeNode(buf100)) {
                            std::string namefx = fx_ps[i][j]->oName.substr(0, fx_ps[i][j]->oName.find_last_of(".")) + std::string(".fx");
                            std::string nameasm = fx_ps[i][j]->oName;

                            ImGui::Text("PS ID %i ##%i_%i_%i", fx_ps[i][j]->id, i, j, fx_ps[i][j]->id);
                            ImGui::Text("CRC_32:  %x ##vs%i_%i_%i", fx_ps[i][j]->id, i, j, fx_ps[i][j]->id);
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
                                }
                            }

                            sprintf(buf100, "Disable PS Shader ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            ImGui::Checkbox(buf100, &fx_ps[i][j]->disable);
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
                            if(fx_ps[i][j]->fxcAsm.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_ps[i][j]->fxcAsm.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Loaded PS ASM ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_ps[i][j]->loadedAsm.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Edited PS ASM ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_ps[i][j]->editedAsm.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Loaded PS HLSL ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->editedAsm.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_ps[i][j]->loadedFx.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Edited PS HLSL ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(fx_ps[i][j]->loadedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_ps[i][j]->editedFx.c_str()); ImGui::TreePop(); }

                            sprintf(buf100, "Last PS Constants ##%i_%i_%i", i, j, fx_ps[i][j]->id);
                            if(ImGui::TreeNode(buf100)) {
                                for(int c = 0; c < 255; c++) {
                                    ImGui::Text("c%i, %f, %f, %f, %f", c, fx_ps[i][j]->constants[c][0], fx_ps[i][j]->constants[c][1], fx_ps[i][j]->constants[c][2], fx_ps[i][j]->constants[c][3]);
                                }
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
                        sprintf(buf100, "%s #vs%i_%i_%i", fx_vs[i][j]->oName.c_str(), i, j, fx_vs[i][j]->id);
                        if(ImGui::TreeNode(buf100)) {
                            std::string namefx = fx_vs[i][j]->oName.substr(0, fx_vs[i][j]->oName.find_last_of(".")) + std::string(".fx");
                            std::string nameasm = fx_vs[i][j]->oName;

                            ImGui::Text("VS ID %i ##%i_%i_%i", fx_vs[i][j]->id, i, j, fx_vs[i][j]->id);
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
                                }
                            }

                            sprintf(buf100, "Disable VS Shader ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            ImGui::Checkbox(buf100, &fx_vs[i][j]->disable);
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

                            sprintf(buf100, "Overwrite Depth ##vs%i_%i_%i", i, j, fx_vs[i][j]->id);
                            ImGui::Checkbox(buf100, &fx_vs[i][j]->overwriteDepth);
                            sprintf(buf100, "Depth write ##vs%i_%i_%i", i, j, fx_vs[i][j]->id);
                            ImGui::Checkbox(buf100, &fx_vs[i][j]->depthWrite);

                            //sprintf(buf100, "Disable depth write ##vs%i_%i_%i", i, j, fx_vs[i][j]->id);
                            //ImGui::Checkbox(buf100, &fx_vs[i][j]->disableDepthWrite);

                            sprintf(buf100, "Original VS ASM ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->fxcAsm.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_vs[i][j]->fxcAsm.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Loaded VS ASM ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_vs[i][j]->loadedAsm.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Edited VS ASM ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_vs[i][j]->editedAsm.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Loaded VS HLSL ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->editedAsm.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_vs[i][j]->loadedFx.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Edited VS HLSL ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->loadedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_vs[i][j]->editedFx.c_str()); ImGui::TreePop(); }

                            sprintf(buf100, "Last VS Constants ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(ImGui::TreeNode(buf100)) {
                                for(int c = 0; c < 255; c++) {
                                    ImGui::Text("c%i, %f, %f, %f, %f", c, fx_vs[i][j]->constants[c][0], fx_vs[i][j]->constants[c][1], fx_vs[i][j]->constants[c][2], fx_vs[i][j]->constants[c][3]);
                                }
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
                
                    sprintf(buf100, "%s #ps2%i %i", ps_2[i]->oName.c_str(), i, ps_2[i]->id);
                    if(ImGui::TreeNode(buf100)) {
                        sprintf(buf100, "ps_%x.fx", ps_2[i]->crc32);
                        std::string namefx = buf100;
                        sprintf(buf100, "ps_%x.asm", ps_2[i]->crc32);
                        std::string nameasm = buf100;
                        //ps_2[i]->oName;

                        ImGui::Text("PS ID %i ##%i", ps_2[i]->id, i);
                        ImGui::Text("Use Counter: %i ##%i_%i", ps_2[i]->used, i, ps_2[i]->id);

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
                                }
                            }
                            if(isInEditor == false) {
                                //if(ps_2[i]->editedAsm.length() < 3) {
                                //	ps_2[i]->editedAsm = ps_2[i]->GetAsm();
                                //}
                                stShaderEditor* s = new stShaderEditor(name, PS_ASM, new TextEditor(), ps_2[i]);
                                s->editor->SetShowWhitespaces(0);
                                s->editor->SetText(s->bs->GetAsm());
                                lst.push_back(s);
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
                            }
                        }
                        sprintf(buf100, "Edit PS Loaded hlsl ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->loadedFx.length() > 0 && ImGui::Button(buf100)) {
                            bool isInEditor = false;
                            sprintf(buf100, "%s#LFX##%i_%i", nameasm.c_str(), i, ps_2[i]->id);
                            std::string name = buf100;
                            for(auto t : lst) {
                                basicShader* bs = static_cast<basicShader*> (ps_2[i]);
                                if(bs && t->bs == bs && t->name == name) {
                                    isInEditor = true;
                                    t->show = true;
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
                            }
                        }
                        sprintf(buf100, "Edit PS Edited hlsl ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->editedFx.length() > 0 && ImGui::Button(buf100)) {
                            bool isInEditor = false;
                            sprintf(buf100, "%s#EFX##%i_%i", nameasm.c_str(), i, ps_2[i]->id);
                            std::string name = buf100;
                            for(auto t : lst) {
                                basicShader* bs = static_cast<basicShader*> (ps_2[i]);
                                if(bs && t->bs == bs && t->name == name) {
                                    isInEditor = true;
                                    t->show = true;
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
                            }
                        }

                        sprintf(buf100, "Disable PS Shader ##%i_%i", i, ps_2[i]->id);
                        ImGui::Checkbox(buf100, &ps_2[i]->disable);
                        if(!ps_2[i]->newShader)
                            ps_2[i]->useNewShader = false;
                        else {
                            sprintf(buf100, "Use New PS Shader ##%i_%i", i, ps_2[i]->id);
                            ImGui::Checkbox(buf100, &ps_2[i]->useNewShader);
                        }
                        static int e = 0;
                        ImGui::RadioButton("FXC", (int*) &ps_2[i]->usingShader, 0); ImGui::SameLine();
                        if(ps_2[i]->compiledShaders[SU_LASM]) { ImGui::RadioButton("Loaded ASM", (int*) &ps_2[i]->usingShader, 1); ImGui::SameLine(); }
                        if(ps_2[i]->compiledShaders[SU_EASM]) { ImGui::RadioButton("Edited ASM", (int*) &ps_2[i]->usingShader, 2); }
                        if(ps_2[i]->compiledShaders[SU_LFX]) { ImGui::RadioButton("Loaded HLSL", (int*) &ps_2[i]->usingShader, 3); ImGui::SameLine(); }
                        if(ps_2[i]->compiledShaders[SU_EFX]) { ImGui::RadioButton("Edited HLSL", (int*) &ps_2[i]->usingShader, 4); ImGui::SameLine(); }

                        ImGui::Text("");

                        sprintf(buf100, "Overwrite Depth ##%i_%i", i, ps_2[i]->id);
                        ImGui::Checkbox(buf100, &ps_2[i]->overwriteDepth);
                        sprintf(buf100, "Depth write ##%i_%i", i, ps_2[i]->id);
                        ImGui::Checkbox(buf100, &ps_2[i]->depthWrite);

                        sprintf(buf100, "Original PS ASM ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->fxcAsm.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(ps_2[i]->fxcAsm.c_str()); ImGui::TreePop(); }
                        sprintf(buf100, "Loaded PS ASM ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(ps_2[i]->loadedAsm.c_str()); ImGui::TreePop(); }
                        sprintf(buf100, "Edited PS ASM ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(ps_2[i]->editedAsm.c_str()); ImGui::TreePop(); }
                        sprintf(buf100, "Loaded PS HLSL ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->editedAsm.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(ps_2[i]->loadedFx.c_str()); ImGui::TreePop(); }
                        sprintf(buf100, "Edited PS HLSL ##%i_%i", i, ps_2[i]->id);
                        if(ps_2[i]->loadedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(ps_2[i]->editedFx.c_str()); ImGui::TreePop(); }

                        sprintf(buf100, "Last PS Constants ##%i_%i", i, ps_2[i]->id);
                        if(ImGui::TreeNode(buf100)) {
                            for(int c = 0; c < 255; c++) {
                                ImGui::Text("c%i, %f, %f, %f, %f", c, ps_2[i]->constants[c][0], ps_2[i]->constants[c][1], ps_2[i]->constants[c][2], ps_2[i]->constants[c][3]);
                            }
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

                sprintf(buf100, "%s #vs2%i %i", vs_2[i]->oName.c_str(), i, vs_2[i]->id);
                if(ImGui::TreeNode(buf100)) {
                    sprintf(buf100, "vs_%x.fx", vs_2[i]->crc32);
                    std::string namefx = buf100;
                    sprintf(buf100, "vs_%x.asm", vs_2[i]->crc32);
                    std::string nameasm = buf100;
                    //vs_2[i]->oName;

                    ImGui::Text("VS ID %i ##%i", vs_2[i]->id, i);
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
                        }
                    }

                    sprintf(buf100, "Disable VS Shader ##%i_%i", i, vs_2[i]->id);
                    ImGui::Checkbox(buf100, &vs_2[i]->disable);
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

                    sprintf(buf100, "Overwrite Depth ##%i_%i", i, vs_2[i]->id);
                    ImGui::Checkbox(buf100, &vs_2[i]->overwriteDepth);
                    sprintf(buf100, "Depth write ##%i_%i", i, vs_2[i]->id);
                    ImGui::Checkbox(buf100, &vs_2[i]->depthWrite);

                    sprintf(buf100, "Original VS ASM ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->fxcAsm.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(vs_2[i]->fxcAsm.c_str()); ImGui::TreePop(); }
                    sprintf(buf100, "Loaded VS ASM ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(vs_2[i]->loadedAsm.c_str()); ImGui::TreePop(); }
                    sprintf(buf100, "Edited VS ASM ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(vs_2[i]->editedAsm.c_str()); ImGui::TreePop(); }
                    sprintf(buf100, "Loaded VS HLSL ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->editedAsm.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(vs_2[i]->loadedFx.c_str()); ImGui::TreePop(); }
                    sprintf(buf100, "Edited VS HLSL ##%i_%i", i, vs_2[i]->id);
                    if(vs_2[i]->loadedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(vs_2[i]->editedFx.c_str()); ImGui::TreePop(); }

                    sprintf(buf100, "Last VS Constants ##%i_%i", i, vs_2[i]->id);
                    if(ImGui::TreeNode(buf100)) {
                        for(int c = 0; c < 255; c++) {
                            ImGui::Text("c%i, %f, %f, %f, %f", c, vs_2[i]->constants[c][0], vs_2[i]->constants[c][1], vs_2[i]->constants[c][2], vs_2[i]->constants[c][3]);
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndChild();
    }

    if(ImGui::CollapsingHeader("Debug editor")) {
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
                        ImGui::Text(t->editor->GetText().c_str());
                        ImGui::TreePop();
                    }
                    static char c1[200] = { 0 };
                    sprintf(c1, "New ASM %s##%i", t->name.c_str(), t->bs->id);
                    if(ImGui::TreeNode(c1)) {
                        ImGui::Text(t->bs->editedAsm.c_str());
                        ImGui::TreePop();
                    }
                    sprintf(c1, "New FX %s##%i", t->name.c_str(), t->bs->id);
                    if(ImGui::TreeNode(c1)) {
                        ImGui::Text(t->bs->editedFx.c_str());
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

#ifdef ___nothing___
    if(ImGui::CollapsingHeader("Vertex shaders 2")) {
        ImGui::BeginChild("ChildVS2", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 2.f / 3.f), false, ImGuiWindowFlags_HorizontalScrollbar);
        for(int i = 0; i < vs_2.size(); i++) {
            m_IDirect3DVertexShader9* pShader2 = static_cast<m_IDirect3DVertexShader9*>(ImGuiVS);
            if(vs_2[i] && vs_2[i] != m_IDirect3DVertexShader9::dummyShader && vs_2[i] != pShader2) {
                sprintf(buf100, "Vertex Shader2 %i", i);
                if(ImGui::TreeNode(buf100)) {
                    ImGui::Text("VS %i", vs_2[i]->id);
                    if(vs_2[i] != vs_2[i]->dummyShader) {
                        sprintf(buf100, "Disable PS Shader2 ##%i_%i", i, vs_2[i]->id);
                        ImGui::Checkbox(buf100, &vs_2[i]->disable);
                    }
                    sprintf(buf100, "VS Assembly2 %i", i);
                    if(ImGui::TreeNode(buf100)) {
                        //char* as = (char*) vs[i]->GetAsm();
                        ImGui::Text("%s\n", vs_2[i]->fxcAsm.c_str());
                        ImGui::TreePop();
                    }
                    sprintf(buf100, "Last VS2 Constants ##%i", i);
                    if(ImGui::TreeNode(buf100)) {
                        for(int c = 0; c < 256; c++) {
                            ImGui::Text("c%i, %f, %f, %f, %f", c, vs_2[i]->constants[c][0], vs_2[i]->constants[c][1], vs_2[i]->constants[c][2], vs_2[i]->constants[c][3]);
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndChild();
    }

    if(ImGui::CollapsingHeader("Pixel shaders 2")) {
        ImGui::BeginChild("ChildPS2", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 2.f / 3.f), false, ImGuiWindowFlags_HorizontalScrollbar);
        for(int i = 0; i < ps_2.size(); i++) {
            m_IDirect3DPixelShader9* pShader2 = static_cast<m_IDirect3DPixelShader9*>(ImGuiPS);
            if(ps_2[i] && ps_2[i] != m_IDirect3DPixelShader9::dummyShader && ps_2[i] != pShader2) {
                sprintf(buf100, "Pixel Shader2 %i", i);
                if(ImGui::TreeNode(buf100)) {
                    //m_IDirect3DPixelShader9* pShader2 = static_cast<m_IDirect3DPixelShader9*>(ImGuiPS);
                    //if(ps_2[i] && ps_2[i]->GetProxyInterface() != m_IDirect3DPixelShader9::dummyShader && ps_2[i] != pShader2) {
                    ImGui::Text("PS %i", ps_2[i]->id);
                    if(ps_2[i] != ps_2[i]->dummyShader) {
                        sprintf(buf100, "Disable PS Shader2 ##%i_%i", i, ps_2[i]->id);
                        ImGui::Checkbox(buf100, &ps_2[i]->disable);
                    }
                    sprintf(buf100, "PS Assembly2 %i", i);
                    if(ImGui::TreeNode(buf100)) {
                        //char* as = (char*) ps[i]->GetAsm();
                        ImGui::Text("%s\n", ps_2[i]->fxcAsm.c_str());
                        ImGui::TreePop();
                    }
                    sprintf(buf100, "Last PS2 Constants ##%i", i);
                    if(ImGui::TreeNode(buf100)) {
                        for(int c = 0; c < 255; c++) {
                            ImGui::Text("c%i, %f, %f, %f, %f", c, ps_2[i]->constants[c][0], ps_2[i]->constants[c][1], ps_2[i]->constants[c][2], ps_2[i]->constants[c][3]);
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndChild();
    }
    if(ImGui::CollapsingHeader("Vertex shaders 2")) {
        ImGui::BeginChild("ChildVS2", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y * 2.f / 3.f), false, ImGuiWindowFlags_HorizontalScrollbar);
        for(int i = 0; i < vs_2.size(); i++) {
            m_IDirect3DVertexShader9* pShader2 = static_cast<m_IDirect3DVertexShader9*>(ImGuiVS);
            if(vs_2[i] && vs_2[i] != m_IDirect3DVertexShader9::dummyShader && vs_2[i] != pShader2) {
                sprintf(buf100, "Vertex Shader2 %i", i);
                if(ImGui::TreeNode(buf100)) {
                    ImGui::Text("VS %i", vs_2[i]->id);
                    if(vs_2[i] != vs_2[i]->dummyShader) {
                        sprintf(buf100, "Disable PS Shader2 ##%i_%i", i, vs_2[i]->id);
                        ImGui::Checkbox(buf100, &vs_2[i]->disable);
                    }
                    sprintf(buf100, "VS Assembly2 %i", i);
                    if(ImGui::TreeNode(buf100)) {
                        //char* as = (char*) vs[i]->GetAsm();
                        ImGui::Text("%s\n", vs_2[i]->fxcAsm.c_str());
                        ImGui::TreePop();
                    }
                    sprintf(buf100, "Last VS2 Constants ##%i", i);
                    if(ImGui::TreeNode(buf100)) {
                        for(int c = 0; c < 256; c++) {
                            ImGui::Text("c%i, %f, %f, %f, %f", c, vs_2[i]->constants[c][0], vs_2[i]->constants[c][1], vs_2[i]->constants[c][2], vs_2[i]->constants[c][3]);
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndChild();
    }

    if(ImGui::CollapsingHeader("Vertex shaders")) {
        ImGui::Checkbox("Show Unused##vs", &showUnused);
        ImGui::Checkbox("Hide Empty FXC##vs", &hideEmptyFXC);
        if(ImGui::Button("Reset Use##vs")) {
            for(int i = 0; i < fx_vs.size(); i++) {
                for(int j = 0; j < fx_vs[i].size(); j++) {
                    if(fx_vs[i][j]) {
                        fx_vs[i][j]->used = 0;
                    }
                }
            }
        }
        if(ImGui::Button("Enable all vs")) {
            for(int i = 0; i < fx_vs.size(); i++) {
                for(int j = 0; j < fx_vs[i].size(); j++) {
                    if(fx_vs[i][j]) {
                        fx_vs[i][j]->disable = false;
                        fx_vs[i][j]->useNewShader = false;
                    }
                }
            }
        }

        ImGui::BeginChild("ChildVS", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 2), false, ImGuiWindowFlags_HorizontalScrollbar);
        for(int i = 0; i < fx_vs.size(); i++) {
            if(ImGui::TreeNode(shader_names_fxc[i])) {
                sprintf(buf100, "Disable all##vs%i", i);
                if(ImGui::Button(buf100)) {
                    for(int j = 0; j < fx_vs[i].size(); j++) {
                        if(fx_vs[i][j]) {
                            fx_vs[i][j]->disable = true;
                            fx_vs[i][j]->useNewShader = false;
                        }
                    }
                }
                sprintf(buf100, "Enable all##vs%i", i);
                ImGui::SameLine();
                if(ImGui::Button(buf100)) {
                    for(int j = 0; j < fx_vs[i].size(); j++) {
                        if(fx_vs[i][j]) {
                            fx_vs[i][j]->disable = false;
                            fx_vs[i][j]->useNewShader = false;
                        }
                    }
                }
                for(int j = 0; j < fx_vs[i].size(); j++) {
                    if(fx_vs[i][j] && ((showUnused == false && fx_vs[i][j]->used > 0) || showUnused == true)) {
                        sprintf(buf100, "%s #vs%i_%i_%i", fx_vs[i][j]->oName.c_str(), i, j, fx_vs[i][j]->id);
                        if(ImGui::TreeNode(buf100)) {
                            std::string namefx = fx_vs[i][j]->oName.substr(0, fx_vs[i][j]->oName.find_last_of(".")) + std::string(".fx");
                            std::string nameasm = fx_vs[i][j]->oName;

                            ImGui::Text("VS ID %i ##%i_%i_%i", fx_vs[i][j]->id, i, j, fx_vs[i][j]->id);
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
                            sprintf(buf100, "Compile VS fx ##%i_%i_%i", i, j, fx_vs[i][j]->id);
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
                                    if(t->bs == fx_vs[i][j]) {
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
                                    if(t->bs == fx_vs[i][j]) {
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
                            sprintf(buf100, "Edit VS asm ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(ImGui::Button(buf100)) {
                                bool isInEditor = false;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_vs[i][j]);
                                    if(t->bs && t->bs == bs && t->name == nameasm) {
                                        isInEditor = true;
                                        t->show = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    if(fx_vs[i][j]->editedAsm.length() < 3) {
                                        fx_vs[i][j]->editedAsm = fx_vs[i][j]->fxcAsm;
                                    }
                                    stShaderEditor* s = new stShaderEditor(fx_vs[i][j]->oName, VS_ASM, new TextEditor(), fx_vs[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->editedAsm);
                                    lst.push_back(s);
                                }
                            }
                            sprintf(buf100, "Edit VS hlsl ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->loadedFx.length() > 0 && ImGui::Button(buf100)) {
                                bool isInEditor = false;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_vs[i][j]);
                                    if(t->bs && t->bs == bs && t->name == namefx) {
                                        isInEditor = true;
                                        t->show = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    if(fx_vs[i][j]->editedFx.length() < 3) {
                                        fx_vs[i][j]->editedFx = fx_vs[i][j]->loadedFx;
                                    }
                                    stShaderEditor* s = new stShaderEditor(fx_vs[i][j]->oName.substr(0, fx_vs[i][j]->oName.find_last_of(".")) + std::string(".fx"), VS_FX, new TextEditor(), fx_vs[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->editedFx);
                                    lst.push_back(s);
                                }
                            }
                            sprintf(buf100, "Disable VS Shader ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            ImGui::Checkbox(buf100, &fx_vs[i][j]->disable);
                            if(!fx_vs[i][j]->newShader)
                                fx_vs[i][j]->useNewShader = false;
                            else {
                                sprintf(buf100, "Use New VS Shader ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                                ImGui::Checkbox(buf100, &fx_vs[i][j]->useNewShader);
                            }
                            sprintf(buf100, "Original VS ASM ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->fxcAsm.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_vs[i][j]->fxcAsm.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Original VS FX ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->loadedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_vs[i][j]->loadedFx.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Edited VS ASM ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->editedAsm.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_vs[i][j]->editedAsm.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Edited VS FX ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(fx_vs[i][j]->editedFx.length() > 1 && ImGui::TreeNode(buf100)) { ImGui::Text(fx_vs[i][j]->editedFx.c_str()); ImGui::TreePop(); }
                            sprintf(buf100, "Last VS Constants ##%i_%i_%i", i, j, fx_vs[i][j]->id);
                            if(ImGui::TreeNode(buf100)) {
                                for(int c = 0; c < 255; c++) {
                                    ImGui::Text("c%i, %f, %f, %f, %f", c, fx_vs[i][j]->constants[c][0], fx_vs[i][j]->constants[c][1], fx_vs[i][j]->constants[c][2], fx_vs[i][j]->constants[c][3]);
                                }
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

    if(false && ImGui::CollapsingHeader("Vertex shaders")) {
        ImGui::Checkbox("showUnused", &showUnused);
        for(int i = 0; i < fx_vs.size(); i++) {
            if(ImGui::TreeNode(shader_names_fxc[i])) {
                for(int j = 0; j < fx_vs[i].size(); j++) {
                    if(fx_vs[i][j] && ((showUnused == false && fx_vs[i][j]->used > 0) || showUnused == true)) {
                        char buf1[100] = { 0 };
                        sprintf(buf1, "%s %i_%i", fx_vs[i][j]->oName.c_str(), i, j);
                        if(ImGui::TreeNode(buf1)) {
                            ImGui::Text("VS ID %i", fx_vs[i][j]->id);
                            ImGui::Text("Use Counter: %i", fx_vs[i][j]->used);
                            char ch[] = "VS Assembly 000000000000";
                            char ch1[] = "Disable Shader##%i0000000000";
                            char ch2[] = "Use New Shader##%i0000000000";
                            char ch3[] = "Last Constants##%i0000000000";
                            char coa[100] = { 0 }; sprintf(coa, "Original ASM##%i%i", i, j);
                            char cof[100] = { 0 }; sprintf(cof, "Original FX##%i%i", i, j);
                            char cea[100] = { 0 }; sprintf(cea, "Edited ASM##%i%i", i, j);
                            char cef[100] = { 0 }; sprintf(cef, "Edited FX##%i%i", i, j);
                            sprintf(ch, "VS Assembly %i", fx_vs[i][j]->id);
                            sprintf(ch1, "Disable Shader##%i", fx_vs[i][j]->id);
                            sprintf(ch2, "Use New Shader##%i", fx_vs[i][j]->id);
                            sprintf(ch3, "Last Constants##%i", fx_vs[i][j]->id);
                            if(ImGui::Button("Compile")) {
                                for(auto t : lst) {
                                    if(t->bs == fx_vs[i][j]) {
                                        t->bs->editedAsm = t->editor->GetText();;
                                        t->bs->compileNewASM();
                                    }
                                }
                            }
                            char reset[] = "reset##xxxx xxxx";
                            sprintf(reset, "Reset##%i%i", i, j);
                            if(ImGui::Button(reset)) {
                                for(auto t : lst) {
                                    if(t->bs == fx_vs[i][j]) {
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
                            if(ImGui::Button("Edit")) {
                                bool isInEditor = false;
                                for(auto t : lst) {
                                    basicShader* bs = static_cast<basicShader*> (fx_vs[i][j]);
                                    if(t->bs == bs) {
                                        isInEditor = true;
                                        t->show = true;
                                    }
                                }
                                if(isInEditor == false) {
                                    if(fx_vs[i][j]->editedAsm.length() < 3) {
                                        fx_vs[i][j]->editedAsm = fx_vs[i][j]->fxcAsm;
                                    }
                                    stShaderEditor* s = new stShaderEditor(fx_vs[i][j]->oName, fx_vs[i][j]->editedAsm, new TextEditor(), fx_vs[i][j]);
                                    s->editor->SetShowWhitespaces(0);
                                    s->editor->SetText(s->bs->editedAsm);
                                    lst.push_back(s);
                                    //editor.SetText(ps[i]->editedAsm);
                                    //editor.SetShowWhitespaces(0);
                                }
                            }
                            ImGui::Checkbox(ch1, &fx_vs[i][j]->disable);
                            if(!fx_vs[i][j]->newShader)
                                fx_vs[i][j]->useNewShader = false;
                            else
                                ImGui::Checkbox(ch2, &fx_vs[i][j]->useNewShader);
                            if(fx_vs[i][j]->fxcAsm.length() > 1 && ImGui::TreeNode(coa)) { ImGui::Text(fx_vs[i][j]->fxcAsm.c_str()); ImGui::TreePop(); }
                            if(fx_vs[i][j]->loadedFx.length() > 1 && ImGui::TreeNode(cof)) { ImGui::Text(fx_vs[i][j]->loadedFx.c_str()); ImGui::TreePop(); }
                            if(fx_vs[i][j]->editedAsm.length() > 1 && ImGui::TreeNode(cea)) { ImGui::Text(fx_vs[i][j]->editedAsm.c_str()); ImGui::TreePop(); }
                            if(fx_vs[i][j]->editedFx.length() > 1 && ImGui::TreeNode(cef)) { ImGui::Text(fx_vs[i][j]->editedFx.c_str()); ImGui::TreePop(); }
                            if(ImGui::TreeNode(ch3)) {
                                for(int c = 0; c < 255; c++) {
                                    ImGui::Text("c%i, %f, %f, %f, %f", c, fx_vs[i][j]->constants[c][0], fx_vs[i][j]->constants[c][1], fx_vs[i][j]->constants[c][2], fx_vs[i][j]->constants[c][3]);
                                }
                                ImGui::TreePop();
                            }
                            ImGui::TreePop();
                        }
                    }
                }
                ImGui::TreePop();
            }
        }
    }

    if(0) {
        //if(ImGui::CollapsingHeader("Pixel shaders")) {
        for(int i = 0; i < shader_names_ps.size(); i++) {
            if(ImGui::TreeNode(shader_names_ps[i])) {
                if(ps[i]) {
                    ImGui::Text("PS ID %i", ps[i]->id);
                    ImGui::Text("Use Counter: %i", ps[i]->used);
                    char ch[] = "PS Assembly 000000000000";
                    char ch1[] = "Disable Shader##%i0000000000";
                    char ch2[] = "Use New Shader##%i0000000000";
                    char ch3[] = "Last Constants##%i0000000000";
                    sprintf(ch, "PS Assembly %i", i);
                    sprintf(ch1, "Disable Shader##%i", i);
                    sprintf(ch2, "Use New Shader##%i", i);
                    sprintf(ch3, "Last Constants##%i", i);
                    if(ImGui::Button("Compile")) {
                        ps[i]->compileNewASM();
                    }
                    if(ImGui::Button("Edit")) {
                        ps[i]->editedAsm = ps[i]->fxcAsm;
                        stShaderEditor* s = new stShaderEditor(ps[i]->oName, ps[i]->editedAsm, new TextEditor(), ps[i]);
                        s->editor->SetShowWhitespaces(0);
                        s->editor->SetText(s->bs->editedAsm);
                        lst.push_back(s);
                        //editor.SetText(ps[i]->editedAsm);
                        //editor.SetShowWhitespaces(0);
                    }
                    ImGui::Checkbox(ch1, &ps[i]->disable);
                    ImGui::Checkbox(ch2, &ps[i]->useNewShader);
                    if(!ps[i]->newShader)
                        ps[i]->useNewShader = false;
                    if(ImGui::TreeNode(ch)) {
                        //char* as = (char*) ps[i]->GetAsm();
                        ImGui::Text("%s\n", ps[i]->fxcAsm.c_str());
                        ImGui::TreePop();
                    }
                    if(ImGui::TreeNode(ch3)) {
                        for(int c = 0; c < 255; c++) {
                            ImGui::Text("c%i, %f, %f, %f, %f", c, ps[i]->constants[c][0], ps[i]->constants[c][1], ps[i]->constants[c][2], ps[i]->constants[c][3]);
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
    }
    if(0) {
        if(ImGui::CollapsingHeader("Vertex shaders")) {
            for(int i = 0; i < shader_names_vs.size(); i++) {
                if(ImGui::TreeNode(shader_names_vs[i])) {
                    if(vs[i]) {
                        ImGui::Text("VS ID %i", vs[i]->id);
                        char c[] = "VS Assembly 000000000000";
                        sprintf(c, "VS Assembly %i", i);
                        if(ImGui::TreeNode(c)) {
                            //char* as = (char*) vs[i]->GetAsm();
                            ImGui::Text("%s\n", vs[i]->fxcAsm.c_str());
                            ImGui::TreePop();
                        }
                        for(int c = 0; c < 256; c++) {
                            ImGui::Text("c%i, %f, %f, %f, %f", c, vs[i]->constants[c][0], vs[i]->constants[c][1], vs[i]->constants[c][2], vs[i]->constants[c][3]);
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
#endif

    if(bDisplayFPSCounter && ImGui::CollapsingHeader("address")) {
        ImGui::Checkbox("Show Demo Window", &showDemoWindow);
        if(GameVersion == 1080 || GameVersion == 1070) {
            ImGui::SliderFloat("Shadow Distance BA + 0xB3E194 ", (float*) (baseAddress + 0xB3E194), 0, 1000);
        }
        if(GameVersion == 1200) {
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

    ImGui::BeginChild("ChildDummy", ImVec2(ImGui::GetContentRegionAvail().x, WindowSize.y), false, ImGuiWindowFlags_NoScrollbar);
    ImGui::EndChild();
    ImGui::End();

    ImGui::SetNextWindowCollapsed(!mShowEditor);
    ImGui::Begin("Shader Editor", nullptr /*&mShowEditor*/, /*ImGuiWindowFlags_HorizontalScrollbar |*/ ImGuiWindowFlags_MenuBar);
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
    if(ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
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
                    if(ImGui::MenuItem("Compile", "F5", nullptr, true)) {
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

    if(io.WantCaptureKeyboard || pauseGame) {
    	*IsGamePaused = true;
    }
    else {
    	if(*IsGameOnMenu) {
    		*IsGamePaused = true;
    	}
    	else {
    		*IsGamePaused = false;
    	}
    }
    if(showDemoWindow)
        ImGui::ShowDemoWindow(&showDemoWindow);

    if(bShowLogWindow) {
        if(mLogPos.y == 0.f) {
            mLogPos.y = gWindowHeight - 230.f;
        }
        if(mLogSize.x == 0.f || mLogSize.y == 0.f) {
            mLogSize = ImVec2(float(gWindowWidth - 5), 200.f);
        }

        if(ImGui::Begin("LOG Window", &bShowLogWindow, 0)) {
            ImGui::SetWindowPos(mLogPos, ImGuiCond_FirstUseEver);
            ImGui::SetWindowSize(mLogSize, ImGuiCond_FirstUseEver);
            //const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
            //if(ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar)) {
            ImGui::InputTextMultiline("Log", (char*) Log::c_str(), Log::length(), ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly | ImGuiWindowFlags_HorizontalScrollbar);

            //ImGui::PopStyleVar();
        //}
        //ImGui::EndChild();

        //ImGui::Text(Log::c_str());
        }
        mLogPos = ImGui::GetWindowPos();
        ImVec2 LogSize = ImGui::GetWindowSize();
        if(LogSize.x > 100 && LogSize.y > 50)
            mLogSize = LogSize;
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
