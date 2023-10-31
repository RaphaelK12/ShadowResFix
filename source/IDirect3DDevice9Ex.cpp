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

HRESULT m_IDirect3DDevice9Ex::QueryInterface(REFIID riid, void** ppvObj) {
    if((riid == IID_IUnknown || riid == WrapperID) && ppvObj) {
        AddRef();

        *ppvObj = this;

        return S_OK;
    }

    HRESULT hr = ProxyInterface->QueryInterface(riid, ppvObj);

    if(SUCCEEDED(hr)) {
        genericQueryInterface(riid, ppvObj, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::GetPixelShaderConstantB(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount) {
    return ProxyInterface->GetPixelShaderConstantB(StartRegister, pConstantData, BoolCount);
}

HRESULT m_IDirect3DDevice9Ex::SetPixelShaderConstantI(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) {
    return ProxyInterface->SetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}

HRESULT m_IDirect3DDevice9Ex::GetPixelShaderConstantI(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount) {
    return ProxyInterface->GetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}

HRESULT m_IDirect3DDevice9Ex::GetVertexShaderConstantB(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount) {
    return ProxyInterface->GetVertexShaderConstantB(StartRegister, pConstantData, BoolCount);
}

HRESULT m_IDirect3DDevice9Ex::SetVertexShaderConstantI(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) {
    return ProxyInterface->SetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}

HRESULT m_IDirect3DDevice9Ex::GetVertexShaderConstantI(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount) {
    return ProxyInterface->GetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}

HRESULT m_IDirect3DDevice9Ex::GetStreamSourceFreq(THIS_ UINT StreamNumber, UINT* Divider) {
    return ProxyInterface->GetStreamSourceFreq(StreamNumber, Divider);
}

HRESULT m_IDirect3DDevice9Ex::GetVertexDeclaration(THIS_ IDirect3DVertexDeclaration9** ppDecl) {
    HRESULT hr = ProxyInterface->GetVertexDeclaration(ppDecl);

    if(SUCCEEDED(hr) && ppDecl) {
        *ppDecl = ProxyAddressLookupTable->FindAddress<m_IDirect3DVertexDeclaration9>(*ppDecl);
    }

    return hr;
}

void m_IDirect3DDevice9Ex::SetCursorPosition(int X, int Y, DWORD Flags) {
    return ProxyInterface->SetCursorPosition(X, Y, Flags);
}

HRESULT m_IDirect3DDevice9Ex::SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) {
    if(pCursorBitmap) {
        pCursorBitmap = static_cast<m_IDirect3DSurface9*>(pCursorBitmap)->GetProxyInterface();
    }

    return ProxyInterface->SetCursorProperties(XHotSpot, YHotSpot, pCursorBitmap);
}

BOOL m_IDirect3DDevice9Ex::ShowCursor(BOOL bShow) {
    return ProxyInterface->ShowCursor(bShow);
}

HRESULT m_IDirect3DDevice9Ex::CreateCubeTexture(THIS_ UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) {
    HRESULT hr = ProxyInterface->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle);

    if(SUCCEEDED(hr) && ppCubeTexture) {
        *ppCubeTexture = new m_IDirect3DCubeTexture9(*ppCubeTexture, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateVolumeTexture(THIS_ UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) {
    HRESULT hr = ProxyInterface->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle);

    if(SUCCEEDED(hr) && ppVolumeTexture) {
        *ppVolumeTexture = new m_IDirect3DVolumeTexture9(*ppVolumeTexture, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::BeginStateBlock() {
    return ProxyInterface->BeginStateBlock();
}

HRESULT m_IDirect3DDevice9Ex::EndStateBlock(THIS_ IDirect3DStateBlock9** ppSB) {
    HRESULT hr = ProxyInterface->EndStateBlock(ppSB);

    if(SUCCEEDED(hr) && ppSB) {
        *ppSB = ProxyAddressLookupTable->FindAddress<m_IDirect3DStateBlock9>(*ppSB);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::GetClipStatus(D3DCLIPSTATUS9* pClipStatus) {
    return ProxyInterface->GetClipStatus(pClipStatus);
}

HRESULT m_IDirect3DDevice9Ex::GetDisplayMode(THIS_ UINT iSwapChain, D3DDISPLAYMODE* pMode) {
    return ProxyInterface->GetDisplayMode(iSwapChain, pMode);
}

HRESULT m_IDirect3DDevice9Ex::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) {
    return ProxyInterface->GetRenderState(State, pValue);
}

HRESULT m_IDirect3DDevice9Ex::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
    return ProxyInterface->GetTransform(State, pMatrix);
}

HRESULT m_IDirect3DDevice9Ex::SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus) {
    return ProxyInterface->SetClipStatus(pClipStatus);
}

HRESULT m_IDirect3DDevice9Ex::SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
    return ProxyInterface->SetTransform(State, pMatrix);
}

void m_IDirect3DDevice9Ex::GetGammaRamp(THIS_ UINT iSwapChain, D3DGAMMARAMP* pRamp) {
    return ProxyInterface->GetGammaRamp(iSwapChain, pRamp);
}

void m_IDirect3DDevice9Ex::SetGammaRamp(THIS_ UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp) {
    return ProxyInterface->SetGammaRamp(iSwapChain, Flags, pRamp);
}

HRESULT m_IDirect3DDevice9Ex::DeletePatch(UINT Handle) {
    return ProxyInterface->DeletePatch(Handle);
}

HRESULT m_IDirect3DDevice9Ex::GetIndices(THIS_ IDirect3DIndexBuffer9** ppIndexData) {
    HRESULT hr = ProxyInterface->GetIndices(ppIndexData);

    if(SUCCEEDED(hr) && ppIndexData) {
        *ppIndexData = ProxyAddressLookupTable->FindAddress<m_IDirect3DIndexBuffer9>(*ppIndexData);
    }

    return hr;
}

UINT m_IDirect3DDevice9Ex::GetAvailableTextureMem() {
    UINT aVRAM = ProxyInterface->GetAvailableTextureMem();
    return aVRAM;
}

HRESULT m_IDirect3DDevice9Ex::GetRasterStatus(THIS_ UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) {
    return ProxyInterface->GetRasterStatus(iSwapChain, pRasterStatus);
}

HRESULT m_IDirect3DDevice9Ex::GetLight(DWORD Index, D3DLIGHT9* pLight) {
    return ProxyInterface->GetLight(Index, pLight);
}

HRESULT m_IDirect3DDevice9Ex::GetLightEnable(DWORD Index, BOOL* pEnable) {
    return ProxyInterface->GetLightEnable(Index, pEnable);
}

HRESULT m_IDirect3DDevice9Ex::GetMaterial(D3DMATERIAL9* pMaterial) {
    return ProxyInterface->GetMaterial(pMaterial);
}

HRESULT m_IDirect3DDevice9Ex::LightEnable(DWORD LightIndex, BOOL bEnable) {
    return ProxyInterface->LightEnable(LightIndex, bEnable);
}

HRESULT m_IDirect3DDevice9Ex::SetLight(DWORD Index, CONST D3DLIGHT9* pLight) {
    return ProxyInterface->SetLight(Index, pLight);
}

HRESULT m_IDirect3DDevice9Ex::SetMaterial(CONST D3DMATERIAL9* pMaterial) {
    return ProxyInterface->SetMaterial(pMaterial);
}

HRESULT m_IDirect3DDevice9Ex::MultiplyTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
    return ProxyInterface->MultiplyTransform(State, pMatrix);
}

HRESULT m_IDirect3DDevice9Ex::ProcessVertices(THIS_ UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) {
    if(pDestBuffer) {
        pDestBuffer = static_cast<m_IDirect3DVertexBuffer9*>(pDestBuffer)->GetProxyInterface();
    }

    if(pVertexDecl) {
        pVertexDecl = static_cast<m_IDirect3DVertexDeclaration9*>(pVertexDecl)->GetProxyInterface();
    }

    return ProxyInterface->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);
}

HRESULT m_IDirect3DDevice9Ex::GetCurrentTexturePalette(UINT* pPaletteNumber) {
    return ProxyInterface->GetCurrentTexturePalette(pPaletteNumber);
}

HRESULT m_IDirect3DDevice9Ex::GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) {
    return ProxyInterface->GetPaletteEntries(PaletteNumber, pEntries);
}

HRESULT m_IDirect3DDevice9Ex::SetCurrentTexturePalette(UINT PaletteNumber) {
    return ProxyInterface->SetCurrentTexturePalette(PaletteNumber);
}

HRESULT m_IDirect3DDevice9Ex::SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    return ProxyInterface->SetPaletteEntries(PaletteNumber, pEntries);
}



