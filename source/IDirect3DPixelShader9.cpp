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

#include "d3d9.h"
#include "D3DX9Mesh.h"
#include "Log.h"

extern std::vector<uint8_t> patternFF;
extern std::vector<m_IDirect3DPixelShader9*> ps;
extern std::vector<m_IDirect3DPixelShader9*> ps_2;
extern std::vector<m_IDirect3DVertexShader9*> vs_2;
extern std::vector<m_IDirect3DPixelShader9*> ps_4;
extern std::vector<m_IDirect3DVertexShader9*> vs_4;
extern std::vector<const char*> shader_names_ps;
extern std::vector<const char*> shader_names_vs;
extern std::vector<const char*> shader_names_fxc;
extern std::vector<std::vector<m_IDirect3DPixelShader9*>> fx_ps;
extern std::vector<std::vector<m_IDirect3DVertexShader9*>> fx_vs;

extern LPDIRECT3DPIXELSHADER9       ImGuiPS;
extern LPDIRECT3DVERTEXSHADER9      ImGuiVS;

char shadersrcps[] =
"\nps_3_0\n"
"def c0, 0, 0, 0, 0\n"
"dcl_texcoord9 v9\n"
"mov oC0, c220\n"
"mov oC1, c220\n"
"mov oC2, c220\n"
"mov oC3, c220\n"
"rcp r20.z, c128.x\n"
"mul r20.x, v9.w, r20.z\n"
"mul r20.y, c128.y, r20.z\n"
"log r20.x, r20.x\n"
"log r20.y, r20.y\n"
"rcp r20.y, r20.y\n"
"mul oDepth, r20.x, r20.y\n"
;


int getSignature(m_IDirect3DPixelShader9* pShader, std::vector<uint8_t>& pattern) {
    static std::vector<uint8_t> pbFunc;
    UINT len;
    pShader->GetFunction(nullptr, &len);
    if(pbFunc.size() < len) {
        pbFunc.resize(len + len % 4);
    }
    pShader->GetFunction(pbFunc.data(), &len);
    int cnt = 0;
    for(int i = 0; i < (int)pbFunc.size(); i++) {
        for(int j = 0; j < (int) pattern.size(); j++) {
            if(pbFunc[i + j] == pattern[j]) {
                cnt = j;
                continue;
            }
            else {
                cnt = 0;
                break;
            }
        }
        if(cnt >= (int) pattern.size() - 1) {
            cnt = i;
            break;
        }
    }
    int c = -1;
    if(cnt > 0) {
        c = *((int*) &pbFunc[cnt + pattern.size()]);
    }
    return c;
}

extern std::string LoadFile(std::string filename);

IDirect3DPixelShader9* CompilePixelShaderFromFile(const char* filename, const char* mainFunction, const char* name, m_IDirect3DDevice9Ex* m_pDeviceEx, bool isAsm, bool addToList) {
    HRESULT hr1 = S_FALSE;
    HRESULT hr2 = S_FALSE;
    ID3DXBuffer* pBufferShader = nullptr;
    ID3DXBuffer* pBufferErrorMessage = nullptr;
    ID3DXConstantTable* pConstantTable = nullptr;

    if(isAsm) {
        hr1 = D3DXAssembleShaderFromFileA((std::string("update/shaders/") + filename).c_str(), 0, 0, 0, &pBufferShader, &pBufferErrorMessage);
        if(!pBufferShader || hr1 != S_OK)
            hr1 = D3DXAssembleShaderFromFileA((std::string("shaders/") + filename).c_str(), 0, 0, 0, &pBufferShader, &pBufferErrorMessage);
        if(!pBufferShader || hr1 != S_OK)
            hr1 = D3DXAssembleShaderFromFileA(filename, 0, 0, 0, &pBufferShader, &pBufferErrorMessage);
    }
    else {
        hr1 = D3DXCompileShaderFromFileA((std::string("update/shaders/") + filename).c_str(), 0, 0, mainFunction, "ps_3_0", 0, &pBufferShader, &pBufferErrorMessage, &pConstantTable);
        if(!pBufferShader || hr1 != S_OK)
            hr1 = D3DXCompileShaderFromFileA((std::string("shaders/") + filename).c_str(), 0, 0, mainFunction, "ps_3_0", 0, &pBufferShader, &pBufferErrorMessage, &pConstantTable);
        if(!pBufferShader || hr1 != S_OK)
            hr1 = D3DXCompileShaderFromFileA(filename, 0, 0, mainFunction, "ps_3_0", 0, &pBufferShader, &pBufferErrorMessage, &pConstantTable);
    }

    m_IDirect3DPixelShader9* ps = nullptr;
    IDirect3DPixelShader9* pShader = nullptr;
    if(hr1 == S_OK) {
        int lenght = pBufferShader->GetBufferSize();
        char* p = (char*) pBufferShader->GetBufferPointer();

        hr2 = m_pDeviceEx->CreatePixelShader2((DWORD*) p, &pShader, SC_NEW);
        if(pShader) {
            ps = static_cast<m_IDirect3DPixelShader9*> (pShader);
            if(ps) {
                ps->disable = false;
                ps->oName = name;
                ps->fileName = filename;
                ps->oName = name;
                if(addToList)
                ps_2.push_back(ps);
            }
        }
    }
    if(hr1 != S_OK || hr2 != S_OK) {
        char* bufferMessage = 0;
        if(pBufferErrorMessage)
            bufferMessage = (char*) pBufferErrorMessage->GetBufferPointer();
        Log::Error("Unable to compile Pixel shader");
        Log::Text(filename);
        if(mainFunction)
            Log::Text(mainFunction);

        if(bufferMessage)
            Log::Text(bufferMessage);
    }

    SAFE_RELEASE(pBufferShader);
    SAFE_RELEASE(pBufferErrorMessage);

    if(ps) {
        if(isAsm) {
            std::string src = LoadFile((std::string("update/shaders/") + filename).c_str());
            if(src.length() < 2)
                src = LoadFile((std::string("shaders/") + filename).c_str());
            if(src.length() < 2)
                src = LoadFile(filename);
            if(src.length() > 1)
                ps->fxcAsm = src;
        }
        else {
            std::string src = LoadFile((std::string("update/shaders/") + filename).c_str());
            if(src.length() < 2)
                src = LoadFile((std::string("shaders/") + filename).c_str());
            if(src.length() < 2)
                src = LoadFile(filename);
            if(src.length() > 1)
                ps->loadedFx = src;
            ps->entryFunction = mainFunction;
        }
    }
    return pShader;
}

