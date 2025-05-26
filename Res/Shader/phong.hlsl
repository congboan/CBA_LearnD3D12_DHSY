struct VertexData{
    float4 position:POSITION;
    float4 texcoord:TEXCOORD0;
    float4 normal:NORMAL;
    float4 tangent:TANGENT;
    uint instanceID:SV_InstanceID;
};

struct VSOut{
    float4 position:SV_POSITION;
    float4 normal:NORMAL;
    float4 texcoord:TEXCOORD0;
    float4 positionWS:TEXCOORD1;
    float3 tangent:TANGENT;
    float3 binormal:BINORMAL;
};

static const float PI=3.141592;
cbuffer globalConstants:register(b0){
    float4x4 ProjectionMatrix;
    float4x4 ViewMatrix;
    float4 misc;
};

Texture2D T_DiffuseTexture:register(t0);
Texture2D T_NormalTexture:register(t1);
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
    float4 offset=SB_MaterialData[inVertexData.instanceID].mOffset;
    VSOut vo;
    vo.normal = mul(IT_ModelMatrix, inVertexData.normal);
    float4 positionWS=float4(mul(ModelMatrix,inVertexData.position).xyz+offset,1.0f);
    float4 positionVS=mul(ViewMatrix,positionWS);
    vo.position=mul(ProjectionMatrix,positionVS);
    vo.texcoord = inVertexData.texcoord;
    vo.positionWS=positionWS;
    float3 tangetnWS=normalize(mul(float4(inVertexData.tangent.xyz,0.0f),ModelMatrix)).xyz;
    vo.tangent=tangetnWS;
    float3 binormalWS=normalize(cross(vo.normal.xyz,tangetnWS).xyz);
    vo.binormal=binormalWS;
    return vo;
}
 
float4 MainPS(VSOut inPSInput) : SV_TARGET
{
    float3 normalTS=T_NormalTexture.Sample(samplerState,inPSInput.texcoord.xy).xyz;
    normalTS=normalize(normalTS*2.0f-float3(1.0f,1.0f,1.0f));
    float3 N=normalize(inPSInput.normal.xyz);
    float3 T=normalize(inPSInput.tangent.xyz);
    float3 B=normalize(inPSInput.binormal.xyz);
    float3x3 tangentToWorld=float3x3(T,B,N);
    N=normalize(mul(normalTS,tangentToWorld));


    float3 bottomColor = float3(0.1f, 0.4f, 0.6f);
    float3 topColor = float3(0.7f, 0.7f, 0.7f);
    float theta = asin(N.y);//-pi/2~pi/2
    theta /= PI; //-0.5~0.5;
    theta += 0.5f;
    
    float ambientColorIntensity = 0.1;
    float3 ambientColor = lerp(bottomColor, topColor, theta) * ambientColorIntensity;

    float3 L=normalize(float3(1.0f,1.0f,-1.0f));
    float diffuseIntensity=max(0.0f,dot(N,L));
    float3 diffuseColor=diffuseIntensity*float3(1.0f,1.0f,1.0f);

    float4 baseColor=T_DiffuseTexture.Sample(samplerState,inPSInput.texcoord.xy);
    float3 surfaceColor=(ambientColor+diffuseColor) ;//*baseColor.rgb;
    return float4(surfaceColor,1.0f);
}