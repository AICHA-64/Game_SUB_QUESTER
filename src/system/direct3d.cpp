// --------------------------------------------
// Direct3Dの初期化関連 [direct3d.cpp]
// ============================================
// Created by: Yasuda Atsushi
// Date: 2025-06-18
// Version: 1.0
// --------------------------------------------
#include <d3d11.h>
#include "direct3d.h"
#include "debug_ostream.h"
using namespace DirectX;

#pragma comment(lib, "d3d11.lib")
// #pragma comment(lib, "dxgi.lib")

/* 各種インターフェース */
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11BlendState* g_pBlendStateMultiply = nullptr;
static ID3D11BlendState* g_pBlendStateAdd = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilStateDepthDisable = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilStateDepthEnable = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilStateDepthWriteDisable = nullptr;


// ラスタライザステートの作成
static ID3D11RasterizerState* g_pRasterizerState = nullptr;
static ID3D11RasterizerState* g_pRasterizerStateShadow = nullptr; // シャドウマップ用（フロントフェイスカリング）

/* バックバッファ関連 */
static ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
static ID3D11Texture2D* g_pDepthStencilBuffer = nullptr;
static ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
static D3D11_TEXTURE2D_DESC g_BackBufferDesc{};
static D3D11_VIEWPORT g_Viewport{}; // ビューポート設定用

static bool configureBackBuffer(); // バックバッファの設定・生成
static void releaseBackBuffer(); // バックバッファの解放

// オフスクリーンレンダリング関連
static ID3D11Texture2D* g_pOffScreen_Buffer = nullptr;
static ID3D11RenderTargetView* g_pOffScreen_RenderTargetView = nullptr;
static ID3D11ShaderResourceView* g_pOffScreen_ShaderResourceView = nullptr;
static ID3D11Texture2D* g_pOffScreen_DepthStencilBuffer = nullptr;
static ID3D11DepthStencilView* g_pOffScreen_DepthStencilView = nullptr;
static D3D11_TEXTURE2D_DESC g_OffScreen_Desc{};
static D3D11_VIEWPORT g_OffScreen_Viewport{}; // ビューポート設定用

static bool configureOffScreen_BackBuffer(); // オフスクリーンバックバッファの設定・生成
static void releaseOffScreen_BackBuffer(); // オフスクリーンバックバッファの解放

// シャドウマップ用のグローバル変数
static ID3D11Texture2D* g_pShadowMapTexture = nullptr;       // テクスチャ本体
static ID3D11DepthStencilView* g_pShadowMapDSV = nullptr;           // 深度書き込み用ビュー
static ID3D11ShaderResourceView* g_pShadowMapSRV = nullptr;     // シェーダー読み込み用ビュー
static D3D11_VIEWPORT g_ShadowMapViewport = {};  // シャドウマップ用ビューポート

// Bloomパス用・メインシーンレンダーターゲット
static ID3D11Texture2D* g_pMainSceneTexture = nullptr;
static ID3D11RenderTargetView* g_pMainSceneRTV = nullptr;
static ID3D11ShaderResourceView* g_pMainSceneSRV = nullptr;
static ID3D11Texture2D* g_pMainSceneDepthBuffer = nullptr;
static ID3D11DepthStencilView* g_pMainSceneDSV = nullptr;


