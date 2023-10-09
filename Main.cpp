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

// �p�C�v���C���I�u�W�F�N�g
ComPtr<ID3D11Device> g_device;                             // �f�o�C�X�C���^�[�t�F�C�X
ComPtr<ID3D11DeviceContext> g_context;                     // �R���e�L�X�g
ComPtr<IDXGISwapChain> g_swapChain;                        // �X���b�v�`�F�C���C���^�[�t�F�C�X
ComPtr<ID3D11RenderTargetView> g_renderTargetView;         // �����_�[�^�[�Q�b�g�r���[
ComPtr<ID3D11InputLayout> g_layout;                        // �C���v�b�g���C�A�E�g
ComPtr<ID3D11VertexShader> g_vertexShader;                 // ���_�V�F�[�_
ComPtr<ID3D11PixelShader> g_pixelShader;                   // �s�N�Z���V�F�[�_
ComPtr<ID3D11Buffer> g_vertexBuffer;                       // ���_�o�b�t�@
ComPtr<ID3D11SamplerState> g_sampler;                      // �e�N�X�`���T���v��
ComPtr<ID3D11ShaderResourceView> g_shaderResourceView;     // �e�N�X�`�����\�[�X
ComPtr<ID3D11Buffer> g_constantBuffer;                     // �萔�o�b�t�@

//�v���O�����̊J�n����
DWORD g_startTime = 0;

// ���_�\����
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT2 uv;
};

// �萔�\����
struct ConstantBuffer
{
    float Time;
};

// �E�B���h�E�v���V�[�W��
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

    // switch�����������Ȃ��������b�Z�[�W������
    return DefWindowProc(hWnd, nMsg, wParam, lParam);
}

HRESULT InitWindow()
{
    // �E�B���h�E�N���X��������
    WNDCLASSEX	windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = WINDOW_CLASS;
    ATOM rt = RegisterClassEx(&windowClass);
    if (rt == 0) {
        WARNING(L"�E�B���h�E�N���X���o�^�ł��܂���ł���");
        return E_FAIL;
    }

    // �E�B���h�E�X�^�C��
    //DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    DWORD windowStyle = WS_POPUP;
    // ���ꂩ�����E�B���h�E�̃T�C�Y�����߂�
    RECT windowRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&windowRect, windowStyle, FALSE);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    //�E�B���h�E�������ɕ\�������悤�ȕ\���ʒu���v�Z����B
    int windowPosX = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
    int windowPosY = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;

    // �E�B���h�E�n���h�����쐬
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
        WARNING(L"�E�B���h�E������܂���ł���");
        return E_FAIL;
    }

    return S_OK;
}

// �f�o�C�X�֘A������
HRESULT InitDevice()
{
    HWND hWnd = FindWindow(WINDOW_CLASS, NULL);

    // �h���C�o�[��ʂ��`
    std::vector<D3D_DRIVER_TYPE> driverTypes
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
        D3D_DRIVER_TYPE_SOFTWARE,
    };

    // �X���b�v�`�F�C���̃p�����^
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

    // �h���C�o�[��ʂ��ォ�猟�؂��I��
    // �I�������f�o�C�X��p���ĕ`�悷��
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
        WARNING(L"DirectX11�ɑΉ����Ă��Ȃ��f�o�C�X�ł��B");
        return E_FAIL;
    }

    // �o�b�N�o�b�t�@���쐬
    ComPtr<ID3D11Texture2D> backBuffer;
    g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
    g_device->CreateRenderTargetView(backBuffer.Get(), nullptr, g_renderTargetView.GetAddressOf());

    // �����_�[�^�[�Q�b�g���o�b�N�o�b�t�@�ɐݒ�
    g_context->OMSetRenderTargets(1, g_renderTargetView.GetAddressOf(), nullptr);

    // �\���̈���쐬
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(viewport));
    viewport.Width = WINDOW_WIDTH;
    viewport.Height = WINDOW_HEIGHT;
    viewport.MinDepth = D3D11_MIN_DEPTH;    // 0.0f
    viewport.MaxDepth = D3D11_MAX_DEPTH;    // 1.0f
    g_context->RSSetViewports(1, &viewport);

    return S_OK;
}

