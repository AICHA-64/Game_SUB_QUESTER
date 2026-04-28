#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include <d3d11.h>
#include <DirectXMath.h>

class ShadowMap
{
public:
    ShadowMap(ID3D11Device* device, int width, int height);
    ~ShadowMap();

    // シャドウマップへの書き込みを開始する（深度バッファのクリアとターゲット設定）
    void BindDsvAndSetNullRenderTarget(ID3D11DeviceContext* deviceContext);

    // シェーダーリソースとしてスロットにセットする
    ID3D11ShaderResourceView* GetShaderResourceView() const { return m_pShaderResourceView; }

    // ビューポート情報
    const D3D11_VIEWPORT& GetViewport() const { return m_Viewport; }

private:
    int m_Width;
    int m_Height;

    ID3D11ShaderResourceView* m_pShaderResourceView; // シェーダーから読む用
    ID3D11DepthStencilView* m_pDepthStencilView;     // 書き込み用
    D3D11_VIEWPORT m_Viewport;
};

#endif // SHADOW_MAP_H
