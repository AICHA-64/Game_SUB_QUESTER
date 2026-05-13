#ifndef LIGHT_CAMERA_H
#define LIGHT_CAMERA_H

#include <DirectXMath.h>

// ライトカメラの初期化
void LightCamera_Initialize();

// ライト視点のビュー行列とプロジェクション行列を更新
void LightCamera_Update(const DirectX::XMFLOAT3& lightDirection, const DirectX::XMFLOAT3& targetPosition, float orthoSize, float nearZ, float farZ);

// ライトのビュー行列を取得
DirectX::XMFLOAT4X4 LightCamera_GetViewMatrix();

// ライトのプロジェクション行列を取得
DirectX::XMFLOAT4X4 LightCamera_GetProjectionMatrix();

// ライトビュープロジェクション行列（シェーダー用）
DirectX::XMFLOAT4X4 LightCamera_GetViewProjectionMatrix();

#endif // LIGHT_CAMERA_H
