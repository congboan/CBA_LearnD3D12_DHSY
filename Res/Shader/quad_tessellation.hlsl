struct VertexData{
    float4 position:POSITION;
    float4 texcoord:TEXCOORD0;
    float4 normal:NORMAL;
    float4 tangent:TANGENT;
};

struct VSOut{
    float4 position:POSITION;
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

VSOut MainVS(VertexData inVertexData){
    VSOut vo;
    vo.position=inVertexData.position;
    return vo;
}
struct PatchTessellationParameter{
    float EdgeTessellation[3]:SV_TessFactor;
    float InteriorTessellation[1]:SV_InsideTessFactor;
};

PatchTessellationParameter TCS(InputPatch<VSOut,3> inPoints,uint inPatchID:SV_PrimitiveID){
    PatchTessellationParameter ptp;
    ptp.EdgeTessellation[0]=1.0;//left
    ptp.EdgeTessellation[1]=1.0;
    ptp.EdgeTessellation[2]=1.0;

    ptp.InteriorTessellation[0]=1.0;
    return ptp;
}

struct HullOut{
    float4 position:POSITION;
};

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("TCS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VSOut,3> inPoints,uint i:SV_OutputControlPointID,uint inPatchID:SV_PrimitiveID){
    HullOut hout;
    hout.position=inPoints[i].position;
    return hout;
}

struct DomainOut{
    float4 position:SV_POSITION;
};

[domain("tri")]
DomainOut DS(PatchTessellationParameter inPatchTessellationParameter,
    float3 uvw:SV_DomainLocation,
    const OutputPatch<HullOut,3> inPoints){
    //0  1
    //2  3
    float3 p=inPoints[0].position.xyz*uvw.x+inPoints[1].position.xyz*uvw.y+inPoints[2].position.xyz*uvw.z;

    DomainOut domainOut;
    domainOut.position=float4(p,1.0f);//clip space
    return domainOut;
}

float4 MainPS(DomainOut inPSInput):SV_TARGET{
    return float4(1.0f,1.0f,1.0f,1.0f);// float4(inPSInput.texcoord.x,inPSInput.texcoord.y,0.0f,1.0f);
}