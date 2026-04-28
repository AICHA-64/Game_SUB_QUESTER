// ----------------------------------------------------
// カメラ制御 [camera.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-11
// Version: 1.0
// ----------------------------------------------------

#include "camera.h"
#include "direct3d.h"
#include <DirectXMath.h>
using namespace DirectX;

#include "key_logger.h"
#include "debug_text.h"
#include <sstream>

static XMFLOAT3 g_CameraPosition = { 0.0f, 0.0f, -5.0f };
static XMFLOAT3 g_CameraFront	 = { 0.0f, 0.0f, 1.0f };
static XMFLOAT3 g_CameraUp		 = { 0.0f, 1.0f, 0.0f };
static XMFLOAT3 g_CameraRight	 = { 1.0f, 0.0f, 0.0f };
static constexpr float CAMERA_MOVE_SPEED = 50.0f;
static constexpr float CAMERA_ROTATION_SPEED = XMConvertToRadians(120);
static XMFLOAT4X4 g_CameraMatrix;
static XMFLOAT4X4 g_PerspectiveMatrix;
static float g_Fov = XMConvertToRadians(60);
static hal::DebugText* g_pDT = nullptr;

static ID3D11Buffer* g_pVSConstantBuffer1 = nullptr; // 定数バッファb1
static ID3D11Buffer* g_pVSConstantBuffer2 = nullptr; // 定数バッファb2

void Camera_Initialize()
{
	g_CameraPosition = { 0.0f, 0.0f, -25.0f };
	g_CameraFront = { 0.0f, 0.0f, 1.0f };
	g_CameraUp = { 0.0f, 1.0f, 0.0f };
	g_CameraRight = { 1.0f, 0.0f, 0.0f };
	g_Fov = XMConvertToRadians(60);
	
	XMStoreFloat4x4(&g_CameraMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&g_PerspectiveMatrix, XMMatrixIdentity());


	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1);
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer2);


#if defined(DEBUG) || defined(_DEBUG)
	g_pDT = new hal::DebugText(Direct3D_GetDevice(), Direct3D_GetContext(),
		L"resource/texture/consolab_ascii_512.png", Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight(),
		0.0f, 28.0f,
		0, 0,
		0.0f, 14.0f);
#endif
}

void Camera_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front, const DirectX::XMFLOAT3& right)
{
	Camera_Initialize();

	g_CameraPosition = position;
	XMVECTOR f = XMVector3Normalize(XMLoadFloat3(&front));
	XMVECTOR r = XMVector3Normalize(XMLoadFloat3(&right) * XMVECTOR { 1.0f, 0.0f, 1.0f });
	XMVECTOR u = XMVector3Normalize(XMVector3Cross(f, r)); // 外積
	XMStoreFloat3(&g_CameraFront, f);
	XMStoreFloat3(&g_CameraRight, r);
	XMStoreFloat3(&g_CameraUp, u);
}

void Camera_Finalize()
{
	SAFE_RELEASE(g_pVSConstantBuffer2);
	SAFE_RELEASE(g_pVSConstantBuffer1);

	delete g_pDT;
}

