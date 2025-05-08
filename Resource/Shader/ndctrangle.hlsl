struct VertexData{
    float4 position:POSITION;
    float4 texcoord:TEXCOORD0;
    float4 normal:NORMAL;
};

struct VSOut{
    float4 position:SV_POSITION;
    float4 color:TEXCOORD0;
};

VSOut MainVS(VertexData inVertexData){
    VSOut vo;
    vo.position=inVertexData.position;
    vo.color=inVertexData.texcoord;
    return vo;
}

float4 MainPS(VSOut inPSInput):SV_TARGET{
    return inPSInput.color;
}