IDirect3DVertexShader9* CompileVertexShaderFromFile(const char* filename, const char* mainFunction, const char* name, m_IDirect3DDevice9Ex* m_pDeviceEx, bool isAsm, bool addToList) {
    HRESULT hr1 = S_FALSE;
    HRESULT hr2 = S_FALSE;
    ID3DXBuffer* pBufferShader = nullptr;
    ID3DXBuffer* pBufferErrorMessage = nullptr;
    ID3DXConstantTable* pConstantTable = nullptr;

    if(isAsm) {
        hr1 = D3DXAssembleShaderFromFileA((std::string("update/shaders/") + filename).c_str(), 0, 0, 0, &pBufferShader, &pBufferErrorMessage);
        if(!pBufferShader || hr1 != S_OK)
            hr1 = D3DXAssembleShaderFromFileA((std::string("shaders/") + filename).c_str(), 0, 0, 0, &pBufferShader, &pBufferErrorMessage);
        if(!pBufferShader || hr1 != S_OK)
            hr1 = D3DXAssembleShaderFromFileA(filename, 0, 0, 0, &pBufferShader, &pBufferErrorMessage);
    }
    else {
        hr1 = D3DXCompileShaderFromFileA((std::string("update/shaders/") + filename).c_str(), 0, 0, mainFunction, "vs_3_0", 0, &pBufferShader, &pBufferErrorMessage, &pConstantTable);
        if(!pBufferShader || hr1 != S_OK)
            hr1 = D3DXCompileShaderFromFileA((std::string("shaders/") + filename).c_str(), 0, 0, mainFunction, "vs_3_0", 0, &pBufferShader, &pBufferErrorMessage, &pConstantTable);
        if(!pBufferShader || hr1 != S_OK)
            hr1 = D3DXCompileShaderFromFileA(filename, 0, 0, mainFunction, "vs_3_0", 0, &pBufferShader, &pBufferErrorMessage, &pConstantTable);
    }

    IDirect3DVertexShader9* pShader = nullptr;
    m_IDirect3DVertexShader9* vs = nullptr;
    if(hr1 == S_OK) {
        int lenght = pBufferShader->GetBufferSize();
        char* p = (char*) pBufferShader->GetBufferPointer();

        hr2 = m_pDeviceEx->CreateVertexShader2((DWORD*) p, &pShader, SC_NEW);
        if(pShader) {
            m_IDirect3DVertexShader9* vs = static_cast<m_IDirect3DVertexShader9*> (pShader);
            if(vs) {
                vs->disable = false;
                vs->oName = name;
                vs->fileName = filename;
                if(addToList)
                vs_2.push_back(vs);
            }
        }
    }
    if(hr1 != S_OK || hr2 != S_OK) {
        char* bufferMessage = 0;
        if(pBufferErrorMessage)
            char* bufferMessage = (char*) pBufferErrorMessage->GetBufferPointer();
        Log::Error("Unable to compile Vertex shader");
        Log::Text(filename);
        if(mainFunction)
            Log::Text(mainFunction);

        if(bufferMessage)
            Log::Text(bufferMessage);
    }
    SAFE_RELEASE(pBufferShader);
    SAFE_RELEASE(pBufferErrorMessage);

    if(vs) {
        if(isAsm) {
            std::string src = LoadFile((std::string("update/shaders/") + filename).c_str());
            if(src.length() < 2)
                src = LoadFile((std::string("shaders/") + filename).c_str());
            if(src.length() < 2)
                src = LoadFile(filename);
            if(src.length() > 1)
                vs->fxcAsm = src;
        }
        else {
            std::string src = LoadFile((std::string("update/shaders/") + filename).c_str());
            if(src.length() < 2)
                src = LoadFile((std::string("shaders/") + filename).c_str());
            if(src.length() < 2)
                src = LoadFile(filename);
            if(src.length() > 1)
                vs->loadedFx = src;
            vs->entryFunction = mainFunction;
        }
    }

    return pShader;
}

