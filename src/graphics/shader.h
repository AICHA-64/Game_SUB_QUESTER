/*==============================================================================

   シェーダー [shader.h]
														 Author : Yasuda Atsushi
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef SHADER_H
#define	SHADER_H

#include <d3d11.h>
#include <DirectXMath.h>

bool Shader_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Shader_Finalize();

void Shader_SetWorldMatrix(const DirectX::XMMATRIX& matrix);
void Shader_SetProjectionMatrix(const DirectX::XMMATRIX& matrix);

void Shader_Begin();
void Shader_BeginCircle();

#endif // SHADER_H
