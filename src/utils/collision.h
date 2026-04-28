// ----------------------------------------------------
// コリジョン判定 [collision.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-07-03
// Version: 1.0
// ----------------------------------------------------
#ifndef COLLISION_H
#define COLLISION_H

#include <d3d11.h>
#include <DirectXMath.h>

struct Sphere
{
	DirectX::XMFLOAT3 center; // 中心位置
	float radius;              // 半径
};

struct Circle
{
	DirectX::XMFLOAT2 center; // 中心位置
	float radius;              // 半径
};

struct Box
{
	DirectX::XMFLOAT2 center;
	float half_width;   // 幅
	float half_height;  // 高さ
};

struct AABB
{
	DirectX::XMFLOAT3 min;
	DirectX::XMFLOAT3 max;

	// インライン関数で中心位置を取得
	DirectX::XMFLOAT3 GetCenter() const {

		DirectX::XMFLOAT3 center;

		center.x = min.x + (max.x - min.x) * 0.5f;
		center.y = min.y + (max.y - min.y) * 0.5f;
		center.z = min.z + (max.z - min.z) * 0.5f;

		return center;
	}

	// インライン関数で半分の大きさを取得
	DirectX::XMFLOAT3 GetHalfSize() const {
		DirectX::XMFLOAT3 half_size;
		half_size.x = (max.x - min.x) * 0.5f;
		half_size.y = (max.y - min.y) * 0.5f;
		half_size.z = (max.z - min.z) * 0.5f;
		return half_size;
	}
};

struct Hit
{
	bool is_Hit; // 当たったかどうか
	DirectX::XMFLOAT3 normal; // 当たった面の向き
};


bool Collision_IsOverlapSphere(const Sphere& a, const Sphere& b);
bool Collision_IsOverlapSphere(const Sphere& a, const DirectX::XMFLOAT3& point);
bool Collision_IsOverlapCircle(const Circle& a, const Circle& b);
bool Collision_IsOverlapBox(const Box& a, const Box& b);
bool Collision_IsOverlapAABB(const AABB& a, const AABB& b);
bool Collision_IsOverlapSphereAABB(const Sphere& sphere, const AABB& aabb);

// aのどの面にbが衝突したか？
Hit Collision_IsHitAABB(const AABB& a, const AABB& b);

void Collision_DebugInitialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Collision_DebugFinalize();
void Collision_DebugDraw(const Circle& circle, const DirectX::XMFLOAT4& color = {1.0f, 0.0f, 0.0f, 1.0f});
void Collision_DebugDraw(const Box& box, const DirectX::XMFLOAT4& color = { 1.0f, 0.0f, 0.0f, 1.0f });

#endif // COLLISION_H
