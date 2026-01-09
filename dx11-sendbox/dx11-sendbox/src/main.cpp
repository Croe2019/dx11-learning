#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <chrono>


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

static const wchar_t* kWndClassName = L"DX11SandboxWindowClass";

/*
* x軸、y軸、z軸の値
* 三色＋アルファ値をもたせる構造体
*/ 
struct Vertex
{
    float x, y, z;
    float r, g, b, a;
};

/*
    定数用のバッファ用の構造体を定義(16バイト境界)

*/
struct CBPerFrame
{
    XMFLOAT4X4 worldViewProj; // 64 bytes (16byte aligned)
};


struct D3D11Context
{
    // GPUリソースを作る工場(factory) 例：頂点バッファ、テクスチャ、シェーダなどGPU側に置くものは基本device->Createxxx()で作る
    ComPtr<ID3D11Device> device;
    // contextは描画コマンドを発行する司令塔
    /*
    * 例：ClearRenderTargetView、IASetVertexBuffer、Draw、VSSetShaderなどすべてcontext経由で行う
    * deviceが「作る」
    * contextが「使う(コマンドを流す)」
    * という役割分担
    */
    ComPtr<ID3D11DeviceContext> context;
    /*
    * 表示用バッファ列（フロント／バックバッファ）を管理する
    * swapChain->Present(1, 0)：VSyncありで表示更新
    */ 
    ComPtr<IDXGISwapChain> swapChain;
    /*
    * 「今描画する先（カラーバッファ）」へのビュー
    * バックバッファ（Texture2D)そのものを直接描画ターゲットにできないため、RTVを作って
    * OMSetRenderTargetsで設定する
    * context->ClearRenderTargetView(rtv, color)はこのrtvをクリアしている
    */
    ComPtr<ID3D11RenderTargetView> rtv;

    // Milestone 1

    /*
        頂点シェーダ（HLSLのVSMainをコンパイルして作ったもの)
        頂点をどこに置くか（座標変換など）を決める
    */
    ComPtr<ID3D11VertexShader> vs;
    /*
        ピクセルシェーダ（HLSLのPSMain)
        各ピクセルの色を決める
    */
    ComPtr<ID3D11PixelShader> ps;

    /*
        頂点バッファのメモリ配置と、シェーダ入力の対応表
        今回の頂点は
        ・POSITION:float3 (12bytes)
        ・COLOR:float4 (16bytes)
        とうい並びだが、それを
        ・シェーダのPOSITIONセマンティクスへ
        ・シェーダのCOLORセマンティクスへ
        どう結びつけるかを定義する
        これがずれると、三角形が出ない／色が変・クラッシュ等が起きる
    */
    ComPtr<ID3D11InputLayout> inputLayout;
    /*
        頂点バッファ(Vertex Buffer)です
        三角形を構成する3頂点データ(座標・色)をGPUにおいたもの
        描画時に
        ・IASetVertexBuffers(...)
        で入力アセンブラ(IA)に渡され、Draw(3,0)で参照される
    */
    ComPtr<ID3D11Buffer> vb;

    /*
        画面サイズ
        ・ウィンドウ作成時のクライアントサイズ
        ・SwapChain作成時のバッファサイズ
        ・Viewport設定
        など複数箇所で同じ値を使うため、Contextにまとめている
    */

