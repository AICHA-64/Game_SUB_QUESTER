#pragma once

#include <unordered_map>
#include <vector>

#include <d3d11.h>
#include <DirectXMath.h>
#include "libs/Assimp/cimport.h"
#include "libs/Assimp/scene.h"
#include "libs/Assimp/postprocess.h"
#include "libs/Assimp/matrix4x4.h"
#pragma comment (lib, "assimp-vc143-mt.lib")

#include "collision.h"

// 3D頂点構造体
struct SkinnedVertex3d
{
	DirectX::XMFLOAT3 position;	// 頂点座標
	DirectX::XMFLOAT3 normal;	// 法線
	DirectX::XMFLOAT2 texcoord;	// UV
	DirectX::XMFLOAT4 color;		// 頂点カラー
	int		 bones[4];  // ボーンインデックス
	float	 bone_weight[4]; // ボーンウェイト
};

struct Bone {
	DirectX::XMFLOAT4X4 m_matrix; // オフセット行列
	DirectX::XMFLOAT4X4 m_offsetmatrix; // オフセット行列
};

class Model_Skinned
{
private:
	const aiScene* AiScene = nullptr;

	ID3D11Buffer** VertexBuffer = nullptr;
	ID3D11Buffer** IndexBuffer = nullptr;

	std::vector<Bone> Bones;
	std::unordered_map<std::string, int> BoneIndex;
	std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;

	static int m_TextureWhite;
public:
	~Model_Skinned();

	void Load(const char* FileName, float scale = 1.0f, bool bBlender = false);
	
	void Draw(const DirectX::XMMATRIX& mtxWorld) const;
};