uint32_t getCRC32(m_IDirect3DPixelShader9* pShader) {
    static std::vector<uint8_t> pbFunc;
    UINT len;
    pShader->GetFunction(nullptr, &len);
    if(pbFunc.size() < len) {
        pbFunc.resize(len);
    }
    pShader->GetFunction(pbFunc.data(), &len);
    uint32_t crc = crc_32(pbFunc.data(), len);
    return crc;
}

int getfxid(int id, std::string oName) {
    int fxid = -1;
    for(int i = 0; i < (int) shader_names_fxc.size(); i++) {
        std::string sh = shader_names_fxc[i] + std::string("/");
        if(oName.length() > sh.length()) {
            auto str = oName.substr(0, sh.length());
            if(str == sh) {
                fxid = i;
            }
        }
    }
    return fxid;
}


std::string getNameFromCRC(UINT crc32) {
    for(int i = 0; i < (int) crclist_ps.size(); i++) {
        if(crclist_ps[i]->crc32 == crc32) {
            return crclist_ps[i]->name;
        }
    }
    return "";
}

int getIdFromCRC(UINT crc32) {
    int id = -1;
    auto name = getNameFromCRC(crc32);
    for(int i = 0; i < (int) shader_names_ps.size(); i++) {
        if(name == shader_names_ps[i]) {
            id = i;
            break;
        }
    }
    return id;
}

std::string LoadFile(std::string filename) {
    int sz = 0;
    std::string src;
    FILE* f = fopen(filename.c_str(), "r");
    if(f) {
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        src.resize(sz);
        fread((void*) src.c_str(), 1, sz, f);
        fclose(f);
    }
    return src;
}

std::string LoadFX(std::string fxname, std::string shadername) {
    size_t found = shadername.find_last_of(".");
    std::string filename = std::string("shaders\\fx\\") + fxname + "\\" + shadername.substr(0, found) + ".hlsl";
    int sz = 0;
    std::string src;
    FILE* f = fopen(filename.c_str(), "r");
    if(f) {
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        src.resize(sz);
        fread((void*) src.c_str(), 1, sz, f);
        fclose(f);
    }
    return src;
}
std::string LoadASM(std::string fxname, std::string shadername) {
    size_t found = shadername.find_last_of(".");
    std::string filename = std::string("shaders\\asm\\") + fxname + "\\" + shadername.substr(0, found) + ".asm";
    int sz = 0;
    std::string src;
    FILE* f = fopen(filename.c_str(), "r");
    if(f) {
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        src.resize(sz);
        fread((void*) src.c_str(), 1, sz, f);
        fclose(f);
    }
    return src;
}

HRESULT SaveFX(std::string fxname, std::string shadername, std::string source, ShaderType shaderType) {
    size_t found = shadername.find_last_of(".");
    std::string dir1 = "shaders\\fx\\";
    std::string dir2 = dir1 + fxname + "\\";
    std::string fileName = dir2 + shadername.substr(0, found) + ".hlsl";
    std::string mk = std::string("mkdir ") + dir2;
    system(mk.c_str());
    FILE* file = fopen(fileName.c_str(), "w");
    if(file) {
        fwrite(source.data(), 1, source.length(), file);
        fclose(file);
        Log::Info("File saved: " + fileName);
        return S_OK;
    }
    else {
        Log::Error("File not saved, unable to open file to write: " + fileName);
        return S_FALSE;
    }
    return S_FALSE;
}
HRESULT SaveASM(std::string fxname, std::string shadername, std::string source, ShaderType shaderType) {
    size_t found = shadername.find_last_of(".");
    std::string dir1 = "shaders\\asm\\";
    std::string dir2 = dir1 + fxname + "\\";
    std::string fileName = dir2 + shadername.substr(0, found) + ".asm";
    std::string mk = std::string("mkdir ") + dir2;
    system(mk.c_str());
    FILE* file = fopen(fileName.c_str(), "w");
    if(file) {
        fwrite(source.data(), 1, source.length(), file);
        fclose(file);
        Log::Info("File saved: " + fileName);
        return S_OK;
    }
    else {
        Log::Error("File not saved, unable to open file to write: " + fileName);
        return S_FALSE;
    }
    return S_FALSE;
}

IDirect3DPixelShader9* m_IDirect3DPixelShader9::dummyShader = nullptr;

extern IDirect3DPixelShader9* FxaaPS;
extern IDirect3DPixelShader9* SunShafts_PS;
extern IDirect3DPixelShader9* SunShafts2_PS;
extern IDirect3DPixelShader9* SunShafts3_PS;
extern IDirect3DPixelShader9* SunShafts4_PS;
extern IDirect3DPixelShader9* SSAdd_PS;
extern IDirect3DPixelShader9* SSDownsampler_PS;
extern IDirect3DPixelShader9* SSDownsampler2_PS;

