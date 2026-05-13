// ----------------------------------------------------
// 水面描画用頂点シェーダー [shader_vertex_water.hlsl]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-22
// Version: 1.0
// ----------------------------------------------------
cbuffer VS_WORLD_MTX : register(b0)
{
    float4x4 world;
};
cbuffer VS_WORLD : register(b1)
{
    float4x4 view;
};
cbuffer VS_PROJ : register(b2)
{
    float4x4 proj;
};
cbuffer VS_CB : register(b3)
{
    float time;
    float waveAmplitude;
    float waveFrequency;
    float waveSpeed;
    float baseHeight;
};
cbuffer VS_REFLECT : register(b4)
{
    float4x4 reflectViewProj;
};
// ライトビュープロジェクション行列
cbuffer VS_LIGHT : register(b5)
{
    float4x4 lightViewProj;
};

struct VS_IN
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};
struct VS_OUT
{
    float4 posH : SV_POSITION;
    float3 normalW : NORMAL;
    float2 uv : TEXCOORD0;
    float3 posW : POSITION1;
    float4 posReflClip : TEXCOORD1; // float4 (クリップ座標)
    float4 posLightSpace : TEXCOORD2; // ライト空間での座標
};

float waveHeight(float3 p)
{
    float a = (p.x * waveFrequency) + time * waveSpeed;
    float b = (p.z * waveFrequency * 0.85) + time * waveSpeed * 1.2;
    float h = sin(a) + cos(b);
    return h * 0.5 * waveAmplitude;
}

VS_OUT main(VS_IN vin)
{
    VS_OUT vout;
    float3 displaced = vin.pos;
    displaced.y = waveHeight(vin.pos) + baseHeight;

    float a = (vin.pos.x * waveFrequency) + time * waveSpeed;
    float b = (vin.pos.z * waveFrequency * 0.85) + time * waveSpeed * 1.2;

    float dhdx = waveAmplitude * 0.5 * waveFrequency * cos(a);
    float dhdz = -waveAmplitude * 0.5 * waveFrequency * 0.85 * sin(b);

    float3 n = normalize(float3(-dhdx, 1.0f, -dhdz));

    float4 wp = mul(float4(displaced, 1), world);
    float4 vp = mul(wp, view);
    float4 hp = mul(vp, proj);

    // クリップ座標そのまま渡す
    vout.posReflClip = mul(wp, reflectViewProj);

    // ライト空間座標の計算
    vout.posLightSpace = mul(wp, lightViewProj);

    vout.posH = hp;
    vout.normalW = n;
    vout.uv = vin.uv;
    vout.posW = wp.xyz;

    return vout;
}