// ----------------------------------------------------
// デバッグ描画 [debug_draw.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-10-31
// Version: 1.0
// ----------------------------------------------------
#include "debug_draw.h"
#include "direct3d.h"
#include "shader3d.h"
#include <DirectXMath.h>
using namespace DirectX;

// AABBの12本のエッジを表す24頂点（各エッジで2頂点）
static constexpr int NUM_AABB_VERTICES = 24;

// 球体の円を描画するための頂点数（3つの円 x セグメント数 x 2）
static constexpr int SPHERE_SEGMENTS = 16;
static constexpr int NUM_SPHERE_VERTICES = SPHERE_SEGMENTS * 3 * 2; // 3つの円（XY, YZ, ZX平面）

// 頂点構造体（位置と色のみ）
struct DebugVertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;   // シェーダー互換のためダミー
	XMFLOAT4 color;
	XMFLOAT2 texcoord; // シェーダー互換のためダミー
};

static ID3D11Buffer* g_pVertexBuffer = nullptr;
static ID3D11Buffer* g_pSphereVertexBuffer = nullptr;
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;
static ID3D11RasterizerState* g_pRasterizerStateWireframe = nullptr;
static ID3D11RasterizerState* g_pRasterizerStateSolid = nullptr;

// ミニマップ描画モードのフラグ
static bool g_IsMinimapMode = false;

void DebugDraw_SetMinimapMode(bool isMinimapMode)
{
	g_IsMinimapMode = isMinimapMode;
}

bool DebugDraw_IsMinimapMode()
{
	return g_IsMinimapMode;
}

void DebugDraw_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 動的頂点バッファの作成（AABB用）
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(DebugVertex) * NUM_AABB_VERTICES;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, nullptr, &g_pVertexBuffer);

	// 動的頂点バッファの作成（Sphere用）
	bd.ByteWidth = sizeof(DebugVertex) * NUM_SPHERE_VERTICES;
	g_pDevice->CreateBuffer(&bd, nullptr, &g_pSphereVertexBuffer);

	// ワイヤーフレーム用ラスタライザーステートの作成
	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = D3D11_FILL_WIREFRAME;
	rd.CullMode = D3D11_CULL_NONE;
	rd.DepthClipEnable = TRUE;
	g_pDevice->CreateRasterizerState(&rd, &g_pRasterizerStateWireframe);

	// ソリッド用ラスタライザーステート（復元用）
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_BACK;
	g_pDevice->CreateRasterizerState(&rd, &g_pRasterizerStateSolid);
}

void DebugDraw_Finalize()
{
	SAFE_RELEASE(g_pRasterizerStateSolid);
	SAFE_RELEASE(g_pRasterizerStateWireframe);
	SAFE_RELEASE(g_pSphereVertexBuffer);
	SAFE_RELEASE(g_pVertexBuffer);
}

void DebugDraw_AABB(const AABB& aabb, const DirectX::XMFLOAT4& color)
{
	// AABBの8頂点を計算
	XMFLOAT3 corners[8] = {
		{ aabb.min.x, aabb.min.y, aabb.min.z }, // 0: 左下手前
		{ aabb.max.x, aabb.min.y, aabb.min.z }, // 1: 右下手前
		{ aabb.max.x, aabb.max.y, aabb.min.z }, // 2: 右上手前
		{ aabb.min.x, aabb.max.y, aabb.min.z }, // 3: 左上手前
		{ aabb.min.x, aabb.min.y, aabb.max.z }, // 4: 左下奥
		{ aabb.max.x, aabb.min.y, aabb.max.z }, // 5: 右下奥
		{ aabb.max.x, aabb.max.y, aabb.max.z }, // 6: 右上奥
		{ aabb.min.x, aabb.max.y, aabb.max.z }, // 7: 左上奥
	};

	// 12本のエッジを構成する頂点インデックス
	int edges[12][2] = {
		// 手前面
		{0, 1}, {1, 2}, {2, 3}, {3, 0},
		// 奥面
		{4, 5}, {5, 6}, {6, 7}, {7, 4},
		// 手前と奥をつなぐエッジ
		{0, 4}, {1, 5}, {2, 6}, {3, 7}
	};

	// 頂点データを作成
	DebugVertex vertices[NUM_AABB_VERTICES];
	for (int i = 0; i < 12; i++)
	{
		vertices[i * 2 + 0].position = corners[edges[i][0]];
		vertices[i * 2 + 0].normal = { 0.0f, 1.0f, 0.0f };
		vertices[i * 2 + 0].color = color;
		vertices[i * 2 + 0].texcoord = { 0.0f, 0.0f };

		vertices[i * 2 + 1].position = corners[edges[i][1]];
		vertices[i * 2 + 1].normal = { 0.0f, 1.0f, 0.0f };
		vertices[i * 2 + 1].color = color;
		vertices[i * 2 + 1].texcoord = { 0.0f, 0.0f };
	}

	// 頂点バッファを更新
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, vertices, sizeof(vertices));
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// シェーダーを設定
	Shader3d_Begin();
	Shader3d_SetWorldMatrix(XMMatrixIdentity());
	Shader3d_SetColor(color);

	// 頂点バッファを設定
	UINT stride = sizeof(DebugVertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// ラインリストで描画
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// 深度テストを一時的に無効化（オプション：常に前面に表示したい場合）
	// Direct3D_SetDepthEnable(false);

	// 描画
	g_pContext->Draw(NUM_AABB_VERTICES, 0);

	// 深度テストを復元
	// Direct3D_SetDepthEnable(true);
}

