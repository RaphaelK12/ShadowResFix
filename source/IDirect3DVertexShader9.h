#pragma once

#include "BasicShader.h"

class m_IDirect3DVertexShader9 : public IDirect3DVertexShader9, public AddressLookupTableObject, public basicShader {
private:
	LPDIRECT3DVERTEXSHADER9 ProxyInterface;
	m_IDirect3DDevice9Ex* m_pDeviceEx = nullptr;

public:
	IDirect3DVertexShader9* compiledShaders[5] = { 0 };
	IDirect3DVertexShader9* newShader = nullptr;
	static IDirect3DVertexShader9* dummyShader;

	// new methods
	std::string GetAsm();
	HRESULT compileNewASM();
	HRESULT compileNewFx();
	HRESULT setCompiledShaderToUse(ShaderUse s);
	HRESULT compileShaderSource(std::string source, ShaderType type, ShaderUse use);
	LPDIRECT3DVERTEXSHADER9 GetProxyInterface() { return ProxyInterface; }

	m_IDirect3DVertexShader9(LPDIRECT3DVERTEXSHADER9 pShader9, m_IDirect3DDevice9Ex* pDevice, ShaderCreationMode extra);
	m_IDirect3DVertexShader9(LPDIRECT3DVERTEXSHADER9 pShader9, m_IDirect3DDevice9Ex* pDevice);
	~m_IDirect3DVertexShader9();

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);

	/*** IDirect3DVertexShader9 methods ***/
	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice);
	STDMETHOD(GetFunction)(THIS_ void* pData, UINT* pSizeOfData);
};