void Camera_Update(double elapsed_time)
{
	XMVECTOR front = XMLoadFloat3(&g_CameraFront);
	XMVECTOR up = XMLoadFloat3(&g_CameraUp);
	XMVECTOR right = XMLoadFloat3(&g_CameraRight);
	XMVECTOR position = XMLoadFloat3(&g_CameraPosition);

	// カメラの下回転
	if (KeyLogger_IsPressed(KK_DOWN)) {
		XMMATRIX rotation = XMMatrixRotationAxis(right, (float)(CAMERA_ROTATION_SPEED * elapsed_time));
		front = XMVector3TransformNormal(front, rotation); // 平行移動はしない関数
		front = XMVector3Normalize(front);
		up = XMVector3Normalize(XMVector3Cross(front, right)); // 外積
	}

	// カメラの上回転
	if (KeyLogger_IsPressed(KK_UP)) {
		XMMATRIX rotation = XMMatrixRotationAxis(right, (float)(-CAMERA_ROTATION_SPEED * elapsed_time));
		front = XMVector3TransformNormal(front, rotation); // 平行移動はしない関数
		front = XMVector3Normalize(front);
		up = XMVector3Normalize(XMVector3Cross(front, right));
	}

	// 右向く
	if (KeyLogger_IsPressed(KK_RIGHT)) {
		//XMMATRIX rotation = XMMatrixRotationAxis(up, CAMERA_ROTATION_SPEED * elapsed_time);
		XMMATRIX rotation = XMMatrixRotationY((float)(CAMERA_ROTATION_SPEED * elapsed_time));
		up = XMVector3TransformNormal(up, rotation); // 平行移動はしない関数
		front = XMVector3TransformNormal(front, rotation); // 平行移動はしない関数
		front = XMVector3Normalize(front);
		right = XMVector3Normalize(XMVector3Cross(up, front) * XMVECTOR {1.0f, 0.0f, 1.0f});
	}

	// 左向く
	if (KeyLogger_IsPressed(KK_LEFT)) {
		//XMMATRIX rotation = XMMatrixRotationAxis(up, -CAMERA_ROTATION_SPEED * elapsed_time);
		XMMATRIX rotation = XMMatrixRotationY((float)(-CAMERA_ROTATION_SPEED * elapsed_time));
		up = XMVector3TransformNormal(up, rotation); // 平行移動はしない関数
		front = XMVector3TransformNormal(front, rotation); // 平行移動はしない関数
		front = XMVector3Normalize(front);
		right = XMVector3Normalize(XMVector3Cross(up, front) * XMVECTOR { 1.0f, 0.0f, 1.0f });
	}

	// 前進
	if (KeyLogger_IsPressed(KK_W)) {
		//position += front * CAMERA_MOVE_SPEED * elapsed_time;
		position += XMVector3Normalize(front * XMVECTOR{ 1.0f, 0.0f, 1.0f }) * (float)(CAMERA_MOVE_SPEED * elapsed_time);
	}
	// 左に移動
	if (KeyLogger_IsPressed(KK_A)) {
		position += -right * (float)(CAMERA_MOVE_SPEED * elapsed_time);
	}
	// 後退
	if (KeyLogger_IsPressed(KK_S)) {
		//position += -front * CAMERA_MOVE_SPEED * elapsed_time;
		position += XMVector3Normalize(-front * XMVECTOR{ 1.0f, 0.0f, 1.0f }) * (float)(CAMERA_MOVE_SPEED * elapsed_time);
	}
	// 右に移動
	if (KeyLogger_IsPressed(KK_D)) {
		position += right * (float)(CAMERA_MOVE_SPEED * elapsed_time);
	}
	// 上昇
	if (KeyLogger_IsPressed(KK_Q)) {
		//position += up * CAMERA_MOVE_SPEED * elapsed_time;
		position += XMVECTOR{0.0f, 1.0f, 0.0f} *(float)(CAMERA_MOVE_SPEED* elapsed_time);
	}
	// 下降
	if (KeyLogger_IsPressed(KK_E)) {
		//position += -up * CAMERA_MOVE_SPEED * elapsed_time;
		position += XMVECTOR{ 0.0f, -1.0f, 0.0f } *(float)(CAMERA_MOVE_SPEED * elapsed_time);
	}

	if (KeyLogger_IsPressed(KK_Z)) {
		g_Fov -= XMConvertToRadians(30) * (float)elapsed_time;
		if (g_Fov < XMConvertToRadians(10)) {
			g_Fov = XMConvertToRadians(10);
		}

	}
	if (KeyLogger_IsPressed(KK_C)) {
		g_Fov += XMConvertToRadians(30) * (float)elapsed_time;
		if (g_Fov > XMConvertToRadians(120)) {
			g_Fov = XMConvertToRadians(120);
		}

	}

	// 更新結果を保存
	XMStoreFloat3(&g_CameraPosition, position);
	XMStoreFloat3(&g_CameraFront, front);
	XMStoreFloat3(&g_CameraRight, right);
	XMStoreFloat3(&g_CameraUp, up);

	// ビュー座標変換行列の作成
	XMMATRIX mtxView = XMMatrixLookAtLH(
		position, // カメラの座標
		position + front, // 視点
		up);// 上向き

	// ビュー変換行列を保存
	XMStoreFloat4x4(&g_CameraMatrix, mtxView);

	// パースペクティブ行列
	float aspectRatio = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
	float nearz = 0.1f;
	float farz = 2000.0f;

	XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(g_Fov, aspectRatio, nearz, farz);

	// パースペクティブ変換行列を保存
	XMStoreFloat4x4(&g_PerspectiveMatrix, mtxPerspective);
}

const DirectX::XMFLOAT4X4& Camera_GetMatrix()
{
	return g_CameraMatrix;
}

const DirectX::XMFLOAT4X4& Camera_GetPerspectiveMatrix()
{
	return g_PerspectiveMatrix;
}

const DirectX::XMFLOAT3& Camera_GetPosition()
{
	return g_CameraPosition;
}

const DirectX::XMFLOAT3& Camera_GetFront()
{
	return g_CameraFront;
}

float Camera_GetFov()
{
	return g_Fov;
}

void Camera_SetMatrix(const DirectX::XMMATRIX& view, const DirectX::XMMATRIX& projection)
{
	// 定数バッファへビュー行列変換とプロジェクション変換行列を設定する
	XMFLOAT4X4 viewFloat4x4, projectionFloat4x4;
	XMStoreFloat4x4(&viewFloat4x4, XMMatrixTranspose(view)); // 転置行列に変換してから設定
	XMStoreFloat4x4(&projectionFloat4x4, XMMatrixTranspose(projection)); // 転置行列に変換してから設定

	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &viewFloat4x4, 0, 0);
	Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &projectionFloat4x4, 0, 0);

	Direct3D_GetContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);
	Direct3D_GetContext()->VSSetConstantBuffers(2, 1, &g_pVSConstantBuffer2);
}

void Camera_DebugDraw()
{
#if defined(DEBUG) || defined(_DEBUG)
	std::stringstream ss;
	ss << "Camera Pos : x = " << g_CameraPosition.x;
	ss << " y = " << g_CameraPosition.y;
	ss << " z = " << g_CameraPosition.z << std::endl;

	ss << "Camera Fro : x = " << g_CameraFront.x;
	ss << " y = " << g_CameraFront.y;
	ss << " z = " << g_CameraFront.z << std::endl;

	ss << "Camera Rig : x = " << g_CameraRight.x;
	ss << " y = " << g_CameraRight.y;
	ss << " z = " << g_CameraRight.z << std::endl;

	ss << "Camera Up  : x = " << g_CameraUp.x;
	ss << " y = " << g_CameraUp.y;
	ss << " z = " << g_CameraUp.z << std::endl;

	ss << "Camera Fov : " << g_Fov << std::endl;


	g_pDT->SetText(ss.str().c_str(), {0.5f, 0.0f, 0.5f, 1.0f});
	g_pDT->Draw();
	g_pDT->Clear();
#endif
}

