#pragma once
#include <windows.h>
#include <cstdint>

class Win32Window
{
public:
    using ResizeCallback = void(*)(void* user, uint32_t w, uint32_t h);

    Win32Window(HINSTANCE hInst, uint32_t w, uint32_t h);

    HWND Hwnd() const { return mHwnd; }
    uint32_t Width() const { return mWidth; }
    uint32_t Height() const { return mHeight; }

    void Show(int nCmdShow = SW_SHOW);
    bool ProcessMessages();

    void SetResizeCallback(ResizeCallback cb, void* user)
    {
        mResizeCb = cb;
        mResizeUser = user;
    }

private:
    void Create(HINSTANCE hInst);

    static LRESULT CALLBACK WndProcSetup(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK WndProcThunk(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    HWND mHwnd = nullptr;
    uint32_t mWidth = 1280;
    uint32_t mHeight = 720;

    ResizeCallback mResizeCb = nullptr;
    void* mResizeUser = nullptr;

    bool mInSizeMove = false; // ドラッグリサイズ安定化用
};
