// ----------------------------------------------------
// ゲームシーン 描画処理分割 [game_render.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: Date: 2026-04-22
// ----------------------------------------------------
#include "game.h"
#include "direct3d.h"
#include "camera.h"
#include "player_camera.h"
#include "shader3d.h"
#include "billboard.h"
#include "sampler.h"
#include "sky.h"
#include "light_camera.h"
#include "shader_field.h"
#include "light.h"
#include "player.h"
#include "Enemy.h"
#include "enemy_bullet.h"
#include "map.h"
#include "bullet.h"
#include "bullet_hit_effect.h"
#include "trajectory3d.h"
#include "water.h"
#include "bloom.h"
#include "underwater_effect.h"
#include "shader_shadow.h"
#include "map_camera.h"
#include "debug_draw.h"
#include "mouse.h"
#include "water_reflect.h"

using namespace DirectX;

extern WaterRT g_WaterRT;

void GameScene::Draw()
{
	Direct3D_SetDepthEnable(true);
	Direct3D_SetAlphaBlendTransparent();

	RenderReflectionPass(m_IsDebug);

	// シャドウマップの生成
	shadowMapRendering();
	
	// ミニマップのレンダリング
	mapRendering();

	// メインシーンの描画開始(Bloom用)
	Direct3D_SetMainSceneRT();
	Direct3D_ClearMainSceneRT();

	Mouse_State ms{};
	Mouse_GetState(&ms);

	XMFLOAT4X4 mtxView = m_IsDebug ? Camera_GetMatrix() : PlayerCamera_GetViewMatrix();
	XMMATRIX view = XMLoadFloat4x4(&mtxView);
	XMMATRIX proj = m_IsDebug ? XMLoadFloat4x4(&Camera_GetPerspectiveMatrix()) : XMLoadFloat4x4(&PlayerCamera_GetPerspectiveMatrix());
	XMFLOAT3 camera_position = m_IsDebug ? Camera_GetPosition() : PlayerCamera_GetPosition();

	// フォグの設定を追加
	float waterSurfaceY = 0.5f;
	if (camera_position.y <= waterSurfaceY) {
		Shader3d_SetFogParameters(camera_position, 0.0f, 110.0f);
		Shader3d_SetFogColor({ 0.3f, 0.45f, 0.65f, 1.0f });
	} else {
		Shader3d_SetFogParameters(camera_position, 5.0f, 220.0f);
		Shader3d_SetFogColor({ 0.3f, 0.45f, 0.6f, 1.0f });
	}

	Shader3d_SetTerrainShadowIntensity(m_TerrainShadowIntensity);

	XMFLOAT3 test_near = Direct3D_ScreenToWorld(ms.x, ms.y, 0.0f, mtxView, PlayerCamera_GetPerspectiveMatrix());
	XMFLOAT3 test_far = Direct3D_ScreenToWorld(ms.x, ms.y, 1.0f, mtxView, PlayerCamera_GetPerspectiveMatrix());

	XMVECTOR vtest = XMLoadFloat3(&test_near) - XMLoadFloat3(&test_far);
	vtest = XMVector3Normalize(vtest);

	float ratio = -XMVectorGetY(XMLoadFloat3(&test_near)) / XMVectorGetY(vtest);
	vtest = XMLoadFloat3(&test_near) + vtest * ratio;

	Camera_SetMatrix(view, proj);
	Billboard_SetViewMatrix(mtxView);
	Sampler_SetFilterAnisotropic();

	Direct3D_SetDepthEnable(false);
	Sky_Draw();
	Direct3D_SetDepthEnable(true);

	Direct3D_SetShadowMapTexture(1);
	Sampler_SetShadowSampler(Direct3D_GetContext());

	XMFLOAT4X4 lightView = LightCamera_GetViewMatrix();
	XMFLOAT4X4 lightProj = LightCamera_GetProjectionMatrix();
	XMMATRIX lightVP = XMLoadFloat4x4(&lightView) * XMLoadFloat4x4(&lightProj);

	Shader3d_SetLightViewProjectionMatrix(lightVP);
	ShaderField_SetLightViewProjectionMatrix(lightVP);

	Light_SetAmbient({ 0.2f, 0.2f, 0.4f });

	XMVECTOR v = XMLoadFloat3(&m_lightDirection);
	v = XMVector3Normalize(v);
	XMFLOAT4 dir;
	DirectX::XMStoreFloat4(&dir, v);
	Light_SetDirectionalWorld(dir, { 1.0f, 1.0f, 1.0f, 1.0f });

	Light_SetPointLightCount(0);

	XMMATRIX mtxTrans = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
	XMMATRIX mtxWorld = mtxTrans;
	
	bool shouldDrawPlayer = true;
	if (m_DamageInvincibleTimer > 0.0) {
		shouldDrawPlayer = ((int)(m_DamageInvincibleTimer * 10) % 2) == 0;
	}
	if (shouldDrawPlayer) {
		Player::GetInstance().Draw();
	}

	Enemy_Draw();
	EnemyBullet_Draw();
	Map_Draw();
	Bullet_Draw();
	
	Direct3D_SetDepthWriteDisable();
	BulletHitEffect_Draw();
	Trajectory3D_Draw();

	Direct3D_SetAlphaBlendAdd();
	Direct3D_SetAlphaBlendTransparent();

	Direct3D_SetDepthEnable(true);

	Water_SetCameraPosition(camera_position);
	XMFLOAT4X4 lightViewProj;
	XMStoreFloat4x4(&lightViewProj, lightVP);
	Water_SetLightMatrix(lightViewProj);
	
	Sampler_SetShadowSampler(Direct3D_GetContext());
	Water_Draw();

	ID3D11ShaderResourceView* pMainSceneSRV = Direct3D_GetMainSceneSRV();
	Bloom_Apply(pMainSceneSRV);

	Direct3D_SetBackBuffer();

	UnderwaterEffect_Draw();

	Direct3D_SetDepthEnable(false);

	DrawUI();

	if(m_IsDebug) {
		Camera_DebugDraw();
	}
}

