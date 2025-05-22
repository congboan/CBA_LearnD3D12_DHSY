#pragma once
#include "BattleFireDirect.h"

struct MaterialData
{
    float m_diffuseColor[4];
    float m_specularColor[4];
};

class Material
{
public:
    ID3D12Resource* m_constantBuffer;
    ID3D12Resource* m_materialDataSB;
    ID3D12DescriptorHeap* m_descriptHeap;
    ID3D12PipelineState* m_pso;
    Material();

    void SetTexture2D(int inSRVIndex, ID3D12Resource* inResource, int inMipMapLevelCount = 1,
                      DXGI_FORMAT inFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
    void SetStructuredBuffer(int inSRVIndex, ID3D12Resource* inResource, int inPerElementSize, int inPerElementCount);

    void Active(ID3D12GraphicsCommandList* inCommandList);

    void Test();
};
