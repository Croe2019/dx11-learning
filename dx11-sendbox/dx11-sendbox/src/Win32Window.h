#pragma once
#include <windows.h>
#include <cstdint>

class Win32Window
{
public:
    using ResizeCallback = void(*)(void* user, uint32_t w, uint32_t h);

    Win32Window(HINSTANCE hInst, uint32_t w, uint32_t h);
    ~Win32Window();

    HWND  Hwnd()  const { return mHwnd; }
    uint32_t Width()  const { return mWidth; }
    uint32_t Height() const { return mHeight; }

    void Show(int nCmdShow = SW_SHOW);
    bool ProcessMessages(); // false Ç»ÇÁèIóπ

    void SetResizeCallback(ResizeCallback cb, void* user);

private:
    static LRESULT CALLBACK WndProcSetup(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK WndProcThunk(HWND, UINT, WPARAM, LPARAM);
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);

    void Create(HINSTANCE);

private:
    HWND mHwnd = nullptr;
    HINSTANCE mHinst = nullptr;
    uint32_t mWidth = 1280;
    uint32_t mHeight = 720;

    ResizeCallback mResizeCb = nullptr;
    void* mResizeUser = nullptr;
};
