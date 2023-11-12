#pragma once
#include <vector>
#include <fstream>
#include <sstream>

#include "Log.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9_shader.h"

#include "Utils.h"
#include "EmbeddedFont.h"
#include "TextEditor.h"
#include "D3DX9Mesh.h"

// found game shaders with signatures
extern std::vector<m_IDirect3DPixelShader9*> ps;
// found game shaders with signatures
extern std::vector<m_IDirect3DVertexShader9*> vs;
// any shader without signature, and any edited shader
extern std::vector<m_IDirect3DPixelShader9*> ps_2;
// any shader without signature, and any edited shader
extern std::vector<m_IDirect3DVertexShader9*> vs_2;
extern std::vector<const char*> shader_names_ps;
extern std::vector<const char*> shader_names_vs;
extern std::vector<const char*> shader_names_fxc;
extern std::vector<std::vector<m_IDirect3DPixelShader9*>> fx_ps;
extern std::vector<std::vector<m_IDirect3DVertexShader9*>> fx_vs;

extern std::vector<uint8_t> patternZS;
extern std::vector<uint8_t> patternFF;
extern std::vector<uint8_t> pattern2;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class ShadowResFix {
public:
    ShadowResFix();
    ~ShadowResFix();

    void Initialize(const uint8_t* baseAddress);

    bool OnWndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Update();

    void OnBeforeD3D9DeviceReset(IDirect3DDevice9* d3d9Device);
    void OnAfterD3D9DeviceReset();

    void OnBeforeD3D9DeviceEndScene(IDirect3DDevice9* d3d9Device);

    bool mDisableMouseControl = false;

    //private:
    void InitializeImGui(IDirect3DDevice9* d3d9Device);
    void InitializeColors();

    void SaveSettings();
    void LoadSettings();

    void DrawMainWindow();
    void DrawSettingsWindow();

    bool mIsImGuiInitialized = false;

    const uint8_t mSettingsFileMajorVersion = 1;
    const uint8_t mSettingsFileMinorVersion = 1;

    bool mShowWindow = false;
    bool mShowEditor = true;
    bool bShowLogWindow = true;
    bool mShowSettingsWindow = false;
    bool pauseGame = false;

    int32_t GameVersion = 0;

    ImGuiKey mOpenWindowKey = ImGuiKey_F10;
    ImGuiKey mCompileShader = ImGuiKey_F5;

    ImVec2 mWindowPos = ImVec2(5.f, 5.f);
    ImVec2 mWindowSize = ImVec2(440.f, 650.f);

    ImVec2 mEditorPos = ImVec2(0.f, 0.f);
    ImVec2 mEditorSize = ImVec2(0.f, 0.f);

    ImVec2 mLogPos = ImVec2(0.f, 0.f);
    ImVec2 mLogSize = ImVec2(0.f, 0.f);

    float mItemInnerSpacing = 4.f;
    float mFontScale = 0.9f;
    ImGuiKey mToggleCameraControlKey = ImGuiKey_None;
};