HRESULT m_IDirect3DDevice9Ex::GetStreamSource(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* OffsetInBytes, UINT* pStride) {
    HRESULT hr = ProxyInterface->GetStreamSource(StreamNumber, ppStreamData, OffsetInBytes, pStride);

    if(SUCCEEDED(hr) && ppStreamData) {
        *ppStreamData = ProxyAddressLookupTable->FindAddress<m_IDirect3DVertexBuffer9>(*ppStreamData);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture) {
    HRESULT hr = ProxyInterface->GetTexture(Stage, ppTexture);

    if(SUCCEEDED(hr) && ppTexture && *ppTexture) {
        switch((*ppTexture)->GetType()) {
            case D3DRTYPE_TEXTURE:
                *ppTexture = ProxyAddressLookupTable->FindAddress<m_IDirect3DTexture9>(*ppTexture);
                break;
            case D3DRTYPE_VOLUMETEXTURE:
                *ppTexture = ProxyAddressLookupTable->FindAddress<m_IDirect3DVolumeTexture9>(*ppTexture);
                break;
            case D3DRTYPE_CUBETEXTURE:
                *ppTexture = ProxyAddressLookupTable->FindAddress<m_IDirect3DCubeTexture9>(*ppTexture);
                break;
            default:
                return D3DERR_INVALIDCALL;
        }
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
    return ProxyInterface->GetTextureStageState(Stage, Type, pValue);
}

HRESULT m_IDirect3DDevice9Ex::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    return ProxyInterface->SetTextureStageState(Stage, Type, Value);
}

HRESULT m_IDirect3DDevice9Ex::ValidateDevice(DWORD* pNumPasses) {
    return ProxyInterface->ValidateDevice(pNumPasses);
}

HRESULT m_IDirect3DDevice9Ex::GetClipPlane(DWORD Index, float* pPlane) {
    return ProxyInterface->GetClipPlane(Index, pPlane);
}

HRESULT m_IDirect3DDevice9Ex::SetFVF(THIS_ DWORD FVF) {
    return ProxyInterface->SetFVF(FVF);
}

HRESULT m_IDirect3DDevice9Ex::GetFVF(THIS_ DWORD* pFVF) {
    return ProxyInterface->GetFVF(pFVF);
}

HRESULT m_IDirect3DDevice9Ex::SetNPatchMode(THIS_ float nSegments) {
    return ProxyInterface->SetNPatchMode(nSegments);
}

float m_IDirect3DDevice9Ex::GetNPatchMode(THIS) {
    return ProxyInterface->GetNPatchMode();
}

int m_IDirect3DDevice9Ex::GetSoftwareVertexProcessing(THIS) {
    return ProxyInterface->GetSoftwareVertexProcessing();
}

unsigned int m_IDirect3DDevice9Ex::GetNumberOfSwapChains(THIS) {
    return ProxyInterface->GetNumberOfSwapChains();
}

HRESULT m_IDirect3DDevice9Ex::SetSoftwareVertexProcessing(THIS_ BOOL bSoftware) {
    return ProxyInterface->SetSoftwareVertexProcessing(bSoftware);
}

HRESULT m_IDirect3DDevice9Ex::GetScissorRect(THIS_ RECT* pRect) {
    return ProxyInterface->GetScissorRect(pRect);
}

HRESULT m_IDirect3DDevice9Ex::GetSamplerState(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) {
    return ProxyInterface->GetSamplerState(Sampler, Type, pValue);
}

HRESULT m_IDirect3DDevice9Ex::ColorFill(THIS_ IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color) {
    if(pSurface) {
        pSurface = static_cast<m_IDirect3DSurface9*>(pSurface)->GetProxyInterface();
    }

    return ProxyInterface->ColorFill(pSurface, pRect, color);
}

HRESULT m_IDirect3DDevice9Ex::StretchRect(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) {
    if(pSourceSurface) {
        pSourceSurface = static_cast<m_IDirect3DSurface9*>(pSourceSurface)->GetProxyInterface();
    }

    if(pDestSurface) {
        pDestSurface = static_cast<m_IDirect3DSurface9*>(pDestSurface)->GetProxyInterface();
    }

    return ProxyInterface->StretchRect(pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter);
}

HRESULT m_IDirect3DDevice9Ex::SetDialogBoxMode(THIS_ BOOL bEnableDialogs) {
    return ProxyInterface->SetDialogBoxMode(bEnableDialogs);
}

HRESULT m_IDirect3DDevice9Ex::SetConvolutionMonoKernel(THIS_ UINT width, UINT height, float* rows, float* columns) {
    return ProxyInterface->SetConvolutionMonoKernel(width, height, rows, columns);
}

HRESULT m_IDirect3DDevice9Ex::ComposeRects(THIS_ IDirect3DSurface9* pSrc, IDirect3DSurface9* pDst, IDirect3DVertexBuffer9* pSrcRectDescs, UINT NumRects, IDirect3DVertexBuffer9* pDstRectDescs, D3DCOMPOSERECTSOP Operation, int Xoffset, int Yoffset) {
    if(pSrc) {
        pSrc = static_cast<m_IDirect3DSurface9*>(pSrc)->GetProxyInterface();
    }

    if(pDst) {
        pDst = static_cast<m_IDirect3DSurface9*>(pDst)->GetProxyInterface();
    }

    if(pSrcRectDescs) {
        pSrcRectDescs = static_cast<m_IDirect3DVertexBuffer9*>(pSrcRectDescs)->GetProxyInterface();
    }

    if(pDstRectDescs) {
        pDstRectDescs = static_cast<m_IDirect3DVertexBuffer9*>(pDstRectDescs)->GetProxyInterface();
    }

    return ProxyInterface->ComposeRects(pSrc, pDst, pSrcRectDescs, NumRects, pDstRectDescs, Operation, Xoffset, Yoffset);
}

HRESULT m_IDirect3DDevice9Ex::GetGPUThreadPriority(THIS_ INT* pPriority) {
    return ProxyInterface->GetGPUThreadPriority(pPriority);
}

HRESULT m_IDirect3DDevice9Ex::SetGPUThreadPriority(THIS_ INT Priority) {
    return ProxyInterface->SetGPUThreadPriority(Priority);
}

HRESULT m_IDirect3DDevice9Ex::WaitForVBlank(THIS_ UINT iSwapChain) {
    return ProxyInterface->WaitForVBlank(iSwapChain);
}

HRESULT m_IDirect3DDevice9Ex::CheckResourceResidency(THIS_ IDirect3DResource9** pResourceArray, UINT32 NumResources) {
    if(pResourceArray) {
        for(UINT32 i = 0; i < NumResources; i++) {
            if(pResourceArray[i]) {
                switch(pResourceArray[i]->GetType()) {
                    case D3DRTYPE_SURFACE:
                        pResourceArray[i] = static_cast<m_IDirect3DSurface9*>(pResourceArray[i])->GetProxyInterface();
                        break;
                    case D3DRTYPE_VOLUME:
                        pResourceArray[i] = (IDirect3DResource9*)reinterpret_cast<m_IDirect3DVolume9*>(pResourceArray[i])->GetProxyInterface();
                        break;
                    case D3DRTYPE_TEXTURE:
                        pResourceArray[i] = static_cast<m_IDirect3DTexture9*>(pResourceArray[i])->GetProxyInterface();
                        break;
                    case D3DRTYPE_VOLUMETEXTURE:
                        pResourceArray[i] = static_cast<m_IDirect3DVolumeTexture9*>(pResourceArray[i])->GetProxyInterface();
                        break;
                    case D3DRTYPE_CUBETEXTURE:
                        pResourceArray[i] = static_cast<m_IDirect3DCubeTexture9*>(pResourceArray[i])->GetProxyInterface();
                        break;
                    case D3DRTYPE_VERTEXBUFFER:
                        pResourceArray[i] = static_cast<m_IDirect3DVertexBuffer9*>(pResourceArray[i])->GetProxyInterface();
                        break;
                    case D3DRTYPE_INDEXBUFFER:
                        pResourceArray[i] = static_cast<m_IDirect3DIndexBuffer9*>(pResourceArray[i])->GetProxyInterface();
                        break;
                    default:
                        return D3DERR_INVALIDCALL;
                }
            }
        }
    }

    return ProxyInterface->CheckResourceResidency(pResourceArray, NumResources);
}

HRESULT m_IDirect3DDevice9Ex::SetMaximumFrameLatency(THIS_ UINT MaxLatency) {
    return ProxyInterface->SetMaximumFrameLatency(MaxLatency);
}

HRESULT m_IDirect3DDevice9Ex::GetMaximumFrameLatency(THIS_ UINT* pMaxLatency) {
    return ProxyInterface->GetMaximumFrameLatency(pMaxLatency);
}

HRESULT m_IDirect3DDevice9Ex::CheckDeviceState(THIS_ HWND hDestinationWindow) {
    return ProxyInterface->CheckDeviceState(hDestinationWindow);
}

HRESULT m_IDirect3DDevice9Ex::GetDisplayModeEx(THIS_ UINT iSwapChain, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation) {
    return ProxyInterface->GetDisplayModeEx(iSwapChain, pMode, pRotation);
}

HRESULT m_IDirect3DDevice9Ex::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** ppSwapChain) {
    HRESULT hr = ProxyInterface->CreateAdditionalSwapChain(pPresentationParameters, ppSwapChain);

    if(SUCCEEDED(hr) && ppSwapChain) {
        *ppSwapChain = new m_IDirect3DSwapChain9Ex((IDirect3DSwapChain9Ex*) *ppSwapChain, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateRenderTarget(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {
    HRESULT hr = ProxyInterface->CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle);

    if(SUCCEEDED(hr) && ppSurface) {
        *ppSurface = new m_IDirect3DSurface9(*ppSurface, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateDepthStencilSurface(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {
    HRESULT hr = ProxyInterface->CreateDepthStencilSurface(Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle);

    if(SUCCEEDED(hr) && ppSurface) {
        *ppSurface = new m_IDirect3DSurface9(*ppSurface, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateOffscreenPlainSurfaceEx(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage) {
    HRESULT hr = ProxyInterface->CreateOffscreenPlainSurfaceEx(Width, Height, Format, Pool, ppSurface, pSharedHandle, Usage);

    if(SUCCEEDED(hr) && ppSurface) {
        *ppSurface = new m_IDirect3DSurface9(*ppSurface, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateDepthStencilSurfaceEx(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage) {
    HRESULT hr = ProxyInterface->CreateDepthStencilSurfaceEx(Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle, Usage);

    if(SUCCEEDED(hr) && ppSurface) {
        *ppSurface = new m_IDirect3DSurface9(*ppSurface, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateRenderTargetEx(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle, DWORD Usage) {
    HRESULT hr = ProxyInterface->CreateRenderTargetEx(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle, Usage);

    if(SUCCEEDED(hr) && ppSurface) {
        *ppSurface = new m_IDirect3DSurface9(*ppSurface, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateOffscreenPlainSurface(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) {
    HRESULT hr = ProxyInterface->CreateOffscreenPlainSurface(Width, Height, Format, Pool, ppSurface, pSharedHandle);

    if(SUCCEEDED(hr) && ppSurface) {
        *ppSurface = new m_IDirect3DSurface9(*ppSurface, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::GetBackBuffer(THIS_ UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) {
    HRESULT hr = ProxyInterface->GetBackBuffer(iSwapChain, iBackBuffer, Type, ppBackBuffer);

    if(SUCCEEDED(hr) && ppBackBuffer) {
        *ppBackBuffer = ProxyAddressLookupTable->FindAddress<m_IDirect3DSurface9>(*ppBackBuffer);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::GetFrontBufferData(THIS_ UINT iSwapChain, IDirect3DSurface9* pDestSurface) {
    if(pDestSurface) {
        pDestSurface = static_cast<m_IDirect3DSurface9*>(pDestSurface)->GetProxyInterface();
    }

    return ProxyInterface->GetFrontBufferData(iSwapChain, pDestSurface);
}

HRESULT m_IDirect3DDevice9Ex::GetRenderTargetData(THIS_ IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) {
    if(pRenderTarget) {
        pRenderTarget = static_cast<m_IDirect3DSurface9*>(pRenderTarget)->GetProxyInterface();
    }

    if(pDestSurface) {
        pDestSurface = static_cast<m_IDirect3DSurface9*>(pDestSurface)->GetProxyInterface();
    }

    return ProxyInterface->GetRenderTargetData(pRenderTarget, pDestSurface);
}

HRESULT m_IDirect3DDevice9Ex::UpdateSurface(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint) {
    if(pSourceSurface) {
        pSourceSurface = static_cast<m_IDirect3DSurface9*>(pSourceSurface)->GetProxyInterface();
    }

    if(pDestinationSurface) {
        pDestinationSurface = static_cast<m_IDirect3DSurface9*>(pDestinationSurface)->GetProxyInterface();
    }

    return ProxyInterface->UpdateSurface(pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);
}
