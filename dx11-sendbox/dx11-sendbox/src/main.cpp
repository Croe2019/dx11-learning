#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

static const wchar_t* kWndClassName = L"DX11SandboxWindowClass";

struct Vertex
{
    float x, y, z;
    float r, g, b, a;
};

struct D3D11Context
{
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11RenderTargetView> rtv;

    // Milestone 1
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11Buffer> vb;

    UINT width = 1280;
    UINT height = 720;
};

static void ShowHResult(const wchar_t* title, HRESULT hr, ID3DBlob* err = nullptr)
{
    wchar_t buf[1024]{};
    wsprintfW(buf, L"%s\nhr=0x%08X", title, (unsigned)hr);
    MessageBoxW(nullptr, buf, L"Error", MB_OK);

    if (err) {
        OutputDebugStringA((const char*)err->GetBufferPointer());
        OutputDebugStringA("\n");
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

bool CreateMainWindow(HINSTANCE hInst, int nCmdShow, UINT width, UINT height, HWND& outHwnd)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = kWndClassName;

    if (!RegisterClassExW(&wc)) return false;

    RECT rc{ 0,0,(LONG)width,(LONG)height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowExW(
        0, kWndClassName,
        L"DX11 Sandbox - Milestone 1 (Triangle)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        nullptr, nullptr, hInst, nullptr
    );
    if (!hWnd) return false;

    ShowWindow(hWnd, nCmdShow);
    outHwnd = hWnd;
    return true;
}

bool InitD3D11(HWND hWnd, D3D11Context& dx)
{
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferDesc.Width = dx.width;
    scd.BufferDesc.Height = dx.height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.SampleDesc.Count = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.BufferCount = 2;
    scd.OutputWindow = hWnd;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL fls[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL outFL{};

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createFlags,
        fls, 1, D3D11_SDK_VERSION,
        &scd,
        dx.swapChain.GetAddressOf(),
        dx.device.GetAddressOf(),
        &outFL,
        dx.context.GetAddressOf()
    );

#if defined(_DEBUG)
    if (FAILED(hr)) {
        createFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            createFlags,
            fls, 1, D3D11_SDK_VERSION,
            &scd,
            dx.swapChain.GetAddressOf(),
            dx.device.GetAddressOf(),
            &outFL,
            dx.context.GetAddressOf()
        );
    }
#endif

    if (FAILED(hr)) return false;

    ComPtr<ID3D11Texture2D> backBuffer;
    hr = dx.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = dx.device->CreateRenderTargetView(backBuffer.Get(), nullptr, dx.rtv.GetAddressOf());
    if (FAILED(hr)) return false;

    dx.context->OMSetRenderTargets(1, dx.rtv.GetAddressOf(), nullptr);

    D3D11_VIEWPORT vp{};
    vp.Width = (FLOAT)dx.width;
    vp.Height = (FLOAT)dx.height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    dx.context->RSSetViewports(1, &vp);

    return true;
}

bool InitTrianglePipeline(D3D11Context& dx)
{
    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> err;

    HRESULT hr = D3DCompileFromFile(
        L"shaders/basic.hlsl",
        nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "VSMain", "vs_5_0",
        flags, 0,
        vsBlob.GetAddressOf(),
        err.GetAddressOf()
    );
    if (FAILED(hr)) { ShowHResult(L"D3DCompileFromFile(VS) failed.", hr, err.Get()); return false; }

    err.Reset();
    hr = D3DCompileFromFile(
        L"shaders/basic.hlsl",
        nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "PSMain", "ps_5_0",
        flags, 0,
        psBlob.GetAddressOf(),
        err.GetAddressOf()
    );
    if (FAILED(hr)) { ShowHResult(L"D3DCompileFromFile(PS) failed.", hr, err.Get()); return false; }

    hr = dx.device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, dx.vs.GetAddressOf());
    if (FAILED(hr)) { ShowHResult(L"CreateVertexShader failed.", hr); return false; }

    hr = dx.device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, dx.ps.GetAddressOf());
    if (FAILED(hr)) { ShowHResult(L"CreatePixelShader failed.", hr); return false; }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = dx.device->CreateInputLayout(
        layout, (UINT)_countof(layout),
        vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
        dx.inputLayout.GetAddressOf()
    );
    if (FAILED(hr)) { ShowHResult(L"CreateInputLayout failed.", hr); return false; }

    Vertex verts[] = {
        {  0.0f,  0.5f, 0.0f,  1,0,0,1 },
        {  0.5f, -0.5f, 0.0f,  0,1,0,1 },
        { -0.5f, -0.5f, 0.0f,  0,0,1,1 },
    };

    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(verts);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = verts;

    hr = dx.device->CreateBuffer(&bd, &init, dx.vb.GetAddressOf());
    if (FAILED(hr)) { ShowHResult(L"CreateBuffer(VB) failed.", hr); return false; }

    return true;
}

void DrawTriangle(D3D11Context& dx)
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    dx.context->IASetInputLayout(dx.inputLayout.Get());
    dx.context->IASetVertexBuffers(0, 1, dx.vb.GetAddressOf(), &stride, &offset);
    dx.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    dx.context->VSSetShader(dx.vs.Get(), nullptr, 0);
    dx.context->PSSetShader(dx.ps.Get(), nullptr, 0);

    dx.context->Draw(3, 0);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    D3D11Context dx;
    HWND hWnd = nullptr;

    if (!CreateMainWindow(hInst, nCmdShow, dx.width, dx.height, hWnd)) {
        MessageBoxW(nullptr, L"CreateMainWindow failed.", L"Error", MB_OK);
        return 1;
    }
    if (!InitD3D11(hWnd, dx)) {
        MessageBoxW(nullptr, L"InitD3D11 failed.", L"Error", MB_OK);
        return 1;
    }
    if (!InitTrianglePipeline(dx)) {
        return 1;
    }

    MSG msg{};
    while (true) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        const float clearColor[4] = { 0.10f, 0.15f, 0.30f, 1.0f };
        dx.context->ClearRenderTargetView(dx.rtv.Get(), clearColor);

        DrawTriangle(dx);

        dx.swapChain->Present(1, 0);
    }
}
