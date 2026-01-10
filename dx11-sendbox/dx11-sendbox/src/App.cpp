#include "App.h"

App::App(HINSTANCE hInst)
    : mWindow(hInst, 1280, 720)
{
    mWindow.Show();
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
    mAngle += dt; // ¡‚Íg‚í‚È‚­‚Ä‚àOK
}

void App::Render()
{
    // ¡‚Í‰½‚à‚µ‚È‚¢iŒã‚Å D3D11Renderer ‚ğŒÄ‚Ôj
}