void GameScene::shadowMapRendering()
{
	Direct3D_ClearShadowMap();
	Direct3D_SetShadowMapRenderTarget();

	XMFLOAT3 targetPosition = Player::GetInstance().GetPosition();
	float orthoSize = 500.0f;
	float nearZ = 1.0f;
	float farZ = 1000.0f;
	
	LightCamera_Update(m_lightDirection, targetPosition, orthoSize, nearZ, farZ);

	XMFLOAT4X4 lightView = LightCamera_GetViewMatrix();
	XMFLOAT4X4 lightProj = LightCamera_GetProjectionMatrix();
	ShaderShadow_SetLightViewMatrix(XMLoadFloat4x4(&lightView));
	ShaderShadow_SetLightProjectionMatrix(XMLoadFloat4x4(&lightProj));

	ShaderShadow_Begin();
	Direct3D_SetDepthEnable(true);

	Player::GetInstance().Draw();
	Enemy_DrawShadow();
	Map_DrawShadow();
	Bullet_DrawShadow();
}

void GameScene::mapRendering()
{
	DebugDraw_SetMinimapMode(true);

	Direct3D_SetOffScreen();
	Direct3D_ClearOffScreen();

	XMFLOAT3 position = Player::GetInstance().GetPosition();
	position.y = 200.0f;
	MapCamera_SetPosition(position);
	MapCamera_SetFront(PlayerCamera_GetFront());
	XMFLOAT4X4 mtxView = MapCamera_GetViewMatrix();
	XMFLOAT4X4 mtxProj = MapCamera_GetProjectionMatrix();
	XMMATRIX view = XMLoadFloat4x4(&mtxView);
	XMMATRIX proj = XMLoadFloat4x4(&mtxProj);

	Camera_SetMatrix(view, proj);
	Sampler_SetFilterAnisotropic();
	Direct3D_SetDepthEnable(true);

	Light_SetAmbient({ 1.0f, 1.0f, 1.0f });
	XMVECTOR v = XMLoadFloat3(&m_lightDirection);
	v = XMVector3Normalize(v);
	XMFLOAT4 dir;
	DirectX::XMStoreFloat4(&dir, v);
	Light_SetDirectionalWorld(dir, { 1.0f, 1.0f, 1.0f, 1.0f });
	Light_SetPointLightCount(0);

	Map_Draw();
	Enemy_Draw();
	EnemyBullet_Draw();
	Player::GetInstance().Draw();

	DebugDraw_SetMinimapMode(false);
}

DirectX::XMFLOAT4X4 BuildReflectionViewProj(const DirectX::XMFLOAT3& cameraPos,
	const DirectX::XMFLOAT3& cameraFront,
	float waterHeight,
	float aspect, float fov, float nearZ, float farZ)
{
	using namespace DirectX;
	XMFLOAT3 reflPos = cameraPos;
	reflPos.y = waterHeight - (cameraPos.y - waterHeight);

	XMFLOAT3 reflFront = cameraFront;
	reflFront.y = -cameraFront.y;

	XMVECTOR pos = XMLoadFloat3(&reflPos);
	XMVECTOR target = pos + XMLoadFloat3(&reflFront);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);
	XMMATRIX vp = view * proj;
	XMFLOAT4X4 out;
	XMStoreFloat4x4(&out, vp);
	return out;
}

