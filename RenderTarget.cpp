#include "RenderTarget.h"

#include "BattleFireDirect.h"

RenderTarget::RenderTarget(int inWidth, int inHeight)
{
    m_width = inWidth;
    m_height = inHeight;
}

void RenderTarget::AttachColorBuffer(DXGI_FORMAT inFormat, const float* inClearColor)
{
    m_colorRTFormat = inFormat;

    D3D12_HEAP_PROPERTIES d3d12HeapProps{};
    d3d12HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC d3d12ResourceDesc{};
    d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d3d12ResourceDesc.Alignment = 0;
    d3d12ResourceDesc.Width = m_width;
    d3d12ResourceDesc.Height = m_height;
    d3d12ResourceDesc.DepthOrArraySize = 1;
    d3d12ResourceDesc.MipLevels = 1;
    d3d12ResourceDesc.Format = m_colorRTFormat;
    d3d12ResourceDesc.SampleDesc.Count = 1;
    d3d12ResourceDesc.SampleDesc.Quality = 0;
    d3d12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = m_colorRTFormat;
    memcpy(m_clearColor, inClearColor, sizeof(float) * 4);
    memcpy(clearValue.Color, inClearColor, sizeof(float) * 4);
    GetD3DDevice()->CreateCommittedResource(&d3d12HeapProps, D3D12_HEAP_FLAG_NONE, &d3d12ResourceDesc,
                                            D3D12_RESOURCE_STATE_GENERIC_READ, &clearValue,
                                            IID_PPV_ARGS(&m_colorBuffer));

    D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDescRTV = {};
    d3d12DescriptorHeapDescRTV.NumDescriptors = 1;
    d3d12DescriptorHeapDescRTV.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    GetD3DDevice()->CreateDescriptorHeap(&d3d12DescriptorHeapDescRTV,IID_PPV_ARGS(&m_rtvDescriptorHeap)); //相当于alloc

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = m_colorRTFormat;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    GetD3DDevice()->CreateRenderTargetView(m_colorBuffer, &rtvDesc,
                                           m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void RenderTarget::AttachDSBuffer(DXGI_FORMAT inFormat)
{
    ID3D12Device* d3d12Device = GetD3DDevice();
    m_dsFormat = inFormat;
    D3D12_HEAP_PROPERTIES d3d12HeapProps{};
    d3d12HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    //d3d12HeapProps.CreationNodeMask和d3d12HeapProps.VisibleNodeMask表示使用那个gpu（主要存在于多显卡交火的情况下）

    D3D12_RESOURCE_DESC d3d12ResourceDesc{};
    d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d3d12ResourceDesc.Alignment = 0;
    d3d12ResourceDesc.Width = m_width;
    d3d12ResourceDesc.Height = m_height;
    d3d12ResourceDesc.DepthOrArraySize = 1;
    d3d12ResourceDesc.MipLevels = 1;
    d3d12ResourceDesc.Format = m_dsFormat;
    d3d12ResourceDesc.SampleDesc.Count = 1;
    d3d12ResourceDesc.SampleDesc.Quality = 0;
    d3d12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; //创建出来之后需要干什么
    d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE dsClearValue{};
    dsClearValue.Format = m_dsFormat;
    dsClearValue.DepthStencil.Depth = 1.0f;
    dsClearValue.DepthStencil.Stencil = 0;

    //@param3 表示这个resource是可写入的depth
    d3d12Device->CreateCommittedResource(&d3d12HeapProps, D3D12_HEAP_FLAG_NONE, &d3d12ResourceDesc,
                                         D3D12_RESOURCE_STATE_GENERIC_READ, &dsClearValue,IID_PPV_ARGS(&m_dsBuffer));

    D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDescDSV = {};
    d3d12DescriptorHeapDescDSV.NumDescriptors = 1;
    d3d12DescriptorHeapDescDSV.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    d3d12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDescDSV,IID_PPV_ARGS(&m_dsDescriptorHeap));


    D3D12_DEPTH_STENCIL_VIEW_DESC d3d12DSViewDesc{};
    d3d12DSViewDesc.Format = m_dsFormat;
    d3d12DSViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    d3d12Device->CreateDepthStencilView(m_dsBuffer, &d3d12DSViewDesc,
                                        m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

RenderTarget* RenderTarget::BeginRendering(ID3D12GraphicsCommandList* inCommandList)
{
    D3D12_RESOURCE_BARRIER barriers[2] =
    {
        InitResourceBarrier(m_colorBuffer, D3D12_RESOURCE_STATE_GENERIC_READ,
                            D3D12_RESOURCE_STATE_RENDER_TARGET),
        InitResourceBarrier(m_dsBuffer, D3D12_RESOURCE_STATE_GENERIC_READ,
                            D3D12_RESOURCE_STATE_DEPTH_WRITE)

    };

    D3D12_CPU_DESCRIPTOR_HANDLE colorRT = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    inCommandList->OMSetRenderTargets(1, &colorRT,FALSE, &dsv);
    D3D12_VIEWPORT viewport = {0.f, 0.f, float(m_width), float(m_height)};
    D3D12_RECT scissorRect = {0, 0, m_width, m_height};
    inCommandList->RSSetViewports(1, &viewport);
    inCommandList->RSSetScissorRects(1, &scissorRect);
    inCommandList->ClearRenderTargetView(colorRT, m_clearColor, 0, nullptr);
    inCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0.f, 0.f,
                                         nullptr);
    inCommandList->ResourceBarrier(1, barriers);
    return this;
}

void RenderTarget::EndRendering(ID3D12GraphicsCommandList* inCommandList)
{
    D3D12_RESOURCE_BARRIER barriers[2] =
    {
        InitResourceBarrier(m_colorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET,
                            D3D12_RESOURCE_STATE_GENERIC_READ),
        InitResourceBarrier(m_dsBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                            D3D12_RESOURCE_STATE_GENERIC_READ)

    };
    inCommandList->ResourceBarrier(1, barriers);
}
