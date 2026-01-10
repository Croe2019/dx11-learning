#include "App.h"
#include <DirectXMath.h>
using namespace DirectX;

App::App(HINSTANCE hInst)
    : mWindow(hInst, 1280, 720)
{
    mWindow.SetResizeCallback(&App::OnResizeStatic, this);
    mWindow.Show();

    mRenderer.Init(mWindow.Hwnd(), mWindow.Width(), mWindow.Height());

    mCamera.SetLookAt({ 0,0,-3 }, { 0,0,0 }, { 0,1,0 });
    mCamera.SetLens(XMConvertToRadians(60.0f), mWindow.Width(), mWindow.Height(), 0.1f, 100.0f);
}

int App::Run()
{
    mTimer.Reset();
    while (mWindow.ProcessMessages())
    {
        float dt = mTimer.Tick();
        Update(dt);
        Render();
    }

    return 0;
}

void App::Update(float dt)
{
    mAngle += dt; // 既存の回転
}

void App::Render()
{
    mRenderer.BeginFrame();

    XMMATRIX rot = XMMatrixRotationY(mAngle);

    // 1回描画ならこれ
    {
        XMMATRIX world = rot;
        XMMATRIX wvp = world * mCamera.View() * mCamera.Proj();
        mRenderer.UpdateCB(wvp);
        mRenderer.DrawTriangle();
    }

    // 2回描画するならこのブロックを追加（必要に応じて）
    /*
    {
        XMMATRIX world = rot * XMMatrixTranslation(0,0,2.0f);
        XMMATRIX wvp = world * mCamera.View() * mCamera.Proj();
        mRenderer.UpdateCB(wvp);
        mRenderer.DrawTriangle();
    }
    */

    mRenderer.EndFrame();
}

void App::OnResizeStatic(void* user, uint32_t w, uint32_t h)
{
    static_cast<App*>(user)->OnResize(w, h);
}

void App::OnResize(uint32_t w, uint32_t h)
{
    if (w == 0 || h == 0) return; // 最小化対策
    mRenderer.OnResize(w, h);
    mCamera.SetLens(XMConvertToRadians(60.0f), w, h, 0.1f, 100.0f);
}
