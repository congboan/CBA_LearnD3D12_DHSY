#include "BattleFireDirect.h"

ID3D12Device* gD3D12Device = nullptr;
ID3D12CommandQueue* gD3D12CommandQueue = nullptr;
IDXGISwapChain3* gSwapChain = nullptr;
ID3D12Resource* gColorRTs[2];
ID3D12Resource* gDSRT = nullptr;
int gCurrentRTIndex = 0;
//RTV和DSV在d3d12中需要手动在GPU上创建内存 就是descriptheap
ID3D12DescriptorHeap* gSwapChainRTVHeap = nullptr;
ID3D12DescriptorHeap* gSwapChainDSVHeap = nullptr;

UINT gRTVDescriptorSize = 0;
UINT gDSVDescriptorSize = 0;
ID3D12CommandAllocator* gCommandAllocator = nullptr;
ID3D12GraphicsCommandList* gGraphicsCommandList = nullptr;
ID3D12Fence* gFence = nullptr;
HANDLE gFenceEvent = nullptr;
UINT64 gFenceValue = 0;

ID3D12Resource* CreateBufferObject(ID3D12GraphicsCommandList* inCommandList, void* inData, int inDataLen,
                                   D3D12_RESOURCE_STATES inFinalResourceState)
{
    D3D12_HEAP_PROPERTIES d3dHeapProperties{};
    d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC d3d12ResourceDesc{};
    d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    d3d12ResourceDesc.Alignment = 0;
    d3d12ResourceDesc.Width = inDataLen;
    d3d12ResourceDesc.Height = 1;
    d3d12ResourceDesc.DepthOrArraySize = 1;
    d3d12ResourceDesc.MipLevels = 1;
    d3d12ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    d3d12ResourceDesc.SampleDesc.Count = 1;
    d3d12ResourceDesc.SampleDesc.Quality = 0;
    d3d12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; //行为主
    d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* bufferObject = nullptr;
    gD3D12Device->CreateCommittedResource(
        &d3dHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &d3d12ResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&bufferObject));
    d3d12ResourceDesc = bufferObject->GetDesc();
    UINT64 memorySizeUsed = 0;
    UINT64 rowSizeInBytes = 0; //一行可以存储多少数据
    UINT rowUsed = 0; //多少航
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT subResourceFootprint{}; //内存布局
    gD3D12Device->GetCopyableFootprints(&d3d12ResourceDesc, 0, 1, 0,
                                        &subResourceFootprint, &rowUsed, &rowSizeInBytes, &memorySizeUsed);

    ID3D12Resource* tempBufferObject = nullptr;
    d3dHeapProperties = {};
    d3dHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    gD3D12Device->CreateCommittedResource(
        &d3dHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &d3d12ResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&tempBufferObject)
    );
    BYTE* pData;
    tempBufferObject->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    BYTE* pDstTempBuffer = reinterpret_cast<BYTE*>(pData + subResourceFootprint.Offset);
    const BYTE* pSrcData = reinterpret_cast<BYTE*>(inData);
    for (UINT i = 0; i < rowUsed; i++)
    {
        //解释：假如我有一个48字节的数据，gpu默认每次分配都是32字节，对于一个buffer来说每一行数据是32，那么48字节的数据就会以2行每行24字节进行存储，
        // subResourceFootprint.Footprint.RowPitch代表gpu每次分配的一行的字节数(32)，rowSizeInBytes则表示一行数据所占的字节数(24)，
        memcpy(pDstTempBuffer + subResourceFootprint.Footprint.RowPitch * i, pSrcData + rowSizeInBytes * i,
               rowSizeInBytes);
    }
    tempBufferObject->Unmap(0, nullptr);
    inCommandList->CopyBufferRegion(bufferObject, 0,
                                    tempBufferObject, 0,
                                    subResourceFootprint.Footprint.Width);
    D3D12_RESOURCE_BARRIER barrier = InitResourceBarrier(bufferObject, D3D12_RESOURCE_STATE_COPY_DEST,
                                                         inFinalResourceState);
    inCommandList->ResourceBarrier(1, &barrier);
    return bufferObject;
}

