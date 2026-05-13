// ----------------------------------------------------
// スキンドモデル描画準備 [model_skinned.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-10-31
// Version: 1.0
// ----------------------------------------------------
#include <assert.h>
#include "direct3d.h"
#include "texture.h"
#include "model_skinned.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "WICTextureLoader11.h"
#include "shader3d.h"
#include "shader3d_unlit.h"

int Model_Skinned::m_TextureWhite = -1;

Model_Skinned::~Model_Skinned()
{
	for (unsigned int m = 0; m < AiScene->mNumMeshes; m++)
	{
		VertexBuffer[m]->Release();
		IndexBuffer[m]->Release();
	}

	delete[] VertexBuffer;
	delete[] IndexBuffer;


	for (std::pair<const std::string, ID3D11ShaderResourceView*> pair : Texture)
	{
		pair.second->Release();
	}


	aiReleaseImport(AiScene);
}

void Model_Skinned::Load(const char* FileName, float scale, bool bBlender)
{
	const std::string modelPath(FileName);

	AiScene = aiImportFile(FileName, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);
	assert(AiScene);

	VertexBuffer = new ID3D11Buffer * [AiScene->mNumMeshes];
	IndexBuffer = new ID3D11Buffer * [AiScene->mNumMeshes];

	// 再帰的にボーン情報の取得


	for (unsigned int m = 0; m < AiScene->mNumMeshes; m++)
	{


		aiMesh* mesh = AiScene->mMeshes[m];

		// 頂点バッファ生成
		{
			SkinnedVertex3d* vertex = new SkinnedVertex3d[mesh->mNumVertices];

			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				if (bBlender) {
					vertex[v].position = XMFLOAT3(mesh->mVertices[v].x * scale, -mesh->mVertices[v].z * scale, mesh->mVertices[v].y * scale);
					vertex[v].normal = XMFLOAT3(mesh->mNormals[v].x, -mesh->mNormals[v].z, mesh->mNormals[v].y);
				}
				else {
					vertex[v].position = XMFLOAT3(mesh->mVertices[v].x * scale, mesh->mVertices[v].y * scale, mesh->mVertices[v].z * scale);
					vertex[v].normal = XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
				}
				vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				vertex[v].texcoord = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);

				for(int b = 0; b < 4; b++ )
				{
					vertex[v].bones[b] = -1;
					vertex[v].bone_weight[b] = 0.0f;
				}
			}

			// 頂点へボーン情報を追加 逆引き
			for (unsigned int b = 0; b < mesh->mNumBones; b++)
			{
				aiBone* bone = mesh->mBones[b];

				for (unsigned int w = 0; w < bone->mNumWeights; w++)
				{
					aiVertexWeight weight = bone->mWeights[w];

					for(int i = 0; i < 4; i++ )
					{
						if( vertex[weight.mVertexId].bones[i] < 0 ){
							//vertex[weight.mVertexId].bones[i] = -b;
							vertex[weight.mVertexId].bone_weight[i] = weight.mWeight;
						}
					}
				}
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(SkinnedVertex3d) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = vertex;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &VertexBuffer[m]);

			delete[] vertex;
		}


		// インデックスバッファ生成
		{
			unsigned int* index = new unsigned int[mesh->mNumFaces * 3];

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];

				assert(face->mNumIndices == 3);

				index[f * 3 + 0] = face->mIndices[0];
				index[f * 3 + 1] = face->mIndices[1];
				index[f * 3 + 2] = face->mIndices[2];
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * mesh->mNumFaces * 3;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = index;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &IndexBuffer[m]);

			delete[] index;
		}
	}

	m_TextureWhite = Texture_Load(L"resource/texture/white.png");


	// FBX内包型
	for (unsigned int i = 0; i < AiScene->mNumTextures; i++)
	{
		aiTexture* aitexture = AiScene->mTextures[i];

		ID3D11ShaderResourceView* texture;
		ID3D11Resource* resource;

		CreateWICTextureFromMemory(
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			(const uint8_t*)aitexture->pcData,
			(size_t)aitexture->mWidth,
			&resource,
			&texture);

		assert(texture);

		resource->Release();

		Texture[aitexture->mFilename.data] = texture;
	}

	//fbxのファイルパスだけ取得

	// 最後の'/'または'\\'の位置を探す
	size_t pos = modelPath.find_last_of("/\\");
	std::string directory;

	if (pos != std::string::npos) {
		directory = modelPath.substr(0, pos); // ファイル名を除いた部分
	}
	else {
		directory = ""; // パスに区切りがない場合（ファイル名のみ）
	}

	// テクスチャ外部型
	for (unsigned int m = 0; m < AiScene->mNumMeshes; m++)
	{
		aiString filename;
		aiMaterial* aimaterial = AiScene->mMaterials[AiScene->mMeshes[m]->mMaterialIndex];
		aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &filename);

		if (filename.length == 0) {
			continue;
		}

		if (Texture.count(filename.C_Str())) {
			continue;
		}

		ID3D11ShaderResourceView* texture;
		ID3D11Resource* resource;

		std::string texfilename = directory + "/" + filename.C_Str();

		int len = MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, nullptr, 0);
		wchar_t* pWideFilename = new wchar_t[len]; // 一時的にメモリを確保
		MultiByteToWideChar(CP_UTF8, 0, texfilename.c_str(), -1, pWideFilename, len);

		CreateWICTextureFromFile(
			Direct3D_GetDevice(),
			Direct3D_GetContext(),
			pWideFilename,
			&resource,
			&texture);

		delete[] pWideFilename; // 一時的に作ったメモリを消す

		assert(texture);

		resource->Release();

		Texture[filename.C_Str()] = texture;
	}

}

void Model_Skinned::Draw(const DirectX::XMMATRIX& mtxWorld) const
{
	// シェーダーを描画パイプラインに設定
	Shader3d_Begin();

	// プリミティブトポロジ設定
	Direct3D_GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点シェーダーにワールド座標変換行列を設定
	Shader3d_SetWorldMatrix(mtxWorld);

	for (unsigned int m = 0; m < AiScene->mNumMeshes; m++)
	{
		// テクスチャの設定
		aiString texture;
		aiMaterial* aimaterial = AiScene->mMaterials[AiScene->mMeshes[m]->mMaterialIndex];
		aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texture);

		if (texture.length != 0)
		{
			// Direct3D_GetContext()->PSSetShaderResources(0, 1, &Texture[texture.data]);
			Shader3d_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}
		else
		{
			Texture_SetTexture(m_TextureWhite); // テクスチャをセット
			aiColor3D diffuse;
			aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
			Shader3d_SetColor({ diffuse.r, diffuse.g, diffuse.b, 1.0f });
		}

		// マテリアル設定
		// aiMaterial* aimaterial = model->AiScene->mMaterials[model->AiScene->mMeshes[m]->mMaterialIndex];



		// 頂点バッファを描画パイプラインに設定
		UINT stride = sizeof(SkinnedVertex3d);
		UINT offset = 0;
		Direct3D_GetContext()->IASetVertexBuffers(0, 1, &VertexBuffer[m], &stride, &offset);

		// インデックスバッファを描画パイプラインに設定
		Direct3D_GetContext()->IASetIndexBuffer(IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0); // 足りなかったのでR32に変えた

		// ポリゴン描画命令発行
		Direct3D_GetContext()->DrawIndexed(AiScene->mMeshes[m]->mNumFaces * 3, 0, 0); // 描画
	}


}
