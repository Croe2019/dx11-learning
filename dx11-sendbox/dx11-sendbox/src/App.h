#pragma once
#include <windows.h>
#include "Win32Window.h"
#include "Timer.h"

class App
{
public:
    explicit App(HINSTANCE hInst);
    int Run();

private:
    void Update(float dt);
    void Render();


private:
    Win32Window mWindow;
    Timer mTimer;
    float mAngle = 0.0f; // «—ˆ‚Ì‰ñ“]‚È‚Ç
};
