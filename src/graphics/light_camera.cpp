// ----------------------------------------------------
// ライトカメラ [light_camera.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-10-28
// Version: 1.0
// ----------------------------------------------------
#include "light_camera.h"
using namespace DirectX;

static XMFLOAT4X4 g_LightViewMatrix;
static XMFLOAT4X4 g_LightProjectionMatrix;

void LightCamera_Initialize()
{
    XMStoreFloat4x4(&g_LightViewMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&g_LightProjectionMatrix, XMMatrixIdentity());
}

void LightCamera_Update(const XMFLOAT3& lightDirection, const XMFLOAT3& targetPosition, float orthoSize, float nearZ, float farZ)
{
    // ライトの位置を計算（ターゲットからライト方向の逆へ）
    XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&lightDirection));
    XMVECTOR target = XMLoadFloat3(&targetPosition);
  
    // ライトカメラの距離はfarZの半分程度に設定し、シーン全体をカバーする
    float lightDistance = farZ * 0.5f;
    XMVECTOR lightPos = target - lightDir * lightDistance;

    // ビュー行列（ライト視点）
    XMMATRIX viewMatrix = XMMatrixLookAtLH(lightPos, target, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    XMStoreFloat4x4(&g_LightViewMatrix, viewMatrix);

    // 正射影行列（平行光源用）
    XMMATRIX projMatrix = XMMatrixOrthographicLH(orthoSize, orthoSize, nearZ, farZ);
    XMStoreFloat4x4(&g_LightProjectionMatrix, projMatrix);
}

XMFLOAT4X4 LightCamera_GetViewMatrix()
{
    return g_LightViewMatrix;
}

XMFLOAT4X4 LightCamera_GetProjectionMatrix()
{
    return g_LightProjectionMatrix;
}

XMFLOAT4X4 LightCamera_GetViewProjectionMatrix()
{
    XMMATRIX view = XMLoadFloat4x4(&g_LightViewMatrix);
    XMMATRIX proj = XMLoadFloat4x4(&g_LightProjectionMatrix);
    XMFLOAT4X4 vp;
    XMStoreFloat4x4(&vp, view * proj);
    return vp;
}
