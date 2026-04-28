/*==============================================================================

   シャドウマップ生成用頂点シェーダー [shader_vertex_shadow.hlsl]
   
==============================================================================*/

// 定数バッファ
cbuffer VS_CONSTANT_BUFFER : register(b0)
{
    float4x4 world;
};

cbuffer VS_CONSTANT_BUFFER : register(b1)
{
    float4x4 view; // ライトのビュー行列
};

cbuffer VS_CONSTANT_BUFFER : register(b2)
{
    float4x4 proj; // ライトのプロジェクション行列
};

struct VS_IN
{
    float4 posL : POSITION0; // ローカル座標
};

struct VS_OUT
{
    float4 posH : SV_POSITION; // クリップ空間座標
};

//=============================================================================
// 頂点シェーダー
//=============================================================================
VS_OUT main(VS_IN vi)
{
    VS_OUT vo;

    // 行列計算 (World -> Light View -> Light Projection)
    // ライトから見た座標系へ変換します
    float4x4 mtxWV = mul(world, view);
    float4x4 mtxWVP = mul(mtxWV, proj);

    // 座標変換
    vo.posH = mul(vi.posL, mtxWVP);

    return vo;
}