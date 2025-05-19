struct VertexData
{
    float4 position : POSITION;
    float4 texcoord : TEXCOORD0;
    float4 normal : NORMAL;
    float4 tangent : TANGENT;
};

struct VSOut
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float4 texcoord : TEXCOORD0;
};

static const float PI = 3.141592;

cbuffer globalConstants : register(b0)
{
    float4 misc;
};

cbuffer DefaultVertexCB : register(b1)
{
    float4x4 ProjectionMatrix;
    float4x4 ViewMatrix;
    float4x4 ModelMatrix;
    float4x4 IT_ModelMatrix;
    float4x4 ReservedMemory[1020];
};

Texture2D T_diffuseTexture : register(t0);
SamplerState samplerState : register(s0);


struct MaterialData
{
    float r;
};

StructuredBuffer<MaterialData> materialData : register(t0, space1);

VSOut MainVS(VertexData inVertexData)
{
    VSOut vo;
    vo.normal = mul(IT_ModelMatrix, inVertexData.normal);
    float3 positionMS = inVertexData.position.xyz + vo.normal.xyz * sin(misc.x);
    float4 positionWS = mul(ModelMatrix, inVertexData.position);
    float4 positionVS = mul(ViewMatrix, positionWS);
    vo.position = mul(ProjectionMatrix, positionVS);
    
    //vo.position = float4(positionWS.xyz + vo.normal.xyz * sin(misc.x)*0.2, 1.0f);
    vo.texcoord = inVertexData.texcoord;
    return vo;
}

[maxvertexcount(4)]
void MainGS(triangle VSOut inPoint[3], uint inPrimitiveID : SV_PrimitiveID, inout TriangleStream<VSOut> outTriangleStream)
{
    float3 N = normalize(inPoint[0].normal.xyz + inPoint[1].normal.xyz + inPoint[2].normal.xyz);
    float scale = materialData[inPrimitiveID].r;
    float3 offset = N * abs(sin(misc.x * 4.0f)) * 0.2f;
    VSOut vo;
    float3 positionWS = inPoint[0].position.xyz + offset;
    float4 positionVS = mul(ViewMatrix, float4(positionWS, 1));
    vo.position = mul(ProjectionMatrix, positionVS);
    vo.normal = inPoint[0].normal;
    vo.texcoord = inPoint[0].texcoord;
    outTriangleStream.Append(vo);
    
    positionWS = inPoint[1].position.xyz + offset;
    positionVS = mul(ViewMatrix, float4(positionWS, 1));
    vo.position = mul(ProjectionMatrix, positionVS);
    vo.normal = inPoint[1].normal;
    vo.texcoord = inPoint[1].texcoord;
    outTriangleStream.Append(vo);
    
    positionWS = inPoint[2].position.xyz + offset;
    positionVS = mul(ViewMatrix, float4(positionWS, 1));
    vo.position = mul(ProjectionMatrix, positionVS);
    vo.normal = inPoint[2].normal;
    vo.texcoord = inPoint[2].texcoord;
    outTriangleStream.Append(vo);
    //float3 positionWS = inPoint[0].position.xyz;
    //float3 N = normalize(inPoint[0].normal.xyz);
    //float3 helperVec = abs(N.y) > 0.99 ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
    //float3 tangent = normalize(cross(N, helperVec));
    //float3 bitangent = normalize(cross(tangent, N));
    
    //float scale = materialData[inPrimitiveID].r;
    
    //VSOut vo;
    //vo.normal = float4(N,0.f);
    
    
    //float3 p0WS = positionWS - (bitangent * 0.5f + tangent * 0.5f)*scale;
    //float4 p0VS = mul(ViewMatrix, float4(p0WS, 1.0f));
    //vo.position = mul(ProjectionMatrix, p0VS);
    //vo.texcoord = float4(0.f, 1.f, 0.f, 0.f);
    //outTriangleStream.Append(vo);
    
    //float3 p1WS = positionWS + (bitangent * 0.5f - tangent * 0.5f) * scale;
    //float4 p1VS = mul(ViewMatrix, float4(p1WS, 1.0f));
    //vo.position = mul(ProjectionMatrix, p1VS);
    //vo.texcoord = float4(1.f, 1.f, 0.f, 0.f);
    //outTriangleStream.Append(vo);
    
    //float3 p2WS = positionWS - (bitangent * 0.5f - tangent * 0.5f) * scale;
    //float4 p2VS = mul(ViewMatrix, float4(p2WS, 1.0f));
    //vo.position = mul(ProjectionMatrix, p2VS);
    //vo.texcoord = float4(0.f, 0.f, 0.f, 0.f);
    //outTriangleStream.Append(vo);
    
    //float3 p3WS = positionWS + (bitangent * 0.5f + tangent * 0.5f) * scale;
    //float4 p3VS = mul(ViewMatrix, float4(p3WS, 1.0f));
    //vo.position = mul(ProjectionMatrix, p3VS);
    //vo.texcoord = float4(1.f, 0.f, 0.f, 0.f);
    //outTriangleStream.Append(vo);
 
}
 
float4 MainPS(VSOut inPSInput) : SV_TARGET
{
    float3 N = normalize(inPSInput.normal.xyz);
    float3 bottomColor = float3(0.1f, 0.4f, 0.6f);
    float3 topColor = float3(0.7f, 0.7f, 0.7f);
    float theta = asin(N.y);//-pi/2~pi/2
    theta /= PI; //-0.5~0.5;
    theta += 0.5f;
    
    float ambientColorIntensity = 1;
    float3 ambientColor = lerp(bottomColor, topColor, theta) * ambientColorIntensity;
    float4 diffusecolor = T_diffuseTexture.Sample(samplerState, inPSInput.texcoord.xy);
    
    float3 surfaceColor = diffusecolor.rgb;
    return float4(surfaceColor, 1);
}