extern IDirect3DPixelShader9* SMAA_EdgeDetection;
extern IDirect3DPixelShader9* SMAA_BlendingWeightsCalculation;
extern IDirect3DPixelShader9* SMAA_NeighborhoodBlending;


extern IDirect3DVertexShader9* SMAA_EdgeDetectionVS;
extern IDirect3DVertexShader9* SMAA_BlendingWeightsCalculationVS;
extern IDirect3DVertexShader9* SMAA_NeighborhoodBlendingVS;

extern IDirect3DPixelShader9* DOF_ps;
extern IDirect3DPixelShader9* dof_blur_ps;
extern IDirect3DPixelShader9* dof_coc_ps;

extern IDirect3DPixelShader9* depth_of_field_ps;
extern IDirect3DPixelShader9* depth_of_field_tent_ps ;
extern IDirect3DPixelShader9* depth_of_field_blur_ps ;
extern IDirect3DPixelShader9* depth_of_field_coc_ps  ;
extern IDirect3DPixelShader9* stipple_filter_ps;

extern IDirect3DPixelShader9* SSAO_ps;
extern IDirect3DPixelShader9* SSAO_ps2;
extern IDirect3DVertexShader9* SSAO_vs;
extern IDirect3DPixelShader9* downsampler_ps;

float m_IDirect3DPixelShader9::globalConstants[256][4] = { {0} }; // constant table, set with Set*ShaderConstantF

int getIdFromOrderCRC(int cnt, UINT crc32) {
    int id = -1;
    if(cnt < (int) crclist_ps.size()) {
        // exact cnt, precise
        if(crclist_ps[cnt]->crc32 == crc32)
            return crclist_ps[cnt]->id;
        // cnt ahead, less precise, but generaly works
        else {
            for(int i = max(0, cnt-1); i < (int) crclist_ps.size(); i++) {
                if(crclist_ps[i]->crc32 == crc32) {
                    return crclist_ps[i]->id;
                }
            }
        }
    }

    // just another way to get the id, does this work?
    for(int i = 0; i < (int) crclist_ps.size(); i++) {
        if(crclist_ps[i]->crc32 == crc32) {
            return crclist_ps[i]->id;
        }
    }

    // just a brute way to get the id, sometimes works
    auto name = getNameFromCRC(crc32);
    for(int i = 0; i < (int) shader_names_ps.size(); i++) {
        if(name == shader_names_ps[i]) {
            id = i;
            break;
        }
    }
    return id;
}