bool Direct3D_Initialize(HWND hWnd)
{
	/* デバイス、スワップチェーン、コンテキスト生成 */
	DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
	swap_chain_desc.Windowed = TRUE;	// full screen
	swap_chain_desc.BufferCount = 2;	// 裏画面が何個用意する
	// swap_chain_desc.BufferDesc.Width = 0;
	// swap_chain_desc.BufferDesc.Height = 0;
	// ⇒ ウィンドウサイズに合わせて自動的に設定される
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// 色のformat
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 何に使う、ここは絵を各場所で使う
	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	//swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // ベンチマークの時はこっち
	swap_chain_desc.OutputWindow = hWnd;

	/*
	IDXGIFactory1* pFactory;
	CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	IDXGIAdapter1* pAdapter;
	pFactory->EnumAdapters1(1, &pAdapter); // セカンダリアダプタを取得
	pFactory->Release();
	DXGI_ADAPTER_DESC1 desc;
	pAdapter->GetDesc1(&desc); // アダプタの情報を取得して確認したい場合
	pAdapter->Release(); // D3D11CreateDeviceAndSwapChain()の第１引数に渡して利用し終わったら解放する
	*/

	UINT device_flags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		device_flags,
		levels,
		ARRAYSIZE(levels),
		D3D11_SDK_VERSION,
		&swap_chain_desc,
		&g_pSwapChain,	// 大事
		&g_pDevice,	// 大事
		&feature_level,
		&g_pDeviceContext	// 大事
	);

	if (FAILED(hr)) {
		MessageBoxW(hWnd, L"Direct3Dの初期化に失敗しました", L"エラー", MB_OK);
		return false;
	}

	if (!configureBackBuffer()) {
		MessageBoxW(hWnd, L"バックバッファの設定に失敗しました", L"エラー", MB_OK);
		return false;
	}

	// オフスクリーン関連のリソース作成
	configureOffScreen_BackBuffer();

	// RGB A -> 好きに使っていい値、基本は透明の表現に使う
	// αテスト、αブレンド

	// ブレンドステート設定
	D3D11_BLEND_DESC bd = {};
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;
	bd.RenderTarget[0].BlendEnable = TRUE;	// αブレンドするしない

	/*------------------ 透過ブレンドの設定 ------------------*/

	// src ... ソース（今から描く絵（色）） dest　...　すでに絵描かれた絵（色）

	// RGB
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;	// 演算子
	// SrcRGB * SrcAlpha + DestRGB * (1 - SrcAlpha)

	// A
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	// SrcBlendAlpha * 1 + DestBlendAlpha * 0

	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	g_pDevice->CreateBlendState(&bd, &g_pBlendStateMultiply);

	/*------------------ --------------- ------------------*/


	/*------------------ 加算ブレンドの設定 ------------------*/

	bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	// SrcRGB * SrcAlpha + DestRGB * 1
	// SrcBlendAlpha * 1 + DestBlendAlpha * 0

	g_pDevice->CreateBlendState(&bd, &g_pBlendStateAdd);

	/*------------------ --------------- ------------------*/

	Direct3D_SetAlphaBlendTransparent(); // デフォルトのブレンドステート

	// 深度ステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthFunc = D3D11_COMPARISON_LESS;
	dsd.StencilEnable = FALSE;
	dsd.DepthEnable = FALSE; // 無効にする
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilStateDepthDisable);

	dsd.DepthEnable = TRUE;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilStateDepthEnable);

	dsd.StencilEnable = FALSE;
	// dsd.DepthFunc = D3D11_COMPARISON_ALWAYS; // 絶対に描画される
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	g_pDevice->CreateDepthStencilState(&dsd, &g_pDepthStencilStateDepthWriteDisable);

	Direct3D_SetDepthEnable(true);

	// ラスタライザステートの作成
	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = D3D11_FILL_SOLID;
	//rd.FillMode = D3D11_FILL_WIREFRAME;
	rd.CullMode = D3D11_CULL_BACK;
	//rd.CullMode = D3D11_CULL_NONE;
	rd.DepthClipEnable = TRUE;
	rd.MultisampleEnable = FALSE;
	g_pDevice->CreateRasterizerState(&rd, &g_pRasterizerState);

	// シャドウマップ用ラスタライザステート（フロントフェイスカリング）
	D3D11_RASTERIZER_DESC rdShadow = {};
	rdShadow.FillMode = D3D11_FILL_SOLID;
	rdShadow.CullMode = D3D11_CULL_FRONT; // 表面をカリングして裏面を描画
	rdShadow.DepthClipEnable = TRUE;
	rdShadow.MultisampleEnable = FALSE;
	rdShadow.DepthBias = 0; // 必要に応じて深度バイアスも追加可能
	rdShadow.DepthBiasClamp = 0.0f;
	rdShadow.SlopeScaledDepthBias = 0.0f;
	g_pDevice->CreateRasterizerState(&rdShadow, &g_pRasterizerStateShadow);

	// デバイスコンテキストにラスタライザーステートを設定
	g_pDeviceContext->RSSetState(g_pRasterizerState);

	// シャドウマップの作成 (例えば 2048x2048)
	// 解像度が高いほど影がきれいになりますが、負荷も上がります
	Direct3D_CreateShadowMap(4096, 4096);

	return true;
}

