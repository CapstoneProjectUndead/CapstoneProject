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

cbuffer gameObjectInfo : register(b0)
{
    float4x4 worldMatrix : packoffset(c0);
};

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
    float4 color : COLOR;
    float3 normal : NORMAL;
};

struct VS_OUTPUT
{
    float4 position_clip : SV_POSITION;
    float3 position_world : POSITION;
    float3 normal : NORMAL;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    
    float4 posW = mul(float4(input.position, 1.0f), worldMatrix);
    output.position_world = posW.xyz;

    output.normal = mul(input.normal, (float3x3) worldMatrix);
    
    output.position_clip = mul(mul(posW, viewMatrix), projectionMatrix);
    
    return (output);
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    input.normal = normalize(input.normal);

    float3 toEyeW = normalize(eyePosWorld - input.position_world);

    float4 ambient = ambientLight * albedo;

    Material mat = { albedo, fresnel, glossiness };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, input.position_world, input.normal, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // 분산 재질 알파 사용
    litColor.a = albedo.a;

    return litColor;
}