m_IDirect3DPixelShader9::m_IDirect3DPixelShader9(LPDIRECT3DPIXELSHADER9 pShader9, m_IDirect3DDevice9Ex* pDevice, ShaderCreationMode extra) :
    ProxyInterface(pShader9), m_pDeviceEx(pDevice) {
    static int loadCounter = 0;
    static bool firstShader = true;
    pDevice->ProxyAddressLookupTable->SaveAddress(this, ProxyInterface);
    static char buf100[100] = { 0 };
    id = getSignature(this, patternFF);
    crc32 = getCRC32(this);
    sprintf(buf100, "ps_%x.asm", crc32);
    if(id == -1) {
        id = getIdFromOrderCRC(loadCounter, crc32);
        //getNameFromCRC(crc32);
    }
    if(id == -1) {
        id = getIdFromCRC(crc32);
    }
    else
        loadCounter++;
    //static std::vector<std::string> LoadOrder;
    fxcAsm = GetAsm();
    if(id >= 0 && id >= (int) shader_names_ps.size()) {
        Log::Error("illegal ps id: " + std::to_string(id));
    }
    if(id >= 0 && id < (int) shader_names_ps.size()) {
        oName = shader_names_ps[id];
        //LoadOrder.push_back(oName);
        fxid = getfxid(id, oName);
        fxName = shader_names_fxc[fxid];
        oName = oName.substr(oName.find_last_of("/") + 1);
        if(extra != SC_NEW) {
            loadedFx = LoadFX(fxName, oName);
            loadedAsm = LoadASM(fxName, oName);
        }
        if(fxid<0 || fxid>(int)fx_ps.size()) {
            Log::Error("fxid not found");
        }
        else if(extra == SC_FXC || extra == SC_GAME) {
            fx_ps[fxid].push_back(this);
            ps[id] = this;
        }
        else {
            if(extra != SC_NEW) {
                ps_2.push_back(this);
            }
        }
        //for(auto& i : shadowGen) {
        //    if(i == oName) {
        //        useBias = true;
        //    }
        //}
        if(extra != SC_NEW) {
            if(loadedAsm.length() > 1) {
                auto hr = compileShaderSource(loadedAsm, PS_ASM, SU_LASM);
                if(hr != S_OK) {
                    //Log::Warning("Unable to compile Loaded ASM: " + oName);
                }
                else {
                    //usingShader = SU_LASM;
                }
            }
            if(loadedFx.length() > 1) {
                auto hr = compileShaderSource(loadedFx, PS_FX, SU_LFX);
                if(hr != S_OK) {
                    //Log::Warning("Unable to compile Loaded HLSL: " + oName);
                }
                else {
                    //usingShader = SU_LFX;
                }
            }
        }
        //printf("%i %i %s\n", id, fxid, oName.c_str());
    }
    else {
        fxName = "nonamed";
        oName = buf100;
        if(extra != SC_NEW) {
            ps_2.push_back(this);
            loadedFx = LoadFX(fxName, oName);
            loadedAsm = LoadASM(fxName, oName);
            if(loadedAsm.length() > 1) {
                auto hr = compileShaderSource(loadedAsm, PS_ASM, SU_LASM);
                if(hr != S_OK) {
                    //Log::Warning("Unable to compile Loaded ASM: " + oName);
                }
                else {
                    //usingShader = SU_LASM;
                }
            }
            if(loadedFx.length() > 1) {
                auto hr = compileShaderSource(loadedFx, PS_FX, SU_LFX);
                if(hr != S_OK) {
                    //Log::Warning("Unable to compile Loaded HLSL: " + oName);
                }
                else {
                    //usingShader = SU_LFX;
                }
            }
        }
    }
    if(firstShader) {
        firstShader = false;
        if(!dummyShader) {
            HRESULT hr2 = S_FALSE;
            ID3DXBuffer* bf1 = nullptr;
            ID3DXBuffer* bf2 = nullptr;
            HRESULT hr1 = D3DXAssembleShader(shadersrcps, sizeof(shadersrcps), 0, 0, 0, &bf1, &bf2);
            if(hr1 == S_OK) {
                int sz = bf1->GetBufferSize();
                char* p = (char*) bf1->GetBufferPointer();
                hr2 = m_pDeviceEx->CreatePixelShader2((DWORD*) p, &dummyShader, SC_NEW);
                if(dummyShader) {
                    ((m_IDirect3DPixelShader9*) dummyShader)->disable = true;
                    ((m_IDirect3DPixelShader9*) dummyShader)->oName = "dummyps";
                }
            }
            if(hr1 != S_OK || hr2 != S_OK) {
                Log::Error("Unable to compile Dummy Pixel shader");
            }
            SAFE_RELEASE(bf1);
            SAFE_RELEASE(bf2);
        }

        if(!FxaaPS)
            FxaaPS = CompilePixelShaderFromFile("FXAA.asm", 0, "FXAA.asm", m_pDeviceEx, true, true);

        if(!downsampler_ps)
            downsampler_ps = CompilePixelShaderFromFile("SSAO3_ps.hlsl", "mainDownsample", "SSAO3_Downsample.hlsl", m_pDeviceEx, false, true);
        
        if(!SunShafts_PS)
            SunShafts_PS = CompilePixelShaderFromFile("SunShafts_PS.hlsl", "SunShafts1", "SunShafts1_PS.hlsl", m_pDeviceEx, false, true);
        
        if(!SunShafts2_PS)
            SunShafts2_PS = CompilePixelShaderFromFile("SunShafts_PS.hlsl", "SunShafts2", "SunShafts2_PS.hlsl", m_pDeviceEx, false, true);
        
        if(!SunShafts3_PS)
            SunShafts3_PS = CompilePixelShaderFromFile("SunShafts_PS.hlsl", "SunShafts3", "SunShafts3_PS.hlsl", m_pDeviceEx, false, true);
        
        if(!SunShafts4_PS)
            SunShafts4_PS = CompilePixelShaderFromFile("SunShafts_PS.hlsl", "SunShafts4", "SunShafts4_PS.hlsl", m_pDeviceEx, false, true);
        
        if(!SSDownsampler_PS)
            SSDownsampler_PS = CompilePixelShaderFromFile("SunShafts_PS.hlsl", "SSDownsampler", "SunShaftsDS_PS.hlsl", m_pDeviceEx, false, true);

        if(!SSDownsampler2_PS)
            SSDownsampler2_PS = CompilePixelShaderFromFile("SunShafts_PS.hlsl", "SSDownsampler2", "SunShaftsDS2_PS.hlsl", m_pDeviceEx, false, true);

        if(!SSAdd_PS)
            SSAdd_PS = CompilePixelShaderFromFile("SunShafts_PS.hlsl", "SSAdd", "SunShafts_PS.hlsl", m_pDeviceEx, false, true);

        if(!SSAO_vs)
            SSAO_vs = CompileVertexShaderFromFile("SSAO_vs.asm", 0, "SSAO_vs.asm", m_pDeviceEx, true, true);

        if(!SSAO_ps)
            SSAO_ps = CompilePixelShaderFromFile("SSAO_ps.asm", 0, "SSAO_ps.asm", m_pDeviceEx, true, true);

        if(!SSAO_ps2)
            SSAO_ps2 = CompilePixelShaderFromFile("SSAO_ps2.asm", 0, "SSAO_ps2.asm", m_pDeviceEx, true, true);

        if(!dof_blur_ps)
            dof_blur_ps = CompilePixelShaderFromFile("dof_blur.asm", 0, "dof_blur.asm", m_pDeviceEx, true, true);

        if(!dof_coc_ps)
            dof_coc_ps = CompilePixelShaderFromFile("dof_coc.asm", 0, "dof_coc.asm", m_pDeviceEx, true, true);

        if(!depth_of_field_ps)
            depth_of_field_ps = CompilePixelShaderFromFile("depth_of_field_ps.asm", 0, "depth_of_field_ps.asm", m_pDeviceEx, true, true);

        if(!depth_of_field_tent_ps)
            depth_of_field_tent_ps = CompilePixelShaderFromFile("depth_of_field_tent_ps.asm", 0, "depth_of_field_tent_ps.asm", m_pDeviceEx, true, true);

        if(!depth_of_field_blur_ps)
            depth_of_field_blur_ps = CompilePixelShaderFromFile("depth_of_field_blur_ps.asm", 0, "depth_of_field_blur_ps.asm", m_pDeviceEx, true, true);

        if(!depth_of_field_coc_ps)
            depth_of_field_coc_ps = CompilePixelShaderFromFile("depth_of_field_coc_ps.asm", 0, "depth_of_field_coc_ps.asm", m_pDeviceEx, true, true);

        if(!stipple_filter_ps)
            stipple_filter_ps = CompilePixelShaderFromFile("stipple_filter_ps.asm", 0, "stipple_filter_ps.asm", m_pDeviceEx, true, true);
    }
}