void DebugDraw_Sphere(const Sphere& sphere, const DirectX::XMFLOAT4& color)
{
	// ミニマップモード以外では描画しない（リリースビルド時）
#ifndef _DEBUG
	if (!g_IsMinimapMode) {
		return;
	}
#endif

	DebugVertex vertices[NUM_SPHERE_VERTICES];
	int vertexIndex = 0;

	const float PI = 3.14159265359f;
	const float angleStep = (2.0f * PI) / SPHERE_SEGMENTS;

	// XY平面の円を描画
	for (int i = 0; i < SPHERE_SEGMENTS; i++)
	{
		float angle1 = angleStep * i;
		float angle2 = angleStep * (i + 1);

		XMFLOAT3 p1 = {
			sphere.center.x + sphere.radius * cosf(angle1),
			sphere.center.y + sphere.radius * sinf(angle1),
			sphere.center.z
		};

		XMFLOAT3 p2 = {
			sphere.center.x + sphere.radius * cosf(angle2),
			sphere.center.y + sphere.radius * sinf(angle2),
			sphere.center.z
		};

		vertices[vertexIndex].position = p1;
		vertices[vertexIndex].normal = { 0.0f, 1.0f, 0.0f };
		vertices[vertexIndex].color = color;
		vertices[vertexIndex].texcoord = { 0.0f, 0.0f };
		vertexIndex++;

		vertices[vertexIndex].position = p2;
		vertices[vertexIndex].normal = { 0.0f, 1.0f, 0.0f };
		vertices[vertexIndex].color = color;
		vertices[vertexIndex].texcoord = { 0.0f, 0.0f };
		vertexIndex++;
	}

	// YZ平面の円を描画
	for (int i = 0; i < SPHERE_SEGMENTS; i++)
	{
		float angle1 = angleStep * i;
		float angle2 = angleStep * (i + 1);

		XMFLOAT3 p1 = {
			sphere.center.x,
			sphere.center.y + sphere.radius * cosf(angle1),
			sphere.center.z + sphere.radius * sinf(angle1)
		};

		XMFLOAT3 p2 = {
			sphere.center.x,
			sphere.center.y + sphere.radius * cosf(angle2),
			sphere.center.z + sphere.radius * sinf(angle2)
		};

		vertices[vertexIndex].position = p1;
		vertices[vertexIndex].normal = { 0.0f, 1.0f, 0.0f };
		vertices[vertexIndex].color = color;
		vertices[vertexIndex].texcoord = { 0.0f, 0.0f };
		vertexIndex++;

		vertices[vertexIndex].position = p2;
		vertices[vertexIndex].normal = { 0.0f, 1.0f, 0.0f };
		vertices[vertexIndex].color = color;
		vertices[vertexIndex].texcoord = { 0.0f, 0.0f };
		vertexIndex++;
	}

	// ZX平面の円を描画
	for (int i = 0; i < SPHERE_SEGMENTS; i++)
	{
		float angle1 = angleStep * i;
		float angle2 = angleStep * (i + 1);

		XMFLOAT3 p1 = {
			sphere.center.x + sphere.radius * cosf(angle1),
			sphere.center.y,
			sphere.center.z + sphere.radius * sinf(angle1)
		};

		XMFLOAT3 p2 = {
			sphere.center.x + sphere.radius * cosf(angle2),
			sphere.center.y,
			sphere.center.z + sphere.radius * sinf(angle2)
		};

		vertices[vertexIndex].position = p1;
		vertices[vertexIndex].normal = { 0.0f, 1.0f, 0.0f };
		vertices[vertexIndex].color = color;
		vertices[vertexIndex].texcoord = { 0.0f, 0.0f };
		vertexIndex++;

		vertices[vertexIndex].position = p2;
		vertices[vertexIndex].normal = { 0.0f, 1.0f, 0.0f };
		vertices[vertexIndex].color = color;
		vertices[vertexIndex].texcoord = { 0.0f, 0.0f };
		vertexIndex++;
	}

	// 頂点バッファを更新
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	g_pContext->Map(g_pSphereVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, vertices, sizeof(vertices));
	g_pContext->Unmap(g_pSphereVertexBuffer, 0);

	// シェーダーを設定
	Shader3d_Begin();
	Shader3d_SetWorldMatrix(XMMatrixIdentity());
	Shader3d_SetColor(color);

	// 頂点バッファを設定
	UINT stride = sizeof(DebugVertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pSphereVertexBuffer, &stride, &offset);

	// ラインリストで描画
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	// ミニマップモードでは深度テストを無効化
	if (g_IsMinimapMode)
	{
		// 深度テストを無効化
		// Direct3D_SetDepthEnable(false);
	}

	// 描画
	g_pContext->Draw(NUM_SPHERE_VERTICES, 0);

	// ミニマップモードでは深度テストを復元しない（常に前面に表示）
}
