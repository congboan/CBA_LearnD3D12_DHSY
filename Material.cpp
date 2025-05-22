#include "Material.h"
#include "Utils.h"
#define STRUCTURED_BUFFER_INDEX_START 16

Material::Material()
{
    ID3D12Device* device = GetD3DDevice();
    D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDescSRV{};
    d3dDescriptorHeapDescSRV.NumDescriptors = 32;
    d3dDescriptorHeapDescSRV.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    d3dDescriptorHeapDescSRV.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&d3dDescriptorHeapDescSRV, IID_PPV_ARGS(&m_descriptHeap));

    m_constantBuffer = CreateCPUGPUBufferObject(65536); //1024*64(4x4矩阵)
    m_materialDataSB = CreateCPUGPUBufferObject(65536);
}

void Material::SetTexture2D(int inSRVIndex, ID3D12Resource* inResource, int inMipMapLevelCount, DXGI_FORMAT inFormat)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

    srvDesc.Format = inFormat;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; //rgba通道的互相映射
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = inMipMapLevelCount;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    ID3D12Device* device = GetD3DDevice();
    D3D12_CPU_DESCRIPTOR_HANDLE srvHeapPtr = m_descriptHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateShaderResourceView(inResource, &srvDesc, srvHeapPtr);
    srvHeapPtr.ptr += inSRVIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Material::SetStructuredBuffer(int inSRVIndex, ID3D12Resource* inResource, int inPerElementSize,
                                   int inPerElementCount)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC sbSRVDesc{};
    sbSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sbSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    sbSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sbSRVDesc.Buffer.FirstElement = 0;
    sbSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    sbSRVDesc.Buffer.NumElements = inPerElementCount;
    sbSRVDesc.Buffer.StructureByteStride = inPerElementSize;

    ID3D12Device* device = GetD3DDevice();
    D3D12_CPU_DESCRIPTOR_HANDLE srvHeapPtr = m_descriptHeap->GetCPUDescriptorHandleForHeapStart();
    srvHeapPtr.ptr += inSRVIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CreateShaderResourceView(m_materialDataSB, &sbSRVDesc, srvHeapPtr);
}

void Material::Active(ID3D12GraphicsCommandList* inCommandList)
{
    inCommandList->SetPipelineState(m_pso);
    ID3D12DescriptorHeap* descriptroHeaps[] = {m_descriptHeap};
    inCommandList->SetDescriptorHeaps(_countof(descriptroHeaps), descriptroHeaps);
    inCommandList->SetGraphicsRootConstantBufferView(1, m_constantBuffer->GetGPUVirtualAddress());
    inCommandList->SetGraphicsRootDescriptorTable(
        2, m_descriptHeap->GetGPUDescriptorHandleForHeapStart());
    inCommandList->SetGraphicsRootShaderResourceView(3, m_materialDataSB->GetGPUVirtualAddress());
}

void Material::Test()
{
    float* materialDatas = new float[3000];
    for (int i = 0; i < 3000; i++)
    {
        materialDatas[i] = srandom() * 0.5f + 0.5f; //0.0~1.0
    }
    UpdateConstantBuffer(m_materialDataSB, materialDatas, sizeof(float) * 3000);
    SetStructuredBuffer(STRUCTURED_BUFFER_INDEX_START, m_materialDataSB, sizeof(float), 3000);
}