D3D12_RESOURCE_BARRIER InitResourceBarrier(ID3D12Resource* inResource,
                                           D3D12_RESOURCE_STATES inPrevState,
                                           D3D12_RESOURCE_STATES inNextState
)
{
    D3D12_RESOURCE_BARRIER d3d12ResourceBarrier{};
    memset(&d3d12ResourceBarrier, 0, sizeof(d3d12ResourceBarrier));
    d3d12ResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    d3d12ResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    d3d12ResourceBarrier.Transition.pResource = inResource;
    d3d12ResourceBarrier.Transition.StateBefore = inPrevState;
    d3d12ResourceBarrier.Transition.StateAfter = inNextState;
    d3d12ResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    return d3d12ResourceBarrier;
}


ID3D12RootSignature* InitRootSignature()
{
    D3D12_ROOT_PARAMETER rootParameters[4]; //最多占64个DWORLD
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[0].Constants.RegisterSpace = 0; //有点类似于namespace  
    rootParameters[0].Constants.ShaderRegister = 0; //对应shader中:register(b0)
    rootParameters[0].Constants.Num32BitValues = 4;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[1].Descriptor.RegisterSpace = 0; //descriptor占两个DWORLD
    rootParameters[1].Descriptor.ShaderRegister = 1;

    D3D12_DESCRIPTOR_RANGE descriptorRange[1];
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].RegisterSpace = 0;
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//2个DWORD
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[3].Descriptor.RegisterSpace = 1;
    rootParameters[3].Descriptor.ShaderRegister = 0;//srv

    D3D12_STATIC_SAMPLER_DESC samplerDesc[1];
    memset(samplerDesc, 0, sizeof(D3D12_STATIC_SAMPLER_DESC) * _countof(samplerDesc));
    samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; //超出纹理坐标范围限制纹理坐标
    samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc[0].RegisterSpace = 0;
    samplerDesc[0].ShaderRegister = 0;
    samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = _countof(samplerDesc);
    rootSignatureDesc.pStaticSamplers = samplerDesc;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


    ID3DBlob* sigature;
    HRESULT hResult = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigature, nullptr);
    ID3D12RootSignature* d3d12RootSignature = nullptr;
    gD3D12Device->CreateRootSignature(0, sigature->GetBufferPointer(), sigature->GetBufferSize(),
                                      IID_PPV_ARGS(&d3d12RootSignature));
    return d3d12RootSignature;
}

void CreateShaderFromFile(
    LPCTSTR inShaderFilePath,
    const char* inMainFunctionName,
    const char* inTarget, //vs  vs_5_1 ps  ps_5_1 版本
    D3D12_SHADER_BYTECODE* inShader)
{
    ID3DBlob* shaderBuffer = nullptr; //编译后的shader文件
    ID3DBlob* errorBuffer = nullptr; //编译后的shader文件
    HRESULT hResult = D3DCompileFromFile(inShaderFilePath, nullptr, nullptr, inMainFunctionName, inTarget,
                                         D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &shaderBuffer,
                                         &errorBuffer);
    if (FAILED(hResult))
    {
        char szlog[1024] = {0};
        /*strcpy(szlog,(char*)errorBuffer->GetBufferPointer());*/
        printf("CreateShaderFormFile error:[%s][%s]:[%s]", inMainFunctionName, inTarget,
               (char*)errorBuffer->GetBufferPointer());
        errorBuffer->Release();
        return;
    }
    inShader->pShaderBytecode = shaderBuffer->GetBufferPointer();
    inShader->BytecodeLength = shaderBuffer->GetBufferSize();
}


