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

extern std::vector<uint8_t> patternFF;
extern std::vector<m_IDirect3DVertexShader9*> vs;
extern std::vector<m_IDirect3DVertexShader9*> vs_2;
extern std::vector<m_IDirect3DVertexShader9*> vs_4;
extern std::vector<const char*> shader_names_ps;
extern std::vector<const char*> shader_names_vs;
extern std::vector<const char*> shader_names_fxc;
extern std::vector<std::vector<m_IDirect3DPixelShader9*>> fx_ps;
extern std::vector<std::vector<m_IDirect3DVertexShader9*>> fx_vs;

char shadersrcvs[] =
"\nvs_3_0\n"
"dcl_position v0\n"
"dcl_color v1\n"
"dcl_texcoord v2\n"
"dcl_position o0\n"
"dcl_texcoord o1.xy\n"
"dcl_texcoord9 o10\n"
"dcl_color o2\n"
"def c0, 0, 0, 0, 0\n"
"mov o0, c0\n"
"mov o1.xy, c0.xy\n"
"mov o2, c0\n"
"mov o10, c0\n\n"
;

int getSignature(m_IDirect3DVertexShader9* pShader, std::vector<uint8_t>& pattern) {
    static std::vector<uint8_t> pbFunc;
    UINT len;
    pShader->GetFunction(nullptr, &len);
    if(pbFunc.size() < len) {
        pbFunc.resize(len + len % 4);
    }
    pShader->GetFunction(pbFunc.data(), &len);
    int cnt = 0;
    for(int i = 0; i < (int) pbFunc.size(); i++) {
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

uint32_t getCRC32(m_IDirect3DVertexShader9* pShader) {
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

IDirect3DVertexShader9* m_IDirect3DVertexShader9::dummyShader = nullptr;

extern IDirect3DVertexShader9* SMAA_EdgeDetectionVS;
extern IDirect3DVertexShader9* SMAA_BlendingWeightsCalculationVS;
extern IDirect3DVertexShader9* SMAA_NeighborhoodBlendingVS;

extern IDirect3DPixelShader9* DOF_ps;
extern IDirect3DPixelShader9* SSAO_ps;
extern IDirect3DVertexShader9* SSAO_vs;

float m_IDirect3DVertexShader9::globalConstants[256][4] = { {0} }; // constant table, set with Set*ShaderConstantF

static int loadCounter = 0;

std::string getNameFromCRC_vs(UINT crc32) {
    for(int i = 0; i < (int) crclist_vs.size(); i++) {
        if(crclist_vs[i]->crc32 == crc32) {
            return crclist_vs[i]->name;
        }
    }
    return "";
}

int getIdFromCRC_vs(UINT crc32) {
    int id = -1;
    auto name = getNameFromCRC_vs(crc32);
    for(int i = 0; i < (int) shader_names_vs.size(); i++) {
        if(name == shader_names_vs[i]) {
            id = i+2000;
            break;
        }
    }
    return id;
}

int getIdFromOrderCRC_vs(int cnt, UINT crc32) {
    int id = -1;
    if(cnt < (int) crclist_vs.size()) {
        // exact cnt, precise
        if(crclist_vs[cnt]->crc32 == crc32)
            return crclist_vs[cnt]->id+2000;
        // cnt ahead, less precise, but generaly works
        else {
            for(int i = max(0, cnt - 1); i < (int) crclist_vs.size(); i++) {
                if(crclist_vs[i]->crc32 == crc32) {
                    loadCounter = i;
                    return crclist_vs[i]->id+2000;
                }
            }
        }
    }

    // just another way to get the id, does this work?
    for(int i = 0; i < (int) crclist_vs.size(); i++) {
        if(crclist_vs[i]->crc32 == crc32) {
            return crclist_vs[i]->id+2000;
        }
    }

    // just a brute way to get the id, sometimes works
    auto name = getNameFromCRC_vs(crc32);
    for(int i = 0; i < (int) shader_names_vs.size(); i++) {
        if(name == shader_names_vs[i]) {
            id = i+2000;
            break;
        }
    }
    return id;
}


m_IDirect3DVertexShader9::m_IDirect3DVertexShader9(LPDIRECT3DVERTEXSHADER9 pShader9, m_IDirect3DDevice9Ex* pDevice, ShaderCreationMode extra) :
    ProxyInterface(pShader9), m_pDeviceEx(pDevice) {
    pDevice->ProxyAddressLookupTable->SaveAddress(this, ProxyInterface);
    static char buf100[100] = { 0 };
    id = getSignature(this, patternFF);
    crc32 = getCRC32(this);
    sprintf(buf100, "vs_%x.asm", crc32);
    if(id == -1) {
        id = getIdFromOrderCRC_vs(loadCounter, crc32);
        //getNameFromCRC(crc32);
    }
    if(id == -1) {
        id = getIdFromCRC_vs(crc32);
    }
    else
        loadCounter++;

    fxcAsm = GetAsm();
    if(id >= 0 && id - 2000 >= (int) shader_names_vs.size()) {
        Log::Error("illegal vs id: " + std::to_string(id));
    }
    if(id >= 0 && id - 2000 >= 0 && id - 2000 < (int) shader_names_vs.size()) {
        id = id - 2000;
        oName = shader_names_vs[id];
        fxid = getfxid(id, oName);
        fxName = shader_names_fxc[fxid];
        oName = oName.substr(oName.find_last_of("/") + 1);
        if(extra != SC_NEW) {
            loadedFx = LoadFX(fxName, oName);
            loadedAsm = LoadASM(fxName, oName);
        }
        if(fxid<0 || fxid>(int)fx_vs.size()) {
            Log::Error("fxid not found");
        }
        else if(extra == SC_FXC || extra == SC_GAME) {
            fx_vs[fxid].push_back(this);
            vs[id] = this;
        }
        else {
            if(extra != SC_NEW) {
                vs_2.push_back(this);
            }
        }
        //for(auto& i : shadowGen) {
        //    if(i == oName) {
        //        useBias = true;
        //    }
        //}
        if(extra != SC_NEW) {
            if(loadedAsm.length() > 1) {
                auto hr = compileShaderSource(loadedAsm, VS_ASM, SU_LASM);
                if(hr != S_OK) {
                    //Log::Warning("Unable to compile Loaded ASM: " + oName);
                }
                else {
                    //usingShader = SU_LASM;
                }
            }
            if(loadedFx.length() > 1) {
                auto hr = compileShaderSource(loadedFx, VS_FX, SU_LFX);
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
            vs_2.push_back(this);
            loadedFx = LoadFX(fxName, oName);
            loadedAsm = LoadASM(fxName, oName);
            if(loadedAsm.length() > 1) {
                auto hr = compileShaderSource(loadedAsm, VS_ASM, SU_LASM);
                if(hr != S_OK) {
                    //Log::Warning("Unable to compile Loaded ASM: " + oName);
                }
                else {
                    //usingShader = SU_LASM;
                }
            }
            if(loadedFx.length() > 1) {
                auto hr = compileShaderSource(loadedFx, VS_FX, SU_LFX);
                if(hr != S_OK) {
                    //Log::Warning("Unable to compile Loaded HLSL: " + oName);
                }
                else {
                    //usingShader = SU_LFX;
                }
            }
        }
    }
    if(!dummyShader) {
        HRESULT hr2 = S_FALSE;
        ID3DXBuffer* bf1 = nullptr;
        ID3DXBuffer* bf2 = nullptr;
        HRESULT hr1 = D3DXAssembleShader(shadersrcvs, sizeof(shadersrcvs), 0, 0, 0, &bf1, &bf2);
        if(hr1 == S_OK) {
            int sz = bf1->GetBufferSize();
            char* p = (char*) bf1->GetBufferPointer();
            hr2 = m_pDeviceEx->CreateVertexShader2((DWORD*) p, &dummyShader, SC_NEW);
            if(dummyShader) {
                ((m_IDirect3DVertexShader9*) dummyShader)->disable = true;
                ((m_IDirect3DVertexShader9*) dummyShader)->oName = "dummyvs";
            }
        }
        if(hr1 != S_OK || hr2 != S_OK) {
            Log::Error("Unable to compile Dummy Vertex shader");
        }
        SAFE_RELEASE(bf1);
        SAFE_RELEASE(bf2);
    }

}

m_IDirect3DVertexShader9::m_IDirect3DVertexShader9(LPDIRECT3DVERTEXSHADER9 pShader9, m_IDirect3DDevice9Ex* pDevice) :
    ProxyInterface(pShader9), m_pDeviceEx(pDevice) {
    static char buf100[100] = { 0 };
    pDevice->ProxyAddressLookupTable->SaveAddress(this, ProxyInterface);
    id = getSignature(this, patternFF);
    crc32 = getCRC32(this);
    sprintf(buf100, "vs_%x.asm", crc32);

    fxcAsm = GetAsm();
    if(id >= 0 && id - 2000 >= (int) shader_names_vs.size()) {
        Log::Error("illegal vs id: " + std::to_string(id));
    }
    if(id >= 0 && id - 2000 < (int) shader_names_vs.size()) {
        id = id - 2000;
        oName = shader_names_vs[id];
        fxid = getfxid(id, oName);
        fxName = shader_names_fxc[fxid];
        oName = oName.substr(oName.find_last_of("/") + 1);
        loadedFx = LoadFX(fxName, oName);
        loadedAsm = LoadASM(fxName, oName);
        if(fxid < 0 || fxid > (int)fx_vs.size()) {
            Log::Error("fxid not found");
        }
        else {
            fx_vs[fxid].push_back(this);
            vs[id] = this;
            //for(auto& i : shadowGen) {
            //    if(i == oName) {
            //        useBias = true;
            //    }
            //}
        }
        //printf("%i %i %s\n", id, fxid, oName.c_str());
    }
    else {
        fxName = "nonamed";
        oName = buf100;
        vs_2.push_back(this);
        loadedFx = LoadFX(fxName, oName);
        loadedAsm = LoadASM(fxName, oName);
        if(loadedAsm.length() > 1) {
            auto hr = compileShaderSource(loadedAsm, VS_ASM, SU_LASM);
            if(hr != S_OK) {
                //Log::Warning("Unable to compile Loaded ASM: " + oName);
            }
            else {
                //usingShader = SU_LASM;
            }
        }
        if(loadedFx.length() > 1) {
            auto hr = compileShaderSource(loadedFx, VS_FX, SU_LFX);
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
        HRESULT hr1 = D3DXAssembleShader(shadersrcvs, sizeof(shadersrcvs), 0, 0, 0, &bf1, &bf2);
        if(hr1 == S_OK) {
            int sz = bf1->GetBufferSize();
            char* p = (char*) bf1->GetBufferPointer();
            hr2 = m_pDeviceEx->CreateVertexShader2((DWORD*) p, &dummyShader, SC_NEW);
            if(dummyShader) {
                ((m_IDirect3DVertexShader9*) dummyShader)->disable = true;
                ((m_IDirect3DVertexShader9*) dummyShader)->oName = "dummyvs";
            }
        }
        if(hr1 != S_OK || hr2 != S_OK) {
            Log::Error("Unable to compile Dummy Vertex shader");
        }
        SAFE_RELEASE(bf1);
        SAFE_RELEASE(bf2);
    }
}

m_IDirect3DVertexShader9::~m_IDirect3DVertexShader9() {
    if(id >= 0) {
        vs[id] = nullptr;
    }
}

std::string m_IDirect3DVertexShader9::GetAsm() {
    m_IDirect3DVertexShader9* pShader = this;
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

HRESULT m_IDirect3DVertexShader9::compileNewASM() {
    ID3DXBuffer* bf1 = nullptr;
    ID3DXBuffer* bf4 = nullptr;
    HRESULT hr = D3DXAssembleShader(editedAsm.c_str(), editedAsm.length(), 0, 0, 0, &bf1, &bf4);
    HRESULT hr2 = S_FALSE;
    if(hr == S_OK) {
        hr2 = m_pDeviceEx->GetProxyInterface()->CreateVertexShader((DWORD*) bf1->GetBufferPointer(), /*(IDirect3DPixelShader9**)(m_IDirect3DPixelShader9**)*/ &newShader);
        if(hr2 != S_OK) {
            Log::Error("failed to Compile new vertex shader asm!");
        }
        return hr2;
    }
    else {
        Log::Error("failed to Assemble new vertex shader asm:");
        Log::Text((char*) bf4->GetBufferPointer());
    }
    SAFE_RELEASE(bf4);
    return hr;
}

HRESULT m_IDirect3DVertexShader9::compileNewFx() {
    ID3DXBuffer* bf1 = nullptr;
    ID3DXBuffer* bf3 = nullptr;
    ID3DXBuffer* bf4 = nullptr;
    ID3DXConstantTable* ppConstantTable = nullptr;
    HRESULT hr1 = D3DXCompileShader(editedFx.c_str(), editedFx.length(), 0, 0, "main", "vs_3_0", 0, &bf3, &bf4, &ppConstantTable);
    HRESULT hr2 = S_FALSE;
    if(hr1 == S_OK) {
        hr2 = m_pDeviceEx->GetProxyInterface()->CreateVertexShader((DWORD*) bf3->GetBufferPointer(), &newShader);
        if(hr2 != S_OK) {
            Log::Error("failed to Compile new vertex shader fx!");
        }
        SAFE_RELEASE(bf1);
        SAFE_RELEASE(bf3);
        SAFE_RELEASE(bf4);
        return hr2;
    }
    else {
        Log::Error("failed to Compile new vertex shader fx:");
        Log::Text((char*) bf4->GetBufferPointer());
    }
    printf("%i %i \n", hr2, hr1);
    SAFE_RELEASE(bf1);
    SAFE_RELEASE(bf3);
    SAFE_RELEASE(bf4);
    return hr1;
}

HRESULT m_IDirect3DVertexShader9::setCompiledShaderToUse(ShaderUse s) {
    if(compiledShaders[UINT(s)] || s == SU_FXC) {
        usingShader = s;
        return S_OK;
    }
    return S_FALSE;
}

HRESULT m_IDirect3DVertexShader9::compileShaderSource(std::string source, ShaderType type, ShaderUse use) {
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
    IDirect3DVertexShader9* shader = nullptr;
    switch(type) {
        case VS_ASM:
        {
            HRESULT hr = D3DXAssembleShader(source.c_str(), source.length(), 0, 0, 0, &bf1, &bf4);
            if(hr == S_OK) {
                HRESULT hr2 = m_pDeviceEx->CreateVertexShader2((DWORD*) bf1->GetBufferPointer(), &shader, SC_NEW);
                if(hr2 != S_OK || !shader) {
                    Log::Error("Failed to create vertex shader asm: " + oName);
                    SAFE_RELEASE(shader);
                }
                else {
                    SAFE_RELEASE(compiledShaders[INT(use)]);
                    compiledShaders[INT(use)] = shader;
                }
                SAFE_RELEASE(bf1);
                SAFE_RELEASE(bf4);
                return hr2;
            }
            else {
                Log::Error("Failed to Assemble vertex shader asm: " + oName);
                Log::Error((char*) bf4->GetBufferPointer());
                SAFE_RELEASE(bf1);
                SAFE_RELEASE(bf4);
            }
            return hr;
        }
        case VS_FX:
        {
            HRESULT hr3 = D3DXCompileShader(source.c_str(), source.length(), 0, 0, "main", "vs_3_0", 0, &bf3, &bf4, &ppConstantTable);
            HRESULT hr2 = S_FALSE;
            if(hr3 == S_OK) {
                hr2 = m_pDeviceEx->CreateVertexShader2((DWORD*) bf3->GetBufferPointer(), &shader, SC_NEW);
                if(hr2 != S_OK || !shader) {
                    Log::Error("Failed to create vertex shader hlsl: " + oName);
                    SAFE_RELEASE(shader);
                }
                else {
                    SAFE_RELEASE(compiledShaders[INT(use)]);
                    compiledShaders[INT(use)] = shader;
                }
                SAFE_RELEASE(bf1);
                SAFE_RELEASE(bf3);
                SAFE_RELEASE(bf4);
                SAFE_RELEASE(ppConstantTable);
                return hr2;
            }
            else {
                Log::Error("Failed to compile vertex shader hlsl: " + oName);
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

HRESULT m_IDirect3DVertexShader9::QueryInterface(THIS_ REFIID riid, void** ppvObj) {
    if((riid == IID_IDirect3DVertexShader9 || riid == IID_IUnknown) && ppvObj) {
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

ULONG m_IDirect3DVertexShader9::AddRef(THIS) {
    return ProxyInterface->AddRef();
}

ULONG m_IDirect3DVertexShader9::Release(THIS) {
    ULONG cnt = ProxyInterface->Release();
    if(cnt == 0) {
        for(int i = 0; i < (int) vs_2.size(); i++) {
            if(vs_2[i] == this) {
                vs_2[i] = 0;
                //delete this;
            }
        }
    }
    return cnt;
}

HRESULT m_IDirect3DVertexShader9::GetDevice(THIS_ IDirect3DDevice9** ppDevice) {
    if(!ppDevice) {
        return D3DERR_INVALIDCALL;
    }

    m_pDeviceEx->AddRef();

    *ppDevice = m_pDeviceEx;

    return D3D_OK;
}

HRESULT m_IDirect3DVertexShader9::GetFunction(THIS_ void* pData, UINT* pSizeOfData) {
    return ProxyInterface->GetFunction(pData, pSizeOfData);
}
