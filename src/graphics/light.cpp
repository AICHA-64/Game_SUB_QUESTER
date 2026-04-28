// ----------------------------------------------------
// ライトの設定 [light.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-30
// Version: 1.0
// ----------------------------------------------------
#include "light.h"
#include "direct3d.h"
using namespace DirectX;

static ID3D11Buffer* g_pPSConstantBuffer1 = nullptr; // 定数バッファb1
static ID3D11Buffer* g_pPSConstantBuffer2 = nullptr; // 定数バッファb2
static ID3D11Buffer* g_pPSConstantBuffer3 = nullptr; // 定数バッファb3
static ID3D11Buffer* g_pPSConstantBuffer4 = nullptr; // 定数バッファb4


// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// 平行高原（拡散反射光の構造体）
struct DirectionalLight
{
	XMFLOAT4 directional; // ライトの向き（ワールド座標系）
	XMFLOAT4 color;     // ライトの色
};

// 鏡面反射光の構造体
struct SpecularLight // float4づつ
{
	XMFLOAT3 CameraPosition; // カメラの位置（ワールド座標系）
	float Power;
	XMFLOAT4 Color;

};

// 点光源
struct PointLight
{
	XMFLOAT3 LightPosition;
	float Range;
	XMFLOAT4 Color;
	// float SpecularPower;
};

struct PointLightList {
	PointLight light[4];
	int count;
	XMFLOAT3 dummy;
};

static PointLightList g_PointLights = {};

void Light_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ

	buffer_desc.ByteWidth = sizeof(XMFLOAT4); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer1); // ambient

	buffer_desc.ByteWidth = sizeof(DirectionalLight); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer2); // directional

	buffer_desc.ByteWidth = sizeof(SpecularLight); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer3); // directional

	buffer_desc.ByteWidth = sizeof(PointLightList); // バッファのサイズ
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer4); // directional

	//PointLightList list{
	//	{
	//		{ {0.0f, 2.0f, 0.0f}, 5.0f, {1.0f, 1.0f, 0.0f, 1.0f} },
	//		{ {3.0f, 2.0f, 0.0f}, 5.0f, {0.0f, 0.0f, 1.0f, 1.0f} },
	//		{ {0.0f, 2.0f, 0.0f}, 5.0f, {1.0f, 1.0f, 1.0f, 1.0f} },
	//		{ {0.0f, 2.0f, 0.0f}, 5.0f, {1.0f, 1.0f, 1.0f, 1.0f} },
	//	},
	//	2
	//	// dummy
	//};

	//
	//g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &list, 0, 0);
	//g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);
}

void Light_Finalize(void)
{
	SAFE_RELEASE(g_pPSConstantBuffer4);
	SAFE_RELEASE(g_pPSConstantBuffer3);
	SAFE_RELEASE(g_pPSConstantBuffer2);
	SAFE_RELEASE(g_pPSConstantBuffer1);
}

void Light_SetAmbient(const XMFLOAT3& color)
{
	// 定数バッファにアンビエントをセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer1, 0, nullptr, &color, 0, 0);
	g_pContext->PSSetConstantBuffers(1, 1, &g_pPSConstantBuffer1);
}

void Light_SetDirectionalWorld(const DirectX::XMFLOAT4& world_directional, const DirectX::XMFLOAT4& color)
{
	DirectionalLight d_light{
		world_directional,
		color,
	};

	// 定数バッファにディレクショナルライトをセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer2, 0, nullptr, &d_light, 0, 0);
	g_pContext->PSSetConstantBuffers(2, 1, &g_pPSConstantBuffer2);
	// g_pContext->VSSetConstantBuffers(4, 1, &g_pPSConstantBuffer2);
}

void Light_SetSpecularWorld(const DirectX::XMFLOAT3& camera_position, float power, const DirectX::XMFLOAT4& color)
{
	SpecularLight light{ camera_position,power,color };

	// 定数バッファに鏡面反射光をセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer3, 0, nullptr, &light, 0, 0); // これが転送
	g_pContext->PSSetConstantBuffers(3, 1, &g_pPSConstantBuffer3); //これがセット
}

void Light_SetPointLightCount(int count)
{
	g_PointLights.count = count;

	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_PointLights, 0, 0);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);
}

void Light_SetPointLight(int n, const DirectX::XMFLOAT3& position, float range, const DirectX::XMFLOAT3& color)
{
	g_PointLights.light[n].LightPosition = position;
	g_PointLights.light[n].Range = range;
	g_PointLights.light[n].Color = { color.x, color.y, color.z, 1.0f };

	g_pContext->UpdateSubresource(g_pPSConstantBuffer4, 0, nullptr, &g_PointLights, 0, 0);
	g_pContext->PSSetConstantBuffers(4, 1, &g_pPSConstantBuffer4);
}
