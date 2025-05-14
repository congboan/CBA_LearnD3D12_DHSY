//
// Created by 77361 on 25-5-7.
//
#include <cstdio>
#include <ctime>
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <DirectXMath.h>
#include "BattleFireDirect.h"
#include "timeapi.h"

#include "StaticMeshComponent.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"winmm.lib")

LPCTSTR gWindowClassName = L"BattleFire";
//@param inWParam 里面会存储案件信息
//@param inLParam createwindowex最后一个参数
LRESULT CALLBACK WindowProc(HWND inHWND, UINT inMSG, WPARAM inWParam, LPARAM inLParam)
{
    switch (inMSG)
    {
    case WM_CREATE:
        break;
    case WM_DESTROY:
        break;
    case WM_CLOSE:
        PostQuitMessage(0); //会向消息队列中添加WM_QUIT
        break;
    }
    return DefWindowProc(inHWND, inMSG, inWParam, inLParam); //默认的系统处理消息
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int inCmdShow)
{
    //注册窗口
    WNDCLASSEX wndClassEx;
    wndClassEx.cbSize = sizeof(WNDCLASSEX);
    wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
    wndClassEx.cbClsExtra = NULL;
    wndClassEx.cbWndExtra = NULL;
    wndClassEx.hInstance = hInstance;
    wndClassEx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClassEx.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClassEx.hbrBackground = NULL;
    wndClassEx.lpszMenuName = NULL;
    wndClassEx.lpszClassName = gWindowClassName;
    wndClassEx.lpfnWndProc = WindowProc;
    if (!RegisterClassEx(&wndClassEx))
    {
        MessageBox(nullptr, L"RegisterClassEx failed!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    //创建窗口
    int viewportWidgth = 1280;
    int viewportHeigth = 720;
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = viewportWidgth;
    rect.bottom = viewportHeigth;
    AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE); //此处会修改rect的值 会包含窗口的边框（比如title  bar）

    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    HWND hwnd = CreateWindowEx(NULL, gWindowClassName, L"My Render Window", WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT,CW_USEDEFAULT, windowWidth, windowHeight, NULL, NULL,
                               hInstance, NULL);
    if (!hwnd)
    {
        MessageBox(nullptr, L"Create Window failed!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    InitD3D12(hwnd, viewportWidgth, viewportHeigth);
    ID3D12GraphicsCommandList* gGraphicsCommandList = GetCommandList();
    ID3D12CommandAllocator* gCommandAllocator = GetCommandAllocator();


    StaticMeshComponent staticMeshComponent;
    staticMeshComponent.InitFromFile(gGraphicsCommandList, "Res/Model/Sphere.lhsm");

    ID3D12RootSignature* rootSignature = InitRootSignature();
    D3D12_SHADER_BYTECODE vs, gs, ps;
    CreateShaderFromFile(L"Res/Shader/gs.hlsl", "MainVS", "vs_5_0", &vs);
    CreateShaderFromFile(L"Res/Shader/gs.hlsl", "MainGS", "gs_5_0", &gs);
    CreateShaderFromFile(L"Res/Shader/gs.hlsl", "MainPS", "ps_5_0", &ps);
    ID3D12PipelineState* pso = CreatePSO(rootSignature, vs, ps, gs);
    ID3D12Resource* cb = CreateConstantBufferObject(65536); //1024*64(4x4矩阵)

    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(
        45.0f / 180.0f * DirectX::XM_PI, 1280.0f / 720.f, 0.1f, 1000.0f);
    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX modelMatrix = DirectX::XMMatrixTranslation(0.f, 0.f, 5.f);
    DirectX::XMFLOAT4X4 tempMatrix;
    modelMatrix *= DirectX::XMMatrixRotationZ(90.f / 180.f * DirectX::XM_PI);
    float matrices[48];

    DirectX::XMStoreFloat4x4(&tempMatrix, projectionMatrix); //xm类型的矩阵是符合cpu编译加速的数据结构，不能直接使用，使用方式是通过XMStoreFloat4x4这种获取
    memcpy(matrices, &tempMatrix, sizeof(float) * 16);

    DirectX::XMStoreFloat4x4(&tempMatrix, viewMatrix); //xm类型的矩阵是符合cpu编译加速的数据结构，不能直接使用，使用方式是通过XMStoreFloat4x4这种获取
    memcpy(matrices + 16, &tempMatrix, sizeof(float) * 16);

    DirectX::XMStoreFloat4x4(&tempMatrix, modelMatrix); //xm类型的矩阵是符合cpu编译加速的数据结构，不能直接使用，使用方式是通过XMStoreFloat4x4这种获取
    memcpy(matrices + 32, &tempMatrix, sizeof(float) * 16);

    DirectX::XMVECTOR determinant;
    DirectX::XMMATRIX inverseModelMatrix = DirectX::XMMatrixInverse(&determinant, modelMatrix);
    if (DirectX::XMVectorGetX(determinant) != 0.0f)
    {
        DirectX::XMMATRIX normalMatrix = DirectX::XMMatrixTranspose(inverseModelMatrix);
        DirectX::XMStoreFloat4x4(&tempMatrix, normalMatrix);
        memcpy(matrices + 48, &tempMatrix, sizeof(float) * 16);
    }

    UpdateConstantBuffer(cb, matrices, sizeof(float) * 64);

    ID3D12Resource* texture = CreateTexture2D(gGraphicsCommandList);
    ID3D12DescriptorHeap* srvHeap = nullptr;
    ID3D12Device* device = GetD3DDevice();
    D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDescSRV{};
    d3dDescriptorHeapDescSRV.NumDescriptors = 1;
    d3dDescriptorHeapDescSRV.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    d3dDescriptorHeapDescSRV.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&d3dDescriptorHeapDescSRV,IID_PPV_ARGS(&srvHeap));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    ID3D12DescriptorHeap* descriptroHeaps[] = {srvHeap};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; //rgba通道的互相映射
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    device->CreateShaderResourceView(texture, &srvDesc, srvHeap->GetCPUDescriptorHandleForHeapStart());
    EndCommandList();

    WaitForCompletionOfCommandList();

    ShowWindow(hwnd, inCmdShow);
    UpdateWindow(hwnd);

    float color[] = {0.5, 0.5, 0.5, 1.0};
    DWORD last_time = timeGetTime();
    DWORD appStartTime = last_time;

    MSG msg;
    while (true)
    {
        ZeroMemory(&msg, sizeof(MSG)); //memset C语言风格
        //peekmessage
        //@param1 过滤窗口 null代表接收所有窗口消息
        //@param2 消息id 0代表不过滤 和后面的参数一个大一个小
        //@param4 从消息队列中拿到后移除消息
        if (PeekMessage(&msg, NULL, 0,
                        0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                break;
            }
            TranslateMessage(&msg); //从系统消息转换为应用程序能识别的消息
            DispatchMessage(&msg); //调用dispatchmessage WindowProc就会被调用
        }
        else
        {
            //render
            WaitForCompletionOfCommandList();
            DWORD current_time = timeGetTime(); //ms
            DWORD frameTime = current_time - last_time;
            DWORD timeSinceAppStartInMS = current_time - appStartTime;
            last_time = current_time;
            float frameTimeInSecond = float(frameTime) / 1000.0f;
            float timeSinceAppStartInSecond = float(timeSinceAppStartInMS) / 1000.0f;
            color[0] = timeSinceAppStartInSecond;

            gCommandAllocator->Reset();
            gGraphicsCommandList->Reset(gCommandAllocator, nullptr);
            BeginRenderToSwapChain(gGraphicsCommandList);
            gGraphicsCommandList->SetPipelineState(pso);
            gGraphicsCommandList->SetGraphicsRootSignature(rootSignature);
            gGraphicsCommandList->SetDescriptorHeaps(_countof(descriptroHeaps), descriptroHeaps);

            gGraphicsCommandList->SetGraphicsRoot32BitConstants(0, 4, color, 0);
            gGraphicsCommandList->SetGraphicsRootConstantBufferView(1, cb->GetGPUVirtualAddress());
            gGraphicsCommandList->SetGraphicsRootDescriptorTable(2, srvHeap->GetGPUDescriptorHandleForHeapStart());
            gGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
            /*staticMeshComponent.Render(gGraphicsCommandList);*/
            D3D12_VERTEX_BUFFER_VIEW vbos[] = {staticMeshComponent.m_vboView};
            gGraphicsCommandList->IASetVertexBuffers(0, 1, vbos);
            gGraphicsCommandList->DrawInstanced(staticMeshComponent.m_vertexCount, 1, 0, 0);


            //gGraphicsCommandList->DrawInstanced(3, 1, 0, 0);

            EndRenderToSwapChain(gGraphicsCommandList);
            EndCommandList();
            SwapD3D12Buffers();
        }
    }
    return 0;
}
