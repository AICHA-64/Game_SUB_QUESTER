/*==============================================================================

   3D描画用頂点シェーダー（ライト無し） [shader_vertex_3d_unlit.hlsl]
														 Author : Yasuda Atsushi
														 Date   : 2025/11/21
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
 
struct VS_IN
{
    float4 posL : POSITION0; // L = ローカル座標の頂点
    float2 uv : TEXCOORD0;
};
 
struct VS_OUT
{
    float4 posH : SV_POSITION; // H = ワールド座標の頂点
    float2 uv : TEXCOORD0;
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
    vo.uv = vi.uv;
    
    return vo;
}