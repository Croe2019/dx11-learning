#include "Camera.h"
using namespace DirectX;

void Camera::SetLookAt(XMFLOAT3 eye, XMFLOAT3 at, XMFLOAT3 up)
{
    mEye = eye; mAt = at; mUp = up;
    RebuildView();
}

void Camera::RebuildView()
{
    mView = XMMatrixLookAtLH(XMLoadFloat3(&mEye), XMLoadFloat3(&mAt), XMLoadFloat3(&mUp));
}

void Camera::SetLens(float fovYRadians, uint32_t width, uint32_t height, float zn, float zf)
{
    mFovY = fovYRadians; mW = width; mH = height; mZn = zn; mZf = zf;
    float aspect = static_cast<float>(mW) / static_cast<float>(mH);
    mProj = XMMatrixPerspectiveFovLH(mFovY, aspect, mZn, mZf);
}
