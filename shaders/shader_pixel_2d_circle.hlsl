/*==============================================================================

   2D描画用ピクセルシェーダー [shader_pixel_2d_circle.hlsl]
														 Author : Yasuda Atsushi
														 Date   : 2025/05/15
--------------------------------------------------------------------------------

==============================================================================*/

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

Texture2D tex; // テクスチャ
SamplerState samp; // テクスチャサンプラ

float4 main(PS_INPUT pi) : SV_TARGET
{
    // UV の中心 (0.5,0.5) を基準に円形にクリップ
    float2 d = pi.uv - float2(0.5f, 0.5f);
    float r = length(d);
    // 半径 0.5 の円。外側は破棄
    clip(0.5f - r);
    return tex.Sample(samp, pi.uv) * pi.color;
}