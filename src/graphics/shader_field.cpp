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
#include "shader_field.h"
#include "sampler.h"


static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11Buffer* g_pVSConstantBuffer0 = nullptr; // 定数バッファb0
static ID3D11Buffer* g_pPSConstantBuffer0 = nullptr; // PS定数バッファb0
static ID3D11Buffer* g_pVSConstantBuffer3 = nullptr; // ライト行列用(b3)
static ID3D11PixelShader* g_pPixelShader = nullptr;

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


bool ShaderField_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr; // 戻り値格納用

	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Shader_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return false;
	}

	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;


	// 事前コンパイル済み頂点シェーダーの読み込み
	std::ifstream ifs_vs("resource/shader/shader_vertex_field.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBoxW(nullptr, L"頂点シェーダーの読み込みに失敗しました\n\nshader_vertex_field.cso", L"エラー", MB_OK);
		return false;
	}

	// ファイルサイズを取得
	ifs_vs.seekg(0, std::ios::end); // ファイルポインタを末尾に移動
	std::streamsize filesize = ifs_vs.tellg(); // ファイルポインタの位置を取得（つまりファイルサイズ）
	ifs_vs.seekg(0, std::ios::beg); // ファイルポインタを先頭に戻す

	// バイナリデータを格納するためのバッファを確保
	unsigned char* vsbinary_pointer = new unsigned char[filesize];

	ifs_vs.read((char*)vsbinary_pointer, filesize); // バイナリデータを読み込む
	ifs_vs.close(); // ファイルを閉じる

	// 頂点シェーダーの作成
	hr = g_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

	if (FAILED(hr)) {
		hal::dout << "Shader_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
		delete[] vsbinary_pointer; // メモリリークしないようにバイナリデータのバッファを解放
		return false;
	}


	// 頂点レイアウトの定義　シェーダーに頂点を流し込む　大切
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",	  0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,		    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
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

	// ライトビュープロジェクション行列用バッファ (b3)
	D3D11_BUFFER_DESC buffer_desc_light = {};
	buffer_desc_light.ByteWidth = sizeof(XMFLOAT4X4);
	buffer_desc_light.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	g_pDevice->CreateBuffer(&buffer_desc_light, nullptr, &g_pVSConstantBuffer3);


	// 事前コンパイル済みピクセルシェーダーの読み込み
	std::ifstream ifs_ps("resource/shader/shader_pixel_field.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBoxW(nullptr, L"ピクセルシェーダーの読み込みに失敗しました\n\nshader_pixel_field.cso", L"エラー", MB_OK);
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

	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer0); // 色をセットするため

	return true;
}

void ShaderField_Finalize()
{
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pPSConstantBuffer0);
	SAFE_RELEASE(g_pVSConstantBuffer3); // 解放
	SAFE_RELEASE(g_pVSConstantBuffer0);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);
}

void ShaderField_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	// 定数バッファ格納用行列の構造体を定義
	XMFLOAT4X4 transpose;

	// 行列を転置して定数バッファ格納用行列に変換
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定数バッファに行列をセット
	g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void ShaderField_SetColor(const XMFLOAT4& color)
{
	// 定数バッファに色をセット
	g_pContext->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void ShaderField_SetLightViewProjectionMatrix(const DirectX::XMMATRIX& matrix)
{
	XMFLOAT4X4 transpose;
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));
	g_pContext->UpdateSubresource(g_pVSConstantBuffer3, 0, nullptr, &transpose, 0, 0);
}

void Shader_Field_Begin()
{
	// 頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

	g_pContext->IASetInputLayout(g_pInputLayout);

	// --- 変更: 定数バッファの設定 ---
	// b0: World, b3: LightViewProj
	// まとめて設定できない(番号が飛んでいる)ので個別に設定
	g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
	g_pContext->VSSetConstantBuffers(3, 1, &g_pVSConstantBuffer3); // 【追加】

	g_pContext->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

	// 【追加】シャドウサンプラーをスロット1 (s1) に設定
	// s0 は通常のサンプラー
	Sampler_SetShadowSampler(g_pContext);

}