ID3D12Resource* CreateConstantBufferObject(int inDataLen)
{
    D3D12_HEAP_PROPERTIES d3dHeapProperties{};
    d3dHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; //cpu和gpu都可以访问

    D3D12_RESOURCE_DESC d3d12ResourceDesc{};
    d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    d3d12ResourceDesc.Alignment = 0;
    d3d12ResourceDesc.Width = inDataLen;
    d3d12ResourceDesc.Height = 1;
    d3d12ResourceDesc.DepthOrArraySize = 1;
    d3d12ResourceDesc.MipLevels = 1;
    d3d12ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    d3d12ResourceDesc.SampleDesc.Count = 1;
    d3d12ResourceDesc.SampleDesc.Quality = 0;
    d3d12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; //行为主
    d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ID3D12Resource* bufferObject = nullptr;
    gD3D12Device->CreateCommittedResource(
        &d3dHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &d3d12ResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&bufferObject));
    return bufferObject;
}

void UpdateConstantBuffer(ID3D12Resource* inCB, void* inData, int inDataLen)
{
    D3D12_RANGE d3d12Range = {0};
    unsigned char* pBuffer = nullptr;
    inCB->Map(0, &d3d12Range, (void**)&pBuffer);
    memcpy(pBuffer, inData, inDataLen);
    inCB->Unmap(0, nullptr);
}

ID3D12PipelineState* CreatePSO(ID3D12RootSignature* inD3D12RootSignature,
                               D3D12_SHADER_BYTECODE inVertexShader,
                               D3D12_SHADER_BYTECODE inPixelShader,
                               D3D12_SHADER_BYTECODE inGeometryShader)
{
    D3D12_INPUT_ELEMENT_DESC vertexDataElementDesc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        },
        {
            "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        }
    };
    D3D12_INPUT_LAYOUT_DESC vertexDataLayoutDesc{};
    vertexDataLayoutDesc.NumElements = _countof(vertexDataElementDesc);
    vertexDataLayoutDesc.pInputElementDescs = vertexDataElementDesc;

    D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc{};
    rtBlendDesc.BlendEnable = FALSE;
    rtBlendDesc.LogicOpEnable = FALSE;
    rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    rtBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = inD3D12RootSignature;
    psoDesc.InputLayout = vertexDataLayoutDesc;
    psoDesc.VS = inVertexShader;
    psoDesc.GS = inGeometryShader;
    psoDesc.PS = inPixelShader;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; //实心模式
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FrontCounterClockwise = false;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = false;
    psoDesc.RasterizerState.AntialiasedLineEnable = false;
    psoDesc.RasterizerState.ForcedSampleCount = 0;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    psoDesc.BlendState.AlphaToCoverageEnable = false;

    psoDesc.DepthStencilState.DepthEnable = true;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; //深度可写
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; //绘制时可以让离眼睛更近的物体覆盖掉更远的
    psoDesc.BlendState = {0};


    for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        psoDesc.BlendState.RenderTarget[i] = rtBlendDesc;
        psoDesc.NumRenderTargets = 1;
        ID3D12PipelineState* d3d12PSO = nullptr;
        HRESULT hReuslt = gD3D12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&d3d12PSO));
        if (FAILED(hReuslt))
        {
            return nullptr;
        }
        return d3d12PSO;
    }
}


