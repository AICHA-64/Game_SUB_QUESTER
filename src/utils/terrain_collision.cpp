// ----------------------------------------------------
// 三角形メッシュ地形との当たり判定 [terrain_collision.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: Date: 2026-01-11
// Version: 1.1 - Octree最適化追加
// ----------------------------------------------------
#include "terrain_collision.h"
#include "model.h"
#include <algorithm>
#include <cmath>
#include <limits>

using namespace DirectX;

// デバッグ用：最後にチェックした三角形数
static int g_LastCheckedTriangleCount = 0;

// 三角形の面法線を計算
static XMFLOAT3 CalculateTriangleNormal(const XMFLOAT3& v0, const XMFLOAT3& v1, const XMFLOAT3& v2)
{
    XMVECTOR e1 = XMVectorSubtract(XMLoadFloat3(&v1), XMLoadFloat3(&v0));
    XMVECTOR e2 = XMVectorSubtract(XMLoadFloat3(&v2), XMLoadFloat3(&v0));
    XMVECTOR normal = XMVector3Normalize(XMVector3Cross(e1, e2));
    
    XMFLOAT3 result;
    XMStoreFloat3(&result, normal);
    return result;
}

// 三角形のAABBを計算
static void GetTriangleAABB(const Triangle& tri, XMFLOAT3& outMin, XMFLOAT3& outMax)
{
    outMin.x = std::min({ tri.v0.x, tri.v1.x, tri.v2.x });
    outMin.y = std::min({ tri.v0.y, tri.v1.y, tri.v2.y });
    outMin.z = std::min({ tri.v0.z, tri.v1.z, tri.v2.z });
    outMax.x = std::max({ tri.v0.x, tri.v1.x, tri.v2.x });
    outMax.y = std::max({ tri.v0.y, tri.v1.y, tri.v2.y });
    outMax.z = std::max({ tri.v0.z, tri.v1.z, tri.v2.z });
}

