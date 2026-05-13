/*==============================================================================

   3D用シェーダー [shader3d.cpp]
														 Author : Yasuda Atsushi
														 Date   : 2025/09/10
--------------------------------------------------------------------------------

==============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "shader3d.h"
#include "sampler.h"


static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr; // World Matrix
static ID3D11Buffer* g_pVSConstantBuffer3 = nullptr; // Light View Projection Matrix (追加)
static ID3D11Buffer* g_pVSConstantBuffer4 = nullptr; // Fog Parameters (追加)
static ID3D11Buffer* g_pPSConstantBuffer0 = nullptr; // PS定数バッファb0
static ID3D11Buffer* g_pPSConstantBuffer5 = nullptr; // Fog Color (追加)
static ID3D11Buffer* g_pPSConstantBuffer6 = nullptr; // Shadow Intensity (シャドウ濃度) (追加)
static ID3D11PixelShader* g_pPixelShader = nullptr;


// 注意!ここで外部から設定される。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


bool Shader3d_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr; // 戻り値格納用

	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Shader_Initialize() : 渡されたデバイスやコンテキストが不正です" << std::endl;
		return false;
	}

	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;


	// 事前コンパイル済み頂点シェーダーの読み込み
	std::ifstream ifs_vs("resource/shader/shader_vertex_3d.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBoxW(nullptr, L"頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_3d.cso", L"エラー", MB_OK);
		return false;
	}

	// ファイルサイズを取得
	ifs_vs.seekg(0, std::ios::end); // ファイルポインタを末尾に移動
	std::streamsize filesize = ifs_vs.tellg(); // ファイルポインタの位置を取得(つまりファイルサイズ)
	ifs_vs.seekg(0, std::ios::beg); // ファイルポインタを先頭に戻す

	// バイナリデータを格納するためのバッファを確保
	unsigned char* vsbinary_pointer = new unsigned char[filesize];

	ifs_vs.read((char*)vsbinary_pointer, filesize); // バイナリデータを読み込み
	ifs_vs.close(); // ファイルを閉じる

	// 頂点シェーダーの作成
	hr = g_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
		delete[] vsbinary_pointer; // メモリリークしないようにバイナリデータのバッファを解放
		return false;
	}


	// 頂点レイアウトの定義 シェーダーに頂点を流す際の 型
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT,	 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },	
		{ "COLOR",	  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT num_elements = ARRAYSIZE(layout); // 配列の要素数を取得

	// 頂点レイアウトの作成
	hr = g_pDevice->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInputLayout);

	delete[] vsbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}


	// 頂点シェーダー用定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // バインドフラグ

	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0);

	// Light View Projection Matrix用の定数バッファを追加 (b3)
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer3);

	// Fog Parameters用の定数バッファを追加 (b4)
	// カメラ位置(float3) + fogStart(float) + fogEnd(float) + padding(float3) = 32 bytes
	buffer_desc.ByteWidth = 32;
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer4);


	// 事前コンパイル済みピクセルシェーダーの読み込み
	std::ifstream ifs_ps("resource/shader/shader_pixel_3d.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBoxW(nullptr, L"ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_3d.cso", L"エラー", MB_OK);
		return false;
	}

	ifs_ps.seekg(0, std::ios::end);
	filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);

	unsigned char* psbinary_pointer = new unsigned char[filesize];
	ifs_ps.read((char*)psbinary_pointer, filesize);
	ifs_ps.close();

	// ピクセルシェーダーの作成
	hr = g_pDevice->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pPixelShader);

	delete[] psbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
		return false;
	}	

	// ピクセルシェーダー用定数バッファの作成
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズ

	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer0);

	// Fog Color用の定数バッファを追加 (b5)
	buffer_desc.ByteWidth = sizeof(XMFLOAT4);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer5);

	// Shadow Intensity用の定数バッファを追加 (b6)
	buffer_desc.ByteWidth = sizeof(XMFLOAT4); // シャドウ濃度は float 1個だが、定数バッファは16バイト境界に合わせる必要がある
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer6);

	return true;
}

void Shader3d_Finalize()
{
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pPSConstantBuffer6); // シャドウ濃度バッファを追加
	SAFE_RELEASE(g_pPSConstantBuffer5);
	SAFE_RELEASE(g_pPSConstantBuffer0);
	SAFE_RELEASE(g_pVSConstantBuffer4);
	SAFE_RELEASE(g_pVSConstantBuffer3);
	SAFE_RELEASE(g_pVSConstantBuffer0);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);
}

void Shader3d_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定数バッファに行列をセット
	g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void Shader3d_SetLightViewProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
	XMFLOAT4X4 transpose;
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));
	g_pContext->UpdateSubresource(g_pVSConstantBuffer3, 0, nullptr, &transpose, 0, 0);
}

void Shader3d_SetFogParameters(const DirectX::XMFLOAT3& cameraPos, float fogStart, float fogEnd)
{
	// カメラ位置、フォグ開始距離、フォグ終了距離を定数バッファにセット
	struct FogParams {
		XMFLOAT3 cameraPos;
		float fogStart;
		float fogEnd;
		XMFLOAT3 padding; // パディング
	} fogParams;

	fogParams.cameraPos = cameraPos;
	fogParams.fogStart = fogStart;
	fogParams.fogEnd = fogEnd;
	fogParams.padding = XMFLOAT3(0, 0, 0);

	g_pContext->UpdateSubresource(g_pVSConstantBuffer4, 0, nullptr, &fogParams, 0, 0);
}

void Shader3d_SetFogColor(const XMFLOAT4& fogColor)
{
	// フォグ色を定数バッファにセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer5, 0, nullptr, &fogColor, 0, 0);
}

// シャドウ濃度設定関数を追加
void Shader3d_SetTerrainShadowIntensity(float intensity)
{
	// シャドウ濃度を定数バッファにセット
	// 0.0f = 影が見えない、1.0f = 完全な影
	float clampedIntensity = (intensity < 0.0f) ? 0.0f : (intensity > 1.0f) ? 1.0f : intensity;
	// float4として扱うため、パディングを含める
	XMFLOAT4 shadowIntensity(clampedIntensity, 0.0f, 0.0f, 0.0f);
	g_pContext->UpdateSubresource(g_pPSConstantBuffer6, 0, nullptr, &shadowIntensity, 0, 0);
}

void Shader3d_SetShadowMapTexture(ID3D11ShaderResourceView* srv)
{
	// ピクセルシェーダーのスロット1番にシャドウマップテクスチャをセット
	g_pContext->PSSetShaderResources(1, 1, &srv);
}

void Shader3d_SetColor(const XMFLOAT4& color)
{
	// 定数バッファに色をセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void Shader3d_Begin()
{
	// 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

	// 頂点レイアウトを描画パイプラインに設定
	g_pContext->IASetInputLayout(g_pInputLayout);

	// 定数バッファを描画パイプラインに設定
	ID3D11Buffer* vsBuffers[] = { g_pVSConstantBuffer0, g_pVSConstantBuffer3 };
	g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
	g_pContext->VSSetConstantBuffers(3, 1, &g_pVSConstantBuffer3);  // b3にライトビュー行列
	g_pContext->VSSetConstantBuffers(4, 1, &g_pVSConstantBuffer4);  // b4にフォグパラメータ
	g_pContext->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);
	g_pContext->PSSetConstantBuffers(5, 1, &g_pPSConstantBuffer5);  // b5にフォグ色
	g_pContext->PSSetConstantBuffers(6, 1, &g_pPSConstantBuffer6);  // b6にシャドウ濃度

	// サンプラーステートを描画パイプラインに設定
	//Sampler_SetFilterAnisotropic();
}
