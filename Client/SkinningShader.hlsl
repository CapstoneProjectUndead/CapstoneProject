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

cbuffer CameraInfo : register(b0)
{
    float4x4 viewMatrix : packoffset(c0);
    float4x4 projectionMatrix : packoffset(c4);
};

cbuffer LightInfo : register(b1)
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
    float2 tex : TEXCOORD;
    nointerpolation uint mat_idx : MATINDEX;
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
    uint bone_offset; // 이 인스턴스의 본 행렬들이 시작되는 위치 (Index)
};

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<float4x4> gFinalBoneTransforms : register(t1, space1);

Texture2D texDiffuse[50] : register(t0);
SamplerState sample : register(s0);

VS_OUTPUT VSMain(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;

    // inst Data Load
    InstanceData instData = gInstanceData[instanceID];
    float4x4 finalWorld = instData.world_matrix;
    output.mat_idx = instData.material.tex_idx;
#ifdef SKINNED
    uint offset = instData.bone_offset;
    
    float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    weights[0] = input.bone_weights.x;
    weights[1] = input.bone_weights.y;
    weights[2] = input.bone_weights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    float3 posL = float3(0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        // gFinalBoneTransforms[offset + 본_인덱스] 로 접근!
        uint boneIdx = offset + input.bone_indices[i];
        posL += weights[i] * mul(float4(input.position, 1.0f), gFinalBoneTransforms[boneIdx]).xyz;
        normalL += weights[i] * mul(input.normal, (float3x3) gFinalBoneTransforms[boneIdx]);
    }
    
    input.position = posL;
    input.normal = normalL;
#endif
    float4 posW = mul(float4(input.position, 1.0f), finalWorld);
    output.position_world = posW.xyz;

    output.normal = mul(input.normal, (float3x3) finalWorld);
    
    output.position_clip = mul(mul(posW, viewMatrix), projectionMatrix);
    output.tex = input.tex;
    
    return output;
}


float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    MaterialData instMat = gInstanceData[input.mat_idx].material;
    // texture
    float4 diffuseAlbedo = texDiffuse[instMat.tex_idx].Sample(sample, input.tex) * instMat.albedo;

    // light
    input.normal = normalize(input.normal);

    float3 toEyeW = normalize(eyePosWorld - input.position_world);

    float4 ambient = ambientLight * diffuseAlbedo;

    Material mat = { diffuseAlbedo, instMat.fresnel, instMat.glossiness };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, input.position_world, input.normal, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;
    
    litColor.a = diffuseAlbedo.a;

    return litColor;
}