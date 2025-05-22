#include "Scene.h"
#include <d3d12.h>
#include "BattleFireDirect.h"
#include "Camera.h"
#include "StaticMeshComponent.h"
#include "Utils.h"
#include "stbi/stb_image.h"
#include "Material.h"
#include "SceneNode.h"

struct LightData
{
    float m_color[4];
    float m_positionAndIntensity[4]; //x,y,z,intensity
};

SceneNode* gSphereNode = nullptr;
ID3D12Resource* gLightDataSB = nullptr;
ID3D12RootSignature* rootSignature = nullptr;
ID3D12DescriptorHeap* gLightDescriptHeap = nullptr;
DirectX::XMMATRIX gProjectionMatrix;

Camera gMainCamera;

void InitScene(int inCanvasWidth, int inCanvasHeight)
{
    ID3D12GraphicsCommandList* gGraphicsCommandList = GetCommandList();
    ID3D12CommandAllocator* gCommandAllocator = GetCommandAllocator();
    gLightDataSB = CreateCPUGPUBufferObject(65536);

    gSphereNode = new SceneNode();
    gSphereNode->m_staticMeshComponent = new StaticMeshComponent();
    gSphereNode->m_staticMeshComponent->InitFromFile(gGraphicsCommandList, "Res/Model/Sphere.lhsm");
    Material* material = new Material();
    gSphereNode->m_staticMeshComponent->m_material = material;
    rootSignature = InitRootSignature();
    D3D12_SHADER_BYTECODE vs, gs, ps;
    CreateShaderFromFile(L"Res/Shader/gs.hlsl", "MainVS", "vs_5_1", &vs);
    CreateShaderFromFile(L"Res/Shader/gs.hlsl", "MainGS", "gs_5_1", &gs);
    CreateShaderFromFile(L"Res/Shader/gs.hlsl", "MainPS", "ps_5_1", &ps);
    material->m_pso = CreatePSO(rootSignature, vs, ps, gs);

    gProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        45.0f / 180.0f * DirectX::XM_PI, 1280.0f / 720.f, 0.1f, 1000.0f);
    gMainCamera.Update(0.0f, 5.0f, -5.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    material->Test();
    Texture2D* tuxture2D = LoadTexture2DFromFile(gGraphicsCommandList, "Res/Image/earth_d.jpg");

    material->SetTexture2D(0, tuxture2D->m_resource, 1, tuxture2D->m_format); //0-15texture
    material->SetStructuredBuffer(17, gLightDataSB, sizeof(LightData), 1);

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
    gGraphicsCommandList->SetGraphicsRootSignature(rootSignature);
    gGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 36, &globalConstants, 0);
    gGraphicsCommandList->SetGraphicsRootShaderResourceView(4, gLightDataSB->GetGPUVirtualAddress());

    /*staticMeshComponent.Render(gGraphicsCommandList);*/
    /*D3D12_VERTEX_BUFFER_VIEW vbos[] = {staticMeshComponent.m_vboView};
    gGraphicsCommandList->IASetVertexBuffers(0, 1, vbos);
    gGraphicsCommandList->DrawInstanced(staticMeshComponent.m_vertexCount, 1, 0, 0);
    */


    //gGraphicsCommandList->DrawInstanced(3, 1, 0, 0);
    gSphereNode->Render(gGraphicsCommandList);
    EndRenderToSwapChain(gGraphicsCommandList);
    EndCommandList();
}
