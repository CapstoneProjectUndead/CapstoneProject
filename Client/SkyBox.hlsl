cbuffer CameraInfo : register(b0)
{
    float4x4 viewMatrix : packoffset(c0);
    float4x4 projectionMatrix : packoffset(c4);
};

struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};

SamplerState sample : register(s0);
TextureCube envMap : register(t0, space3);

VertexOut VSMain(VertexIn pin)
{
    VertexOut sout;
    sout.PosL = pin.PosL;
    
    float3 rotatedPos = mul(pin.PosL, (float3x3) viewMatrix);
    sout.PosH = mul(float4(rotatedPos, 1.0f), projectionMatrix);
    // z 값은 항상 멀게 처리
    sout.PosH = sout.PosH.xyww;
    
    return sout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    return envMap.Sample(sample, pin.PosL);
}