void GameScene::RenderReflectionPass(bool useDebugCamera)
{
	ID3D11DeviceContext* ctx = Direct3D_GetContext();

	ID3D11ShaderResourceView* nullSRV[16] = {0};
	ctx->PSSetShaderResources(0, 16, nullSRV);
	ctx->VSSetShaderResources(0, 16, nullSRV);

	float clearColor[4] = {0.3f, 0.4f, 0.5f, 1.0f};
	ctx->OMSetRenderTargets(1, &g_WaterRT.pReflectionRTV, g_WaterRT.pReflectionDSV);
	ctx->ClearRenderTargetView(g_WaterRT.pReflectionRTV, clearColor);
	ctx->ClearDepthStencilView(g_WaterRT.pReflectionDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	D3D11_VIEWPORT prevVP[16];
	UINT numPrev = 16;
	ctx->RSGetViewports(&numPrev, prevVP);

	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (FLOAT)Direct3D_GetBackBufferWidth();
	vp.Height = (FLOAT)Direct3D_GetBackBufferHeight();
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	ctx->RSSetViewports(1, &vp);

	DirectX::XMFLOAT3 camPos = useDebugCamera ? Camera_GetPosition() : PlayerCamera_GetPosition();
	DirectX::XMFLOAT3 camFront = useDebugCamera ? Camera_GetFront() : PlayerCamera_GetFront();
	float fov = useDebugCamera ? Camera_GetFov() : XMConvertToRadians(60.0f);
	float aspect = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
	float nearZ = 0.1f;
	float farZ = 200.0f;

	const float waterHeight = 0.3f;

	XMFLOAT3 reflPos = camPos;
	reflPos.y = waterHeight - (camPos.y - waterHeight);

	XMFLOAT3 reflFront = camFront;
	reflFront.y = -camFront.y;

	XMVECTOR pos = XMLoadFloat3(&reflPos);
	XMVECTOR target = pos + XMLoadFloat3(&reflFront);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);

	Camera_SetMatrix(view, proj);

	XMFLOAT4X4 view4x4;
	XMStoreFloat4x4(&view4x4, view);
	Billboard_SetViewMatrix(view4x4);

	XMMATRIX vpM = view * proj;
	XMFLOAT4X4 reflVP;
	XMStoreFloat4x4(&reflVP, vpM);
	Water_SetReflectionMatrix(reflVP);

	ID3D11RasterizerState* prevRS = nullptr;
	ctx->RSGetState(&prevRS);

	if (m_pReflectionNoCullRS) {
		ctx->RSSetState(m_pReflectionNoCullRS);
	}

	switch (m_ReflectionDebugMode) {
	case 0:
		Sky_Draw();
		ctx->PSSetShaderResources(0, 16, nullSRV);
		Map_Draw();
		ctx->PSSetShaderResources(0, 16, nullSRV);
		Player::GetInstance().Draw();
		ctx->PSSetShaderResources(0, 16, nullSRV);
		Bullet_Draw();
		Trajectory3D_Draw();
		BulletHitEffect_Draw();
		break;
	case 1:
		Sky_Draw();
		break;
	case 2:
		ctx->PSSetShaderResources(0, 16, nullSRV);
		Map_Draw();
		break;
	case 3:
		ctx->PSSetShaderResources(0, 16, nullSRV);
		Player::GetInstance().Draw();
		break;
	case 4:
		ctx->PSSetShaderResources(0, 16, nullSRV);
		Bullet_Draw();
		break;
	case 5:
		Trajectory3D_Draw();
		BulletHitEffect_Draw();
		break;
	case 6:
		break;
	default:
		Sky_Draw();
		ctx->PSSetShaderResources(0, 16, nullSRV);
		Map_Draw();
		ctx->PSSetShaderResources(0, 16, nullSRV);
		Player::GetInstance().Draw();
		ctx->PSSetShaderResources(0, 16, nullSRV);
		Bullet_Draw();
		Trajectory3D_Draw();
		BulletHitEffect_Draw();
		break;
	}

	ctx->RSSetState(prevRS);
	if (prevRS) prevRS->Release();

	ctx->RSSetViewports(numPrev, prevVP);
}
