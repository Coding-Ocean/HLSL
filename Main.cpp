#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "winmm.lib")

#include <windows.h>
#include <vector>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

#include <d3d11.h>
#include <directxmath.h>
using namespace DirectX;

#define WINDOW_CLASS    L"Pixel Shader"
#define WINDOW_TITLE    WINDOW_CLASS
#define WINDOW_WIDTH    1920
#define WINDOW_HEIGHT   1080

#define SAFE_RELEASE(x) if(x) x->Release();
#define WARNING(msg) MessageBox(NULL, msg, WINDOW_TITLE, MB_OK | MB_ICONERROR);

// パイプラインオブジェクト
ComPtr<ID3D11Device> g_device;                             // デバイスインターフェイス
ComPtr<ID3D11DeviceContext> g_context;                     // コンテキスト
ComPtr<IDXGISwapChain> g_swapChain;                        // スワップチェインインターフェイス
ComPtr<ID3D11RenderTargetView> g_renderTargetView;         // レンダーターゲットビュー
ComPtr<ID3D11InputLayout> g_layout;                        // インプットレイアウト
ComPtr<ID3D11VertexShader> g_vertexShader;                 // 頂点シェーダ
ComPtr<ID3D11PixelShader> g_pixelShader;                   // ピクセルシェーダ
ComPtr<ID3D11Buffer> g_vertexBuffer;                       // 頂点バッファ
ComPtr<ID3D11SamplerState> g_sampler;                      // テクスチャサンプラ
ComPtr<ID3D11ShaderResourceView> g_shaderResourceView;     // テクスチャリソース
ComPtr<ID3D11Buffer> g_constantBuffer;                     // 定数バッファ

//プログラムの開始時間
DWORD g_startTime = 0;

// 頂点構造体
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT2 uv;
};

// 定数構造体
struct ConstantBuffer
{
    float Time;
};

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
    case WM_KEYDOWN:
        if(wParam==VK_ESCAPE) PostMessage(hWnd, WM_CLOSE, 0, 0);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // switch文が処理しなかったメッセージを処理
    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

HRESULT InitWindow()
{
    // ウィンドウクラスを初期化
    WNDCLASSEX	windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = WINDOW_CLASS;
    ATOM rt = RegisterClassEx(&windowClass);
    if (rt == 0) {
        WARNING(L"ウィンドウクラスが登録できませんでした");
        return E_FAIL;
    }

    // ウィンドウスタイル
    //DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    DWORD windowStyle = WS_POPUP;
    // これからつくるウィンドウのサイズを求める
    RECT windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&windowRect, windowStyle, FALSE);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    //ウィンドウが中央に表示されるような表示位置を計算する。
    int windowPosX = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
    int windowPosY = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;

    // ウィンドウハンドルを作成
    HWND hWnd = CreateWindow(
        WINDOW_CLASS,
        WINDOW_TITLE,
        windowStyle,
        windowPosX,
        windowPosY,
        windowWidth,
        windowHeight,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    if (hWnd == 0) {
        WARNING(L"ウィンドウがつくれませんでした");
        return E_FAIL;
    }

    return S_OK;
}

// デバイス関連初期化
HRESULT InitDevice()
{
    HWND hWnd = FindWindow(WINDOW_CLASS, NULL);

    // ドライバー種別を定義
    std::vector<D3D_DRIVER_TYPE> driverTypes
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
        D3D_DRIVER_TYPE_SOFTWARE,
    };

    // スワップチェインのパラメタ
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferDesc.Width = WINDOW_WIDTH;
    swapChainDesc.BufferDesc.Height = WINDOW_HEIGHT;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.Windowed = TRUE;

    // ドライバー種別を上から検証し選択
    // 選択したデバイスを用いて描画する
    HRESULT hr= E_FAIL;
    for (size_t i = 0; i < driverTypes.size(); i++)
    {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            driverTypes[i],
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            g_swapChain.GetAddressOf(),
            g_device.GetAddressOf(),
            nullptr,
            g_context.GetAddressOf()
        );
        if (SUCCEEDED(hr)) break;
    }
    if (FAILED(hr))
    {
        WARNING(L"DirectX11に対応していないデバイスです。");
        return E_FAIL;
    }

    // バックバッファを作成
    ComPtr<ID3D11Texture2D> backBuffer;
    g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
    g_device->CreateRenderTargetView(backBuffer.Get(), nullptr, g_renderTargetView.GetAddressOf());

    // レンダーターゲットをバックバッファに設定
    g_context->OMSetRenderTargets(1, g_renderTargetView.GetAddressOf(), nullptr);

    // 表示領域を作成
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(viewport));
    viewport.Width = WINDOW_WIDTH;
    viewport.Height = WINDOW_HEIGHT;
    viewport.MinDepth = D3D11_MIN_DEPTH;    // 0.0f
    viewport.MaxDepth = D3D11_MAX_DEPTH;    // 1.0f
    g_context->RSSetViewports(1, &viewport);

    return S_OK;
}