void Direct3D_Finalize()
{

	// Same as below
	SAFE_RELEASE(g_pDepthStencilStateDepthDisable);
	SAFE_RELEASE(g_pDepthStencilStateDepthEnable);
	SAFE_RELEASE(g_pBlendStateMultiply);
	SAFE_RELEASE(g_pRasterizerState);
	SAFE_RELEASE(g_pRasterizerStateShadow); // シャドウマップ用ラスタライザステートの解放

	releaseBackBuffer();
	releaseOffScreen_BackBuffer();
	ReleaseShadowMap(); // シャドウマップの解放も追加
	Direct3D_ReleaseMainSceneRT(); // Bloomパス用メインシーンレンダーターゲットの解放

	SAFE_RELEASE(g_pSwapChain);
	SAFE_RELEASE(g_pDeviceContext);
	SAFE_RELEASE(g_pDevice);


	//if (g_pSwapChain) {
	//	g_pSwapChain->Release();
	// 	g_pSwapChain = nullptr;
	//}

	//if (g_pDeviceContext) {
	//	g_pDeviceContext->Release();
	//	g_pDeviceContext = nullptr;
	//}
	//
 //   if (g_pDevice) {
	//	g_pDevice->Release();
	//	g_pDevice = nullptr;
	//}
}

void Direct3D_Clear()
{
	// バックバッファのクリア
	float clear_color[4] = { 0.0f, 0.8f, 0.8f, 1.0f }; // RGBA
	g_pDeviceContext->ClearRenderTargetView(g_pRenderTargetView, clear_color);
	// デプスステンシルビューのクリア
	g_pDeviceContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	// レンダーターゲットビューとデプスステンシルビューの設定 
	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
}

void Direct3D_Present()
{
	// スワップチェーンの表示
	// 貯まった描画コマンドをグラフィックに転送
	g_pSwapChain->Present(1, 0); // ベンチマークをとるときは(0, 0)
}

unsigned int Direct3D_GetBackBufferWidth()
{
	return g_BackBufferDesc.Width;
}
unsigned int Direct3D_GetBackBufferHeight()
{
	return g_BackBufferDesc.Height;
}

ID3D11Device* Direct3D_GetDevice()
{
	return g_pDevice;
}

ID3D11DeviceContext* Direct3D_GetContext()
{
	return g_pDeviceContext;
}

ID3D11ShaderResourceView* Direct3D_GetShadowMapSRV()
{
	return g_pShadowMapSRV;
}

void Direct3D_SetAlphaBlendTransparent()
{
	float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pDeviceContext->OMSetBlendState(g_pBlendStateMultiply, blend_factor, 0xffffffff);
}

void Direct3D_SetAlphaBlendAdd()
{
	float blend_factor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pDeviceContext->OMSetBlendState(g_pBlendStateAdd, blend_factor, 0xffffffff);
}

void Direct3D_SetDepthEnable(bool enable)
{
	if (enable) {
		g_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilStateDepthEnable, NULL);
	}
	else
	{
		g_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilStateDepthDisable, NULL);
	}
}

void Direct3D_SetDepthWriteDisable()
{
	g_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilStateDepthWriteDisable, NULL);
}

DirectX::XMMATRIX Direct3D_MatrixViewport()
{
	float half_Width = Direct3D_GetBackBufferWidth() * 0.5f;
	float half_Height = Direct3D_GetBackBufferHeight() * 0.5f;
	float max_depth = g_Viewport.MaxDepth;
	float min_depth = g_Viewport.MinDepth;
	
	return DirectX::XMMatrixSet(
		half_Width, 0.0f,		  0.0f,					 0.0f,
		0.0f,		-half_Height, 0.0f,					 0.0f,
		0.0f,		0.0f,		  (max_depth - min_depth), 0.0f,
		half_Width, half_Height,  min_depth,				 1.0f
	);
}

