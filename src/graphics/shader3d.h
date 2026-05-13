/*==============================================================================

   3D用シェーダー [shader3d.h]
														 Author : Yasuda Atsushi
														 Date   : 2025/05/15
--------------------------------------------------------------------------------
   
==============================================================================*/
#ifndef SHADER3D_H
#define	SHADER3D_H

#include <d3d11.h>
#include <DirectXMath.h>

bool Shader3d_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader3d_Finalize();

void Shader3d_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader3d_SetColor(const DirectX::XMFLOAT4& color);

// シャドウマッピング用の関数を追加
void Shader3d_SetLightViewProjectionMatrix(const DirectX::XMMATRIX& matrix);
void Shader3d_SetShadowMapTexture(ID3D11ShaderResourceView* srv);

// フォグ関連の関数
// cameraPos: カメラのワールド座標位置
// fogStart: フォグが開始する距離（カメラからの距離）
// fogEnd: フォグが完全になる距離（カメラからの距離）
void Shader3d_SetFogParameters(const DirectX::XMFLOAT3& cameraPos, float fogStart, float fogEnd);

// fogColor: フォグの色(RGBA、通常のアルファは1.0)
void Shader3d_SetFogColor(const DirectX::XMFLOAT4& fogColor);

// シャドウ濃度設定関数を追加
void Shader3d_SetTerrainShadowIntensity(float intensity);

void Shader3d_Begin();

#endif // SHADER3D_H
