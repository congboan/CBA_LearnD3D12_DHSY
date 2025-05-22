#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <DirectXMath.h>

ID3D12Resource* CreateBufferObject(ID3D12GraphicsCommandList* inCommandList, void* inData, int inDataLen,
                                   D3D12_RESOURCE_STATES inFinalResourceState);

D3D12_RESOURCE_BARRIER InitResourceBarrier(ID3D12Resource* inResource,
                                           D3D12_RESOURCE_STATES inPrevState,
                                           D3D12_RESOURCE_STATES inNextState);

ID3D12RootSignature* InitRootSignature();

void CreateShaderFromFile(LPCTSTR inShaderFilePath, const char* inMainFunctionName, const char* inTarget,
                          //vs  vs_5_1 ps  ps_5_1 版本
                          D3D12_SHADER_BYTECODE* inShader);

ID3D12Resource* CreateCPUGPUBufferObject(int inDataLen);

void UpdateConstantBuffer(ID3D12Resource* inCB, void* inData, int inDataLen);

ID3D12PipelineState* CreatePSO(ID3D12RootSignature* inD3D12RootSignature,
                               D3D12_SHADER_BYTECODE inVertexShader,
                               D3D12_SHADER_BYTECODE inPixelShader,
                               D3D12_SHADER_BYTECODE inGeometryShader);

bool InitD3D12(HWND inHWND, int inWidth, int inHeight);

ID3D12GraphicsCommandList* GetCommandList();
ID3D12CommandAllocator* GetCommandAllocator();

void WaitForCompletionOfCommandList();

void EndCommandList();

void BeginRenderToSwapChain(ID3D12GraphicsCommandList* inCommandList);

void EndRenderToSwapChain(ID3D12GraphicsCommandList* inCommandList);

void SwapD3D12Buffers();

ID3D12Resource* CreateTexture2D(ID3D12GraphicsCommandList* inCommandList, const void* inPixelData, int inDataSizeInByte,
                                int inWidth, int inHeight, DXGI_FORMAT inFormat);

ID3D12Device* GetD3DDevice();

struct Texture2D
{
    ID3D12Resource* m_resource;
    DXGI_FORMAT m_format;
};

Texture2D* LoadTexture2DFromFile(ID3D12GraphicsCommandList*inCommandList,const char* inFilePath);