m_IDirect3DPixelShader9::m_IDirect3DPixelShader9(LPDIRECT3DPIXELSHADER9 pShader9, m_IDirect3DDevice9Ex* pDevice) :
    ProxyInterface(pShader9), m_pDeviceEx(pDevice) {
    static char buf100[100] = { 0 };
    pDevice->ProxyAddressLookupTable->SaveAddress(this, ProxyInterface);
    id = getSignature(this, patternFF);
    crc32 = getCRC32(this);
    sprintf(buf100, "ps_%x.asm", crc32);

    fxcAsm = GetAsm();
    if(id >= 0 && id >= (int) shader_names_ps.size()) {
        Log::Error("illegal ps id: " + std::to_string(id));
    }
    if(id >= 0 && id < (int) shader_names_ps.size()) {

        oName = shader_names_ps[id];
        fxid = getfxid(id, oName);
        fxName = shader_names_fxc[fxid];
        oName = oName.substr(oName.find_last_of("/") + 1);
        loadedFx = LoadFX(fxName, oName);
        loadedAsm = LoadASM(fxName, oName);
        if(fxid < 0 || fxid > (int)fx_ps.size()) {
            Log::Error("fxid not found");
        }
        else {
            fx_ps[fxid].push_back(this);
            ps[id] = this;
        }
        printf("%i %i %s\n", id, fxid, oName.c_str());
    }
    else {
        fxName = "nonamed";
        oName = buf100;
        ps_2.push_back(this);
        loadedFx = LoadFX(fxName, oName);
        loadedAsm = LoadASM(fxName, oName);
        if(loadedAsm.length() > 1) {
            auto hr = compileShaderSource(loadedAsm, PS_ASM, SU_LASM);
            if(hr != S_OK) {
                //Log::Warning("Unable to compile Loaded ASM: " + oName);
            }
            else {
                //usingShader = SU_LASM;
            }
        }
        if(loadedFx.length() > 1) {
            auto hr = compileShaderSource(loadedFx, PS_FX, SU_LFX);
            if(hr != S_OK) {
                //Log::Warning("Unable to compile Loaded HLSL: " + oName);
            }
            else {
                //usingShader = SU_LFX;
            }
        }
    }
    if(!dummyShader ) {
        HRESULT hr2 = S_FALSE;
        ID3DXBuffer* bf1 = nullptr;
        ID3DXBuffer* bf2 = nullptr;
        HRESULT hr1 = D3DXAssembleShader(shadersrcps, sizeof(shadersrcps), 0, 0, 0, &bf1, &bf2);
        if(hr1 == S_OK) {
            int sz = bf1->GetBufferSize();
            char* p = (char*) bf1->GetBufferPointer();
            hr2 = m_pDeviceEx->CreatePixelShader2((DWORD*) p, &dummyShader, SC_NEW);
            if(dummyShader) {
                ((m_IDirect3DPixelShader9*) dummyShader)->disable = true;
                ((m_IDirect3DPixelShader9*) dummyShader)->oName = "dummyps";
            }
        }
        if(hr1 != S_OK || hr2 != S_OK) {
            Log::Error("Unable to compile Dummy Pixel shader");
        }
        SAFE_RELEASE(bf1);
        SAFE_RELEASE(bf2);
    }
}

m_IDirect3DPixelShader9::~m_IDirect3DPixelShader9() {
    for(int i = 0; i < (int) ps_2.size(); i++) {
        if(ps_2[i] == this) {
            ps_2[i] = 0;
        }
    }
    if(id >= 0) {
        ps[id] = nullptr;
    }
}

