// ----------------------------------------------------
// 水面描画用ピクセルシェーダ [shader_pixel_water.hlsl]
// ----------------------------------------------------
cbuffer PS_CB : register(b0)
{
    float4 uvScroll01;
    float4 waterColor;
    float4 cameraPos_fresnel;
};
Texture2D texDiffuse : register(t0);
Texture2D texNormal : register(t1);
Texture2D texReflection : register(t2);
Texture2D shadowMap : register(t3); // シャドウマップ

SamplerState samp : register(s0);
SamplerComparisonState shadowSampler : register(s1); // 比較サンプラー

struct PS_IN
{
    float4 posH : SV_POSITION;
    float3 normalW : NORMAL;
    float2 uv : TEXCOORD0;
    float3 posW : POSITION1;
    float4 posReflClip : TEXCOORD1; // クリップ座標を受け取る
    float4 posLightSpace : TEXCOORD2; // ライト空間座標
};

// 調整用定数
static const float SHADOW_INTENSITY = 0.75f; // 0 = 影をほぼ消す, 1 = 元の影の強さ
static const float SHADOW_DARK_FACTOR = 0.5f; // 影時の暗さ（0..1） 水面ベース色に掛ける
static const float LIGHT_SHADOW_AVERAGE = 0.9f; // ライト寄与の最小倍率（影中）


// 影の計算関数
float CalculateShadow(float4 posLightSpace)
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
    
    if (projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f;
    }
    
    float currentDepth = projCoords.z;
    float bias = 0.00005f; // 水面用に少し大きめのバイアス
    float shadow = 0.0f;
    float2 texelSize = 1.0f / float2(4056.0f, 4056.0f);
    
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
    
    // 影を薄くする（1.0 = 無変更、0.0 = 影なし）
    shadow = lerp(1.0f, shadow, SHADOW_INTENSITY);

    return shadow;
}

float3 applyLighting(float3 n, float shadow)
{
    float3 lightDir = normalize(float3(-0.4, -1.0, 0.3));
    float diff = saturate(dot(-lightDir, n) * 0.5 + 0.5);
    // 影を適用（影の部分は暗くする）
    return float3(0.2, 0.3, 0.5) * diff * lerp(0.3, 1.0, shadow);
}

float4 main(PS_IN pin) : SV_TARGET
{
    // UVスクロールとテクスチャのサンプリング
    float2 uv0 = pin.uv + uvScroll01.xy; // カラー用UV
    float2 uv1 = pin.uv + uvScroll01.zw; // 法線マップ用UV

    // 水面のベースカラーと法線マップを取得
    float3 baseCol = texDiffuse.Sample(samp, uv0).rgb * waterColor.rgb;
    float3 nTS = texNormal.Sample(samp, uv1).rgb * 2.0 - 1.0;

    // ワールド空間への法線変換
    float3 N = normalize(pin.normalW);
    // 上方向(Y軸)と平行にならないようにヘルパーベクトルを決定
    float3 helper = (abs(N.y) > 0.9) ? float3(0, 0, 1) : float3(0, 1, 0);
    float3 T = normalize(cross(helper, N)); // 接線ベクトル
    float3 B = normalize(cross(N, T)); // 従法線ベクトル
    
    // 法線マップの値をワールド空間に変換
    float3 worldN = normalize(nTS.x * T + nTS.y * B + nTS.z * N);

    // シャドウ計算
    float shadow = CalculateShadow(pin.posLightSpace); // 1.0=ライト, 0.0=影

    // 水面の「影色」を作る（baseCol を使って暗い色を作る）
    float3 shadowBase = baseCol * SHADOW_DARK_FACTOR;

    // baseCol と shadowBase を shadow パラメータで混ぜる（影部分ほど shadowBase に寄せる）
    float3 baseColShaded = lerp(shadowBase, baseCol, shadow);

    // ライティングにシャドウを適用
    float3 lightCol = applyLighting(worldN, shadow);

    // フレネル反射の計算
    float3 V = normalize(cameraPos_fresnel.xyz - pin.posW);
    // フレネルの強さを計算（カメラに対して面が垂直なほど強くなる）
    float fresnel = pow(1.0 - saturate(dot(worldN, V)), cameraPos_fresnel.w);

    // 射影テクスチャマッピング
    // ピクセル単位で補間されたクリップ座標を使ってUVを計算
    float2 refUV = pin.posReflClip.xy / pin.posReflClip.w;
        
    // Y軸はUV空間(下向き正) と NDC(上向き正) で逆なのでマイナスを掛ける
    refUV.x = refUV.x * 0.5f + 0.5f;
    refUV.y = -refUV.y * 0.5f + 0.5f;

    // 歪み処理 (法線マップを使ってずらす)
    refUV += worldN.xz * 0.05f; // 強さは好みで調整

    // 範囲外の反射を抑制
    if (refUV.x < 0.0 || refUV.x > 1.0 || refUV.y < 0.0 || refUV.y > 1.0)
        fresnel *= 0.0;

    // 反射テクスチャから色を取得
    float3 reflectionColor = texReflection.Sample(samp, refUV).rgb;

    // 最終合成
    // reflection はフレネルで重み、baseColShaded をメインにして水面色に影を馴染ませる
    float3 finalCol = baseColShaded * 0.55f + lightCol * 0.5f + reflectionColor * fresnel * 0.8f;

    return float4(finalCol, 0.7f); // 少し透明度を下げる
}
