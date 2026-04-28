// ----------------------------------------------------
// XZ平面グリッドの表示 [grid.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-11
// Version: 1.0
// ----------------------------------------------------

#include "grid.h"
#include "direct3d.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "shader3d.h"

static constexpr int GRID_H_COUNT = 10;
static constexpr int GRID_V_COUNT = 10;
static constexpr int GRID_H_LINE_COUNT = GRID_H_COUNT + 1;
static constexpr int GRID_V_LINE_COUNT = GRID_V_COUNT + 1;
static constexpr int NUM_VERTEX = GRID_H_LINE_COUNT * 2 + GRID_V_LINE_COUNT * 2; // 頂点数 一つの面に三角形×2

static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// 3D頂点構造体
struct Vertex3d
{
	XMFLOAT3 position;	// 頂点座標
	XMFLOAT4 color;		// 色
	// XMFLOAT2 uv;	// テクスチャー
};


static Vertex3d g_GridVertex[NUM_VERTEX]{
};


void Grid_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex3d) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA sd{};
	sd.pSysMem = g_GridVertex; // このあと読み書きできない

	//g_GridVertex[0] = { { 0.0f , 0.0f, 5.0f }, { 0.0f , 1.0f, 0.0f, 1.0f } };
	//g_GridVertex[1] = { { 0.0f , 0.0f,-5.0f }, { 0.0f , 1.0f, 0.0f, 1.0f } };

	//縦線
	float x = -5.0f;
	for (int i = 0; i < GRID_H_LINE_COUNT * 2; i += 2)
	{
		g_GridVertex[i	  ] = { { x , 0.01f,  5.0f }, { 1.0f , 1.0f, 1.0f, 1.0f } };
		g_GridVertex[i + 1] = { { x , 0.01f, -5.0f }, { 1.0f , 1.0f, 1.0f, 1.0f } };
		x += 1.0f;
	}

	// 横線
	float z = -5.0f;
	for (int i = GRID_V_LINE_COUNT * 2; i < NUM_VERTEX; i += 2)
	{
		g_GridVertex[i	  ] = { {  5.0f , 0.01f,  z }, { 1.0f , 1.0f, 1.0f, 1.0f } };
		g_GridVertex[i + 1] = { { -5.0f , 0.01f,  z }, { 1.0f , 1.0f, 1.0f, 1.0f } };
		z += 1.0f;
	}

	g_pDevice->CreateBuffer(&bd, &sd, &g_pVertexBuffer);
}

void Grid_Finalize(void)
{
	SAFE_RELEASE(g_pVertexBuffer);
}

void Grid_Draw(void)
{
	// シェーダーを描画パイプラインに設定
	Shader3d_Begin();

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex3d);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// ワールド座標変換行列の作成
	XMMATRIX mtxWorld = XMMatrixIdentity(); // 単位行列の作成

	// 頂点シェーダーにワールド座標変換行列を設定
	Shader3d_SetWorldMatrix(mtxWorld);

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// ポリゴン描画命令発行
	g_pContext->Draw(NUM_VERTEX, 0); // 今だけ前面描画
}
