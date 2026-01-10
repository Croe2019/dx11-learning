#pragma once
#pragma once
#include <DirectXMath.h>
#include <cstdint>

class Camera
{
public:
    void SetLookAt(DirectX::XMFLOAT3 eye, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up);
    void SetLens(float fovYRadians, uint32_t width, uint32_t height, float zn, float zf);

    DirectX::XMMATRIX View() const { return mView; }
    DirectX::XMMATRIX Proj() const { return mProj; }

private:
    void RebuildView();

    DirectX::XMFLOAT3 mEye{ 0,0,-3 };
    DirectX::XMFLOAT3 mAt{ 0,0, 0 };
    DirectX::XMFLOAT3 mUp{ 0,1, 0 };

    float mFovY = DirectX::XMConvertToRadians(60.0f);
    float mZn = 0.1f;
    float mZf = 100.0f;
    uint32_t mW = 1280;
    uint32_t mH = 720;

    DirectX::XMMATRIX mView = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX mProj = DirectX::XMMatrixIdentity();
};
