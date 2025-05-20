#pragma once
#include "BattleFireDirect.h"

class Material
{
public:
    ID3D12Resource* m_constantBuffer = nullptr;
    ID3D12Resource* m_structuredBuffer = nullptr;
    ID3D12DescriptorHeap* m_descriptHeap = nullptr;
    ID3D12PipelineState* m_pso = nullptr;
    Material();
};

