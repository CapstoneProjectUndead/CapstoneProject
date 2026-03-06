#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

#include "Light.hlsl"

cbuffer CameraInfo : register(b1)
{
    float4x4 viewMatrix : packoffset(c0);
    float4x4 projectionMatrix : packoffset(c4);
};

cbuffer MaterialInfo : register(b2)
{
    float4 albedo;
    float3 fresnel;
    float glossiness;
};

cbuffer LightInfo : register(b3)
{
    float4 ambientLight;
    float3 eyePosWorld;
    
    Light gLights[MaxLights];
}

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position_clip : SV_POSITION;
    float3 position_world : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct InstanceData
{
    float4x4 world_matrix;
};

StructuredBuffer<InstanceData> gInstanceData : register(t100);

Texture2D texDiffuse : register(t0);
SamplerState sample : register(s0);

VS_OUTPUT VSMain(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    
    float4x4 finalWorld = gInstanceData[instanceID].world_matrix;
    
    float4 posW = mul(float4(input.position, 1.0f), finalWorld);
    output.position_world = posW.xyz;

    output.normal = mul(input.normal, (float3x3) finalWorld);
    
    output.position_clip = mul(mul(posW, viewMatrix), projectionMatrix);

    output.tex = input.tex;

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    // texture
    float4 diffuseAlbedo = texDiffuse.Sample(sample, input.tex) * albedo;

    // light
    input.normal = normalize(input.normal);

    float3 toEyeW = normalize(eyePosWorld - input.position_world);

    float4 ambient = ambientLight * diffuseAlbedo;

    Material mat = { diffuseAlbedo, fresnel, glossiness };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, input.position_world, input.normal, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;
    
    litColor.a = diffuseAlbedo.a;

    return litColor;
}