// �V�F�[�_�֘A������
#include<fstream>
HRESULT InitShader()
{
    //�R���p�C���ς݃V�F�[�_��ǂݍ��ރt�@�C���o�b�t�@
    class BUFFER {
    public:
        void operator= (const char* fileName) {
            std::ifstream ifs(fileName, std::ios::binary);
            if (ifs.fail()) {
                *(long*)(0xcbcbcbcbcbcbcbcb) = 0;//�����I��
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

    //���_�V�F�[�_������------------------------------------------------------
#ifdef _DEBUG
    buf = "x64\\Debug\\VertexShader.cso";
#else
    buf = "x64\\Release\\VertexShader.cso";
#endif
    HRESULT hr;
    hr = g_device->CreateVertexShader(buf.pointer(), buf.size(), NULL, g_vertexShader.GetAddressOf());
    if (FAILED(hr))
    {
        WARNING(L"���_�V�F�[�_������܂���ł����B");
        return E_FAIL;
    }

    // ���_�C���v�b�g���C�A�E�g���`
    D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    // �C���v�b�g���C�A�E�g�̃T�C�Y
    UINT numElements = sizeof(inputElementDescs) / sizeof(inputElementDescs[0]);
    // ���_�C���v�b�g���C�A�E�g���쐬
    hr = g_device->CreateInputLayout(inputElementDescs, numElements, buf.pointer(), buf.size(), g_layout.GetAddressOf());
    if (FAILED(hr))
    {
        WARNING(L"���_�C���v�b�g���C�A�E�g������܂���ł����B");
        return E_FAIL;
    }
    // ���_�C���v�b�g���C�A�E�g���Z�b�g
    g_context->IASetInputLayout(g_layout.Get());


    //�s�N�Z���V�F�[�_�[������-------------------------------------------------
#ifdef _DEBUG
    buf = "x64\\Debug\\PixelShader.cso";
#else
    buf = "x64\\Release\\PixelShader.cso";
#endif
    g_device->CreatePixelShader(buf.pointer(), buf.size(), NULL, g_pixelShader.GetAddressOf());
    if (FAILED(hr))
    {
        WARNING(L"�s�N�Z���V�F�[�_������܂���ł����B");
        return E_FAIL;
    }

    return S_OK;
}

// �o�b�t�@�֘A������
HRESULT InitBuffer()
{
    // �l�p�`�̃W�I���g�����`
    float aspect = (float)WINDOW_HEIGHT/WINDOW_WIDTH;
    Vertex vertices[] = {
        {{-1.0f*aspect,  1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f*aspect,  1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-1.0f*aspect, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f*aspect, -1.0f, 0.0f}, {1.0f, 1.0f}},
    };

    // �o�b�t�@���쐬
    D3D11_BUFFER_DESC bufferDesc{};

    // ���_�o�b�t�@�̐ݒ�
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;                 // �f�t�H���g�A�N�Z�X
    bufferDesc.ByteWidth = sizeof(Vertex) * _countof(vertices); // �T�C�Y��Vertex�\���́~4
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;        // ���_�o�b�t�@���g�p
    bufferDesc.CPUAccessFlags = 0;                          // CPU�̃o�b�t�@�ւ̃A�N�Z�X����

    // ���\�[�X�̐ݒ�
    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = vertices;

    // ���_�o�b�t�@���쐬
    if (FAILED(g_device->CreateBuffer(&bufferDesc, &initData, g_vertexBuffer.GetAddressOf())))
    {
        WARNING(L"���_�o�b�t�@���쐬�ł��܂���ł����B");
        return E_FAIL;
    }

    // �\�����钸�_�o�b�t�@��I��
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_context->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);

    // �g�p����v���~�e�B�u�^�C�v��ݒ�
    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // �萔�o�b�t�@�̐ݒ�
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(ConstantBuffer);
    bufferDesc.CPUAccessFlags = 0;
    //�s�N�Z���V�F�[�_�̎�
    bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    //�o�[�e�b�N�X�V�F�[�_�̎�
    //bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    // �萔�o�b�t�@���쐬
    if (FAILED(g_device->CreateBuffer(&bufferDesc, nullptr, g_constantBuffer.GetAddressOf())))
    {
        WARNING(L"�萔�o�b�t�@���쐬�ł��܂���ł����B");
        return E_FAIL;
    }

    return S_OK;
}

// �e�N�X�`���֘A������
HRESULT InitTexture()
{
    // 2X2�̃e�N�X�`��������
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
        WARNING(L"�e�N�X�`��������܂���ł����B");
        return E_FAIL;
    }
    hr = g_device->CreateShaderResourceView(texture.Get(), nullptr, g_shaderResourceView.GetAddressOf());
    if (FAILED(hr))
    {
        WARNING(L"�e�N�X�`���r���[������܂���ł����B");
        return E_FAIL;
    }

    // �T���v���̐ݒ�
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    // �T���v���X�e�[�g���쐬
    if (FAILED(g_device->CreateSamplerState(&samplerDesc, g_sampler.GetAddressOf())))
    {
        WARNING(L"�T���v���X�e�[�g���쐬�ł��܂���ł����B");
        return E_FAIL;
    }

    return S_OK;
}

// �`��
VOID OnRender()
{
    // �p�����[�^�̎󂯓n��
    ConstantBuffer cBuffer;
    cBuffer.Time = (timeGetTime() - g_startTime) / 1000.f;
    // GPU(�V�F�[�_��)�֓]������
    g_context->UpdateSubresource(g_constantBuffer.Get(), 0, nullptr, &cBuffer, 0, 0);

    // �����_�[�^�[�Q�b�g�r���[���w�肵���F�ŃN���A
    FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_context->ClearRenderTargetView(g_renderTargetView.Get(), clearColor);

    // �V�F�[�_�I�u�W�F�N�g���Z�b�g
    g_context->VSSetShader(g_vertexShader.Get(), nullptr, 0);
    g_context->PSSetShader(g_pixelShader.Get(), nullptr, 0);
    g_context->PSSetConstantBuffers(0, 1, g_constantBuffer.GetAddressOf());

    // �e�N�X�`�����V�F�[�_�ɓo�^
    g_context->PSSetSamplers(0, 1, g_sampler.GetAddressOf());
    g_context->PSSetShaderResources(0, 1, g_shaderResourceView.GetAddressOf());

    // ���_�o�b�t�@���o�b�N�o�b�t�@�ɕ`��
    g_context->Draw(4, 0);

    // �t���[�����ŏI�o��
    g_swapChain->Present(0, 0);     // �t���b�v
}

VOID OnDestroy()
{
    timeEndPeriod(1);
}

VOID OnStart()
{
    // �E�B���h�E�̕\��
    HWND hWnd = FindWindow(WINDOW_CLASS, NULL);
    ShowWindow(hWnd, SW_SHOW);

    timeBeginPeriod(1);
    g_startTime = timeGetTime();
}

// ���C���֐�
INT WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ INT)
{
    //�E�B���h�E������
    if (FAILED(InitWindow())) return 0;
    //�c�����������w������
    if (FAILED(InitDevice())) return 0;
    if (FAILED(InitShader())) return 0;
    if (FAILED(InitBuffer())) return 0;
    if (FAILED(InitTexture())) return 0;
    // ���C�����[�v
    OnStart();
    MSG	msg{};
    while (msg.message != WM_QUIT)
    {
        // �L���[���̃��b�Z�[�W������
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // �`��
            OnRender();
        }
    }
    OnDestroy();
    
    return 0;
}