bool InitD3D12(HWND inHWND, int inWidth, int inHeight)
{
    //dxgifactory->adapter->device->commandQueue->swapchain->buffer->rtv->comandlist->fence
    HRESULT hResult;
    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    {
        ID3D12Debug* debugController = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif
    // ---------------------创建DXGI工厂------------------------
    IDXGIFactory4* dxgiFactory;
    hResult = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hResult))
    {
        return false;
    }
    // ---------------------创建adapter------------------------
    IDXGIAdapter1* adapter;
    int adapterIndex = 0;
    bool adapterFound = false;
    while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) && (desc.DedicatedSystemMemory > 512 * 1024)) //判断显存大小永远判断是否为独立显卡
        {
            continue;
        }
        // ---------------------创建device------------------------
        hResult = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&gD3D12Device));
        if (SUCCEEDED(hResult))
        {
            adapterFound = true;
            break;
        }
        adapterIndex++;
    }
    if (adapterFound == false)
    {
        return false;
    }

    if (FAILED(hResult))
    {
        return false;
    }
    // ---------------------创建commandqueue------------------------
    D3D12_COMMAND_QUEUE_DESC d3d12CommandQueueDesc{};
    hResult = gD3D12Device->CreateCommandQueue(&d3d12CommandQueueDesc, IID_PPV_ARGS(&gD3D12CommandQueue));
    if (FAILED(hResult))
    {
        return false;
    }
    // ---------------------创建swapchain----------------------
    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc = {};
    swapChainDesc.BufferDesc.Width = inWidth;
    swapChainDesc.BufferDesc.Height = inHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.OutputWindow = inHWND;
    swapChainDesc.SampleDesc.Count = 1; //不开启MSAA
    swapChainDesc.Windowed = true; //是一个窗口程序不全屏
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //交换缓冲区时 怎么处理前面这个图片的内容 此处代表丢弃 FLIP支持多缓冲区
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    IDXGISwapChain* swapChain = nullptr;
    dxgiFactory->CreateSwapChain(gD3D12CommandQueue, &swapChainDesc, &swapChain);
    swapChain->QueryInterface(IID_PPV_ARGS(&gSwapChain));


    // ---------------------创建buffer----------------------
    D3D12_HEAP_PROPERTIES d3d12HeapProps{};
    d3d12HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    //d3d12HeapProps.CreationNodeMask和d3d12HeapProps.VisibleNodeMask表示使用那个gpu（主要存在于多显卡交火的情况下）

    D3D12_RESOURCE_DESC d3d12ResourceDesc{};
    d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d3d12ResourceDesc.Alignment = 0;
    d3d12ResourceDesc.Width = inWidth;
    d3d12ResourceDesc.Height = inHeight;
    d3d12ResourceDesc.DepthOrArraySize = 1;
    d3d12ResourceDesc.MipLevels = 1;
    d3d12ResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    d3d12ResourceDesc.SampleDesc.Count = 1;
    d3d12ResourceDesc.SampleDesc.Quality = 0;
    d3d12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; //创建出来之后需要干什么
    d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE dsClearValue{};
    dsClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsClearValue.DepthStencil.Depth = 1.0f;
    dsClearValue.DepthStencil.Stencil = 0;

    //@param3 表示这个resource是可写入的depth
    gD3D12Device->CreateCommittedResource(&d3d12HeapProps, D3D12_HEAP_FLAG_NONE, &d3d12ResourceDesc,
                                          D3D12_RESOURCE_STATE_DEPTH_WRITE, &dsClearValue,IID_PPV_ARGS(&gDSRT));


    // ---------------------创建rtv和dsv----------------------

    D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDescRTV = {};
    d3d12DescriptorHeapDescRTV.NumDescriptors = 2;
    d3d12DescriptorHeapDescRTV.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    gD3D12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDescRTV,IID_PPV_ARGS(&gSwapChainRTVHeap)); //相当于alloc
    gRTVDescriptorSize = gD3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC d3d12DescriptorHeapDescDSV = {};
    d3d12DescriptorHeapDescDSV.NumDescriptors = 1;
    d3d12DescriptorHeapDescDSV.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    gD3D12Device->CreateDescriptorHeap(&d3d12DescriptorHeapDescDSV,IID_PPV_ARGS(&gSwapChainDSVHeap));
    gDSVDescriptorSize = gD3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapStart = gSwapChainRTVHeap->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < 2; i++)
    {
        gSwapChain->GetBuffer(i,IID_PPV_ARGS(&gColorRTs[i]));
        D3D12_CPU_DESCRIPTOR_HANDLE rtvPointer;
        rtvPointer.ptr = rtvHeapStart.ptr + i * gRTVDescriptorSize;
        //此处rtv是系统创建的 所以不需要desc
        gD3D12Device->CreateRenderTargetView(gColorRTs[i], nullptr, rtvPointer);
    }
    D3D12_DEPTH_STENCIL_VIEW_DESC d3d12DSViewDesc{};
    d3d12DSViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    d3d12DSViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    gD3D12Device->CreateDepthStencilView(gDSRT, &d3d12DSViewDesc,
                                         gSwapChainDSVHeap->GetCPUDescriptorHandleForHeapStart());
    // ---------------------创建commandlist----------------------
    //@param0 可以绘图和computeshader
    gD3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&gCommandAllocator));
    gD3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gCommandAllocator, nullptr,
                                    IID_PPV_ARGS(&gGraphicsCommandList));
    // ---------------------创建fence----------------------
    gD3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gFence));
    gFenceEvent = CreateEvent(nullptr,FALSE,FALSE, nullptr);

    return true;
}