std::string m_IDirect3DPixelShader9::GetAsm() {
    m_IDirect3DPixelShader9* pShader = this;
    static std::vector<uint8_t> pbFunc;
    UINT len;
    pShader->GetFunction(nullptr, &len);
    if(pbFunc.size() < len) {
        pbFunc.resize(len + len % 4);
    }
    pShader->GetFunction(pbFunc.data(), &len);
    std::string source;
    ID3DXBuffer* pShaderAsm = NULL;
    HRESULT hr = D3DXDisassembleShader((DWORD*) pbFunc.data(), FALSE, NULL, &pShaderAsm);
    if(SUCCEEDED(hr) && pShaderAsm) {
        source = (char*) pShaderAsm->GetBufferPointer();
    }
    SAFE_RELEASE(pShaderAsm);
    return source;
}

HRESULT m_IDirect3DPixelShader9::compileNewASM() {
    ID3DXBuffer* bf1 = nullptr;
    ID3DXBuffer* bf4 = nullptr;
    HRESULT hr = D3DXAssembleShader(editedAsm.c_str(), editedAsm.length(), 0, 0, 0, &bf1, &bf4);
    HRESULT hr2 = S_FALSE;
    if(hr == S_OK) {
        hr2 = m_pDeviceEx->GetProxyInterface()->CreatePixelShader((DWORD*) bf1->GetBufferPointer(), /*(IDirect3DPixelShader9**)(m_IDirect3DPixelShader9**)*/ &newShader);
        if(hr2 != S_OK) {
            Log::Error("failed to Compile new pixel shader asm!");
        }
        return hr2;
    }
    else {
        Log::Error("failed to Assemble new pixel shader asm!");
        Log::Text((char*) bf4->GetBufferPointer());
    }
    SAFE_RELEASE(bf1);
    SAFE_RELEASE(bf4);
    return hr;
}

HRESULT m_IDirect3DPixelShader9::compileNewFx() {
    ID3DXBuffer* bf1 = nullptr;
    ID3DXBuffer* bf3 = nullptr;
    ID3DXBuffer* bf4 = nullptr;
    ID3DXConstantTable* ppConstantTable = nullptr;
    HRESULT hr1 = D3DXCompileShader(editedFx.c_str(), editedFx.length(), 0, 0, "main", "ps_3_0", 0, &bf3, &bf4, &ppConstantTable);
    HRESULT hr2 = S_FALSE;
    if(hr1 == S_OK) {
        hr2 = m_pDeviceEx->GetProxyInterface()->CreatePixelShader((DWORD*) bf3->GetBufferPointer(), &newShader);
        if(hr2 != S_OK) {
            Log::Error("failed to Compile new pixel shader hlsl!");
        }
        SAFE_RELEASE(bf1);
        SAFE_RELEASE(bf3);
        SAFE_RELEASE(bf4);
        return hr2;
    }
    else {
        Log::Error("failed to Compile new pixel shader hlsl!");
        Log::Error((char*) bf4->GetBufferPointer());
    }
    SAFE_RELEASE(bf1);
    SAFE_RELEASE(bf3);
    SAFE_RELEASE(bf4);
    return hr1;
}

HRESULT m_IDirect3DPixelShader9::setCompiledShaderToUse(ShaderUse s) {
    if(compiledShaders[UINT(s)] || s == SU_FXC) {
        usingShader = s;
        return S_OK;
    }
    return S_FALSE;
}

