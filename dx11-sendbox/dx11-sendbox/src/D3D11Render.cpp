#include "D3D11Render.h"

struct CBPerFrame
{
    DirectX::XMFLOAT4X4 worldViewProj;
};

void D3D11Renderer::UpdateCB(const DirectX::XMMATRIX& wvp)
{
    using namespace DirectX;
    CBPerFrame cb{};
    XMStoreFloat4x4(&cb.worldViewProj, XMMatrixTranspose(wvp));

    D3D11_MAPPED_SUBRESOURCE ms{};
    if (SUCCEEDED(mContext->Map(mCBPerFrame.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
        memcpy(ms.pData, &cb, sizeof(cb));
        mContext->Unmap(mCBPerFrame.Get(), 0);
    }
    ID3D11Buffer* buf = mCBPerFrame.Get();
    mContext->VSSetConstantBuffers(0, 1, &buf);
}
