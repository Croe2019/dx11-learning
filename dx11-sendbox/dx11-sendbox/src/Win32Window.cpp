#include "Win32Window.h"

Win32Window::Win32Window(HINSTANCE hInst, uint32_t w, uint32_t h)
    : mWidth(w), mHeight(h)
{
    Create(hInst);
}

void Win32Window::Create(HINSTANCE hInst)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &Win32Window::WndProcSetup; // ←最初は Setup
    wc.hInstance = hInst;
    wc.lpszClassName = L"DX11WindowClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    RECT rc{ 0, 0, (LONG)mWidth, (LONG)mHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    mHwnd = CreateWindowExW(
        0, wc.lpszClassName, L"DirectX11",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInst,
        this // ←ここが重要：WM_NCCREATE で取り出す
    );
}

bool Win32Window::ProcessMessages()
{
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

LRESULT CALLBACK Win32Window::WndProcSetup(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        auto* self = reinterpret_cast<Win32Window*>(cs->lpCreateParams);

        // 1) this を保存
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));

        // 2) WndProc を Thunk に切り替え（以降は this 経由で呼べる）
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Win32Window::WndProcThunk));

        // 3) インスタンスの hwnd も覚えておく（任意だが便利）
        self->mHwnd = hwnd;

        return self->WndProc(hwnd, msg, wp, lp);
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT CALLBACK Win32Window::WndProcThunk(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto* self = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    return self ? self->WndProc(hwnd, msg, wp, lp) : DefWindowProcW(hwnd, msg, wp, lp);
}

LRESULT Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ENTERSIZEMOVE:
        mInSizeMove = true;
        return 0;

    case WM_EXITSIZEMOVE:
        mInSizeMove = false;
        // ここで現在のクライアントサイズを取って 1 回だけ通知
        {
            RECT rc{};
            GetClientRect(hwnd, &rc);
            uint32_t w = rc.right - rc.left;
            uint32_t h = rc.bottom - rc.top;

            if (w != 0 && h != 0)
            {
                mWidth = w; mHeight = h;
                if (mResizeCb) mResizeCb(mResizeUser, mWidth, mHeight);
            }
        }
        return 0;

    case WM_SIZE:
    {
        uint32_t w = LOWORD(lp);
        uint32_t h = HIWORD(lp);

        if (wp == SIZE_MINIMIZED || w == 0 || h == 0)
            return 0;

        mWidth = w;
        mHeight = h;

        // ドラッグ中は通知しない（EXITSIZEMOVE でまとめて通知）
        if (!mInSizeMove && mResizeCb)
            mResizeCb(mResizeUser, mWidth, mHeight);

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}


