// ----------------------------------------------------
// 3Dキューブの表示 [cube.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-09
// Version: 1.0
// ----------------------------------------------------

#include "cube.h"
#include "direct3d.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "shader3d.h"
#include "shader3d_unlit.h"
#include "shader_shadow.h"
#include "texture.h"


static constexpr int NUM_VERTEX = 4 * 6; // 頂点数 一つの面に三角形×2
static constexpr int NUM_INDEX  = 3 * 2 * 6; // インデックス数 一つの面に三角形×2

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr; // インデックスバッファ

// static int g_CubeTexId = -1; // テクスチャID

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


// 3D頂点構造体
struct Vertex3d
{
	XMFLOAT3 position;	// 頂点座標
	XMFLOAT3 normal;	// 法線
	XMFLOAT4 color;		// 色
	XMFLOAT2 texcoord;	// UV
};

// インデックス

static Vertex3d g_CubeVertex[]{
	// cubeの前面
	{{ -0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f, -1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
	{{  0.5f, -0.5f, -0.5f}, { 0.0f, 0.0f, -1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.25f}},
	{{ -0.5f, -0.5f, -0.5f}, { 0.0f, 0.0f, -1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.25f}},
//  {{ -0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f, -1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
	{{  0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f, -1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.0f}},
//  {{  0.5f, -0.5f, -0.5f}, { 0.0f, 0.0f, -1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.25f}},

	// cubeの背面
	{{  0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.75f, 0.0f}},
	{{ -0.5f, -0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.25f}},
	{{  0.5f, -0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.75f, 0.25f}},
//  {{  0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.75f, 0.0f}},
	{{ -0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
//  {{ -0.5f, -0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.25f}},

	// cubeの上面	
	{{ -0.5f,  0.5f, -0.5f}, { 0.0f, 1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.25f}},
	{{  0.5f,  0.5f,  0.5f}, { 0.0f, 1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},
	{{  0.5f,  0.5f, -0.5f}, { 0.0f, 1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.5f}},
//  {{ -0.5f,  0.5f, -0.5f}, { 0.0f, 1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.25f}},
	{{ -0.5f,  0.5f,  0.5f}, { 0.0f, 1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.25f}},
//  {{  0.5f,  0.5f,  0.5f}, { 0.0f, 1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},

	// cubeの下面
	{{ -0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.25f}},
	{{  0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.5f}},
	{{ -0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.5f}},
//  {{ -0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.25f}},
	{{  0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.25f}},
//  {{  0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.5f}},

	// cubeの右面
	{{  0.5f,  0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.0f}},
	{{  0.5f, -0.5f,  0.5f}, { 1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.25f}},
	{{  0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.25f}},
//  {{  0.5f,  0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.0f}},
	{{  0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.0f}},
//  {{  0.5f, -0.5f,  0.5f}, { 1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.25f}},

	// cubeの左面
	{{ -0.5f,  0.5f,  0.5f}, { -1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.0f}},
	{{ -0.5f, -0.5f, -0.5f}, { -1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.75f, 0.25f}},
	{{ -0.5f, -0.5f,  0.5f}, { -1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.25f}},
//  {{ -0.5f,  0.5f,  0.5f}, { -1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.0f}},
	{{ -0.5f,  0.5f, -0.5f}, { -1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.75f, 0.0f}},
//  {{ -0.5f, -0.5f, -0.5f}, { -1.0f, 0.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f}, {0.75f, 0.25f}},
};

// 65536頂点まで
static unsigned short g_CubeIndex[36]{
	0, 1, 2,  0, 3, 1,
	4, 5, 6,  4, 7, 5,
	8, 9,10,  8,11, 9,
   12,13,14, 12,15,13,
   16,17,18, 16,19,17,
   20,21,22, 20,23,21,
};

void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex3d) * NUM_VERTEX; // sizeof(g_CubeVertex)
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = g_CubeVertex; // このあと読み書きできない

	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

	// インデックスバッファ作成
	bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX; // sizeof(g_CubeIndex)
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	sd.pSysMem = g_CubeIndex; // このあと読み書きできない

	g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);


	// g_CubeTexId = Texture_Load(L"resource/texture/cat.png");
}

void Cube_Finalize(void)
{
	SAFE_RELEASE(g_pVertexBuffer);
	SAFE_RELEASE(g_pIndexBuffer);
}

void Cube_Draw(int texId, const DirectX::XMMATRIX& mtxWorld)
{
	// シェーダーの準備
	Shader3d_Begin();

	Shader3d_SetColor(XMFLOAT4(1.0f,1.0f,1.0f,1.0f)); // default color

	if (texId >=0) {
		Texture_SetTexture(texId); // bind texture
	} else {
		// unbind texture to avoid sampling artifacts in debug
		ID3D11ShaderResourceView* nullSRV = nullptr;
		Direct3D_GetContext()->PSSetShaderResources(0,1, &nullSRV);
	}

	// IA setup
	UINT stride = sizeof(Vertex3d);
	UINT offset =0;
	g_pContext->IASetVertexBuffers(0,1, &g_pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT,0);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ワールド行列をセット
	Shader3d_SetWorldMatrix(mtxWorld);

	g_pContext->DrawIndexed(NUM_INDEX,0,0);
}

void Cube_Draw_Minimal(const DirectX::XMMATRIX& mtxWorld)
{
	Shader3d_Unlit_Begin();
	Shader3d_Unlit_SetColor({0.6f,0.6f,0.8f,1.0f });

	// IA setup
	UINT stride = sizeof(Vertex3d);
	UINT offset =0;
	g_pContext->IASetVertexBuffers(0,1, &g_pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT,0);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ワールド行列をセット
	Shader3d_Unlit_SetWorldMatrix(mtxWorld);

	g_pContext->DrawIndexed(NUM_INDEX,0,0);
}

// シャドウマップ専用描画関数（深度のみ）
void Cube_DrawShadow(const DirectX::XMMATRIX& mtxWorld)
{
	// IA setup
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ワールド行列をシャドウシェーダーに設定
	ShaderShadow_SetWorldMatrix(mtxWorld);

	g_pContext->DrawIndexed(NUM_INDEX, 0, 0);
}

AABB Cube_GetAABB(const DirectX::XMFLOAT3& position)
{
	return { {position.x - 0.5f,position.y - 0.5f,position.z - 0.5f},
			 {position.x + 0.5f,position.y + 0.5f,position.z + 0.5f} };
}
