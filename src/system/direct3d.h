/*==============================================================================

   Direct3Dの初期化関連 [direct3d.cpp]
														 Author : Yasuda Atsushi
														 Date   : 2025/05/12
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef DIRECT3D_H
#define DIRECT3D_H


#include <Windows.h>
#include <d3d11.h>
#include <DirectXMath.h>


// セーフリリースマクロ
#define SAFE_RELEASE(o) if (o) { (o)->Release(); o = NULL; }


bool Direct3D_Initialize(HWND hWnd); // Direct3Dの初期化
void Direct3D_Finalize(); // Direct3Dの終了処理

void Direct3D_Clear(); // バックバッファのクリア
void Direct3D_Present(); // バックバッファの表示

unsigned int Direct3D_GetBackBufferWidth();
unsigned int Direct3D_GetBackBufferHeight();

ID3D11Device* Direct3D_GetDevice();
ID3D11DeviceContext* Direct3D_GetContext();
ID3D11ShaderResourceView* Direct3D_GetShadowMapSRV();

// αブレンド設定関数
void Direct3D_SetAlphaBlendTransparent(); // 透過
void Direct3D_SetAlphaBlendAdd();		 // 加算

//深度バッファの設定
void Direct3D_SetDepthEnable(bool enable);
void Direct3D_SetDepthWriteDisable();

// ビューポート行列の作成
DirectX::XMMATRIX Direct3D_MatrixViewport();

// スクリーン座標から3D座標変換
DirectX::XMFLOAT3 Direct3D_ScreenToWorld(int x, int y, float depth, const DirectX::XMFLOAT4X4&  view, const DirectX::XMFLOAT4X4& projection);

// 3D座標変換からスクリーン座標
DirectX::XMFLOAT2 Direct3D_WorldToScreen(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4X4&  view, const DirectX::XMFLOAT4X4& projection);

// バックバッファのレンダリングに切り替える
void Direct3D_ClearBackBuffer();

// バックバッファへのレンダリングに切り替える
void Direct3D_SetBackBuffer();

// テクスチャのレンダリングに切り替える
void Direct3D_ClearOffScreen();

// テクスチャへのレンダリングに切り替える
void Direct3D_SetOffScreen();

// オフスクリーンレンダリングテクスチャの設定
void Direct3D_SetOffScreenTexture(int slot);

// シャドウマップ関連の宣言を追加
void Direct3D_CreateShadowMap(int width, int height); // シャドウマップ作成
void Direct3D_SetShadowMapRenderTarget();             // シャドウマップへの描画設定
void Direct3D_SetShadowMapTexture(int slot);          // シェーダーにシャドウマップをセット
void Direct3D_ClearShadowMap();                       // シャドウマップのクリア

void ReleaseShadowMap();

// ビュー行列・射影行列の取得（ライト視点用）
DirectX::XMMATRIX Direct3D_GetLightViewMatrix();
DirectX::XMMATRIX Direct3D_GetLightProjectionMatrix();

// Bloomエフェクト用のメインシーンレンダーターゲット関数を追加
void Direct3D_CreateMainSceneRT(int width, int height);
void Direct3D_SetMainSceneRT();
void Direct3D_ClearMainSceneRT();
ID3D11ShaderResourceView* Direct3D_GetMainSceneSRV();
void Direct3D_ReleaseMainSceneRT();

#endif // DIRECT3D_H
