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

const char* getFormatName(D3DFORMAT Format) {
    switch(Format) {
        case D3DFMT_UNKNOWN:   return "D3DFMT_UNKNOWN";

        case D3DFMT_R8G8B8:   return  "D3DFMT_R8G8B8";
        case D3DFMT_A8R8G8B8:   return  "D3DFMT_A8R8G8B8";
        case D3DFMT_X8R8G8B8:   return  "D3DFMT_X8R8G8B8";
        case D3DFMT_R5G6B5:   return  "D3DFMT_R5G6B5";
        case D3DFMT_X1R5G5B5:   return  "D3DFMT_X1R5G5B5";
        case D3DFMT_A1R5G5B5:   return  "D3DFMT_A1R5G5B5";
        case D3DFMT_A4R4G4B4:   return  "D3DFMT_A4R4G4B4";
        case D3DFMT_R3G3B2:   return  "D3DFMT_R3G3B2";
        case D3DFMT_A8:   return  "D3DFMT_A8";
        case D3DFMT_A8R3G3B2:   return  "D3DFMT_A8R3G3B2";
        case D3DFMT_X4R4G4B4:   return  "D3DFMT_X4R4G4B4";
        case D3DFMT_A2B10G10R10:   return  "D3DFMT_A2B10G10R10";
        case D3DFMT_A8B8G8R8:   return  "D3DFMT_A8B8G8R8";
        case D3DFMT_X8B8G8R8:   return  "D3DFMT_X8B8G8R8";
        case D3DFMT_G16R16:   return  "D3DFMT_G16R16";
        case D3DFMT_A2R10G10B10:   return  "D3DFMT_A2R10G10B10";
        case D3DFMT_A16B16G16R16:   return  "D3DFMT_A16B16G16R16";

        case D3DFMT_A8P8:   return  "D3DFMT_A8P8";
        case D3DFMT_P8:   return  "D3DFMT_P8";

        case D3DFMT_L8:   return  "D3DFMT_L8";
        case D3DFMT_A8L8:   return  "D3DFMT_A8L8";
        case D3DFMT_A4L4:   return  "D3DFMT_A4L4";

        case D3DFMT_V8U8:   return  "D3DFMT_V8U8";
        case D3DFMT_L6V5U5:   return  "D3DFMT_L6V5U5";
        case D3DFMT_X8L8V8U8:   return  "D3DFMT_X8L8V8U8";
        case D3DFMT_Q8W8V8U8:   return  "D3DFMT_Q8W8V8U8";
        case D3DFMT_V16U16:   return  "D3DFMT_V16U16";
        case D3DFMT_A2W10V10U10:   return  "D3DFMT_A2W10V10U10";

        case D3DFMT_UYVY:   return  "D3DFMT_UYVY";
        case D3DFMT_R8G8_B8G8:   return  "D3DFMT_R8G8_B8G8";
        case D3DFMT_YUY2:   return  "D3DFMT_YUY2";
        case D3DFMT_G8R8_G8B8:   return  "D3DFMT_G8R8_G8B8";
        case D3DFMT_DXT1:   return  "D3DFMT_DXT1";
        case D3DFMT_DXT2:   return  "D3DFMT_DXT2";
        case D3DFMT_DXT3:   return  "D3DFMT_DXT3";
        case D3DFMT_DXT4:   return  "D3DFMT_DXT4";
        case D3DFMT_DXT5:   return  "D3DFMT_DXT5";

        case D3DFMT_D16_LOCKABLE:   return  "D3DFMT_D16_LOCKABLE";
        case D3DFMT_D32:   return  "D3DFMT_D32";
        case D3DFMT_D15S1:   return  "D3DFMT_D15S1";
        case D3DFMT_D24S8:   return  "D3DFMT_D24S8";
        case D3DFMT_D24X8:   return  "D3DFMT_D24X8";
        case D3DFMT_D24X4S4:   return  "D3DFMT_D24X4S4";
        case D3DFMT_D16:   return  "D3DFMT_D16";
            ;
        case D3DFMT_D32F_LOCKABLE:   return  "D3DFMT_D32F_LOCKABLE";
        case D3DFMT_D24FS8:   return  "D3DFMT_D24FS8";

            //case D3DFORMAT(1515474505u)    :    "D3DFORMAT(1515474505u)"
        case MAKEFOURCC('I', 'N', 'T', 'Z'):    return   "INTZ";
            /* D3D9Ex only -- */

            /* Z-Stencil formats valid for CPU access */
        case D3DFMT_D32_LOCKABLE:   return   "D3DFMT_D32_LOCKABLE";
        case D3DFMT_S8_LOCKABLE:   return   "D3DFMT_S8_LOCKABLE";

            /* -- D3D9Ex only */

        case D3DFMT_L16:   return    "D3DFMT_L16";

        case D3DFMT_VERTEXDATA:   return "D3DFMT_VERTEXDATA";
        case D3DFMT_INDEX16:   return "D3DFMT_INDEX16";
        case D3DFMT_INDEX32:   return "D3DFMT_INDEX32";

        case D3DFMT_Q16W16V16U16:   return "D3DFMT_Q16W16V16U16";

        case D3DFMT_MULTI2_ARGB8:   return "D3DFMT_MULTI2_ARGB8";

            // Floating point surface formats

            // s10e5 formats (16-bits per channel)
        case D3DFMT_R16F:   return "D3DFMT_R16F";
        case D3DFMT_G16R16F:   return "D3DFMT_G16R16F";
        case D3DFMT_A16B16G16R16F:   return "D3DFMT_A16B16G16R16F";

            // IEEE s23e8 formats (32-bits per channel)
        case D3DFMT_R32F:   return "D3DFMT_R32F";
        case D3DFMT_G32R32F:   return "D3DFMT_G32R32F";
        case D3DFMT_A32B32G32R32F:   return "D3DFMT_A32B32G32R32F";

        case D3DFMT_CxV8U8:   return "D3DFMT_CxV8U8";

            /* D3D9Ex only -- */

            // Monochrome 1 bit per pixel format
        case D3DFMT_A1:   return "D3DFMT_A1";

            // 2.8 biased fixed point
        case D3DFMT_A2B10G10R10_XR_BIAS:   return "D3DFMT_A2B10G10R10_XR_BIAS";

            // Binary format indicating that the data has no inherent type
        case D3DFMT_BINARYBUFFER:   return "D3DFMT_BINARYBUFFER";

            /* -- D3D9Ex only */

        case D3DFMT_FORCE_DWORD:   return "D3DFMT_FORCE_DWORD";

        default:   return "D3DFMT_UNKNOWN";
    }
    return "D3DFMT_UNKNOWN";
}