DirectX::XMFLOAT3 Direct3D_ScreenToWorld(int x, int y, float depth, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	XMMATRIX xview{ XMLoadFloat4x4(&view) };
	XMMATRIX xproj{ XMLoadFloat4x4(&projection) };
	XMVECTOR xpoint{ static_cast<float>(x), static_cast<float>(y), depth, 1.0f };

	XMMATRIX inv{ XMMatrixInverse(nullptr, xview * xproj * Direct3D_MatrixViewport()) };

	xpoint = XMVector3TransformCoord(xpoint, inv);

	XMFLOAT3 ret;

	XMStoreFloat3(&ret, xpoint);

	return ret;
}

DirectX::XMFLOAT2 Direct3D_WorldToScreen(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	XMMATRIX xview{ XMLoadFloat4x4(&view) };
	XMMATRIX xproj{ XMLoadFloat4x4(&projection) };
	XMVECTOR xpoint{ XMLoadFloat3(&position)};

	xpoint = XMVector3TransformCoord(xpoint, xview * xproj * Direct3D_MatrixViewport());

	XMFLOAT2 ret;

	XMStoreFloat2(&ret, xpoint);

	return ret;
}

void Direct3D_ClearBackBuffer()
{
	float clear_color[4] = { 0.0f, 0.8f, 0.8f, 1.0f };
	g_pDeviceContext->ClearRenderTargetView(g_pRenderTargetView, clear_color);
	g_pDeviceContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Direct3D_SetBackBuffer()
{
	g_pDeviceContext->RSSetViewports(1, &g_Viewport); // ビューポートの設定

	// レンダーターゲットビューとデプスステンシルビューの設定 
	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// 通常のバックフェイスカリングに戻す
	g_pDeviceContext->RSSetState(g_pRasterizerState);
}

void Direct3D_ClearOffScreen()
{
	float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_pDeviceContext->ClearRenderTargetView(g_pOffScreen_RenderTargetView, clear_color);
	g_pDeviceContext->ClearDepthStencilView(g_pOffScreen_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Direct3D_SetOffScreen()
{
	g_pDeviceContext->RSSetViewports(1, &g_OffScreen_Viewport); // ビューポートの設定

	// レンダーターゲットビューとデプスステンシルビューの設定 
	g_pDeviceContext->OMSetRenderTargets(1, &g_pOffScreen_RenderTargetView, g_pOffScreen_DepthStencilView);
}

void Direct3D_SetOffScreenTexture(int slot)
{
	// テクスチャ設定
	g_pDeviceContext->PSSetShaderResources(slot, 1, &g_pOffScreen_ShaderResourceView);
}

void Direct3D_CreateShadowMap(int width, int height)
{
	HRESULT hr;

	// 1. テクスチャリソースの作成
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	// シャドウマップは深度情報として扱うため、R24G8やR32_TYPELESSなどが一般的です
	texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	// 深度ステンシルとしてバインドし、かつシェーダーリソースとしてもバインドする
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	hr = g_pDevice->CreateTexture2D(&texDesc, nullptr, &g_pShadowMapTexture);
	if (FAILED(hr)) {
		MessageBoxW(nullptr, L"シャドウマップテクスチャ作成失敗", L"エラー", MB_OK);
		return;
	}

	// 2. 深度ステンシルビュー (DSV) の作成
	// 書き込み用なので、具体的な深度フォーマットを指定します
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	hr = g_pDevice->CreateDepthStencilView(g_pShadowMapTexture, &dsvDesc, &g_pShadowMapDSV);
	if (FAILED(hr)) {
		MessageBoxW(nullptr, L"シャドウマップDSV作成失敗", L"エラー", MB_OK);
		return;
	}

	// 3. シェーダーリソースビュー (SRV) の作成
	// シェーダーで読み込むときは、深度値を赤成分(R)として読むフォーマットなどを指定します
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // 深度部分を読み取る
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	hr = g_pDevice->CreateShaderResourceView(g_pShadowMapTexture, &srvDesc, &g_pShadowMapSRV);
	if (FAILED(hr)) {
		MessageBoxW(nullptr, L"シャドウマップSRV作成失敗", L"エラー", MB_OK);
		return;
	}

	// 4. ビューポートの設定
	g_ShadowMapViewport.TopLeftX = 0.0f;
	g_ShadowMapViewport.TopLeftY = 0.0f;
	g_ShadowMapViewport.Width = static_cast<float>(width);
	g_ShadowMapViewport.Height = static_cast<float>(height);
	g_ShadowMapViewport.MinDepth = 0.0f;
	g_ShadowMapViewport.MaxDepth = 1.0f;
}

// シャドウマップへのレンダリング開始設定
void Direct3D_SetShadowMapRenderTarget()
{
	// レンダーターゲットは不要(NULL)で、深度ステンシルビューだけを設定する
	ID3D11RenderTargetView* nullRTV = nullptr;
	g_pDeviceContext->OMSetRenderTargets(1, &nullRTV, g_pShadowMapDSV);

	// シャドウマップ用のビューポートを設定
	g_pDeviceContext->RSSetViewports(1, &g_ShadowMapViewport);

	// フロントフェイスカリングを適用（シャドウアクネ軽減）
	g_pDeviceContext->RSSetState(g_pRasterizerStateShadow);
}

// シャドウマップのクリア
void Direct3D_ClearShadowMap()
{
	// 深度バッファを1.0(最奥)でクリア
	g_pDeviceContext->ClearDepthStencilView(g_pShadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

// シェーダーリソースとしてセットする
void Direct3D_SetShadowMapTexture(int slot)
{
	g_pDeviceContext->PSSetShaderResources(slot, 1, &g_pShadowMapSRV);
}

// 解放処理（Direct3D_Finalize内などで呼ぶ必要がある）
void ReleaseShadowMap() // ※これをFinalizeに追加してください
{
	SAFE_RELEASE(g_pShadowMapSRV);
	SAFE_RELEASE(g_pShadowMapDSV);
	SAFE_RELEASE(g_pShadowMapTexture);
}


bool configureBackBuffer()
{
	HRESULT hr;

	ID3D11Texture2D* back_buffer_pointer = nullptr;

	// バックバッファの取得
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer_pointer);

	if (FAILED(hr)) {
		hal::dout << "バックバッファの取得に失敗しました" << std::endl;
		return false;
	}

	// バックバッファのレンダーターゲットビューの生成
	hr = g_pDevice->CreateRenderTargetView(back_buffer_pointer, nullptr, &g_pRenderTargetView);

	if (FAILED(hr)) {
		back_buffer_pointer->Release();
		hal::dout << "バックバッファのレンダーターゲットビューの生成に失敗しました" << std::endl;
		return false;
	}

	// バックバッファの状態（情報）を取得
	back_buffer_pointer->GetDesc(&g_BackBufferDesc);

	back_buffer_pointer->Release(); // バックバッファのポインタは不要なので解放

	// デプスステンシルバッファの生成
	D3D11_TEXTURE2D_DESC depth_stencil_desc{};
	depth_stencil_desc.Width = g_BackBufferDesc.Width;
	depth_stencil_desc.Height = g_BackBufferDesc.Height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_stencil_desc.SampleDesc.Count = 1;
	depth_stencil_desc.SampleDesc.Quality = 0;
	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;
	hr = g_pDevice->CreateTexture2D(&depth_stencil_desc, nullptr, &g_pDepthStencilBuffer);

	if (FAILED(hr)) {
		hal::dout << "デプスステンシルバッファの生成に失敗しました" << std::endl;
		return false;
	}

	// デプスステンシルビューの生成
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
	depth_stencil_view_desc.Format = depth_stencil_desc.Format;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;
	depth_stencil_view_desc.Flags = 0;
	hr = g_pDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &depth_stencil_view_desc, &g_pDepthStencilView);

	if (FAILED(hr)) {
		hal::dout << "デプスステンシルビューの生成に失敗しました" << std::endl;
		return false;
	}

	// ビューポートの設定 
	g_Viewport.TopLeftX = 0.0f;
	g_Viewport.TopLeftY = 0.0f;
	g_Viewport.Width = static_cast<FLOAT>(g_BackBufferDesc.Width);
	g_Viewport.Height = static_cast<FLOAT>(g_BackBufferDesc.Height);
	g_Viewport.MinDepth = 0.0f;
	g_Viewport.MaxDepth = 1.0f;

	// g_pDeviceContext->RSSetViewports(1, &g_Viewport); // ビューポートの設定

	return true;
}

void releaseBackBuffer()
{
	// Same as below
	SAFE_RELEASE(g_pRenderTargetView);
	SAFE_RELEASE(g_pDepthStencilBuffer);
	SAFE_RELEASE(g_pDepthStencilView);
	//if (g_pRenderTargetView) {
	//	g_pRenderTargetView->Release();
	//	g_pRenderTargetView = nullptr;
	//}

	//if (g_pDepthStencilBuffer) {
	//	g_pDepthStencilBuffer->Release();
	//	g_pDepthStencilBuffer = nullptr;
	//}

	//if (g_pDepthStencilView) {
	//	g_pDepthStencilView->Release();
	//	g_pDepthStencilView = nullptr;
	//}
}

bool configureOffScreen_BackBuffer()
{
	g_OffScreen_Desc.Width = 512;
	g_OffScreen_Desc.Height = 512;
	g_OffScreen_Desc.MipLevels = 1; // 必要なら後で自動生成
	g_OffScreen_Desc.ArraySize = 1;
	g_OffScreen_Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // sRGB を使うなら _UNORM_SRGB
	g_OffScreen_Desc.SampleDesc.Count = 1; // MSAA するなら >1
	g_OffScreen_Desc.SampleDesc.Quality = 0;
	g_OffScreen_Desc.Usage = D3D11_USAGE_DEFAULT;
	g_OffScreen_Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	g_OffScreen_Desc.CPUAccessFlags = 0;
	g_OffScreen_Desc.MiscFlags = 0;

	g_pDevice->CreateTexture2D(&g_OffScreen_Desc, nullptr, &g_pOffScreen_Buffer);

	g_pDevice->CreateRenderTargetView(g_pOffScreen_Buffer, nullptr, &g_pOffScreen_RenderTargetView);
	g_pDevice->CreateShaderResourceView(g_pOffScreen_Buffer, nullptr, &g_pOffScreen_ShaderResourceView);

	// デプスステンシルバッファの生成
	D3D11_TEXTURE2D_DESC depth_stencil_desc{};
	depth_stencil_desc.Width = g_OffScreen_Desc.Width;
	depth_stencil_desc.Height = g_OffScreen_Desc.Height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度24ビット、ステンシル8ビット
	depth_stencil_desc.SampleDesc.Count = 1;
	depth_stencil_desc.SampleDesc.Quality = 0;
	depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0;
	depth_stencil_desc.MiscFlags = 0;
	g_pDevice->CreateTexture2D(&depth_stencil_desc, nullptr, &g_pOffScreen_DepthStencilBuffer);

	// デプスステンシルビューの生成
	D3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc{};
	depth_stencil_view_desc.Format = depth_stencil_desc.Format;
	depth_stencil_view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depth_stencil_view_desc.Texture2D.MipSlice = 0;
	depth_stencil_view_desc.Flags = 0;
	g_pDevice->CreateDepthStencilView(g_pOffScreen_DepthStencilBuffer, &depth_stencil_view_desc, &g_pOffScreen_DepthStencilView);

	// ビューポートの設定 
	g_OffScreen_Viewport.TopLeftX = 0.0f;
	g_OffScreen_Viewport.TopLeftY = 0.0f;
	g_OffScreen_Viewport.Width = static_cast<FLOAT>(g_OffScreen_Desc.Width);
	g_OffScreen_Viewport.Height = static_cast<FLOAT>(g_OffScreen_Desc.Height);
	g_OffScreen_Viewport.MinDepth = 0.0f;
	g_OffScreen_Viewport.MaxDepth = 1.0f;

	// g_pDeviceContext->RSSetViewports(1, &g_OffScreen_Viewport); // ビューポートの設定

	return true;
}

void releaseOffScreen_BackBuffer()
{
	SAFE_RELEASE(g_pOffScreen_Buffer);
	SAFE_RELEASE(g_pOffScreen_RenderTargetView);
	SAFE_RELEASE(g_pOffScreen_ShaderResourceView);
	SAFE_RELEASE(g_pOffScreen_DepthStencilBuffer);
	SAFE_RELEASE(g_pOffScreen_DepthStencilView);
}

// Bloom用メインシーンレンダーターゲットの作成
void Direct3D_CreateMainSceneRT(int width, int height)
{
	HRESULT hr;

	// メインシーンレンダーターゲット用テクスチャの作成
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	hr = g_pDevice->CreateTexture2D(&texDesc, nullptr, &g_pMainSceneTexture);
	if (FAILED(hr)) {
		MessageBoxW(nullptr, L"メインシーンレンダーターゲット用テクスチャ作成失敗", L"エラー", MB_OK);
		return;
	}

	hr = g_pDevice->CreateRenderTargetView(g_pMainSceneTexture, nullptr, &g_pMainSceneRTV);
	if (FAILED(hr)) {
		MessageBoxW(nullptr, L"メインシーンレンダーターゲットRTV作成失敗", L"エラー", MB_OK);
		return;
	}

	hr = g_pDevice->CreateShaderResourceView(g_pMainSceneTexture, nullptr, &g_pMainSceneSRV);
	if (FAILED(hr)) {
		MessageBoxW(nullptr, L"メインシーンレンダーターゲットSRV作成失敗", L"エラー", MB_OK);
		return;
	}

	// 深度バッファの生成
	D3D11_TEXTURE2D_DESC depthDesc = {};
	depthDesc.Width = width;
	depthDesc.Height = height;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;

	hr = g_pDevice->CreateTexture2D(&depthDesc, nullptr, &g_pMainSceneDepthBuffer);
	if (FAILED(hr)) {
		MessageBoxW(nullptr, L"メインシーンレンダーターゲット深度バッファ作成失敗", L"エラー", MB_OK);
		return;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = depthDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	hr = g_pDevice->CreateDepthStencilView(g_pMainSceneDepthBuffer, &dsvDesc, &g_pMainSceneDSV);
	if (FAILED(hr)) {
		MessageBoxW(nullptr, L"メインシーンレンダーターゲットDSV作成失敗", L"エラー", MB_OK);
		return;
	}
}

void Direct3D_SetMainSceneRT()
{
	g_pDeviceContext->OMSetRenderTargets(1, &g_pMainSceneRTV, g_pMainSceneDSV);
	// メインシーンレンダーターゲット用のビューポートを設定
	g_pDeviceContext->RSSetViewports(1, &g_Viewport);
	// 通常のバックフェイスカリングに戻す
	g_pDeviceContext->RSSetState(g_pRasterizerState);
}

void Direct3D_ClearMainSceneRT()
{
	float clearColor[4] = { 0.0f, 0.8f, 0.8f, 1.0f };
	g_pDeviceContext->ClearRenderTargetView(g_pMainSceneRTV, clearColor);
	g_pDeviceContext->ClearDepthStencilView(g_pMainSceneDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

ID3D11ShaderResourceView* Direct3D_GetMainSceneSRV()
{
	return g_pMainSceneSRV;
}

void Direct3D_ReleaseMainSceneRT()
{
	SAFE_RELEASE(g_pMainSceneSRV);
	SAFE_RELEASE(g_pMainSceneRTV);
	SAFE_RELEASE(g_pMainSceneTexture);
	SAFE_RELEASE(g_pMainSceneDSV);
	SAFE_RELEASE(g_pMainSceneDepthBuffer);
}
