#pragma once
#pragma once
#include <wrl/client.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include <cstdint>

class D3D11Renderer
{
public:
    D3D11Renderer() = default;
    void Init(HWND hwnd, uint32_t w, uint32_t h);
    void OnResize(uint32_t w, uint32_t h);

    void BeginFrame();
    void EndFrame();

    void UpdateCB(const DirectX::XMMATRIX& wvp); // Ç†Ç»ÇΩÇÃä÷êîÇÉÅÉìÉoÅ[âª
    void DrawTriangle();

private:
    void CreateSwapChainAndDevice(HWND hwnd);
    void CreateRTVDSV(uint32_t w, uint32_t h);
    void CreatePipeline(); // VS/PS/InputLayout/VB/CB/DepthState

private:
    Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;

    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> mRTV;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> mDepth;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> mDSV;

    Microsoft::WRL::ComPtr<ID3D11Buffer> mVB;
    Microsoft::WRL::ComPtr<ID3D11Buffer> mCBPerFrame;

    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
};
