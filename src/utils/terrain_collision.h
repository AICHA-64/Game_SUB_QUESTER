// ----------------------------------------------------
// 三角形メッシュ地形との当たり判定 [terrain_collision.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-01-11
// Version: 1.1
// ----------------------------------------------------
#ifndef TERRAIN_COLLISION_H
#define TERRAIN_COLLISION_H

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>

// 前方宣言
struct MODEL;

// 三角形構造体
struct Triangle
{
    DirectX::XMFLOAT3 v0, v1, v2;  // 3頂点
    DirectX::XMFLOAT3 normal;      // 面法線
};

// Octreeノード
struct OctreeNode
{
    DirectX::XMFLOAT3 center;      // ノードの中心
    float halfSize;                 // ノードの半分のサイズ
    std::vector<int> triangleIndices; // このノードに含まれる三角形のインデックス
    std::unique_ptr<OctreeNode> children[8]; // 子ノード
    bool isLeaf;                    // 葉ノードかどうか
    
    OctreeNode() : center{ 0, 0, 0 }, halfSize(0), isLeaf(true) { }
};

// 地形メッシュデータ
struct TerrainMesh
{
    std::vector<Triangle> triangles;
    DirectX::XMFLOAT3 aabbMin{}; // バウンディングボックス最小
    DirectX::XMFLOAT3 aabbMax{}; // バウンディングボックス最大
    
    // Octree（空間分割による高速化）
    std::unique_ptr<OctreeNode> octreeRoot;
    int maxDepth{};              // Octreeの最大深度
    int maxTrianglesPerNode{};   // ノードあたりの最大三角形数
};

// レイキャスト結果
struct RaycastHit
{
    bool hit;                    // ヒットしたか
    float distance;              // レイ原点からの距離
    DirectX::XMFLOAT3 point;     // 衝突点
    DirectX::XMFLOAT3 normal;    // 衝突面の法線
    int triangleIndex;           // ヒットした三角形のインデックス
};

// 球体衝突結果
struct SphereCollisionResult
{
    bool hit;                    // 衝突したか
    DirectX::XMFLOAT3 pushVector;// 押し戻しベクトル
    DirectX::XMFLOAT3 normal;    // 衝突面の法線
    DirectX::XMFLOAT3 contactPoint; // 接触点
};

// ===============================================
// 地形メッシュの初期化・解放
// ===============================================

// MODELから三角形ポリを抽出してTerrainMeshを作成
TerrainMesh* TerrainCollision_CreateFromModel(MODEL* model, 
    const DirectX::XMFLOAT3& position = {0,0,0}, // position: モデルのワールド位置
    float scale = 1.0f,                          // scale: モデルのスケール（ModelLoadのscaleと同じ値）
    int maxOctreeDepth = 6,                      // maxOctreeDepth: Octreeの最大深度（デフォルト6、大きいほど精度UPするけどメモリ増）
    int maxTrianglesPerNode = 16);               // maxTrianglesPerNode: ノードあたりの最大三角形数（これ以下なら分割しない）

void TerrainCollision_Release(TerrainMesh* terrain);

// レイと地形の衝突判定
RaycastHit TerrainCollision_Raycast(
    const TerrainMesh* terrain,
    const DirectX::XMFLOAT3& origin,
    const DirectX::XMFLOAT3& direction,
    float maxDistance = 1000.0f);

// 下方向へのレイキャストで地面高さを取得
bool TerrainCollision_GetGroundHeight(
    const TerrainMesh* terrain,
    const DirectX::XMFLOAT3& position,
    float& groundY,
    DirectX::XMFLOAT3* outNormal = nullptr);

// 球体と地形メッシュの衝突判定
SphereCollisionResult TerrainCollision_SphereVsTerrain(
    const TerrainMesh* terrain,
    const DirectX::XMFLOAT3& center,
    float radius);

// 球体と地形メッシュの衝突を解決（押し戻し）
bool TerrainCollision_ResolveSphereCollision(
    const TerrainMesh* terrain,
    DirectX::XMFLOAT3& center,
    DirectX::XMFLOAT3& velocity,
    float radius);

// カプセルと地形メッシュの衝突を解決
bool TerrainCollision_ResolveCapsuleCollision(
    const TerrainMesh* terrain,
    DirectX::XMFLOAT3& bottom,
    float height,
    float radius,
    DirectX::XMFLOAT3& velocity);

// 点から三角形への最近接点を計算
DirectX::XMFLOAT3 TerrainCollision_ClosestPointOnTriangle(
    const DirectX::XMFLOAT3& point,
    const Triangle& triangle);

// デバッグ情報取得
int TerrainCollision_GetTriangleCount(const TerrainMesh* terrain);
int TerrainCollision_GetLastCheckedTriangleCount(); // 最後の判定でチェックした三角形数

#endif // TERRAIN_COLLISION_H
