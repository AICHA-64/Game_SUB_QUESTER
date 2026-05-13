// ----------------------------------------------------
// コリジョン判定 [collision.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-07-03
// Version: 1.0
// ----------------------------------------------------

#include "collision.h"
#include "direct3d.h"
#include "texture.h"
#include "shader.h"
using namespace DirectX;
#include <algorithm>
static constexpr int NUM_VERTEX = 5000; // 頂点数
static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static int g_WhiteTexId = -1; // 白テクスチャのID


bool Collision_IsOverlapSphere(const Sphere& a, const Sphere& b)
{
	XMVECTOR ac = XMLoadFloat3(&a.center);
	XMVECTOR bc = XMLoadFloat3(&b.center);
	XMVECTOR lsq = XMVector3LengthSq(bc - ac); // 中心間の距離の二乗

	return (a.radius + b.radius) * (a.radius + b.radius) > XMVectorGetX(lsq);
}

bool Collision_IsOverlapSphere(const Sphere& a, const DirectX::XMFLOAT3& point)
{
	XMVECTOR ac = XMLoadFloat3(&a.center);
	XMVECTOR bc = XMLoadFloat3(&point);
	XMVECTOR lsq = XMVector3LengthSq(bc - ac); // 中心間の距離の二乗

	return a.radius * a.radius > XMVectorGetX(lsq);
}

bool Collision_IsOverlapCircle(const Circle& a, const Circle& b)
{
	float xl = b.center.x - a.center.x;
	float yl = b.center.y - a.center.y;

	return (a.radius + b.radius) * (a.radius + b.radius) > (xl * xl + yl * yl);

	//XMVECTOR ac = XMLoadFloat2(&a.center);
	//XMVECTOR bc = XMLoadFloat2(&b.center);
	//XMVECTOR lsq = XMVector2LengthSq(bc - ac); // 中心間の距離の二乗

	//return (a.radius + b.radius) * (a.radius + b.radius) > XMVectorGetX(lsq); // 半径の和の二乗と比較
}

bool Collision_IsOverlapBox(const Box& a, const Box& b)
{
	float at = a.center.y - a.half_height;
	float ab = a.center.y + a.half_height;
	float al = a.center.x - a.half_width;
	float ar = a.center.x + a.half_width;
	float bt = b.center.y - b.half_height;
	float bb = b.center.y + b.half_height;
	float bl = b.center.x - b.half_width;
	float br = b.center.x + b.half_width;

	return al < br && ar > bl && at < bb && ab > bt;
}

bool Collision_IsOverlapAABB(const AABB& a, const AABB& b)
{
	return a.min.x < b.max.x
		&& a.max.x > b.min.x
		&& a.min.y < b.max.y
		&& a.max.y > b.min.y
		&& a.min.z < b.max.z
		&& a.max.z > b.min.z;
}

bool Collision_IsOverlapSphereAABB(const Sphere& sphere, const AABB& aabb)
{
	// AABBの中で、球の中心に最も近い点（最接近点）を求める
	float closestX = std::max(aabb.min.x, std::min(sphere.center.x, aabb.max.x));
	float closestY = std::max(aabb.min.y, std::min(sphere.center.y, aabb.max.y));
	float closestZ = std::max(aabb.min.z, std::min(sphere.center.z, aabb.max.z));

	// 最接近点から球の中心までの距離を計算する
	float distanceX = sphere.center.x - closestX;
	float distanceY = sphere.center.y - closestY;
	float distanceZ = sphere.center.z - closestZ;
	
	float distanceSq = (distanceX * distanceX) + (distanceY * distanceY) + (distanceZ * distanceZ);

	// 距離の2乗が球の半径の2乗より小さければ衝突している
	return distanceSq <= (sphere.radius * sphere.radius);
}