    ComPtr<ID3D11Buffer> cbPerFrame;
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

bool ResizeD3D11(D3D11Context& dx, UINT newW, UINT newH)
{
    if (!dx.swapChain || !dx.context) return false;
    if (newW == 0 || newH == 0) return true; // 最小化中

    dx.width = newW;
    dx.height = newH;

    // バインド解除（重要：RTVを持ったままResizeBuffersすると失敗しやすい）
    dx.context->OMSetRenderTargets(0, nullptr, nullptr);
    dx.rtv.Reset();

    HRESULT hr = dx.swapChain->ResizeBuffers(
        0, // 0=既存のバッファ数維持
        dx.width, dx.height,
        DXGI_FORMAT_UNKNOWN, // 既存を維持
        0
    );
    if (FAILED(hr)) {
        ShowHResult(L"ResizeBuffers failed.", hr);
        return false;
    }

    // 新しいBackBufferからRTV作成
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = dx.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
    if (FAILED(hr)) {
        ShowHResult(L"GetBuffer after ResizeBuffers failed.", hr);
        return false;
    }

    hr = dx.device->CreateRenderTargetView(backBuffer.Get(), nullptr, dx.rtv.GetAddressOf());
    if (FAILED(hr)) {
        ShowHResult(L"CreateRenderTargetView after ResizeBuffers failed.", hr);
        return false;
    }

    dx.context->OMSetRenderTargets(1, dx.rtv.GetAddressOf(), nullptr);

    // Viewport更新
    D3D11_VIEWPORT vp{};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = (FLOAT)dx.width;
    vp.Height = (FLOAT)dx.height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    dx.context->RSSetViewports(1, &vp);

    return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_SIZE:
    {
        auto* dx = (D3D11Context*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        if (dx && dx->swapChain) {
            UINT w = LOWORD(lParam);
            UINT h = HIWORD(lParam);
            ResizeD3D11(*dx, w, h);
        }
        return 0;
    }

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}


bool CreateMainWindow(HINSTANCE hInst, int nCmdShow, D3D11Context* dx, HWND& outHwnd)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = kWndClassName;

    if (!RegisterClassExW(&wc)) return false;

    RECT rc{ 0,0,(LONG)dx->width,(LONG)dx->height };
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
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)dx);
    if (!hWnd) return false;

    ShowWindow(hWnd, nCmdShow);
    outHwnd = hWnd;
    return true;
}

bool InitD3D11(HWND hWnd, D3D11Context& dx)
{
    // SwapChainDescを作る
    /*
        dx.width/dx.heightを使ってDXGI_SWAPCHAIN_DESC_scdを構築
        (ここでのwidth/heghtは「バックバッファのサイズ」
    */
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
    /*
        ここで一気に3つ生成される
        dx.device(工場)
        dx.context(司令塔)
        dx.swapChain(画面に出すバッファ列)
        この1行で「GPUに命令できる状態になる」
    
    */
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

    /*
        BackBufferを取得し、RTVを作成
        ・dx.swapChain->GetBuffer(...)でバックバッファ(Texture2D)を取る
        ・dx.device->CreateRenderTargetView(...)でdx.rtvを作る
    */
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = dx.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = dx.device->CreateRenderTargetView(backBuffer.Get(), nullptr, dx.rtv.GetAddressOf());
    if (FAILED(hr)) return false;

    /*
    * 出力先を設定(OM)
    * dx.context->OMSetRenderTargets(1, dx.rtv.GetAddressOf(), nullptr);
    * ここで「以降の描画はrtvに向かう」状態になる
    * 
    */
    dx.context->OMSetRenderTargets(1, dx.rtv.GetAddressOf(), nullptr);

    D3D11_VIEWPORT vp{};
    vp.Width = (FLOAT)dx.width;
    vp.Height = (FLOAT)dx.height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    /*
        ViewPortの設定(RS)
        dx.context->RSSetViewports(...)
        ここで「画面のどこに描くか(0,0～width,height)」が確定する
        この時点でClear →　Presentまで可能になる
    */
    dx.context->RSSetViewports(1, &vp);

    return true;
}

 // InitTrianglePipeline(D3D11Context& dx)　←はdeviceで作って、contextで使えるようにするための準備
