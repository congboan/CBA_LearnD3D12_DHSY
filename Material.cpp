#include "Material.h"

Material::Material()
{
    ID3D12Device* device = GetD3DDevice();
    D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDescSRV{};
    d3dDescriptorHeapDescSRV.NumDescriptors = 16;
    d3dDescriptorHeapDescSRV.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    d3dDescriptorHeapDescSRV.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&d3dDescriptorHeapDescSRV, IID_PPV_ARGS(&m_descriptHeap));
}
