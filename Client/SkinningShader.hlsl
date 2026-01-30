#define SKINNED

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

cbuffer SkinningInfo : register(b4)
{
    float4x4 gBoneTransforms[100];
};

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
#ifdef SKINNED
    uint4 bone_indices : BLENDINDICES;
    float4 bone_weights : BLENDWEIGHT;
#endif
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

#ifdef SKINNED
    float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    weights[0] = input.bone_weights.x;
    weights[1] = input.bone_weights.y;
    weights[2] = input.bone_weights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    float3 posL = float3(0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
            // Assume no nonuniform scaling when transforming normals, so 
            // that we do not have to use the inverse-transpose.

        posL += weights[i] * mul(float4(input.position, 1.0f), gBoneTransforms[input.bone_indices[i]]).xyz;
        normalL += weights[i] * mul(input.normal, (float3x3) gBoneTransforms[input.bone_indices[i]]);
    }

    input.position = posL;
    input.normal = normalL;
#endif
    float4 posW = mul(float4(input.position, 1.0f), worldMatrix);
    output.position_world = posW.xyz;

    output.normal = mul(input.normal, (float3x3) worldMatrix);
    
    output.position_clip = mul(mul(posW, viewMatrix), projectionMatrix);
    return output;
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