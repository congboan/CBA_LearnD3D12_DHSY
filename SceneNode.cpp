#include "SceneNode.h"
#include "Material.h"
#include "BattleFireDirect.h"
#include "StaticMeshComponent.h"

SceneNode::SceneNode()
{
    m_bIsNeedUpdate = true;
    m_staticMeshComponent = nullptr;
}

void SceneNode::Update()
{
}

void SceneNode::SetPosition(float inX, float inY, float inZ)
{
    m_bIsNeedUpdate = true;
    m_position = DirectX::XMVectorSet(inX, inY, inZ, 1.0f);
}

void SceneNode::Render(ID3D12GraphicsCommandList* inCommandlist)
{
    if (m_bIsNeedUpdate)
    {
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

        UpdateConstantBuffer(m_staticMeshComponent->m_material->m_constantBuffer, matrices, sizeof(float) * 32);
        m_bIsNeedUpdate = false;
    }
    m_staticMeshComponent->Render(inCommandlist);
}
