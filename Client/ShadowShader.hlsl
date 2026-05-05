#include "Common.hlsli"

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position_clip : SV_POSITION;
    float4 shadow_pos : POSITION0;
    float3 position_world : POSITION1;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;

    nointerpolation uint instanceID : INSTANCEID;
};

struct MaterialData
{
    float4 albedo;
    float3 fresnel;
    float glossiness;
    uint tex_idx;
};

struct InstanceData
{
    float4x4 world_matrix;
    MaterialData material;
};

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);

VS_OUTPUT VSMain(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    
    InstanceData instData = gInstanceData[instanceID];
    float4 posW = mul(float4(input.position, 1.0f), instData.world_matrix);
    output.position_clip = mul(posW, gShadowViewProj);

    return output;
}