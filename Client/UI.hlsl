cbuffer CameraInfo : register(b0)
{
    float4x4 gOrthoProj : packoffset(c0); // UI용 정사영 행렬
};

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position_clip : SV_POSITION; // 최종 스크린 좌표
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
    nointerpolation uint instanceID : INSTANCEID;
};

struct InstanceData
{
    float4x4 world_matrix; // UI의 위치, 크기, 피벗이 반영된 행렬
    float4 color; // UI 기본 색상
};

StructuredBuffer<InstanceData> gInstanceData : register(t0);

// UIShader.hlsl
VS_OUTPUT VSMain(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    output.instanceID = instanceID;
    InstanceData instData = gInstanceData[instanceID];
    
    float4 posW = mul(float4(input.position, 1.0f), instData.world_matrix);
    output.position_clip = mul(posW, gOrthoProj);
    
    output.normal = input.normal;
    output.tex = input.tex;
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    float4 finalColor = gInstanceData[input.instanceID].color;
    
    return finalColor;
}