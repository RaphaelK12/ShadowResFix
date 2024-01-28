#pragma once

#include "BasicShader.h"

class m_IDirect3DPixelShader9 : public IDirect3DPixelShader9, public AddressLookupTableObject, public basicShader {
private:
    LPDIRECT3DPIXELSHADER9 ProxyInterface;
    m_IDirect3DDevice9Ex* m_pDeviceEx = nullptr;

public:
    IDirect3DPixelShader9* compiledShaders[5] = { 0 };;
    IDirect3DPixelShader9* newShader = nullptr;
    static IDirect3DPixelShader9* dummyShader;
    static float globalConstants[256][4]; // constant table, set with Set*ShaderConstantF

    // new methods
    std::string GetAsm();
    HRESULT compileNewASM();
    HRESULT compileNewFx();
    HRESULT setCompiledShaderToUse(ShaderUse s);
    HRESULT compileShaderSource(std::string source, ShaderType type, ShaderUse use);
    LPDIRECT3DPIXELSHADER9 GetProxyInterface() { return ProxyInterface; }

    m_IDirect3DPixelShader9(LPDIRECT3DPIXELSHADER9 pShader9, m_IDirect3DDevice9Ex* pDevice, ShaderCreationMode extra);
    m_IDirect3DPixelShader9(LPDIRECT3DPIXELSHADER9 pShader9, m_IDirect3DDevice9Ex* pDevice);
    ~m_IDirect3DPixelShader9();


    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /*** IDirect3DPixelShader9 methods ***/
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice);
    STDMETHOD(GetFunction)(THIS_ void* pData, UINT* pSizeOfData);
};
