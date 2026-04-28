/*==============================================================================

   3D描画用ピクセルシェーダー（ライト無し） [shader_pixel_3d_unlit.hlsl]
														 Author : Yasuda Atsushi
														 Date   : 2025/11/21
--------------------------------------------------------------------------------

==============================================================================*/

cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4 diffuse_color;
};

struct PS_INPUT
{
    float4 posH : SV_POSITION; // H = ワールド座標の頂点
    float2 uv : TEXCOORD0;
};

Texture2D tex; // テクスチャ
SamplerState samp; // テクスチャサンプラ

float4 main(PS_INPUT pi) : SV_TARGET
{
    return tex.Sample(samp, pi.uv) * diffuse_color;
}
