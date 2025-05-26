#pragma once
#include <d3d12.h>
#include <dxgiformat.h>

class RenderTarget
{
public:
    float m_clearColor[4];
    DXGI_FORMAT m_colorRTFormat, m_dsFormat;
    ID3D12Resource *m_colorBuffer, *m_dsBuffer;
    ID3D12DescriptorHeap *m_rtvDescriptorHeap, *m_dsDescriptorHeap;
    int m_width, m_height;
    RenderTarget(int inWidth, int inHeight);
    void AttachColorBuffer(DXGI_FORMAT inFormat, const float* inClearColor);
    void AttachDSBuffer(DXGI_FORMAT inFormat = DXGI_FORMAT_D24_UNORM_S8_UINT);
    RenderTarget* BeginRendering(ID3D12GraphicsCommandList* inCommandList);
    void EndRendering(ID3D12GraphicsCommandList* inCommandList);
};
