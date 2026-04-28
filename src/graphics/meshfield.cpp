// ----------------------------------------------------
// メッシュフィールドの表示 [meshfield.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-19
// Version: 1.0
// ----------------------------------------------------

#include "meshfield.h"
#include "direct3d.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "shader_field.h"
#include "texture.h"
#include "camera.h"

static constexpr float FIELD_MESH_WIDTH = 1.0f; // メッシュフィールドの一辺の長さ
static constexpr float FIELD_MESH_DEPTH = 1.0f;
static constexpr int FIELD_MESH_H_COUNT = 50; // 横のメッシュ数
static constexpr int FIELD_MESH_V_COUNT = 50;
static constexpr int FIELD_MESH_H_VERTEX_COUNT = FIELD_MESH_H_COUNT + 1; // 横の頂点数
static constexpr int FIELD_MESH_V_VERTEX_COUNT = FIELD_MESH_V_COUNT + 1;

static constexpr int NUM_VERTEX = FIELD_MESH_H_VERTEX_COUNT * FIELD_MESH_V_VERTEX_COUNT; // 頂点数 一つの面に三角形×2
static constexpr int NUM_INDEX = 3 * 2 * FIELD_MESH_H_COUNT * FIELD_MESH_V_COUNT; // インデックス数 一つの面に三角形×2

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ
static ID3D11Buffer* g_pIndexBuffer = nullptr; // インデックスバッファ

static int g_MeshfieldTexId = -1; // テクスチャID

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

static Vertex3d g_MeshFieldVertex[NUM_VERTEX]{
	/* cubeの前面
	{{ -0.5f,  0.0f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.25f}},
	{{  0.5f,  0.0f,  0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},
	{{  0.5f,  0.0f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.5f}},
	//{{ -0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.25f, 0.25f}}
	{{ -0.5f,  0.0f,  0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.25f}},
	//{{  0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.5f, 0.5f}},
	*/
};

// 65536頂点まで
static unsigned short g_MeshFieldIndex[NUM_INDEX]{
	// 0, 1, 2,  0, 3, 1,
};

void Meshfield_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
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

	// ここにfor分でメッシュフィールドの頂点を書く
	for (int z = 0; z < FIELD_MESH_V_VERTEX_COUNT; z++)
	{
		for (int x = 0; x < FIELD_MESH_H_VERTEX_COUNT; x++)
		{
			// 横　＋　横の最大数　×　縦
			int index = x + FIELD_MESH_H_VERTEX_COUNT * z;
			g_MeshFieldVertex[index].position = { x * FIELD_MESH_WIDTH, 0.0f, z * FIELD_MESH_DEPTH };
			g_MeshFieldVertex[index].normal = { 0.0f, 1.0f, 0.0f }; // 上向き
			// カラーを変えたい場合はここを変更
			g_MeshFieldVertex[index].color = { 0.0f, 1.0f, 0.0f, 1.0f };
			g_MeshFieldVertex[index].texcoord = { x * 1.0f, z * 1.0f };
		}
	}

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = g_MeshFieldVertex; // このあと読み書きできない

	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);

	// インデックスバッファ作成
	bd.ByteWidth = sizeof(unsigned short) * NUM_INDEX; // sizeof(g_CubeIndex)
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

	// ここにfor分でメッシュフィールドのインデックスを書く
	int idx = 0;
	for (int v = 0; v < FIELD_MESH_V_COUNT; v++)
	{
		for (int h = 0; h < FIELD_MESH_H_COUNT; h++)
		{
			int v0 = h + FIELD_MESH_H_VERTEX_COUNT * v;
			int v1 = (h + 1) + FIELD_MESH_H_VERTEX_COUNT * (v + 1);
			int v2 = h + FIELD_MESH_H_VERTEX_COUNT * (v + 1);
			int v3 = (h + 1) + FIELD_MESH_H_VERTEX_COUNT * v;
			g_MeshFieldIndex[idx++] = static_cast<unsigned short>(v0);
			g_MeshFieldIndex[idx++] = static_cast<unsigned short>(v2);
			g_MeshFieldIndex[idx++] = static_cast<unsigned short>(v1);
			g_MeshFieldIndex[idx++] = static_cast<unsigned short>(v0);
			g_MeshFieldIndex[idx++] = static_cast<unsigned short>(v1);
			g_MeshFieldIndex[idx++] = static_cast<unsigned short>(v3);
		}
	}

	sd.pSysMem = g_MeshFieldIndex; // このあと読み書きできない

	g_pDevice->CreateBuffer(&bd, &sd, &g_pIndexBuffer);

	g_MeshfieldTexId = Texture_Load(L"resource/texture/terra-cotta-tile-mid.png");

	ShaderField_Initialize(g_pDevice, g_pContext); // ほかのシェーダーはmain.cppで初期化している
}

void Meshfield_Finalize(void)
{
	ShaderField_Finalize();
	SAFE_RELEASE(g_pVertexBuffer);
	SAFE_RELEASE(g_pIndexBuffer);
}

void Meshfield_Draw(void)
{
	// シェーダーを描画パイプラインに設定
	Shader_Field_Begin();

	Texture_SetTexture(g_MeshfieldTexId); // テクスチャをセット

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// インデックスバッファを描画パイプラインに設定
	g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0); // インデックス数によってR32に変える

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点シェーダーにワールド座標変換行列の作成
	float offset_x = FIELD_MESH_H_COUNT * FIELD_MESH_DEPTH * 0.5f;
	float offset_z = FIELD_MESH_V_COUNT * FIELD_MESH_DEPTH * 0.5f;
	ShaderField_SetWorldMatrix(XMMatrixTranslation(-offset_x, 0.0f, -offset_z));

	ShaderField_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 色セット

	// ポリゴン描画命令発行
	g_pContext->DrawIndexed(NUM_INDEX, 0, 0); // 描画
}
