#include "Scene.h"
#include <d3d12.h>
#include "BattleFireDirect.h"
#include "Camera.h"
#include "StaticMeshComponent.h"
#include "Utils.h"
#include "stbi/stb_image.h"
#include "Material.h"

StaticMeshComponent staticMeshComponent;
ID3D12RootSignature* rootSignature = nullptr;

Material* gMaterial = nullptr;
DirectX::XMMATRIX gProjectionMatrix;

Camera gMainCamera;

void InitScene(int inCanvasWidth, int inCanvasHeight)
{
    gMaterial = new Material();
    ID3D12GraphicsCommandList* gGraphicsCommandList = GetCommandList();
    ID3D12CommandAllocator* gCommandAllocator = GetCommandAllocator();
    staticMeshComponent.InitFromFile(gGraphicsCommandList, "Res/Model/Sphere.lhsm");

    rootSignature = InitRootSignature();
    D3D12_SHADER_BYTECODE vs, gs, ps;
    CreateShaderFromFile(L"Res/Shader/gs.hlsl", "MainVS", "vs_5_1", &vs);
    CreateShaderFromFile(L"Res/Shader/gs.hlsl", "MainGS", "gs_5_1", &gs);
    CreateShaderFromFile(L"Res/Shader/gs.hlsl", "MainPS", "ps_5_1", &ps);
    gMaterial->m_pso = CreatePSO(rootSignature, vs, ps, gs);
    gMaterial->m_constantBuffer = CreateConstantBufferObject(65536); //1024*64(4x4矩阵)

    gProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        45.0f / 180.0f * DirectX::XM_PI, 1280.0f / 720.f, 0.1f, 1000.0f);
    gMainCamera.Update(0.0f, 5.0f, -5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixTranslation(0.f, 0.f, 0.f);
    DirectX::XMFLOAT4X4 tempMatrix;
    //modelMatrix *= DirectX::XMMatrixRotationZ(90.f / 180.f * DirectX::XM_PI);
    float matrices[32];


    DirectX::XMStoreFloat4x4(&tempMatrix, modelMatrix); //xm类型的矩阵是符合cpu编译加速的数据结构，不能直接使用，使用方式是通过XMStoreFloat4x4这种获取
    memcpy(matrices, &tempMatrix, sizeof(float) * 16);

    DirectX::XMVECTOR determinant;
    DirectX::XMMATRIX inverseModelMatrix = DirectX::XMMatrixInverse(&determinant, modelMatrix);
    if (DirectX::XMVectorGetX(determinant) != 0.0f)
    {
        DirectX::XMMATRIX normalMatrix = DirectX::XMMatrixTranspose(inverseModelMatrix);
        DirectX::XMStoreFloat4x4(&tempMatrix, normalMatrix);
        memcpy(matrices + 16, &tempMatrix, sizeof(float) * 16);
    }

    UpdateConstantBuffer(gMaterial->m_constantBuffer, matrices, sizeof(float) * 32);

    gMaterial->m_structuredBuffer = CreateConstantBufferObject(65536);
    struct MaterialData
    {
        float r;
    };
    MaterialData* materialDatas = new MaterialData[3000];
    for (int i = 0; i < 3000; i++)
    {
        materialDatas[i].r = srandom() * 0.5f + 0.5f; //0.0~1.0
    }
    UpdateConstantBuffer(gMaterial->m_structuredBuffer, materialDatas, sizeof(MaterialData) * 3000);

    unsigned char* particlePixels = new unsigned char[256 * 256 * 4];
    memset(particlePixels, 0, 256 * 256 * 4);

    for (int y = 0; y < 256; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            float radiusSqrt = float((x - 128) * (x - 128) + (y - 128) * (y - 128));
            if (radiusSqrt <= 128 * 128)
            {
                float radius = sqrtf(radiusSqrt);
                float alpha = radius / 128.0f;
                alpha = alpha > 1.0f ? 1.0f : alpha;
                alpha = 1 - alpha;
                alpha = powf(alpha, 2.0f);
                int pixelIndex = y * 256 + x;
                particlePixels[pixelIndex * 4] = 255;
                particlePixels[pixelIndex * 4 + 1] = 255;
                particlePixels[pixelIndex * 4 + 2] = 255;
                particlePixels[pixelIndex * 4 + 3] = unsigned char(255.0f * alpha);
            }
        }
    }
    ID3D12Resource* textureParticle = CreateTexture2D(gGraphicsCommandList, particlePixels, 256 * 256 * 4,
                                                      256, 256,
                                                      DXGI_FORMAT_R8G8B8A8_UNORM);
    delete[]particlePixels;

    int inmageWidth, imageHeight, imageChannel;
    stbi_uc* pixels = stbi_load("Res/Image/earth_d.jpg", &inmageWidth, &imageHeight, &imageChannel, 4);
    ID3D12Resource* texture = CreateTexture2D(gGraphicsCommandList, pixels, inmageWidth * imageHeight * imageChannel,
                                              inmageWidth, imageHeight,
                                              DXGI_FORMAT_R8G8B8A8_UNORM);
    delete[]pixels;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; //rgba通道的互相映射
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    
    ID3D12Device* device = GetD3DDevice();
    D3D12_CPU_DESCRIPTOR_HANDLE srvHeapPtr = gMaterial->m_descriptHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateShaderResourceView(texture, &srvDesc, srvHeapPtr);
    srvHeapPtr.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device->CreateShaderResourceView(textureParticle, &srvDesc, srvHeapPtr);
    srvHeapPtr.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_SHADER_RESOURCE_VIEW_DESC sbSRVDesc{};
    sbSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sbSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    sbSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    sbSRVDesc.Buffer.FirstElement = 0;
    sbSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    sbSRVDesc.Buffer.NumElements = 3000;
    sbSRVDesc.Buffer.StructureByteStride = sizeof(MaterialData);
    device->CreateShaderResourceView(gMaterial->m_structuredBuffer, &sbSRVDesc, srvHeapPtr);

    EndCommandList();

    WaitForCompletionOfCommandList();
}

