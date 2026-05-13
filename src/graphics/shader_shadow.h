#ifndef SHADER_SHADOW_H
#define SHADER_SHADOW_H

#include <d3d11.h>
#include <DirectXMath.h>

bool ShaderShadow_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void ShaderShadow_Finalize();

void ShaderShadow_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void ShaderShadow_SetLightViewMatrix(const DirectX::XMMATRIX& matrix);  // 追加
void ShaderShadow_SetLightProjectionMatrix(const DirectX::XMMATRIX& matrix);  // 追加
void ShaderShadow_Begin();

#endif