bool InitTrianglePipeline(D3D11Context& dx)
{
    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> err;

    /*
        HLSLをコンパイル(実行時に読む)
        ここでよくある失敗が、「作業ディレクトリ不一致（パスが見つからない）
    */
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

    /*
        Shaderを生成(device)
    */
    hr = dx.device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, dx.vs.GetAddressOf());
    if (FAILED(hr)) { ShowHResult(L"CreateVertexShader failed.", hr); return false; }

    hr = dx.device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, dx.ps.GetAddressOf());
    if (FAILED(hr)) { ShowHResult(L"CreatePixelShader failed.", hr); return false; }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    /*
        InputLayoutを生成(device)
        ・dx.device->CreateInputLayout(layout, ..., vsBlob, ...) -> dx.inputLayout
        InputLayoutはvsBlob（頂点シェーダの入力定義）とセットで作るのがポイント
        ここがずれると描画が崩れる
    
    */
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

    /*
        VertexBufferを生成（device)
        ・頂点配列Vertex verts[3]を用意
        ・dx.device->CreateBuffer(...)dx.vb
        ここまでで、三角形描画に必要な資材が揃う状態
    */
    hr = dx.device->CreateBuffer(&bd, &init, dx.vb.GetAddressOf());
    if (FAILED(hr)) { ShowHResult(L"CreateBuffer(VB) failed.", hr); return false; }

    // Constant Buffer (b0)
    D3D11_BUFFER_DESC cbd{};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(CBPerFrame);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = dx.device->CreateBuffer(&cbd, nullptr, dx.cbPerFrame.GetAddressOf());
    if (FAILED(hr)) { ShowHResult(L"CreateBuffer(CB) failed.", hr); return false; }


    return true;
}

// 描画パイプラインに材料をセットする Draw
void DrawTriangle(D3D11Context& dx)
{
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    /*
        IA（入力アセンブラ）に入力を渡す
        ・IASetInputLayout(dx.inputLayout)
        ・IASetVertexBuffers(dx.vb)
        ・IASetPrimitiveTopology(TRIANGLELIST)
    
    */
    dx.context->IASetInputLayout(dx.inputLayout.Get());
    dx.context->IASetVertexBuffers(0, 1, dx.vb.GetAddressOf(), &stride, &offset);
    dx.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // VS/VPをセットする
    dx.context->VSSetShader(dx.vs.Get(), nullptr, 0);
    dx.context->PSSetShader(dx.ps.Get(), nullptr, 0);

    // Draw呼び出し
    dx.context->Draw(3, 0);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    D3D11Context dx;
    HWND hWnd = nullptr;
    auto t0 = std::chrono::steady_clock::now();


    if (!CreateMainWindow(hInst, nCmdShow, &dx, hWnd)) {
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
        /*
            背景塗りつぶし
            rtvが「描画先」である前提
            背景が青いのはこれ
        
        */
        dx.context->ClearRenderTargetView(dx.rtv.Get(), clearColor);
        // time (seconds)
        auto t1 = std::chrono::steady_clock::now();
        float sec = std::chrono::duration<float>(t1 - t0).count();

        // Matrices
        XMMATRIX world = XMMatrixRotationZ(sec);
        XMMATRIX view = XMMatrixIdentity();
        /*XMMATRIX proj = XMMatrixIdentity();
        XMMATRIX wvp = world * view * proj;*/

        // HLSL mul(float4, matrix) で安全にするため転置して渡す
        CBPerFrame cb{};
        float aspect = (dx.height != 0) ? (float)dx.width / (float)dx.height : 1.0f;

        // 2Dのままでも良いが、後で3D化しやすい形に
        XMMATRIX proj = XMMatrixOrthographicOffCenterLH(-aspect, aspect, -1.0f, 1.0f, 0.0f, 1.0f);

        XMMATRIX wvp = world * view * proj;
        XMStoreFloat4x4(&cb.worldViewProj, XMMatrixTranspose(wvp));


        // Update constant buffer
        D3D11_MAPPED_SUBRESOURCE ms{};
        HRESULT hr = dx.context->Map(dx.cbPerFrame.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
        if (SUCCEEDED(hr)) {
            memcpy(ms.pData, &cb, sizeof(cb));
            dx.context->Unmap(dx.cbPerFrame.Get(), 0);
        }

        // Bind to VS b0
        dx.context->VSSetConstantBuffers(0, 1, dx.cbPerFrame.GetAddressOf());


        DrawTriangle(dx);

        // Present画面に出す
        dx.swapChain->Present(1, 0);
    }
}
