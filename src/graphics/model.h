#pragma once

#include <unordered_map>

#include <d3d11.h>
#include <DirectXMath.h>
#include "libs/Assimp/cimport.h"
#include "libs/Assimp/scene.h"
#include "libs/Assimp/postprocess.h"
#include "libs/Assimp/matrix4x4.h"
#pragma comment (lib, "assimp-vc143-mt.lib")

#include "collision.h"

struct MODEL
{
	const aiScene* AiScene = nullptr;

	ID3D11Buffer** VertexBuffer = nullptr;
	ID3D11Buffer** IndexBuffer = nullptr;

	std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;

	AABB local_aabb{}; // ローカルAABB
};


MODEL* ModelLoad(const char* FileName, float scale = 1.0f, bool bBlender = false);
void ModelRelease(MODEL* model);

void ModelDraw(MODEL* model, const DirectX::XMMATRIX& mtxWorld);

void ModelDrawUnlit(MODEL* model, const DirectX::XMMATRIX& mtxWorld);

// シャドウマップ専用描画関数（深度のみ描画、ピクセルシェーダーなし）
void ModelDrawShadow(MODEL* model, const DirectX::XMMATRIX& mtxWorld);

AABB Model_GetAABB(MODEL* model, const DirectX::XMFLOAT3& position);