// AABBとAABBの交差判定
static bool AABBIntersects(const XMFLOAT3& aMin, const XMFLOAT3& aMax,
                           const XMFLOAT3& bMin, const XMFLOAT3& bMax)
{
    return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
           (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
           (aMin.z <= bMax.z && aMax.z >= bMin.z);
}

// 球とAABBの交差判定
static bool SphereAABBIntersects(const XMFLOAT3& center, float radius,
                                  const XMFLOAT3& boxMin, const XMFLOAT3& boxMax)
{
    float sqDist = 0.0f;
    
    if (center.x < boxMin.x) sqDist += (boxMin.x - center.x) * (boxMin.x - center.x);
    else if (center.x > boxMax.x) sqDist += (center.x - boxMax.x) * (center.x - boxMax.x);
    
    if (center.y < boxMin.y) sqDist += (boxMin.y - center.y) * (boxMin.y - center.y);
    else if (center.y > boxMax.y) sqDist += (center.y - boxMax.y) * (center.y - boxMax.y);
    
    if (center.z < boxMin.z) sqDist += (boxMin.z - center.z) * (boxMin.z - center.z);
    else if (center.z > boxMax.z) sqDist += (center.z - boxMax.z) * (center.z - boxMax.z);
    
    return sqDist <= radius * radius;
}

// レイとAABBの交差判定
static bool RayAABBIntersects(const XMFLOAT3& origin, const XMFLOAT3& dirInv,
                               const XMFLOAT3& boxMin, const XMFLOAT3& boxMax,
                               float maxDist)
{
    float t1 = (boxMin.x - origin.x) * dirInv.x;
    float t2 = (boxMax.x - origin.x) * dirInv.x;
    float t3 = (boxMin.y - origin.y) * dirInv.y;
    float t4 = (boxMax.y - origin.y) * dirInv.y;
    float t5 = (boxMin.z - origin.z) * dirInv.z;
    float t6 = (boxMax.z - origin.z) * dirInv.z;
    
    float tmin = std::max({ std::min(t1, t2), std::min(t3, t4), std::min(t5, t6) });
    float tmax = std::min({ std::max(t1, t2), std::max(t3, t4), std::max(t5, t6) });
    
    return tmax >= 0 && tmin <= tmax && tmin <= maxDist;
}

// ===============================================
// Octree構築
// ===============================================

static void BuildOctreeNode(OctreeNode* node, const std::vector<Triangle>& triangles,
                            const std::vector<int>& indices, int depth, int maxDepth, int maxTriPerNode)
{
    node->triangleIndices = indices;
    node->isLeaf = true;
    
    // 終了条件：最大深度に達したか、三角形数が少ない
    if (depth >= maxDepth || static_cast<int>(indices.size()) <= maxTriPerNode)
    {
        return;
    }
    
    // 子ノードを作成
    node->isLeaf = false;
    float childHalf = node->halfSize * 0.5f;
    
    // 8つの子ノードのオフセット
    const float offsets[8][3] = {
        {-1, -1, -1}, {+1, -1, -1}, {-1, +1, -1}, {+1, +1, -1},
        {-1, -1, +1}, {+1, -1, +1}, {-1, +1, +1}, {+1, +1, +1}
    };
    
    for (int i = 0; i < 8; i++)
    {
        node->children[i] = std::make_unique<OctreeNode>();
        node->children[i]->center = {
            node->center.x + offsets[i][0] * childHalf,
            node->center.y + offsets[i][1] * childHalf,
            node->center.z + offsets[i][2] * childHalf
        };
        node->children[i]->halfSize = childHalf;
        
        // このノードのAABB
        XMFLOAT3 nodeMin = {
            node->children[i]->center.x - childHalf,
            node->children[i]->center.y - childHalf,
            node->children[i]->center.z - childHalf
        };
        XMFLOAT3 nodeMax = {
            node->children[i]->center.x + childHalf,
            node->children[i]->center.y + childHalf,
            node->children[i]->center.z + childHalf
        };
        
        // この子ノードに含まれる三角形を収集
        std::vector<int> childIndices;
        for (int idx : indices)
        {
            XMFLOAT3 triMin, triMax;
            GetTriangleAABB(triangles[idx], triMin, triMax);
            
            if (AABBIntersects(triMin, triMax, nodeMin, nodeMax))
            {
                childIndices.push_back(idx);
            }
        }
        
        if (!childIndices.empty())
        {
            BuildOctreeNode(node->children[i].get(), triangles, childIndices,
                           depth + 1, maxDepth, maxTriPerNode);
        }
    }
    
    // 親ノードの三角形リストはクリア（子に移動したため）
    node->triangleIndices.clear();
}

// ===============================================
// Octree検索
// ===============================================

// 球との交差する三角形を収集
static void QueryOctreeSphere(const OctreeNode* node, const std::vector<Triangle>& triangles,
                               const XMFLOAT3& center, float radius,
                               std::vector<int>& outIndices)
{
    if (!node) return;
    
    // ノードのAABB
    XMFLOAT3 nodeMin = {
        node->center.x - node->halfSize,
        node->center.y - node->halfSize,
        node->center.z - node->halfSize
    };
    XMFLOAT3 nodeMax = {
        node->center.x + node->halfSize,
        node->center.y + node->halfSize,
        node->center.z + node->halfSize
    };
    
    // 球がこのノードと交差しなければスキップ
    if (!SphereAABBIntersects(center, radius, nodeMin, nodeMax))
    {
        return;
    }
    
    if (node->isLeaf)
    {
        // 葉ノード：三角形インデックスを追加
        for (int idx : node->triangleIndices)
        {
            outIndices.push_back(idx);
        }
    }
    else
    {
        // 子ノードを再帰的に検索
        for (int i = 0; i < 8; i++)
        {
            if (node->children[i])
            {
                QueryOctreeSphere(node->children[i].get(), triangles, center, radius, outIndices);
            }
        }
    }
}

// レイとの交差する三角形を収集
static void QueryOctreeRay(const OctreeNode* node, const std::vector<Triangle>& triangles,
                           const XMFLOAT3& origin, const XMFLOAT3& dirInv, float maxDist,
                           std::vector<int>& outIndices)
{
    if (!node) return;
    
    XMFLOAT3 nodeMin = {
        node->center.x - node->halfSize,
        node->center.y - node->halfSize,
        node->center.z - node->halfSize
    };
    XMFLOAT3 nodeMax = {
        node->center.x + node->halfSize,
        node->center.y + node->halfSize,
        node->center.z + node->halfSize
    };
    
    if (!RayAABBIntersects(origin, dirInv, nodeMin, nodeMax, maxDist))
    {
        return;
    }
    
    if (node->isLeaf)
    {
        for (int idx : node->triangleIndices)
        {
            outIndices.push_back(idx);
        }
    }
    else
    {
        for (int i = 0; i < 8; i++)
        {
            if (node->children[i])
            {
                QueryOctreeRay(node->children[i].get(), triangles, origin, dirInv, maxDist, outIndices);
            }
        }
    }
}

// レイと三角形の交差判定 (Möller–Trumbore algorithm)
static bool RayTriangleIntersect(
    const XMFLOAT3& rayOrigin,
    const XMFLOAT3& rayDir,
    const Triangle& tri,
    float& outDist,
    float& outBaryU,
    float& outBaryV)
{
    const float epsilon = 0.0000001f;
    
    XMVECTOR triV0 = XMLoadFloat3(&tri.v0);
    XMVECTOR triV1 = XMLoadFloat3(&tri.v1);
    XMVECTOR triV2 = XMLoadFloat3(&tri.v2);
    XMVECTOR origin = XMLoadFloat3(&rayOrigin);
    XMVECTOR dir    = XMLoadFloat3(&rayDir);
    
    XMVECTOR edge1   = XMVectorSubtract(triV1, triV0);
    XMVECTOR edge2   = XMVectorSubtract(triV2, triV0);
    
    XMVECTOR perpVec = XMVector3Cross(dir, edge2);
    float det = XMVectorGetX(XMVector3Dot(edge1, perpVec));
    
    if (det > -epsilon && det < epsilon)
        return false;
    
    float invDet = 1.0f / det;
    XMVECTOR toOrigin = XMVectorSubtract(origin, triV0);
    outBaryU = invDet * XMVectorGetX(XMVector3Dot(toOrigin, perpVec));
    
    if (outBaryU < 0.0f || outBaryU > 1.0f)
        return false;
    
    XMVECTOR crossVec = XMVector3Cross(toOrigin, edge1);
    outBaryV = invDet * XMVectorGetX(XMVector3Dot(dir, crossVec));
    
    if (outBaryV < 0.0f || outBaryU + outBaryV > 1.0f)
        return false;
    
    outDist = invDet * XMVectorGetX(XMVector3Dot(edge2, crossVec));
    
    return outDist > epsilon;
}

// 点が三角形の内側にあるかチェック
static bool PointInTriangle(const XMFLOAT3& p, const Triangle& tri)
{
    XMVECTOR v0 = XMLoadFloat3(&tri.v0);
    XMVECTOR v1 = XMLoadFloat3(&tri.v1);
    XMVECTOR v2 = XMLoadFloat3(&tri.v2);
    XMVECTOR pt = XMLoadFloat3(&p);
    
    XMVECTOR e0 = XMVectorSubtract(v1, v0);
    XMVECTOR e1 = XMVectorSubtract(v2, v0);
    XMVECTOR e2 = XMVectorSubtract(pt, v0);
    
    float d00 = XMVectorGetX(XMVector3Dot(e0, e0));
    float d01 = XMVectorGetX(XMVector3Dot(e0, e1));
    float d11 = XMVectorGetX(XMVector3Dot(e1, e1));
    float d20 = XMVectorGetX(XMVector3Dot(e2, e0));
    float d21 = XMVectorGetX(XMVector3Dot(e2, e1));
    
    float denom = d00 * d11 - d01 * d01;
    if (fabsf(denom) < 0.0001f) return false;
    
    float vv = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float uu = 1.0f - vv - w;
    
    return (uu >= 0.0f && vv >= 0.0f && w >= 0.0f);
}

// 線分上の最近接点
static XMFLOAT3 ClosestPointOnSegment(const XMFLOAT3& p, const XMFLOAT3& a, const XMFLOAT3& b)
{
    XMVECTOR pt = XMLoadFloat3(&p);
    XMVECTOR va = XMLoadFloat3(&a);
    XMVECTOR vb = XMLoadFloat3(&b);
    
    XMVECTOR ab = XMVectorSubtract(vb, va);
    float lenSq = XMVectorGetX(XMVector3Dot(ab, ab));
    if (lenSq < 0.0001f)
    {
        return a;
    }
    float t = XMVectorGetX(XMVector3Dot(XMVectorSubtract(pt, va), ab)) / lenSq;
    t = std::max(0.0f, std::min(1.0f, t));
    
    XMFLOAT3 result;
    XMStoreFloat3(&result, XMVectorAdd(va, XMVectorScale(ab, t)));
    return result;
}

TerrainMesh* TerrainCollision_CreateFromModel(MODEL* model, const XMFLOAT3& position, float scale,
                                               int maxOctreeDepth, int maxTrianglesPerNode)
{
    if (!model || !model->AiScene) return nullptr;
    
    TerrainMesh* terrain = new TerrainMesh();
    terrain->maxDepth = maxOctreeDepth;
    terrain->maxTrianglesPerNode = maxTrianglesPerNode;
    
    // AABBの初期化
    terrain->aabbMin = { FLT_MAX, FLT_MAX, FLT_MAX };
    terrain->aabbMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    
    const aiScene* scene = model->AiScene;
    
    // 全メッシュの三角形を抽出
    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        const aiMesh* mesh = scene->mMeshes[m];
        
        for (unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            const aiFace& face = mesh->mFaces[f];
            
            if (face.mNumIndices != 3) continue;
            
            Triangle tri;
            
            unsigned int i0 = face.mIndices[0];
            unsigned int i1 = face.mIndices[1];
            unsigned int i2 = face.mIndices[2];
            
            tri.v0 = { 
                mesh->mVertices[i0].x * scale + position.x,
                mesh->mVertices[i0].y * scale + position.y,
                mesh->mVertices[i0].z * scale + position.z 
            };
            tri.v1 = { 
                mesh->mVertices[i1].x * scale + position.x,
                mesh->mVertices[i1].y * scale + position.y,
                mesh->mVertices[i1].z * scale + position.z 
            };
            tri.v2 = { 
                mesh->mVertices[i2].x * scale + position.x,
                mesh->mVertices[i2].y * scale + position.y,
                mesh->mVertices[i2].z * scale + position.z 
            };
            
            tri.normal = CalculateTriangleNormal(tri.v0, tri.v1, tri.v2);
            
            terrain->triangles.push_back(tri);
            
            // AABB更新
            terrain->aabbMin.x = std::min({ terrain->aabbMin.x, tri.v0.x, tri.v1.x, tri.v2.x });
            terrain->aabbMin.y = std::min({ terrain->aabbMin.y, tri.v0.y, tri.v1.y, tri.v2.y });
            terrain->aabbMin.z = std::min({ terrain->aabbMin.z, tri.v0.z, tri.v1.z, tri.v2.z });
            terrain->aabbMax.x = std::max({ terrain->aabbMax.x, tri.v0.x, tri.v1.x, tri.v2.x });
            terrain->aabbMax.y = std::max({ terrain->aabbMax.y, tri.v0.y, tri.v1.y, tri.v2.y });
            terrain->aabbMax.z = std::max({ terrain->aabbMax.z, tri.v0.z, tri.v1.z, tri.v2.z });
        }
    }
    
    // Octreeを構築
    if (!terrain->triangles.empty())
    {
        terrain->octreeRoot = std::make_unique<OctreeNode>();
        
        // ルートノードの設定
        XMFLOAT3 center = {
            (terrain->aabbMin.x + terrain->aabbMax.x) * 0.5f,
            (terrain->aabbMin.y + terrain->aabbMax.y) * 0.5f,
            (terrain->aabbMin.z + terrain->aabbMax.z) * 0.5f
        };
        float halfSize = std::max({
            terrain->aabbMax.x - center.x,
            terrain->aabbMax.y - center.y,
            terrain->aabbMax.z - center.z
        }) * 1.01f; // 少し余裕を持たせる
        
        terrain->octreeRoot->center = center;
        terrain->octreeRoot->halfSize = halfSize;
        
        // 全三角形のインデックス
        std::vector<int> allIndices(terrain->triangles.size());
        for (size_t i = 0; i < terrain->triangles.size(); i++)
        {
            allIndices[i] = static_cast<int>(i);
        }
        
        BuildOctreeNode(terrain->octreeRoot.get(), terrain->triangles, allIndices,
                       0, maxOctreeDepth, maxTrianglesPerNode);
    }
    
    return terrain;
}

