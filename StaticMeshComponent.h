#pragma once
#include <d3d12.h>
#include <string>
#include <unordered_map>

class Material;

struct StaticMeshComponentVertexData
{
    float m_position[4];
    float m_texcoord[4];
    float m_normal[4];
    float m_tangent[4];
};

struct SubMesh
{
    ID3D12Resource* m_ibo;
    D3D12_INDEX_BUFFER_VIEW m_ibView;
    int m_indexCount;
};

class StaticMeshComponent
{
public:
    StaticMeshComponent();
protected:
    ID3D12Resource* m_vbo;
    D3D12_VERTEX_BUFFER_VIEW m_vboView;
    StaticMeshComponentVertexData* m_vertexData;
    bool m_bRenderWithSubMesh;
    
    D3D_PRIMITIVE_TOPOLOGY m_PrimitiveType;
    int m_vertexCount;
    std::unordered_map<std::string, SubMesh*> m_subMeshes;
public:
    void InitFromFile(ID3D12GraphicsCommandList* inCommandList, const char* inFilePath);
    void Render(ID3D12GraphicsCommandList* inCommandList);
    void SetPrimitive(D3D_PRIMITIVE_TOPOLOGY inPrimitiveType);
    void SetIsRenderWithSubMesh(bool inIsRenderWithSubMesh);
    Material* m_material = nullptr;
};
