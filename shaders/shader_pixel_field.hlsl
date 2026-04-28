/*==============================================================================

   フィールド描画用ピクセルシェーダー [shader_pixel_field.hlsl]
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
    float4 ambient_color; // float4じゃないとダメらしい α成分は使わんけども
};

cbuffer PS_COSTANT_BUFFER : register(b2)
{
    float4 directional_world_vector;
    float4 directional_color = { 1.0f, 1.0f, 1.0f, 1.0f };
};

cbuffer PS_COSTANT_BUFFER : register(b3) // float4づつ
{
    float3 eye_posW;
    float specular_power = 30.0f;
    float4 specular_color = { 1.0f, 1.0f, 1.0f, 1.0f };

};

struct PointLight // float4づつ
{
    float3 posW;
    float range;
    float4 color;
};

cbuffer PS_COSTANT_BUFFER : register(b4) // float4づつ
{
    PointLight point_light[4];
    int point_light_count;
    float3 pointlight_dummy;
};

struct PS_IN
{
    float4 posH : SV_POSITION; // H = ワールド座標の頂点
    float4 posW : POSITION0;
    float4 normalW : NORMAL0; // 法線情報
    float4 blend : COLOR0;
    float2 uv : TEXCOORD0;
    float4 posLightSpace : TEXCOORD1; // 追加
};

Texture2D tex0 : register(t0);
// Texture2D tex1 : register(t1); // 削除または置換
Texture2D shadowMap : register(t1); // シャドウマップ (MainのGame_Drawでt1にセットされるため)


SamplerState samp : register(s0); // テクスチャサンプラ
SamplerComparisonState shadowSampler : register(s1); // 比較サンプラー

// 影の計算関数 (shader_pixel_3d.hlslと同じ処理)
float CalculateShadow(float4 posLightSpace, float3 normal, float3 lightDir)
{
    float3 projCoords = posLightSpace.xyz / posLightSpace.w;
    float2 shadowTexCoord;
    shadowTexCoord.x = projCoords.x * 0.5f + 0.5f;
    shadowTexCoord.y = -projCoords.y * 0.5f + 0.5f;
    
    if (shadowTexCoord.x < 0.0f || shadowTexCoord.x > 1.0f ||
        shadowTexCoord.y < 0.0f || shadowTexCoord.y > 1.0f)
    {
        return 1.0f;
    }
    
    // 深度範囲外の場合、影なしとする
    if (projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f;
    }
    
 float currentDepth = projCoords.z;
    
    // 法線ベースの動的バイアス（シャドウアクネ軽減）
    float NdotL = max(dot(normal, -lightDir), 0.0f);
    float bias = max(0.005f * (1.0f - NdotL), 0.001f);
    
    float shadow = 0.0f;
    float2 texelSize = 1.0f / float2(4096.0f, 4096.0f);
    
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            float pcfDepth = shadowMap.SampleCmp(shadowSampler, shadowTexCoord + offset, currentDepth - bias);
            shadow += pcfDepth;
        }
    }
    shadow /= 9.0f;
    return shadow;

}
//=============================================================================
float4 main(PS_IN pi) : SV_TARGET
{
    // UV を回転させてテクスチャを回転させたサンプル
    float2 uv;
  float angle = 3.14159f * 45 / 180.0f;
    uv.x = pi.uv.x * cos(angle) + pi.uv.y * sin(angle);
    uv.y = -pi.uv.x * sin(angle) + pi.uv.y * cos(angle);

    // 2のテクスチャをブレンドパラメーターに応じてブレンドしたサンプル
    float4 tex_color = tex0.Sample(samp, pi.uv);

    // 個の色
    float3 material_color = tex_color.rgb * diffuse_color.rgb;

    // 並行光源（ディフューズライト）
    float4 normalW = normalize(pi.normalW);
    
    // シャドウ計算とディフューズ計算 - 法線がライトに向いている場合のみ
    float NdotL = dot(normalW.xyz, -directional_world_vector.xyz);
    float shadow = 1.0f;
    float dl = 0.0f;
    
    if (NdotL > 0.0f) // ライトに向いている面のみ照らす
    {
   // ライトに向いている面のみ影を計算
        shadow = CalculateShadow(pi.posLightSpace, normalW.xyz, directional_world_vector.xyz);
        
        // ディフューズは NdotL に基づいて計算（ハーフランバート式は使用しない）
   dl = NdotL;
    }
    else
    {
     // ライトに背を向けている面は完全に暗い（ディフューズなし）
  shadow = 1.0f;
     dl = 0.0f;
    }

  float3 diffuse = material_color * directional_color.rgb * dl * shadow;

    // 環境光（アンビエントライト）
    float3 ambient = material_color * ambient_color.rgb;
    // float3 ambient = { 0.0f, 0.0f, 0.0f}; // 環境光を無視しても使える
    
 // スペキュラ（背面では発動させない）
    float3 toEye = normalize(eye_posW - pi.posW.xyz);
  float3 r = reflect(directional_world_vector, normalW).xyz;
    float t = pow(max(dot(r, toEye), 0.0f), specular_power);
    float3 specular = (NdotL > 0.0f) ? specular_color.rgb * t : float3(0.0f, 0.0f, 0.0f);
  
    float3 color = ambient + diffuse + specular;
    
    // 点光源　ポイントライトサンプル
    
    for (int i = 0; i < point_light_count; i++)
    {
        // 点光源から面（ピクセル）へのベクトルを求める
        float3 LightToPixel = pi.posW.xyz - point_light[i].posW;
        
        // 面（ピクセル）とライトとの距離を測る
        float D = length(LightToPixel); // distance
    
        // 影響力の計算
        float A = pow(max(1.0f - 1.0f / point_light[i].range * D, 0.0f), 2.0f); // Attenuation
        // range 400 length = 0   ... A = 1         A * A = 1
        //                  = 100 ... A = 0.75            = 0.5625
        //                  = 200 ... A = 0.5             = 0.25
        //                  = 300 ... A = 0.25            = 0.0625
        //                  = 400 ... A = 0               = 0
    
        // 点光源と面（ピクセル）との向きを考慮に入れる
        
        float dl = max(0.0f, dot(-normalize(LightToPixel), normalW.xyz));
        // float dl = (dot(-directional_world_vector, normalW) + 1.0f) * 0.5f; // 影を柔らかくする
        
        // 点光源の影響を加算する
        color += material_color * point_light[i].color.rgb * A * dl;
        
        // 点光源のスペキュラを加算する
        
        float3 r = reflect(normalize(LightToPixel), normalW.xyz);
        float t = pow(max(dot(r, toEye), 0.0f), specular_power);
        
        color += point_light[i].color.rgb * t;
    }
    
    
    return float4(color, 1.0f); // カラーとαを足す
    
}


