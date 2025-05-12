#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <DirectXMath.h>

ID3D12Resource* CreateBufferObject(ID3D12GraphicsCommandList* inCommandList, void* inData, int inDataLen,
                                   D3D12_RESOURCE_STATES inFinalResourceState);

