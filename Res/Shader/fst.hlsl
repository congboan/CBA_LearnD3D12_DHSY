struct VertexData{
    float4 position:POSITION;
    float4 texcoord:TEXCOORD0;
};

struct VSOut{
    float4 position:SV_POSITION;
    float4 normal:NORMAL;
    float4 texcoord:TEXCOORD0;
    float4 positionWS:TEXCOORD1;
};

static const float PI=3.141592;
cbuffer globalConstants:register(b0){
    float4x4 ProjectionMatrix;
    float4x4 ViewMatrix;
    float4 misc;
};

Texture2D T_DiffuseTexture:register(t0);
SamplerState samplerState:register(s0);

struct MaterialData{
    float4 mOffset;
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
    vo.position=inVertexData.position;
    vo.texcoord = inVertexData.texcoord;
    return vo;
}
 
float4 MainPS(VSOut inPSInput) : SV_TARGET
{
    return T_DiffuseTexture.Sample(samplerState, inPSInput.texcoord.xy);
}