void TerrainCollision_Release(TerrainMesh* terrain)
{
    if (terrain)
    {
        terrain->triangles.clear();
        terrain->octreeRoot.reset();
        delete terrain;
    }
}

RaycastHit TerrainCollision_Raycast(
    const TerrainMesh* terrain,
    const XMFLOAT3& origin,
    const XMFLOAT3& direction,
    float maxDistance)
{
    RaycastHit result = {};
    result.hit = false;
    result.distance = maxDistance;
    result.triangleIndex = -1;
    
    if (!terrain || !terrain->octreeRoot) return result;
    
    // レイ方向の逆数（AABBとの交差判定用）
    XMFLOAT3 dirInv = {
        (fabsf(direction.x) > 0.0001f) ? 1.0f / direction.x : 1e10f,
        (fabsf(direction.y) > 0.0001f) ? 1.0f / direction.y : 1e10f,
        (fabsf(direction.z) > 0.0001f) ? 1.0f / direction.z : 1e10f
    };
    
    // Octreeから候補三角形を取得
    std::vector<int> candidates;
    QueryOctreeRay(terrain->octreeRoot.get(), terrain->triangles, origin, dirInv, maxDistance, candidates);
    
    // 重複を除去
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    
    g_LastCheckedTriangleCount = static_cast<int>(candidates.size());
    
    float closestT = maxDistance;
    
    for (int idx : candidates)
    {
        float hitDist, baryU, baryV;
        if (RayTriangleIntersect(origin, direction, terrain->triangles[idx], hitDist, baryU, baryV))
        {
            if (hitDist < closestT && hitDist > 0.0f)
            {
                closestT = hitDist;
                result.hit = true;
                result.distance = hitDist;
                result.triangleIndex = idx;
                result.normal = terrain->triangles[idx].normal;
                
                XMVECTOR hitPoint = XMVectorAdd(
                    XMLoadFloat3(&origin),
                    XMVectorScale(XMLoadFloat3(&direction), t)
                );
                XMStoreFloat3(&result.point, hitPoint);
            }
        }
    }
    
    return result;
}

