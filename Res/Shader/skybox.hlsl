struct VertexData{
    float4 position:POSITION;
    float4 texcoord:TEXCOORD0;
    float4 normal:NORMAL;
    float4 tangent:TANGENT;
};

struct VSOut{
    float4 position:SV_POSITION;
    float4 texcoord:TEXCOORD0;
};

static const float PI=3.141592;
cbuffer globalConstants:register(b0){
    float4x4 ProjectionMatrix;
    float4x4 ViewMatrix;
    float4 misc;
};

TextureCube T_CubeMap:register(t0);
SamplerState samplerStateLC:register(s0);

struct MaterialData{
    float4 mDiffuseMaterial;
    float4 mSpecularMaterial;
};
struct LightData{
    float4 mColor;
    float4 mPositionAndIntensity;
};
StructuredBuffer<MaterialData> SB_MaterialData:register(t0,space1);//material data
StructuredBuffer<LightData> SB_LightData:register(t1,space1);//light data
cbuffer DefaultVertexCB:register(b1){
    float4x4 ModelMatrix;
    float4x4 IT_ModelMatrix;
    float4x4 ReservedMemory[1020];
};
VSOut MainVS(VertexData inVertexData)
{
    VSOut vo;
    float4 positionWS=mul(ModelMatrix, inVertexData.position);
    float4 positionVS=mul(ViewMatrix,positionWS);
    vo.position=mul(ProjectionMatrix,positionVS);
    vo.texcoord = inVertexData.position;
    return vo;
}
 
float4 MainPS(VSOut inPSInput) : SV_TARGET
{
    float3 texcoord=normalize(inPSInput.texcoord.xyz);

    float4 baseColor=T_CubeMap.Sample(samplerStateLC,texcoord);

    return baseColor;
}