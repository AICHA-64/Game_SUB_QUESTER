/*==============================================================================

   3D描画用頂点シェーダー [shader_vertex_field.hlsl]
														 Author : Yasuda Atsushi
														 Date   : 2025/10/20
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
cbuffer VS_COSTANT_BUFFER : register(b0)
{
    float4x4 world;
};

cbuffer VS_COSTANT_BUFFER : register(b1)
{
    float4x4 view;
};
    
cbuffer VS_COSTANT_BUFFER : register(b2)
{
    float4x4 proj;
};
 
// ライトビュープロジェクション行列
cbuffer VS_COSTANT_BUFFER : register(b3)
{
    float4x4 lightViewProj;
};
 
struct VS_IN
{
    float4 posL : POSITION0; // L = ローカル座標の頂点
    float4 normalL : NORMAL0; // 法線情報
    float4 blend : COLOR0;
    float2 uv : TEXCOORD0;
};
 
struct VS_OUT
{
    float4 posH : SV_POSITION; // H = ワールド座標の頂点
    float4 posW : POSITION0;
    float4 normalW : NORMAL0; // 法線情報
    float4 blend : COLOR0;
    float2 uv : TEXCOORD0;
    float4 posLightSpace : TEXCOORD1; // ライト空間での座標
};
 
 
//=============================================================================
// 頂点シェーダ
//=============================================================================
VS_OUT main(VS_IN vi)
{ 
    VS_OUT vo;
    
    // 座標変換
    float4x4 mtxWV = mul(world, view);
    float4x4 mtxWVP = mul(mtxWV, proj);
    vo.posH = mul(vi.posL, mtxWVP);
    
    // 普通のワールド変換行列ではダメ
    // ワールド変換行列の転地逆行列
    float4 normalW = mul(float4(vi.normalL.xyz, 0.0f), world); // 法線をワールド座標に変換
    vo.normalW = normalW; // 単位ベクトル化
    vo.posW = mul(vi.posL, world);
    
    vo.blend = vi.blend; // 地面のテクスチャのブレンド値はそのままパススルー
    
    vo.uv = vi.uv;
    
    // ライト空間座標の計算
    float4x4 mtxWL = mul(world, lightViewProj);
    vo.posLightSpace = mul(vi.posL, mtxWL);
    
    return vo;
}