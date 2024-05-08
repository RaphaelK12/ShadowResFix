#include "d3d9.h"

// gtaiv used functions

ULONG m_IDirect3DDevice9Ex::AddRef() {
    return ProxyInterface->AddRef();
}

ULONG m_IDirect3DDevice9Ex::Release() {
    ULONG count = ProxyInterface->Release();

    if(count == 0) {
        delete this;
    }

    return count;
}

HRESULT m_IDirect3DDevice9Ex::GetVertexShader(THIS_ IDirect3DVertexShader9** ppShader) {
    HRESULT hr = ProxyInterface->GetVertexShader(ppShader);

    if(SUCCEEDED(hr) && ppShader && *ppShader != m_IDirect3DVertexShader9::dummyShader) {
        *ppShader = ProxyAddressLookupTable->FindAddress<m_IDirect3DVertexShader9>(*ppShader);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::GetPixelShader(THIS_ IDirect3DPixelShader9** ppShader) {
    HRESULT hr = ProxyInterface->GetPixelShader(ppShader);

    if(SUCCEEDED(hr) && ppShader && *ppShader != m_IDirect3DPixelShader9::dummyShader) {
        *ppShader = ProxyAddressLookupTable->FindAddress<m_IDirect3DPixelShader9>(*ppShader);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetPixelShaderConstantB(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT  BoolCount) {
    return ProxyInterface->SetPixelShaderConstantB(StartRegister, pConstantData, BoolCount);
}

HRESULT m_IDirect3DDevice9Ex::GetPixelShaderConstantF(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount) {
    return ProxyInterface->GetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}

HRESULT m_IDirect3DDevice9Ex::SetVertexShaderConstantB(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT  BoolCount) {
    //static BOOL b8[4] = { TRUE, TRUE, TRUE, TRUE };
    //if(StartRegister == 8 && BoolCount == 1) {
    //    return ProxyInterface->SetVertexShaderConstantB(StartRegister, b8, BoolCount);
    //}
    return ProxyInterface->SetVertexShaderConstantB(StartRegister, pConstantData, BoolCount);
}

HRESULT m_IDirect3DDevice9Ex::GetVertexShaderConstantF(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount) {
    return ProxyInterface->GetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}

HRESULT m_IDirect3DDevice9Ex::SetStreamSourceFreq(THIS_ UINT StreamNumber, UINT Divider) {
    return ProxyInterface->SetStreamSourceFreq(StreamNumber, Divider);
}

HRESULT m_IDirect3DDevice9Ex::CreateVertexDeclaration(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) {
    HRESULT hr = ProxyInterface->CreateVertexDeclaration(pVertexElements, ppDecl);

    if(SUCCEEDED(hr) && ppDecl) {
        *ppDecl = new m_IDirect3DVertexDeclaration9(*ppDecl, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetVertexDeclaration(THIS_ IDirect3DVertexDeclaration9* pDecl) {
    if(pDecl) {
        pDecl = static_cast<m_IDirect3DVertexDeclaration9*>(pDecl)->GetProxyInterface();
    }

    return ProxyInterface->SetVertexDeclaration(pDecl);
}

HRESULT m_IDirect3DDevice9Ex::CreateIndexBuffer(THIS_ UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) {
    HRESULT hr = ProxyInterface->CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);

    if(SUCCEEDED(hr) && ppIndexBuffer) {
        *ppIndexBuffer = new m_IDirect3DIndexBuffer9(*ppIndexBuffer, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateVertexBuffer(THIS_ UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) {
    HRESULT hr = ProxyInterface->CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);

    if(SUCCEEDED(hr) && ppVertexBuffer) {
        *ppVertexBuffer = new m_IDirect3DVertexBuffer9(*ppVertexBuffer, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::CreateStateBlock(THIS_ D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) {
    HRESULT hr = ProxyInterface->CreateStateBlock(Type, ppSB);

    if(SUCCEEDED(hr) && ppSB) {
        *ppSB = new m_IDirect3DStateBlock9(*ppSB, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::SetIndices(THIS_ IDirect3DIndexBuffer9* pIndexData) {
    if(pIndexData) {
        pIndexData = static_cast<m_IDirect3DIndexBuffer9*>(pIndexData)->GetProxyInterface();
    }

    return ProxyInterface->SetIndices(pIndexData);
}

HRESULT m_IDirect3DDevice9Ex::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) {
    return ProxyInterface->GetCreationParameters(pParameters);
}

HRESULT m_IDirect3DDevice9Ex::GetDeviceCaps(D3DCAPS9* pCaps) {
    return ProxyInterface->GetDeviceCaps(pCaps);
}

HRESULT m_IDirect3DDevice9Ex::GetDirect3D(IDirect3D9** ppD3D9) {
    if(ppD3D9) {
        m_pD3DEx->AddRef();

        *ppD3D9 = m_pD3DEx;

        return D3D_OK;
    }
    return D3DERR_INVALIDCALL;
}

HRESULT m_IDirect3DDevice9Ex::TestCooperativeLevel() {
    return ProxyInterface->TestCooperativeLevel();
}

HRESULT m_IDirect3DDevice9Ex::SetStreamSource(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) {
    if(pStreamData) {
        pStreamData = static_cast<m_IDirect3DVertexBuffer9*>(pStreamData)->GetProxyInterface();
    }

    return ProxyInterface->SetStreamSource(StreamNumber, pStreamData, OffsetInBytes, Stride);
}

HRESULT m_IDirect3DDevice9Ex::UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) {
    if(pSourceTexture) {
        switch(pSourceTexture->GetType()) {
            case D3DRTYPE_TEXTURE:
                pSourceTexture = static_cast<m_IDirect3DTexture9*>(pSourceTexture)->GetProxyInterface();
                break;
            case D3DRTYPE_VOLUMETEXTURE:
                pSourceTexture = static_cast<m_IDirect3DVolumeTexture9*>(pSourceTexture)->GetProxyInterface();
                break;
            case D3DRTYPE_CUBETEXTURE:
                pSourceTexture = static_cast<m_IDirect3DCubeTexture9*>(pSourceTexture)->GetProxyInterface();
                break;
            default:
                return D3DERR_INVALIDCALL;
        }
    }
    if(pDestinationTexture) {
        switch(pDestinationTexture->GetType()) {
            case D3DRTYPE_TEXTURE:
                pDestinationTexture = static_cast<m_IDirect3DTexture9*>(pDestinationTexture)->GetProxyInterface();
                break;
            case D3DRTYPE_VOLUMETEXTURE:
                pDestinationTexture = static_cast<m_IDirect3DVolumeTexture9*>(pDestinationTexture)->GetProxyInterface();
                break;
            case D3DRTYPE_CUBETEXTURE:
                pDestinationTexture = static_cast<m_IDirect3DCubeTexture9*>(pDestinationTexture)->GetProxyInterface();
                break;
            default:
                return D3DERR_INVALIDCALL;
        }
    }

    return ProxyInterface->UpdateTexture(pSourceTexture, pDestinationTexture);
}

HRESULT m_IDirect3DDevice9Ex::SetClipPlane(DWORD Index, CONST float* pPlane) {
    return ProxyInterface->SetClipPlane(Index, pPlane);
}

HRESULT m_IDirect3DDevice9Ex::GetViewport(D3DVIEWPORT9* pViewport) {
    return ProxyInterface->GetViewport(pViewport);
}

HRESULT m_IDirect3DDevice9Ex::SetViewport(CONST D3DVIEWPORT9* pViewport) {
    return ProxyInterface->SetViewport(pViewport);
}

HRESULT m_IDirect3DDevice9Ex::CreateQuery(THIS_ D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) {
    HRESULT hr = ProxyInterface->CreateQuery(Type, ppQuery);

    if(SUCCEEDED(hr) && ppQuery) {
        *ppQuery = new m_IDirect3DQuery9(*ppQuery, this);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::EvictManagedResources(THIS) {
    return ProxyInterface->EvictManagedResources();
}

HRESULT m_IDirect3DDevice9Ex::SetScissorRect(THIS_ CONST RECT* pRect) {
    return ProxyInterface->SetScissorRect(pRect);
}

HRESULT m_IDirect3DDevice9Ex::SetSamplerState(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) {
    return ProxyInterface->SetSamplerState(Sampler, Type, Value);
}

HRESULT m_IDirect3DDevice9Ex::GetSwapChain(THIS_ UINT iSwapChain, IDirect3DSwapChain9** ppSwapChain) {
    HRESULT hr = ProxyInterface->GetSwapChain(iSwapChain, ppSwapChain);

    if(SUCCEEDED(hr) && ppSwapChain) {
        *ppSwapChain = ProxyAddressLookupTable->FindAddress<m_IDirect3DSwapChain9Ex>(*ppSwapChain);
    }

    return hr;
}

extern IDirect3DSurface9* pSurfaceLevel;
HRESULT m_IDirect3DDevice9Ex::SetDepthStencilSurface(THIS_ IDirect3DSurface9* pNewZStencil) {
    //if(pSurfaceLevel && pSurfaceLevel == pNewZStencil) {
    //	printf("depth");
    //}

    if(pNewZStencil) {
        pNewZStencil = static_cast<m_IDirect3DSurface9*>(pNewZStencil)->GetProxyInterface();
    }

    return ProxyInterface->SetDepthStencilSurface(pNewZStencil);
}

HRESULT m_IDirect3DDevice9Ex::SetRenderTarget(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) {
    if(pRenderTarget) {
        pRenderTarget = static_cast<m_IDirect3DSurface9*>(pRenderTarget)->GetProxyInterface();
    }

    return ProxyInterface->SetRenderTarget(RenderTargetIndex, pRenderTarget);
}

HRESULT m_IDirect3DDevice9Ex::GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface) {
    HRESULT hr = ProxyInterface->GetDepthStencilSurface(ppZStencilSurface);

    if(SUCCEEDED(hr) && ppZStencilSurface) {
        *ppZStencilSurface = ProxyAddressLookupTable->FindAddress<m_IDirect3DSurface9>(*ppZStencilSurface);
    }

    return hr;
}

HRESULT m_IDirect3DDevice9Ex::GetRenderTarget(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) {
    HRESULT hr = ProxyInterface->GetRenderTarget(RenderTargetIndex, ppRenderTarget);

    if(SUCCEEDED(hr) && ppRenderTarget) {
        *ppRenderTarget = ProxyAddressLookupTable->FindAddress<m_IDirect3DSurface9>(*ppRenderTarget);
    }

    return hr;
}
