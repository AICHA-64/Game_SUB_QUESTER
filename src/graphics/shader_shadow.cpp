#include "shader_shadow.h"
#include <d3d11.h>
#include <fstream>
#include "direct3d.h"

static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr; // World Matrix用
static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; // View Matrix用
static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr; // Projection Matrix用

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

bool ShaderShadow_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;
    HRESULT hr;

    // shader_vertex_shadow.cso を読み込む
    std::ifstream ifs_vs("resource/shader/shader_vertex_shadow.cso", std::ios::binary);
    if (!ifs_vs) return false;

    ifs_vs.seekg(0, std::ios::end);
    std::streamsize filesize = ifs_vs.tellg();
    ifs_vs.seekg(0, std::ios::beg);

    unsigned char* vsbinary = new unsigned char[filesize];
    ifs_vs.read((char*)vsbinary, filesize);
    ifs_vs.close();

    hr = g_pDevice->CreateVertexShader(vsbinary, filesize, nullptr, &g_pVertexShader);
    if (FAILED(hr)) { delete[] vsbinary; return false; }

    // Modelと一致するレイアウト
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = g_pDevice->CreateInputLayout(layout, 4, vsbinary, filesize, &g_pInputLayout);
    delete[] vsbinary;
    if (FAILED(hr)) return false;

    // World行列用バッファ
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    g_pDevice->CreateBuffer(&bd, nullptr, &g_pVSConstantBuffer0);

    // View行列用バッファ
    bd.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
    g_pDevice->CreateBuffer(&bd, nullptr, &g_pVSConstantBuffer1);

    // Projection行列用バッファ
    bd.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
    g_pDevice->CreateBuffer(&bd, nullptr, &g_pVSConstantBuffer2);

    return true;
}

void ShaderShadow_Finalize()
{
    if (g_pVSConstantBuffer0) g_pVSConstantBuffer0->Release();
    if (g_pVSConstantBuffer1) g_pVSConstantBuffer1->Release();
    if (g_pVSConstantBuffer2) g_pVSConstantBuffer2->Release();
    if (g_pInputLayout) g_pInputLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release();
}

void ShaderShadow_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
    DirectX::XMFLOAT4X4 t;
    DirectX::XMStoreFloat4x4(&t, DirectX::XMMatrixTranspose(matrix));
    g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &t, 0, 0);
}

void ShaderShadow_SetLightViewMatrix(const DirectX::XMMATRIX& matrix)
{
    DirectX::XMFLOAT4X4 t;
    DirectX::XMStoreFloat4x4(&t, DirectX::XMMatrixTranspose(matrix));
    g_pContext->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &t, 0, 0);
}

void ShaderShadow_SetLightProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
    DirectX::XMFLOAT4X4 t;
    DirectX::XMStoreFloat4x4(&t, DirectX::XMMatrixTranspose(matrix));
    g_pContext->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &t, 0, 0);
}

void ShaderShadow_Begin()
{
    g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pContext->PSSetShader(nullptr, nullptr, 0); // ピクセルシェーダーは不要（NULL）
    g_pContext->IASetInputLayout(g_pInputLayout);

    ID3D11Buffer* buffers[] = { g_pVSConstantBuffer0, g_pVSConstantBuffer1, g_pVSConstantBuffer2 };
    g_pContext->VSSetConstantBuffers(0, 3, buffers);
}