bool TerrainCollision_GetGroundHeight(
    const TerrainMesh* terrain,
    const XMFLOAT3& position,
    float& groundY,
    XMFLOAT3* outNormal)
{
    XMFLOAT3 origin = { position.x, position.y + 100.0f, position.z };
    XMFLOAT3 direction = { 0.0f, -1.0f, 0.0f };
    
    RaycastHit hit = TerrainCollision_Raycast(terrain, origin, direction, 200.0f);
    
    if (hit.hit)
    {
        groundY = hit.point.y;
        if (outNormal)
        {
            *outNormal = hit.normal;
        }
        return true;
    }
    
    return false;
}

XMFLOAT3 TerrainCollision_ClosestPointOnTriangle(const XMFLOAT3& point, const Triangle& tri)
{
    XMVECTOR p = XMLoadFloat3(&point);
    XMVECTOR v0 = XMLoadFloat3(&tri.v0);
    XMVECTOR v1 = XMLoadFloat3(&tri.v1);
    XMVECTOR v2 = XMLoadFloat3(&tri.v2);
    
    XMVECTOR normal = XMLoadFloat3(&tri.normal);
    XMVECTOR v0ToP = XMVectorSubtract(p, v0);
    float dist = XMVectorGetX(XMVector3Dot(v0ToP, normal));
    XMVECTOR projected = XMVectorSubtract(p, XMVectorScale(normal, dist));
    
    XMFLOAT3 projectedF3;
    XMStoreFloat3(&projectedF3, projected);
    
    if (PointInTriangle(projectedF3, tri))
    {
        return projectedF3;
    }
    
    XMFLOAT3 closest0 = ClosestPointOnSegment(point, tri.v0, tri.v1);
    XMFLOAT3 closest1 = ClosestPointOnSegment(point, tri.v1, tri.v2);
    XMFLOAT3 closest2 = ClosestPointOnSegment(point, tri.v2, tri.v0);
    
    float dist0 = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(p, XMLoadFloat3(&closest0))));
    float dist1 = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(p, XMLoadFloat3(&closest1))));
    float dist2 = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(p, XMLoadFloat3(&closest2))));
    
    if (dist0 <= dist1 && dist0 <= dist2) return closest0;
    if (dist1 <= dist2) return closest1;
    return closest2;
}