Hit Collision_IsHitAABB(const AABB& a, const AABB& b)
{
	Hit hit{};

	// AABB同士の当たり判定 重なったか？
	hit.is_Hit = Collision_IsOverlapAABB(a, b);

	
	if (!hit.is_Hit) {
		return hit; // 当たっていなければ終了
	}

	// 各軸の深度を調べる
	float xdepth = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
	float ydepth = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
	float zdepth = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

	// 一番浅い軸を調べる
	bool isShallowX = false;
	bool isShallowY = false;
	bool isShallowZ = false;

	if (xdepth > ydepth) {
		if (ydepth > zdepth) {
			isShallowZ = true; // Zの面
		}
		else {
			isShallowY = true; // Yの面
		}
	}
	else {
		if(zdepth > xdepth){
			isShallowX = true; // Xの面
		}
		else {
			isShallowZ = true; // Zの面
		}
	}

	// +-どっちから当たったか
	XMFLOAT3 a_center = a.GetCenter();
	XMFLOAT3 b_center = b.GetCenter();
	XMVECTOR normal = XMLoadFloat3(&b_center) - XMLoadFloat3(&a_center);

	if(isShallowX){
		normal = XMVector3Normalize(normal * XMVECTOR{ 1.0f, 0.0f, 0.0f });
	}
	else if(isShallowY){
		normal = XMVector3Normalize(normal * XMVECTOR{ 0.0f, 1.0f, 0.0f });
	}
	else if(isShallowZ){
		normal = XMVector3Normalize(normal * XMVECTOR{ 0.0f, 0.0f, 1.0f });
	}

	XMStoreFloat3(&hit.normal, normal);
	return hit;
}


// 頂点構造体
struct Vertex
{
	XMFLOAT3 position;	// 頂点座標
	XMFLOAT4 color;		// 色
	XMFLOAT2 uv;	// テクスチャー
};


void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	g_WhiteTexId = Texture_Load(L"resource/texture/white.png"); // 白テクスチャの読み込み
}

void Collision_DebugFinalize()
{
	SAFE_RELEASE(g_pVertexBuffer); // 頂点バッファの解放
}

void Collision_DebugDraw(const Circle& circle, const DirectX::XMFLOAT4& color)
{
	// 点の数を検出
	int numVertex = (int)(circle.radius * 2.0f * XM_PI + 1);

	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;

	// 頂点情報を書き込み
	//const float SCREEN_WIDTH = (float)Direct3D_GetBackBufferWidth();
	//const float SCREEN_HEIGHT = (float)Direct3D_GetBackBufferHeight();

	//float x = 32.0f;
	//float y = 32.0f;
	//float w = 512;
	//float h = 512;

	// 画面の左上から右下に向かう線分を描画する 時計回り

	const float rad = XM_2PI / numVertex;

	for (int i = 0; i < numVertex; i++)
	{
		v[i].position.x = cosf(rad * i) * circle.radius + circle.center.x;
		v[i].position.y = sinf(rad * i) * circle.radius + circle.center.y;
		v[i].position.z = 0.0f;
		v[i].color = color;
		v[i].uv = { 0.0f, 0.0f };
	}

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// ワールド変換行列を設定
	Shader_SetWorldMatrix(XMMatrixIdentity());

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// テクスチャの設定
	Texture_SetTexture(g_WhiteTexId);

	// ポリゴン描画命令発行
	g_pContext->Draw(numVertex, 0);
}

void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4& color)
{
	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* v = (Vertex*)msr.pData;
		  
	// 頂点情報を書き込み
	v[0].position = { box.center.x - box.half_width, box.center.y - box.half_height, 0.0f };
	v[1].position = { box.center.x + box.half_width, box.center.y - box.half_height, 0.0f };
	v[2].position = { box.center.x + box.half_width, box.center.y + box.half_height, 0.0f };
	v[3].position = { box.center.x - box.half_width, box.center.y + box.half_height, 0.0f };
	v[4].position = { box.center.x - box.half_width, box.center.y - box.half_height, 0.0f };

	for (int i = 0; i < 5; i++) {
		v[i].color = color;
		v[i].uv = { 0.0f, 0.0f };
	}

	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// ワールド変換行列を設定
	Shader_SetWorldMatrix(XMMatrixIdentity());

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

	// テクスチャの設定
	Texture_SetTexture(g_WhiteTexId);

	// ポリゴン描画命令発行
	g_pContext->Draw(5, 0);
}