HRESULT m_IDirect3DTexture9::QueryInterface(THIS_ REFIID riid, void** ppvObj) {
    if((riid == IID_IDirect3DTexture9 || riid == IID_IUnknown || riid == IID_IDirect3DResource9 || riid == IID_IDirect3DBaseTexture9) && ppvObj) {
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

ULONG m_IDirect3DTexture9::AddRef(THIS) {
    return ProxyInterface->AddRef();
}

ULONG m_IDirect3DTexture9::Release(THIS) {
    if(ProxyInterface) {
        auto hr = ProxyInterface->Release();
        if(hr == 0) {
            delete this;
        }
        return hr;
    }
    else {
        delete this;
    }
}

HRESULT m_IDirect3DTexture9::GetDevice(THIS_ IDirect3DDevice9** ppDevice) {
    if(!ppDevice) {
        return D3DERR_INVALIDCALL;
    }

    m_pDeviceEx->AddRef();

    *ppDevice = m_pDeviceEx;

    return D3D_OK;
}

HRESULT m_IDirect3DTexture9::SetPrivateData(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return ProxyInterface->SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT m_IDirect3DTexture9::GetPrivateData(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return ProxyInterface->GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT m_IDirect3DTexture9::FreePrivateData(THIS_ REFGUID refguid) {
    return ProxyInterface->FreePrivateData(refguid);
}

DWORD m_IDirect3DTexture9::SetPriority(THIS_ DWORD PriorityNew) {
    return ProxyInterface->SetPriority(PriorityNew);
}

DWORD m_IDirect3DTexture9::GetPriority(THIS) {
    return ProxyInterface->GetPriority();
}

void m_IDirect3DTexture9::PreLoad(THIS) {
    return ProxyInterface->PreLoad();
}

D3DRESOURCETYPE m_IDirect3DTexture9::GetType(THIS) {
    return ProxyInterface->GetType();
}

DWORD m_IDirect3DTexture9::SetLOD(THIS_ DWORD LODNew) {
    return ProxyInterface->SetLOD(LODNew);
}

DWORD m_IDirect3DTexture9::GetLOD(THIS) {
    return ProxyInterface->GetLOD();
}

DWORD m_IDirect3DTexture9::GetLevelCount(THIS) {
    return ProxyInterface->GetLevelCount();
}

HRESULT m_IDirect3DTexture9::SetAutoGenFilterType(THIS_ D3DTEXTUREFILTERTYPE FilterType) {
    return ProxyInterface->SetAutoGenFilterType(FilterType);
}

D3DTEXTUREFILTERTYPE m_IDirect3DTexture9::GetAutoGenFilterType(THIS) {
    return ProxyInterface->GetAutoGenFilterType();
}

void m_IDirect3DTexture9::GenerateMipSubLevels(THIS) {
    return ProxyInterface->GenerateMipSubLevels();
}

HRESULT m_IDirect3DTexture9::GetLevelDesc(THIS_ UINT Level, D3DSURFACE_DESC* pDesc) {
    return ProxyInterface->GetLevelDesc(Level, pDesc);
}

IDirect3DSurface9* pSurfaceLevel = nullptr;

HRESULT m_IDirect3DTexture9::GetSurfaceLevel(THIS_ UINT Level, IDirect3DSurface9** ppSurfaceLevel) {
    HRESULT hr = ProxyInterface->GetSurfaceLevel(Level, ppSurfaceLevel);

    if(SUCCEEDED(hr) && ppSurfaceLevel) {
        *ppSurfaceLevel = m_pDeviceEx->ProxyAddressLookupTable->FindAddress<m_IDirect3DSurface9>(*ppSurfaceLevel);
        //if(this->name == std::string("Any depth stencil") && *ppSurfaceLevel) {
        //    pSurfaceLevel = *ppSurfaceLevel;
        //}
    }

    return hr;
}

HRESULT m_IDirect3DTexture9::LockRect(THIS_ UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    return ProxyInterface->LockRect(Level, pLockedRect, pRect, Flags);
}

HRESULT m_IDirect3DTexture9::UnlockRect(THIS_ UINT Level) {
    return ProxyInterface->UnlockRect(Level);
}

HRESULT m_IDirect3DTexture9::AddDirtyRect(THIS_ CONST RECT* pDirtyRect) {
    return ProxyInterface->AddDirtyRect(pDirtyRect);
}