HRESULT m_IDirect3DPixelShader9::compileShaderSource(std::string source, ShaderType type, ShaderUse use) {
    switch(use) {
        case SU_LASM:
        case SU_EASM:
        case SU_LFX:
        case SU_EFX:
            break;
        default:
            return S_FALSE;
    }
    ID3DXBuffer* bf1 = nullptr;
    ID3DXBuffer* bf3 = nullptr;
    ID3DXBuffer* bf4 = nullptr;
    ID3DXConstantTable* ppConstantTable = nullptr;
    IDirect3DPixelShader9* shader = nullptr;
    switch(type) {
        case PS_ASM:
        {
            HRESULT hr = D3DXAssembleShader(source.c_str(), source.length(), 0, 0, 0, &bf1, &bf4);
            if(hr == S_OK) {
                HRESULT hr2 = m_pDeviceEx->CreatePixelShader2((DWORD*) bf1->GetBufferPointer(), &shader, SC_NEW);
                if(hr2 != S_OK || !shader) {
                    Log::Error("Failed to create pixel shader asm: " + oName);
                    SAFE_RELEASE(shader);
                }
                else {
                    SAFE_RELEASE(compiledShaders[INT(use)]);
                    compiledShaders[INT(use)] = shader;
                    {
                        static std::vector<uint8_t> pbFunc;
                        UINT len;
                        shader->GetFunction(nullptr, &len);
                        if(pbFunc.size() < len) {
                            pbFunc.resize(len + len % 4);
                        }
                        shader->GetFunction(pbFunc.data(), &len);
                        std::string source;
                        ID3DXBuffer* pShaderAsm = NULL;
                        HRESULT hr = D3DXDisassembleShader((DWORD*) pbFunc.data(), FALSE, NULL, &pShaderAsm);
                        if(SUCCEEDED(hr) && pShaderAsm) {
                            source = (char*) pShaderAsm->GetBufferPointer();
                        }
                        SAFE_RELEASE(pShaderAsm);
                        if(use == SU_EASM) {
                            this->editedAsm = source;
                        }
                    }
                }
                SAFE_RELEASE(bf1);
                SAFE_RELEASE(bf4);
                return hr2;
            }
            else {
                Log::Error("Failed to Assemble pixel shader asm: " + oName);
                Log::Error((char*) bf4->GetBufferPointer());
                SAFE_RELEASE(bf1);
                SAFE_RELEASE(bf4);
            }
            return hr;
        }
        case PS_FX:
        {
            HRESULT hr3 = 0;
            if(entryFunction.length() > 1) {
                hr3 = D3DXCompileShader(source.c_str(), source.length(), 0, 0, entryFunction.c_str(), "ps_3_0", 0, &bf3, &bf4, &ppConstantTable);
                if(hr3 != S_OK)
                    hr3 = D3DXCompileShader(source.c_str(), source.length(), 0, 0, "main", "ps_3_0", 0, &bf3, &bf4, &ppConstantTable);
            }
            else
                hr3 = D3DXCompileShader(source.c_str(), source.length(), 0, 0, "main", "ps_3_0", 0, &bf3, &bf4, &ppConstantTable);
            HRESULT hr2 = S_FALSE;
            IDirect3DPixelShader9* shader = nullptr;
            if(hr3 == S_OK) {
                hr2 = m_pDeviceEx->CreatePixelShader2((DWORD*) bf3->GetBufferPointer(), &shader, SC_NEW);
                if(hr2 != S_OK || !shader) {
                    Log::Error("Failed to create pixel shader hlsl: " + oName);
                    SAFE_RELEASE(shader);
                }
                else {
                    SAFE_RELEASE(compiledShaders[INT(use)]);
                    compiledShaders[INT(use)] = shader;
                    {
                        static std::vector<uint8_t> pbFunc;
                        UINT len;
                        shader->GetFunction(nullptr, &len);
                        if(pbFunc.size() < len) {
                            pbFunc.resize(len + len % 4);
                        }
                        shader->GetFunction(pbFunc.data(), &len);
                        std::string source;
                        ID3DXBuffer* pShaderAsm = NULL;
                        HRESULT hr = D3DXDisassembleShader((DWORD*) pbFunc.data(), FALSE, NULL, &pShaderAsm);
                        if(SUCCEEDED(hr) && pShaderAsm) {
                            source = (char*) pShaderAsm->GetBufferPointer();
                        }
                        SAFE_RELEASE(pShaderAsm);
                        if(use == SU_EFX) {
                            this->editedAsm = source;
                        }
                        //m_IDirect3DPixelShader9* pShader2 = static_cast<m_IDirect3DPixelShader9*>(shader);
                        //if(pShader2)
                        //    pShader2->id == id;
                    }
                }
                SAFE_RELEASE(bf1);
                SAFE_RELEASE(bf3);
                SAFE_RELEASE(bf4);
                SAFE_RELEASE(ppConstantTable);
                return hr2;
            }
            else {
                Log::Error("Failed to compile pixel shader hlsl: " + oName);
                Log::Error((char*) bf4->GetBufferPointer());
                SAFE_RELEASE(bf1);
                SAFE_RELEASE(bf3);
                SAFE_RELEASE(bf4);
                SAFE_RELEASE(ppConstantTable);
            }
            return hr3;

            break;
        }
        default:
            return S_FALSE;
    }
    return S_FALSE;
}

HRESULT m_IDirect3DPixelShader9::QueryInterface(THIS_ REFIID riid, void** ppvObj) {
    if((riid == IID_IDirect3DPixelShader9 || riid == IID_IUnknown) && ppvObj) {
        AddRef();

        *ppvObj = this;

        return S_OK;
    }

    HRESULT hr = ProxyInterface->QueryInterface(riid, ppvObj);

    if(SUCCEEDED(hr)) {
        genericQueryInterface(riid, ppvObj, m_pDeviceEx);
    }

    return hr;
}

ULONG m_IDirect3DPixelShader9::AddRef(THIS) {
    return ProxyInterface->AddRef();
}

ULONG m_IDirect3DPixelShader9::Release(THIS) {
    for(int i = 0; i < (int) ps_2.size(); i++) {
        if(ps_2[i] == this) {
            //ps_2[i] = 0;
        }
    }
    for(auto& f : fx_ps) {
        for(int i = 0; i < f.size(); i++) {
            if(f[i] == this) {
                //f[i] = 0;
            }
        }
    }
    return ProxyInterface->Release();
}

HRESULT m_IDirect3DPixelShader9::GetDevice(THIS_ IDirect3DDevice9** ppDevice) {
    if(!ppDevice) {
        return D3DERR_INVALIDCALL;
    }

    m_pDeviceEx->AddRef();

    *ppDevice = m_pDeviceEx;

    return D3D_OK;
}

HRESULT m_IDirect3DPixelShader9::GetFunction(THIS_ void* pData, UINT* pSizeOfData) {
    return ProxyInterface->GetFunction(pData, pSizeOfData);
}

void m_IDirect3DPixelShader9::replaceConstants() {
    for(auto& [key, value] : constantReplace) {
        m_pDeviceEx->GetProxyInterface()->SetPixelShaderConstantF(key, (FLOAT*) value, 1);
    }
}

