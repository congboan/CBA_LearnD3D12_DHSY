#include "StaticMeshComponent.h"
#include <stdio.h>

#include "BattleFireDirect.h"


StaticMeshComponent::StaticMeshComponent(): m_bRenderWithSubMesh(true),
                                            m_PrimitiveType(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST),
                                            m_vertexCount(0)
{
}

void StaticMeshComponent::InitFromFile(ID3D12GraphicsCommandList* inCommandList, const char* inFilePath)
{
    FILE* pFile = nullptr;
    errno_t err = fopen_s(&pFile, inFilePath, "rb");
    if (err == 0)
    {
        int temp = 0;
        fread(&temp, 4, 1, pFile);
        m_vertexCount = temp;
        m_vertexData = new StaticMeshComponentVertexData[m_vertexCount];
        fread(m_vertexData, 1, sizeof(StaticMeshComponentVertexData) * m_vertexCount, pFile);

        m_vbo = CreateBufferObject(inCommandList, m_vertexData,
                                   m_vertexCount * sizeof(StaticMeshComponentVertexData),
                                   D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        m_vboView.BufferLocation = m_vbo->GetGPUVirtualAddress();
        m_vboView.SizeInBytes = sizeof(StaticMeshComponentVertexData) * m_vertexCount;
        m_vboView.StrideInBytes = sizeof(StaticMeshComponentVertexData);

        while (!feof(pFile))
        {
            fread(&temp, 4, 1, pFile);
            if (feof(pFile))
            {
                break;
            }
            char name[256] = {0};
            fread(name, 1, temp, pFile);
            fread(&temp, 4, 1, pFile);
            SubMesh* subMesh = new SubMesh;
            subMesh->m_indexCount = temp;
            unsigned int* indexes = new unsigned int[temp];
            fread(indexes, 1, sizeof(unsigned int) * temp, pFile);
            subMesh->m_ibo = CreateBufferObject(inCommandList, indexes,
                                                sizeof(unsigned int) * temp,
                                                D3D12_RESOURCE_STATE_INDEX_BUFFER);
            subMesh->m_ibView.BufferLocation = subMesh->m_ibo->GetGPUVirtualAddress();
            subMesh->m_ibView.SizeInBytes = sizeof(unsigned int) * temp;
            subMesh->m_ibView.Format = DXGI_FORMAT_R32_UINT;
            m_subMeshes.insert(std::pair<std::string, SubMesh*>(name, subMesh));
            delete [] indexes;
        }
        fclose(pFile);
    }
}

void StaticMeshComponent::Render(ID3D12GraphicsCommandList* inCommandList)
{
    if (m_vertexCount == 0)
    {
        return;
    }
    inCommandList->IASetPrimitiveTopology(m_PrimitiveType);
    D3D12_VERTEX_BUFFER_VIEW vbos[] = {m_vboView};
    inCommandList->IASetVertexBuffers(0, 1, vbos);
    if (m_bRenderWithSubMesh)
    {
        if (!m_subMeshes.empty())
        {
            for (auto iter = m_subMeshes.begin(); iter != m_subMeshes.end();
                 iter++)
            {
                inCommandList->IASetIndexBuffer(&iter->second->m_ibView);
                inCommandList->DrawIndexedInstanced(iter->second->m_indexCount, 1, 0, 0, 0);
            }
            return;
        }
    }


    inCommandList->DrawInstanced(m_vertexCount, 1, 0, 0);
}

void StaticMeshComponent::SetPrimitive(D3D_PRIMITIVE_TOPOLOGY inPrimitiveType)
{
    m_PrimitiveType = inPrimitiveType;
}

void StaticMeshComponent::SetIsRenderWithSubMesh(bool inIsRenderWithSubMesh)
{
    m_bRenderWithSubMesh = inIsRenderWithSubMesh;
}