SphereCollisionResult TerrainCollision_SphereVsTerrain(
    const TerrainMesh* terrain,
    const XMFLOAT3& center,
    float radius)
{
    SphereCollisionResult result = {};
    result.hit = false;
    result.pushVector = { 0.0f, 0.0f, 0.0f };
    
    if (!terrain || !terrain->octreeRoot) return result;
    
    // Octreeから候補三角形を取得
    std::vector<int> candidates;
    QueryOctreeSphere(terrain->octreeRoot.get(), terrain->triangles, center, radius, candidates);
    
    // 重複を除去
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    
    g_LastCheckedTriangleCount = static_cast<int>(candidates.size());
    
    XMVECTOR sphereCenter = XMLoadFloat3(&center);
    XMVECTOR totalPush = XMVectorZero();
    bool anyHit = false;
    
    for (int idx : candidates)
    {
        const Triangle& tri = terrain->triangles[idx];
        XMFLOAT3 closestPoint = TerrainCollision_ClosestPointOnTriangle(center, tri);
        XMVECTOR closest = XMLoadFloat3(&closestPoint);
        
        XMVECTOR toCenter = XMVectorSubtract(sphereCenter, closest);
        float distSq = XMVectorGetX(XMVector3LengthSq(toCenter));
        
        if (distSq < radius * radius && distSq > 0.0001f)
        {
            float dist = sqrtf(distSq);
            float penetration = radius - dist;
            
            XMVECTOR pushDir = XMVector3Normalize(toCenter);
            XMVECTOR push = XMVectorScale(pushDir, penetration);
            
            totalPush = XMVectorAdd(totalPush, push);
            anyHit = true;
            
            if (!result.hit)
            {
                result.contactPoint = closestPoint;
                result.normal = tri.normal;
            }
        }
    }
    
    if (anyHit)
    {
        result.hit = true;
        XMStoreFloat3(&result.pushVector, totalPush);
    }
    
    return result;
}

