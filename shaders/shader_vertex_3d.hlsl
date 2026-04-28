/*==============================================================================

   3D描画用頂点シェーダー [shader_vertex_3d.hlsl]
														 Author : Yasuda Atsushi
														 Date   : 2025/09/10
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4x4 world;
};

cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 view;
};
    
cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4x4 proj;
};

cbuffer VS_CONSTANT_BUFFER : register(b3)
{
    float4x4 lightViewProj; // ライトビュープロジェクション行列
};

cbuffer VS_CONSTANT_BUFFER : register(b4)
{
    float3 cameraPos; // カメラ位置（ワールド空間）
    float fogStart; // フォグ開始距離
    float fogEnd; // フォグ終了距離
    float3 fogPadding; // パディング
};

struct VS_IN
{
    float4 posL : POSITION0;
    float4 normalL : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};
 
struct VS_OUT
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float4 posLightSpace : TEXCOORD1; // ライト空間での座標
    float fogFactor : FOG; // フォグ係数（0.0 = 完全にフォグ, 1.0 = フォグなし）
};

//=============================================================================
// 頂点シェーダー
//=============================================================================
VS_OUT main(VS_IN vi)
{
    VS_OUT vo;
    
    // 座標変換
    float4x4 mtxWV = mul(world, view);
    float4x4 mtxWVP = mul(mtxWV, proj);
    vo.posH = mul(vi.posL, mtxWVP);
    
    // 法線のワールド変換
    float4 normalW = mul(float4(vi.normalL.xyz, 0.0f), world);
    vo.normalW = normalW;
    vo.posW = mul(vi.posL, world);
    
    vo.color = vi.color;
    vo.uv = vi.uv;
    
    // ライト空間での座標を計算
    float4x4 mtxWL = mul(world, lightViewProj);
    vo.posLightSpace = mul(vi.posL, mtxWL);
    
    // フォグ計算（線形フォグ）
    // カメラからの距離を計算
    float distanceToCamera = length(vo.posW.xyz - cameraPos);
    
    // フォグ係数を計算（fogStart～fogEnd の範囲で線形補間）
    // fogFactor = 1.0 (フォグなし) から 0.0 (完全にフォグ) へ
    vo.fogFactor = saturate((fogEnd - distanceToCamera) / (fogEnd - fogStart));
    
    return vo;
}