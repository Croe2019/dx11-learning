#pragma once
#pragma once
#include <windows.h>
#include "Win32Window.h"
#include "D3D11Render.h"
#include "Camera.h"
#include "Timer.h"

class App
{
public:
    explicit App(HINSTANCE hInst);
    int Run();

private:
    void Update(float dt);
    void Render();
    static void OnResizeStatic(void* user, uint32_t w, uint32_t h);
    void OnResize(uint32_t w, uint32_t h);

private:
    Win32Window   mWindow;
    D3D11Renderer mRenderer;
    Camera        mCamera;
    Timer         mTimer;

    float mAngle = 0.0f;
};
