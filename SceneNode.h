#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
class StaticMeshComponent;

class SceneNode
{
public:
    SceneNode();

    void Update();

    void SetPosition(float inX, float inY, float inZ);

    void Render(ID3D12GraphicsCommandList* inCommandlist);

    bool m_bIsNeedUpdate;
    DirectX::XMVECTOR m_position;
    StaticMeshComponent* m_staticMeshComponent;
};
