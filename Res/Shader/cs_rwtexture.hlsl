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

VSOut MainVS(VertexData inVertexData)
{
    VSOut vo;
    vo.normal = mul(IT_ModelMatrix, inVertexData.normal);
    float3 positionMS = inVertexData.position.xyz + vo.normal.xyz * sin(misc.x);
    float4 positionWS = mul(ModelMatrix, float4(positionMS, 1.f));
    //float4 positionVS = mul(ViewMatrix, positionWS);
    //vo.position = mul(ProjectionMatrix, positionVS);
    
    vo.position = positionWS;
    return vo;
}

[maxvertexcount(4)]
void MainGS(point VSOut inPoint[1], uint inPrimitiveID : SV_PrimitiveID, inout TriangleStream<VSOut> outTriangleStream)
{
    VSOut vo;
    float3 positionWS = inPoint[0].position.xyz;
    float3 N = normalize(inPoint[0].normal.xyz);
    float3 helperVec = abs(N.y) > 0.999 ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
    float3 tangent = normalize(cross(N, helperVec)); //u
    float3 bitangent = normalize(cross(tangent, N)); //v
    
    
    float3 p0WS = positionWS - bitangent * 0.5f - tangent * 0.5f;
    float4 p0VS = mul(ViewMatrix, float4(p0WS, 1.0f));
    vo.position = mul(ProjectionMatrix, p0VS);
    vo.normal = float4(N, 0.0f);
    outTriangleStream.Append(vo);
    
    float3 p1WS = positionWS + bitangent * 0.5f;
    float4 p1VS = mul(ViewMatrix, float4(p1WS, 1.0f));
    vo.position = mul(ProjectionMatrix, p1VS);
    outTriangleStream.Append(vo);
    
    float3 p2WS = positionWS - bitangent * 0.5f + tangent * 0.5f;
    float4 p2VS = mul(ViewMatrix, float4(p2WS, 1.0f));
    vo.position = mul(ProjectionMatrix, p2VS);
    outTriangleStream.Append(vo);
    
  
    

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
   
    float3 surfaceColor = ambientColor;
    return float4(surfaceColor, 1.f);
}