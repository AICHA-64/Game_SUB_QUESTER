/*==============================================================================

   3D描画用ピクセルシェーダー [shader_pixel_3d.hlsl]
                                                         Author : Yasuda Atsushi
                                                         Date   : 2025/09/10
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
cbuffer PS_CONSTANT_BUFFER : register(b0)
{
    float4 diffuse_color;
};

cbuffer PS_COSTANT_BUFFER : register(b1)
{
    float4 ambient_color;
};

cbuffer PS_COSTANT_BUFFER : register(b2)
{
    float4 directional_world_vector;
    float4 directional_color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

cbuffer PS_COSTANT_BUFFER : register(b3)
{
    float3 eye_posW;
    float specular_power;
    float4 specular_color;
};

struct PointLight
{
    float3 posW;
    float range;
    float4 color;
};

cbuffer PS_COSTANT_BUFFER : register(b4)
{
    PointLight point_light[4];
    int point_light_count;
    float3 pointlight_dummy;
};

cbuffer PS_CONSTANT_BUFFER : register(b5)
{
    float4 fogColor; // フォグの色
};

cbuffer PS_CONSTANT_BUFFER : register(b6)
{
    float terrainShadowIntensity; // 地形に落ちるシャドウの濃度 (0.0~1.0)
};

struct PS_IN
{
    float4 posH : SV_POSITION;
    float4 posW : POSITION0;
    float4 normalW : NORMAL0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
    float4 posLightSpace : TEXCOORD1; // ライト空間での座標
    float fogFactor : FOG; // フォグ係数
};

Texture2D tex : register(t0); // テクスチャ
Texture2D shadowMap : register(t1); // シャドウマップ
SamplerState samp : register(s0); // テクスチャサンプラー
SamplerComparisonState shadowSampler : register(s1); // シャドウマップ用比較サンプラー

// シャドウ計算関数
float CalculateShadow(float4 posLightSpace, float3 normal, float3 lightDir)
{
    // 透視変換後の座標を正規化デバイス座標に変換
    float3 projCoords = posLightSpace.xyz / posLightSpace.w;
    
    // [-1, 1] -> [0, 1] に変換
    float2 shadowTexCoord;
    shadowTexCoord.x = projCoords.x * 0.5f + 0.5f;
    shadowTexCoord.y = -projCoords.y * 0.5f + 0.5f;
    
    // テクスチャ座標が範囲外なら影なし
    if (shadowTexCoord.x < 0.0f || shadowTexCoord.x > 1.0f ||
        shadowTexCoord.y < 0.0f || shadowTexCoord.y > 1.0f)
    {
        return 1.0f; // 影なし
    }
    
    // 深度範囲外なら影なし
    if (projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f; // 影なしS
    }
    
    // 深度値を取得
    float currentDepth = projCoords.z;
    
    // 法線ベースの動的バイアス（シャドウアクネ軽減）
    float NdotL = max(dot(normal, -lightDir), 0.0f);
    float bias = max(0.001f * (1.0f - NdotL), 0.001f);
    
    // PCF (Percentage Closer Filtering) でソフトシャドウ
    float shadow = 0.0f;
    float2 texelSize = 1.0f / float2(4096.0f, 4096.0f); // シャドウマップのサイズ
    
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            float pcfDepth = shadowMap.SampleCmp
                (shadowSampler, shadowTexCoord + offset, currentDepth - bias);
            shadow += pcfDepth;
        }
    }
    shadow /= 9.0f;
    
    return shadow;
}

float4 main(PS_IN pi) : SV_TARGET
{
    // 材質の色
    float3 material_color = tex.Sample(samp, pi.uv).rgb * pi.color.rgb * diffuse_color.rgb;

    // 並行光源（ディフューズライト）
    float4 normalW = normalize(pi.normalW); // ワールド空間の法線
    
    // シャドウ計算とディフューズ計算 - 法線がライトに向いている場合のみ
    float NdotL = dot(normalW.xyz, -directional_world_vector.xyz);
    float shadow = 1.0f;
    float dl = 0.0f;
    
    if (NdotL > 0.0f) // ライトに向いている面のみ照らす
    {
        // ライトに向いている面のみ影を計算
        shadow = CalculateShadow(pi.posLightSpace, normalW.xyz, directional_world_vector.xyz);
    // シャドウ濃度を適用（0.0 = 影が見えない、1.0 = 完全な影）
        shadow = lerp(1.0f, shadow, terrainShadowIntensity);
        
        // ディフューズは NdotL に基づいて計算（ハーフランバート式は使用しない）
        dl = NdotL;
    }
    else
    {
        // ライトに背を向けている面は完全に暗い（ディフューズなし）
        shadow = 1.0f;
     dl = 0.0f;
    }
    
    // ディフューズにシャドウを適用
    float3 diffuse = material_color * directional_color.rgb * dl * shadow;

    // 環境光
    float3 ambient = material_color * ambient_color.rgb;

    // スペキュラ（背面では発動させない）
    float3 toEye = normalize(eye_posW - pi.posW.xyz);
    float3 r = reflect(directional_world_vector, normalW).xyz;
    float t = pow(max(dot(r, toEye), 0.0f), specular_power);
    float3 specular = (NdotL > 0.0f) ? specular_color.rgb * t * shadow * 0.2f : float3(0.0f, 0.0f, 0.0f);

    float alpha = tex.Sample(samp, pi.uv).a * diffuse_color.a;
    float3 color = ambient + diffuse + specular;

    // リムライト
    float lim = (1.0f - max(dot(normalW.rgb, toEye),0.0f)) *0.95f;
    // pow() requires a non-negative base for fractional exponents. Clamp to >=0 to avoid undefined behavior
    lim = pow(max(lim,0.0f),3.2f);
    
    // 点光源
    for (int i = 0; i < point_light_count; i++)
    {
        float3 LightToPixel = pi.posW.xyz - point_light[i].posW;
        float D = length(LightToPixel);
        float A = pow(max(1.0f - 1.0f / point_light[i].range * D, 0.0f), 2.0f);
        float dl = max(0.0f, dot(-normalize(LightToPixel), normalW.xyz));
        color += material_color * point_light[i].color.rgb * A * dl;
        
        float3 r = reflect(normalize(LightToPixel), normalW.xyz);
        float t = pow(max(dot(r, toEye), 0.0f), specular_power);
        color += point_light[i].color.rgb * t;
    }
    
    // フォグを適用
    // fogFactor = 1.0 ならフォグなし、0.0 なら完全にフォグ色
    color = lerp(fogColor.rgb, color, pi.fogFactor);
    
    return float4(color, alpha);
}