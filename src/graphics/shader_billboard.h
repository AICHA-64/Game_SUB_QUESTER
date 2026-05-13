/*==============================================================================

   ビルボード用シェーダー [shader_billboard.h]
														 Author : Yasuda Atsushi
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADER_BILLBOARD_H
#define	SHADER_BILLBOARD_H

#include <d3d11.h>
#include <DirectXMath.h>

bool ShaderBillboard_Initialize();
void ShaderBillboard_Finalize();

void ShaderBillboard_SetWorldMatrix(const DirectX::XMMATRIX& matrix);

void ShaderBillboard_SetColor(const DirectX::XMFLOAT4& color);

struct UVParameter
{
	DirectX::XMFLOAT2 scale;
	DirectX::XMFLOAT2 translation;
};

void ShaderBillboard_SetUVParameter(const UVParameter& parameter);

void ShaderBillboard_Begin();

#endif // SHADER_BILLBOARD_H
