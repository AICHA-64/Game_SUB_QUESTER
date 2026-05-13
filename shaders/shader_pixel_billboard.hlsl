/*==============================================================================

   ビルボード描画用ピクセルシェーダー [shader_pixel_billboard.hlsl]
														 Author : Yasuda Atsushi
														 Date   : 2025/11/14
--------------------------------------------------------------------------------

==============================================================================*/

cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4 diffuse_color;
};

struct PS_INPUT
{
    float4 posH : SV_POSITION; // H = ワールド座標の頂点
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

Texture2D tex; // テクスチャ
SamplerState samp; // テクスチャサンプラ


float4 main(PS_INPUT pi) : SV_TARGET
{    
    float4 color = tex.Sample(samp, pi.uv) * pi.color * diffuse_color;
    if (color.a < 0.4f)
    {
        discard; // 透明部分は描画しない
    }
    return color;
}