// シェーダ関連初期化
#include<fstream>
HRESULT InitShader()
{
    //コンパイル済みシェーダを読み込むファイルバッファ
    class BUFFER {
    public:
        void operator= (const char* fileName) {
            std::ifstream ifs(fileName, std::ios::binary);
            if (ifs.fail()) {
                *(long*)(0xcbcbcbcbcbcbcbcb) = 0;//強制終了
            }
            std::istreambuf_iterator<char> it(ifs);
            std::istreambuf_iterator<char> last;
            Buffer.assign(it, last);
            ifs.close();
        }
        const char* pointer() const {
            return Buffer.data();
        }
        size_t size() {
            return Buffer.size();
        }
    private:
        std::string Buffer;
    }buf;

    //頂点シェーダをつくる------------------------------------------------------
#ifdef _DEBUG
    buf = "x64\\Debug\\VertexShader.cso";
#else
    buf = "x64\\Release\\VertexShader.cso";
#endif
    HRESULT hr;
    hr = g_device->CreateVertexShader(buf.pointer(), buf.size(), NULL, g_vertexShader.GetAddressOf());
    if (FAILED(hr))
    {
        WARNING(L"頂点シェーダがつくれませんでした。");
        return E_FAIL;
    }

    // 頂点インプットレイアウトを定義
    D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    // インプットレイアウトのサイズ
    UINT numElements = sizeof(inputElementDescs) / sizeof(inputElementDescs[0]);
    // 頂点インプットレイアウトを作成
    hr = g_device->CreateInputLayout(inputElementDescs, numElements, buf.pointer(), buf.size(), g_layout.GetAddressOf());
    if (FAILED(hr))
    {
        WARNING(L"頂点インプットレイアウトがつくれませんでした。");
        return E_FAIL;
    }
    // 頂点インプットレイアウトをセット
    g_context->IASetInputLayout(g_layout.Get());


    //ピクセルシェーダーをつくる-------------------------------------------------
#ifdef _DEBUG
    buf = "x64\\Debug\\PixelShader.cso";
#else
    buf = "x64\\Release\\PixelShader.cso";
#endif
    g_device->CreatePixelShader(buf.pointer(), buf.size(), NULL, g_pixelShader.GetAddressOf());
    if (FAILED(hr))
    {
        WARNING(L"ピクセルシェーダがつくれませんでした。");
        return E_FAIL;
    }

    return S_OK;
}

// バッファ関連初期化
HRESULT InitBuffer()
{
    // 四角形のジオメトリを定義
    float aspect = (float)WINDOW_HEIGHT/WINDOW_WIDTH;
    Vertex vertices[] = {
        {{-1.0f*aspect,  1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f*aspect,  1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-1.0f*aspect, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f*aspect, -1.0f, 0.0f}, {1.0f, 1.0f}},
    };

    // バッファを作成
    D3D11_BUFFER_DESC bufferDesc{};

    // 頂点バッファの設定
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;                 // デフォルトアクセス
    bufferDesc.ByteWidth = sizeof(Vertex) * _countof(vertices); // サイズはVertex構造体×4
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;        // 頂点バッファを使用
    bufferDesc.CPUAccessFlags = 0;                          // CPUのバッファへのアクセス拒否

    // リソースの設定
    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = vertices;

    // 頂点バッファを作成
    if (FAILED(g_device->CreateBuffer(&bufferDesc, &initData, g_vertexBuffer.GetAddressOf())))
    {
        WARNING(L"頂点バッファを作成できませんでした。");
        return E_FAIL;
    }

    // 表示する頂点バッファを選択
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);

    // 使用するプリミティブタイプを設定
    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 定数バッファの設定
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(ConstantBuffer);
    bufferDesc.CPUAccessFlags = 0;
    //ピクセルシェーダの時
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    //バーテックスシェーダの時
    //bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    // 定数バッファを作成
    if (FAILED(g_device->CreateBuffer(&bufferDesc, nullptr, g_constantBuffer.GetAddressOf())))
    {
        WARNING(L"定数バッファを作成できませんでした。");
        return E_FAIL;
    }

    return S_OK;
}

