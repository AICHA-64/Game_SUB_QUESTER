/*==============================================================================

   3Dフィールド用シェーダー [shader_field.h]
														 Author : Yasuda Atsushi
														 Date   : 2025/09/26
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADER_FIELD_H
#define	SHADER_FIELD_H

#include <d3d11.h>
#include <DirectXMath.h>

bool ShaderField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void ShaderField_Finalize();

void ShaderField_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

void ShaderField_SetColor(const DirectX::XMFLOAT4& color);

// 追加
void ShaderField_SetLightViewProjectionMatrix(const DirectX::XMMATRIX& matrix);

void Shader_Field_Begin();

#endif // SHADER_FIELD_H
