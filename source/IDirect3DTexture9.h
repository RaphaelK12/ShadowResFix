#pragma once

extern std::list<m_IDirect3DTexture9*> textureList;

extern const char* getFormatName(D3DFORMAT Format);

class m_IDirect3DTexture9 : public IDirect3DTexture9, public AddressLookupTableObject {
private:
    LPDIRECT3DTEXTURE9 ProxyInterface;
    m_IDirect3DDevice9Ex* m_pDeviceEx = nullptr;

public:
    UINT Width;
    UINT Height;
    UINT Levels;
    DWORD Usage;
    D3DFORMAT Format;
    D3DPOOL Pool;
    const char* FormatName;
    std::string name;
    UINT useCounter = 0;

    m_IDirect3DTexture9(LPDIRECT3DTEXTURE9 pTexture9, m_IDirect3DDevice9Ex* pDevice) : ProxyInterface(pTexture9), m_pDeviceEx(pDevice),
        Width(0), Height(0), Levels(0), Usage(0), Format(D3DFMT_UNKNOWN), Pool(D3DPOOL_FORCE_DWORD), name() {
        FormatName = getFormatName(Format);
        pDevice->ProxyAddressLookupTable->SaveAddress(this, ProxyInterface);
    }
    m_IDirect3DTexture9(LPDIRECT3DTEXTURE9 pTexture9, m_IDirect3DDevice9Ex* pDevice, UINT _Width, UINT _Height, UINT _Levels, DWORD _Usage, D3DFORMAT _Format, D3DPOOL _Pool, std::string& _name, bool addToList = false) : ProxyInterface(pTexture9), m_pDeviceEx(pDevice),
        Width(_Width), Height(_Height), Levels(_Levels), Usage(_Usage), Format(_Format), Pool(_Pool), name(_name) {
        FormatName = getFormatName(Format);
        pDevice->ProxyAddressLookupTable->SaveAddress(this, ProxyInterface);
        if(addToList)
            textureList.push_back(this);
    }
    ~m_IDirect3DTexture9() {
        textureList.remove(this);
    }

    LPDIRECT3DTEXTURE9 GetProxyInterface() { return ProxyInterface; }

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /*** IDirect3DBaseTexture9 methods ***/
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice);
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags);
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData);
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid);
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew);
    STDMETHOD_(DWORD, GetPriority)(THIS);
    STDMETHOD_(void, PreLoad)(THIS);
    STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS);
    STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew);
    STDMETHOD_(DWORD, GetLOD)(THIS);
    STDMETHOD_(DWORD, GetLevelCount)(THIS);
    STDMETHOD(SetAutoGenFilterType)(THIS_ D3DTEXTUREFILTERTYPE FilterType);
    STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)(THIS);
    STDMETHOD_(void, GenerateMipSubLevels)(THIS);
    STDMETHOD(GetLevelDesc)(THIS_ UINT Level, D3DSURFACE_DESC* pDesc);
    STDMETHOD(GetSurfaceLevel)(THIS_ UINT Level, IDirect3DSurface9** ppSurfaceLevel);
    STDMETHOD(LockRect)(THIS_ UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags);
    STDMETHOD(UnlockRect)(THIS_ UINT Level);
    STDMETHOD(AddDirtyRect)(THIS_ CONST RECT* pDirtyRect);
};