// テクスチャ関連初期化
HRESULT InitTexture()
{
    // 2X2のテクスチャをつくる
    UINT texWidth = 2;
    UINT texHeight = 2;
    unsigned char pixels[]{ 
        255,0,0,255, 
        255,255,0,255,
        0,255,0,255, 
        0,0,255,255
    };
    D3D11_TEXTURE2D_DESC td;
    td.Width = texWidth;
    td.Height = texHeight;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA sd;
    sd.pSysMem = (void*)pixels;
    sd.SysMemPitch = texWidth * 4;
    sd.SysMemSlicePitch = texWidth * texHeight * 4;

    HRESULT hr;
    ComPtr<ID3D11Texture2D> texture = 0;
    hr = g_device->CreateTexture2D(&td, &sd, texture.GetAddressOf());
    if (FAILED(hr))
    {
        WARNING(L"テクスチャをつくれませんでした。");
        return E_FAIL;
    }
    hr = g_device->CreateShaderResourceView(texture.Get(), nullptr, g_shaderResourceView.GetAddressOf());
    if (FAILED(hr))
    {
        WARNING(L"テクスチャビューをつくれませんでした。");
        return E_FAIL;
    }

    // サンプラの設定
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    // サンプラステートを作成
    if (FAILED(g_device->CreateSamplerState(&samplerDesc, g_sampler.GetAddressOf())))
    {
        WARNING(L"サンプラステートを作成できませんでした。");
        return E_FAIL;
    }

    return S_OK;
}

// 描画
VOID OnRender()
{
    // パラメータの受け渡し
    ConstantBuffer cBuffer;
    cBuffer.Time = (timeGetTime() - g_startTime) / 1000.f;
    // GPU(シェーダ側)へ転送する
    g_context->UpdateSubresource(g_constantBuffer.Get(), 0, nullptr, &cBuffer, 0, 0);

    // レンダーターゲットビューを指定した色でクリア
    FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_context->ClearRenderTargetView(g_renderTargetView.Get(), clearColor);

    // シェーダオブジェクトをセット
    g_context->VSSetShader(g_vertexShader.Get(), nullptr, 0);
    g_context->PSSetShader(g_pixelShader.Get(), nullptr, 0);
    g_context->PSSetConstantBuffers(0, 1, g_constantBuffer.GetAddressOf());

    // テクスチャをシェーダに登録
    g_context->PSSetSamplers(0, 1, g_sampler.GetAddressOf());
    g_context->PSSetShaderResources(0, 1, g_shaderResourceView.GetAddressOf());

    // 頂点バッファをバックバッファに描画
    g_context->Draw(4, 0);

    // フレームを最終出力
    g_swapChain->Present(0, 0);     // フリップ
}

VOID OnDestroy()
{
    timeEndPeriod(1);
}

VOID OnStart()
{
    // ウィンドウの表示
    HWND hWnd = FindWindow(WINDOW_CLASS, NULL);
    ShowWindow(hWnd, SW_SHOW);

    timeBeginPeriod(1);
    g_startTime = timeGetTime();
}

// メイン関数
INT WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ INT)
{
    //ウィンドウ初期化
    if (FAILED(InitWindow())) return 0;
    //ＤｉｒｅｃｔＸ初期化
    if (FAILED(InitDevice())) return 0;
    if (FAILED(InitShader())) return 0;
    if (FAILED(InitBuffer())) return 0;
    if (FAILED(InitTexture())) return 0;
    // メインループ
    OnStart();
    MSG	msg{};
    while (msg.message != WM_QUIT)
    {
        // キュー内のメッセージを処理
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // 描画
            OnRender();
        }
    }
    OnDestroy();
    
    return 0;
}
