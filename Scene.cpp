#include "Scene.h"
#include <d3d12.h>
#include "BattleFireDirect.h"
#include "Camera.h"
#include "StaticMeshComponent.h"
#include "Utils.h"
#include "stbi/stb_image.h"
#include "Material.h"
#include "SceneNode.h"
#include "RenderTarget.h"

struct LightData
{
    float m_color[4];
    float m_positionAndIntensity[4]; //x,y,z,intensity
};

SceneNode *gSphereNode = nullptr, *gSkyBoxNode = nullptr, *gFSTNode = nullptr, *gPatchNode = nullptr;
ID3D12Resource* gLightDataSB = nullptr;
ID3D12DescriptorHeap* gLightDescriptHeap = nullptr;
DirectX::XMMATRIX gProjectionMatrix;
Camera gMainCamera;
ID3D12Resource* gTextureCube = nullptr;
RenderTarget* gRTLDR = nullptr;

void InitScene(int inCanvasWidth, int inCanvasHeight)
{
    ID3D12GraphicsCommandList* gGraphicsCommandList = GetCommandList();
    ID3D12CommandAllocator* gCommandAllocator = GetCommandAllocator();
    InitRootSignature();

    gRTLDR = new RenderTarget(inCanvasWidth, inCanvasHeight);
    float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    gRTLDR->AttachColorBuffer(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor);
    gRTLDR->AttachDSBuffer();

    const char* image[] = {
        "Res/Image/px.jpg", "Res/Image/nx.jpg", "Res/Image/py.jpg", "Res/Image/ny.jpg", "Res/Image/pz.jpg",
        "Res/Image/nz.jpg"
    };
    gTextureCube = CreateTextureCube(gGraphicsCommandList, image);
    gLightDataSB = CreateCPUGPUBufferObject(65536);

    // gSphereNode = new SceneNode();
    // gSphereNode->m_staticMeshComponent = new StaticMeshComponent();
    // gSphereNode->m_staticMeshComponent->InitFromFile(gGraphicsCommandList, "Res/Model/Sphere.lhsm");
    // Material* material = new Material(L"Res/Shader/phong.hlsl");
    // gSphereNode->m_staticMeshComponent->m_material = material;
    // gProjectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
    //     90.0f / 180.0f * DirectX::XM_PI, 1280.0f / 720.f, 0.1f, 1000.0f);
    // gMainCamera.Update(0.0f, 1.0f, -3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    // material->InitMaterialData();
    // Texture2D* tuxture2D = LoadTexture2DFromFile(gGraphicsCommandList, "Res/Image/earth_d.jpg");
    // Texture2D* normalTexture = LoadTexture2DFromFile(gGraphicsCommandList, "Res/Image/Normal.png");
    // material->SetTexture2D(0, tuxture2D->m_resource, 1, tuxture2D->m_format); //0-15texture
    // material->SetTexture2D(1, normalTexture->m_resource, 1, normalTexture->m_format); //0-15texture
    // material->SetStructuredBuffer(17, gLightDataSB, sizeof(LightData), 1);
    // ///-----------------天空盒---------------------///
    // gSkyBoxNode = new SceneNode();
    // gSkyBoxNode->m_staticMeshComponent = new StaticMeshComponent();
    // gSkyBoxNode->m_staticMeshComponent->InitFromFile(gGraphicsCommandList, "Res/Model/skybox.lhsm");
    // material = new Material(L"Res/Shader/skybox.hlsl");
    // gSkyBoxNode->m_staticMeshComponent->m_material = material;
    // material->InitMaterialData();
    // material->SetTextureCube(0, gTextureCube);
    // material->SetStructuredBuffer(17, gLightDataSB, sizeof(LightData), 1);
    // material->SetCullMode(D3D12_CULL_MODE_FRONT);
    // material->EnableDepthTest(false);
    //
    // ///-----------------全屏三角形---------------------///
    // gFSTNode = new SceneNode();
    // FullScreenTriangle* fst = new FullScreenTriangle();
    // fst->Init(gGraphicsCommandList);
    // gFSTNode->m_staticMeshComponent = fst;
    // material = new Material(L"Res/Shader/fst.hlsl");
    // gFSTNode->m_staticMeshComponent->m_material = material;
    // material->InitMaterialData();
    // material->SetTexture2D(0, gRTLDR->m_colorBuffer, 1, tuxture2D->m_format); //0-15texture
    // material->SetStructuredBuffer(17, gLightDataSB, sizeof(LightData), 1);
    // material->EnableDepthTest(false);
    ///-----------------tesselatioshader---------------------///
    gPatchNode = new SceneNode();
    StaticMeshComponent* patchMesh = new FullScreenTriangle();
    patchMesh->SetVertexCount(3);
    patchMesh->SetVertexPosition(0, -0.8f, -0.8f, 0.5f);
    patchMesh->SetVertexPosition(1, 0.0f, 0.8f, 0.5f);
    patchMesh->SetVertexPosition(2, 0.8f, -0.8f, 0.5f);
    patchMesh->SetPrimitive(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

    patchMesh->m_vbo = CreateBufferObject(gGraphicsCommandList, patchMesh->m_vertexData,
                               patchMesh->m_vertexCount * sizeof(StaticMeshComponentVertexData),
                               D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    patchMesh->m_vboView.BufferLocation = patchMesh->m_vbo->GetGPUVirtualAddress();
    patchMesh->m_vboView.SizeInBytes = sizeof(StaticMeshComponentVertexData) * patchMesh->m_vertexCount;
    patchMesh->m_vboView.StrideInBytes = sizeof(StaticMeshComponentVertexData);
    gPatchNode->m_staticMeshComponent = patchMesh;
    Material*material = new Material(L"Res/Shader/quad_tessellation.hlsl");
    gPatchNode->m_staticMeshComponent->m_material = material;
    material->InitMaterialData();
    material->SetStructuredBuffer(17, gLightDataSB, sizeof(LightData), 1);
    material->EnableDepthTest(false);

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

    /*float color[] = {inDeltaFrameTime, 0.5, 0.5, 1.0};*/
    ID3D12GraphicsCommandList* gGraphicsCommandList = GetCommandList();
    ID3D12CommandAllocator* gCommandAllocator = GetCommandAllocator();
    /*gCommandAllocator->Reset();
    gGraphicsCommandList->Reset(gCommandAllocator, nullptr);*/


    /*RenderTarget* currentRT = gRTLDR->BeginRendering(gGraphicsCommandList);
    gGraphicsCommandList->SetGraphicsRootSignature(GetRootSignature());
    gGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 36, &globalConstants, 0);
    gGraphicsCommandList->SetGraphicsRootShaderResourceView(4, gLightDataSB->GetGPUVirtualAddress());
    gSkyBoxNode->SetPosition(gMainCamera.m_position);
    gSkyBoxNode->Render(gGraphicsCommandList); //over draw
    gSphereNode->Render(gGraphicsCommandList);
    gRTLDR->EndRendering(gGraphicsCommandList);
    WaitForCompletionOfCommandList();*/


    gCommandAllocator->Reset();
    gGraphicsCommandList->Reset(gCommandAllocator, nullptr);
    gGraphicsCommandList->SetGraphicsRootSignature(GetRootSignature());
    gGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 36, &globalConstants, 0);
    gGraphicsCommandList->SetGraphicsRootShaderResourceView(4, gLightDataSB->GetGPUVirtualAddress());
    BeginRenderToSwapChain(gGraphicsCommandList);
    gPatchNode->Render(gGraphicsCommandList);


    /*staticMeshComponent.Render(gGraphicsCommandList);*/
    /*D3D12_VERTEX_BUFFER_VIEW vbos[] = {staticMeshComponent.m_vboView};
    gGraphicsCommandList->IASetVertexBuffers(0, 1, vbos);
    gGraphicsCommandList->DrawInstanced(staticMeshComponent.m_vertexCount, 1, 0, 0);
    */


    //gGraphicsCommandList->DrawInstanced(3, 1, 0, 0);

    /*gFSTNode->Render(gGraphicsCommandList);*/

    EndRenderToSwapChain(gGraphicsCommandList);
    EndCommandList();
}