ID3D12GraphicsCommandList* GetCommandList()
{
    return gGraphicsCommandList;
}

ID3D12CommandAllocator* GetCommandAllocator()
{
    return gCommandAllocator;
}

void WaitForCompletionOfCommandList()
{
    if (gFence->GetCompletedValue() < gFenceValue)
    {
        gFence->SetEventOnCompletion(gFenceValue, gFenceEvent);
        WaitForSingleObject(gFenceEvent, INFINITE);
    }
}

void EndCommandList()
{
    gGraphicsCommandList->Close();
    ID3D12CommandList* ppCommandLists[] = {gGraphicsCommandList};
    gD3D12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
    gFenceValue += 1;
    gD3D12CommandQueue->Signal(gFence, gFenceValue);
}

void BeginRenderToSwapChain(ID3D12GraphicsCommandList* inCommandList)
{
    gCurrentRTIndex = gSwapChain->GetCurrentBackBufferIndex();
    D3D12_RESOURCE_BARRIER barrier = InitResourceBarrier(gColorRTs[gCurrentRTIndex], D3D12_RESOURCE_STATE_PRESENT,
                                                         D3D12_RESOURCE_STATE_RENDER_TARGET);
    inCommandList->ResourceBarrier(1, &barrier);
    D3D12_CPU_DESCRIPTOR_HANDLE colorRT, dsv;
    colorRT.ptr = gSwapChainRTVHeap->GetCPUDescriptorHandleForHeapStart().ptr + gCurrentRTIndex * gRTVDescriptorSize;
    dsv.ptr = gSwapChainDSVHeap->GetCPUDescriptorHandleForHeapStart().ptr;
    inCommandList->OMSetRenderTargets(1, &colorRT,FALSE, &dsv);
    D3D12_VIEWPORT viewport = {0.f, 0.f, 1280.f, 720.f};
    D3D12_RECT scissorRect = {0, 0, 1280, 720};
    inCommandList->RSSetViewports(1, &viewport);
    inCommandList->RSSetScissorRects(1, &scissorRect);
    const float clearColor[] = {0.f, 0.f, 0.f, 1.0f};
    inCommandList->ClearRenderTargetView(colorRT, clearColor, 0, nullptr);
    inCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0.f, 0.f,
                                         nullptr);
}

void EndRenderToSwapChain(ID3D12GraphicsCommandList* inCommandList)
{
    D3D12_RESOURCE_BARRIER barrier = InitResourceBarrier(gColorRTs[gCurrentRTIndex], D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                         D3D12_RESOURCE_STATE_PRESENT);
    inCommandList->ResourceBarrier(1, &barrier);
}

void SwapD3D12Buffers()
{
    gSwapChain->Present(0, 0);
}