void RenderOneFrame(float inDeltaFrameTime, float inTimeSinceAppStart)
{
    GlobalConstants globalConstants;

    DirectX::XMFLOAT4X4 tempMatrix;
    globalConstants.mMisc[0] = inTimeSinceAppStart;
    DirectX::XMStoreFloat4x4(&tempMatrix, gProjectionMatrix); //xm类型的矩阵是符合cpu编译加速的数据结构，不能直接使用，使用方式是通过XMStoreFloat4x4这种获取
    memcpy(globalConstants.mProjectionMatrix, &tempMatrix, sizeof(float) * 16);

    DirectX::XMStoreFloat4x4(&tempMatrix, gMainCamera.m_viewMatrix);
    //xm类型的矩阵是符合cpu编译加速的数据结构，不能直接使用，使用方式是通过XMStoreFloat4x4这种获取
    memcpy(globalConstants.mViewMatrix, &tempMatrix, sizeof(float) * 16);

    float color[] = {inDeltaFrameTime, 0.5, 0.5, 1.0};
    ID3D12GraphicsCommandList* gGraphicsCommandList = GetCommandList();
    ID3D12CommandAllocator* gCommandAllocator = GetCommandAllocator();
    gCommandAllocator->Reset();
    gGraphicsCommandList->Reset(gCommandAllocator, nullptr);
    BeginRenderToSwapChain(gGraphicsCommandList);
    gGraphicsCommandList->SetPipelineState(gMaterial->m_pso);
    gGraphicsCommandList->SetGraphicsRootSignature(rootSignature);
    ID3D12DescriptorHeap* descriptroHeaps[] = {gMaterial->m_descriptHeap};
    gGraphicsCommandList->SetDescriptorHeaps(_countof(descriptroHeaps), descriptroHeaps);

    gGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 36, &globalConstants, 0);
    gGraphicsCommandList->SetGraphicsRootConstantBufferView(1, gMaterial->m_constantBuffer->GetGPUVirtualAddress());
    gGraphicsCommandList->SetGraphicsRootDescriptorTable(
        2, gMaterial->m_descriptHeap->GetGPUDescriptorHandleForHeapStart());
    gGraphicsCommandList->SetGraphicsRootShaderResourceView(3, gMaterial->m_structuredBuffer->GetGPUVirtualAddress());
    /*staticMeshComponent.Render(gGraphicsCommandList);*/
    /*D3D12_VERTEX_BUFFER_VIEW vbos[] = {staticMeshComponent.m_vboView};
    gGraphicsCommandList->IASetVertexBuffers(0, 1, vbos);
    gGraphicsCommandList->DrawInstanced(staticMeshComponent.m_vertexCount, 1, 0, 0);
    */


    //gGraphicsCommandList->DrawInstanced(3, 1, 0, 0);
    staticMeshComponent.Render(gGraphicsCommandList);
    EndRenderToSwapChain(gGraphicsCommandList);
    EndCommandList();
}