bool TerrainCollision_ResolveSphereCollision(
    const TerrainMesh* terrain,
    XMFLOAT3& center,
    XMFLOAT3& velocity,
    float radius)
{
    SphereCollisionResult result = TerrainCollision_SphereVsTerrain(terrain, center, radius);
    
    if (result.hit)
    {
        XMVECTOR pos = XMLoadFloat3(&center);
        pos = XMVectorAdd(pos, XMLoadFloat3(&result.pushVector));
        XMStoreFloat3(&center, pos);
        
        XMVECTOR vel = XMLoadFloat3(&velocity);
        XMVECTOR normal = XMVector3Normalize(XMLoadFloat3(&result.pushVector));
        float dot = XMVectorGetX(XMVector3Dot(vel, normal));
        
        if (dot < 0.0f)
        {
            vel = XMVectorSubtract(vel, XMVectorScale(normal, dot));
            XMStoreFloat3(&velocity, vel);
        }
        
        return true;
    }
    
    return false;
}

bool TerrainCollision_ResolveCapsuleCollision(
    const TerrainMesh* terrain,
    XMFLOAT3& bottom,
    float height,
    float radius,
    XMFLOAT3& velocity)
{
    if (!terrain) return false;
    
    bool anyCollision = false;
    
    // カプセルを複数の球で近似
    const int numSpheres = 3;
    float sphereSpacing = height / (numSpheres - 1);
    
    XMVECTOR bottomPos = XMLoadFloat3(&bottom);
    XMVECTOR totalPush = XMVectorZero();
    
    for (int i = 0; i < numSpheres; i++)
    {
        XMFLOAT3 sphereCenter;
        XMStoreFloat3(&sphereCenter, XMVectorAdd(bottomPos, XMVectorSet(0, sphereSpacing * i, 0, 0)));
        
        SphereCollisionResult result = TerrainCollision_SphereVsTerrain(terrain, sphereCenter, radius);
        
        if (result.hit)
        {
            totalPush = XMVectorAdd(totalPush, XMLoadFloat3(&result.pushVector));
            anyCollision = true;
        }
    }
    
    if (anyCollision)
    {
        bottomPos = XMVectorAdd(bottomPos, totalPush);
        XMStoreFloat3(&bottom, bottomPos);
        
        XMVECTOR vel = XMLoadFloat3(&velocity);
        float pushLen = XMVectorGetX(XMVector3Length(totalPush));
        if (pushLen > 0.001f)
        {
            XMVECTOR pushNormal = XMVector3Normalize(totalPush);
            float dot = XMVectorGetX(XMVector3Dot(vel, pushNormal));
            
            if (dot < 0.0f)
            {
                vel = XMVectorSubtract(vel, XMVectorScale(pushNormal, dot));
                XMStoreFloat3(&velocity, vel);
            }
        }
    }
    
    return anyCollision;
}

int TerrainCollision_GetTriangleCount(const TerrainMesh* terrain)
{
    return terrain ? static_cast<int>(terrain->triangles.size()) : 0;
}

int TerrainCollision_GetLastCheckedTriangleCount()
{
    return g_LastCheckedTriangleCount;
}
