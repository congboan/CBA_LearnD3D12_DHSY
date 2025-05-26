#pragma once
#include "BattleFireDirect.h"

struct MaterialData
{
    float m_diffuseMaterial[4];
    float m_specularMaterial[4];
};

class Material
{
public:
    ID3D12Resource* m_constantBuffer;
    ID3D12Resource* m_materialDataSB;
    ID3D12DescriptorHeap* m_descriptHeap;
    ID3D12PipelineState* m_pso;
    D3D12_SHADER_BYTECODE m_vertexShader, m_pixelShader,m_HullShader,m_DomainShader;
    D3D12_CULL_MODE m_cullMode;
    bool m_bEnableDepthTest;
    bool m_bNeedUpdatePSO;
    Material(LPCTSTR inShaderFilePath);

    void EnableDepthTest(bool inEnableDepthTest);
    void SetCullMode(D3D12_CULL_MODE inCullMode);

    void SetTexture2D(int inSRVIndex, ID3D12Resource* inResource, int inMipMapLevelCount = 1,
                      DXGI_FORMAT inFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
    void SetTextureCube(int inSRVIndex, ID3D12Resource* inResource, int inMipMapLevelCount = 1,
                     DXGI_FORMAT inFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
    void SetStructuredBuffer(int inSRVIndex, ID3D12Resource* inResource, int inPerElementSize, int inPerElementCount);

    void Active(ID3D12GraphicsCommandList* inCommandList);

    void InitMaterialData();
};