ID3D12Resource* CreateTexture2D(ID3D12GraphicsCommandList* inCommandList, const void* inPixelData, int inDataSizeInByte,
                                int inWidth, int inHeight, DXGI_FORMAT inFormat)
{
    D3D12_HEAP_PROPERTIES d3d12HeapProps{};
    d3d12HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC d3d12ResourceDesc{};
    d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d3d12ResourceDesc.Alignment = 0;
    d3d12ResourceDesc.Width = inWidth;
    d3d12ResourceDesc.Height = inHeight;
    d3d12ResourceDesc.DepthOrArraySize = 1;
    d3d12ResourceDesc.MipLevels = 1;
    d3d12ResourceDesc.Format = inFormat;
    d3d12ResourceDesc.SampleDesc.Count = 1;
    d3d12ResourceDesc.SampleDesc.Quality = 0;
    d3d12ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    ID3D12Resource* texture = nullptr;
    gD3D12Device->CreateCommittedResource(&d3d12HeapProps, D3D12_HEAP_FLAG_NONE, &d3d12ResourceDesc,
                                          D3D12_RESOURCE_STATE_COPY_DEST, nullptr,IID_PPV_ARGS(&texture));


    d3d12ResourceDesc = texture->GetDesc();
    UINT64 memorySizeUsed = 0;
    UINT64 rowSizeInBytes = 0; //一行可以存储多少数据
    UINT rowUsed = 0; //多少航
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT subResourceFootprint{}; //内存布局
    gD3D12Device->GetCopyableFootprints(&d3d12ResourceDesc, 0, 1, 0,
                                        &subResourceFootprint, &rowUsed, &rowSizeInBytes, &memorySizeUsed);

    ID3D12Resource* tempBufferObject = nullptr;

    D3D12_HEAP_PROPERTIES d3d12TempHeapProps = {};
    d3d12TempHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC d3d12TempResourceDesc{};
    d3d12TempResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    d3d12TempResourceDesc.Alignment = 0;
    d3d12TempResourceDesc.Width = memorySizeUsed;
    d3d12TempResourceDesc.Height = 1;
    d3d12TempResourceDesc.DepthOrArraySize = 1;
    d3d12TempResourceDesc.MipLevels = 1;
    d3d12TempResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    d3d12TempResourceDesc.SampleDesc.Count = 1;
    d3d12TempResourceDesc.SampleDesc.Quality = 0;
    d3d12TempResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    d3d12TempResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    

    gD3D12Device->CreateCommittedResource(
        &d3d12TempHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &d3d12TempResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&tempBufferObject)
    );

   

    BYTE* pData;
    tempBufferObject->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    BYTE* pDstTempBuffer = reinterpret_cast<BYTE*>(pData + subResourceFootprint.Offset);
    const BYTE* pSrcData = reinterpret_cast<const BYTE*>(inPixelData); //reinterpret_cast<BYTE*>(inData);
    for (UINT i = 0; i < rowUsed; i++)
    {
        //解释：假如我有一个48字节的数据，gpu默认每次分配都是32字节，对于一个buffer来说每一行数据是32，那么48字节的数据就会以2行每行24字节进行存储，
        // subResourceFootprint.Footprint.RowPitch代表gpu每次分配的一行的字节数(32)，rowSizeInBytes则表示一行数据所占的字节数(24)，
        memcpy(pDstTempBuffer + subResourceFootprint.Footprint.RowPitch * i, pSrcData + rowSizeInBytes * i,
               rowSizeInBytes);
    }
    tempBufferObject->Unmap(0, nullptr);

    D3D12_TEXTURE_COPY_LOCATION dst{};
    dst.pResource = texture;
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = tempBufferObject;
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = subResourceFootprint;

    inCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    D3D12_RESOURCE_BARRIER barrier = InitResourceBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST,
                                                         D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    inCommandList->ResourceBarrier(1, &barrier);
    return texture;
}

ID3D12Device* GetD3DDevice()
{
    return gD3D12Device;
}
