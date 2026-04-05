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
    nointerpolation uint instanceID : INSTANCEID;
};

struct MaterialData
{
    float4 albedo;
    float3 fresnel;
    float glossiness;
    uint tex_idx;
};

struct AnimationData
{
    // 첫 번째 애니메이션 정보
    uint start_offset_A;
    uint cur_frame_A;
    // 두 번째 애니메이션 정보
    uint start_offset_B;
    uint cur_frame_B;
    
    uint bone_count;
    int mask_id;
    float blend_weight; // 0.0이면 A, 1.0이면 B
};

struct InstanceData
{
    float4x4 world_matrix;
    MaterialData material;
    AnimationData animation;
};

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<float4x4> gAnimBuffer : register(t1, space1);

// 본별 마스크 정보를 담는 버퍼 (미리 CPU에서 넘겨줌)
// 예: 0번~10번 본은 0.0(하반신), 11번~20번 본은 1.0(상반신)
StructuredBuffer<float> gBoneMasks : register(t2, space1);

Texture2D texDiffuse[50] : register(t0);
SamplerState sample : register(s0);

VS_OUTPUT VSMain(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;

    // inst Data Load
    InstanceData instData = gInstanceData[instanceID];
    float4x4 finalWorld = instData.world_matrix;
    output.instanceID = instanceID;
#ifdef SKINNED
    // boneIdx = 해당 클립의 시작점 (anim_start_offset) + 현재 프레임까지 건너뛰기 (cur_frame * bone_count) + 그 안에서 내 뼈의 번호 (input.bone_indices[i])
    uint frameBaseA = instData.animation.start_offset_A + (instData.animation.cur_frame_A * instData.animation.bone_count);
    uint frameBaseB = instData.animation.start_offset_B + (instData.animation.cur_frame_B * instData.animation.bone_count);
    float alpha = instData.animation.blend_weight;

    int maskID = instData.animation.mask_id;
    uint boneCount = instData.animation.bone_count;
    
    float weights[4];
    weights[0] = input.bone_weights.x;
    weights[1] = input.bone_weights.y;
    weights[2] = input.bone_weights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    float3 posL = float3(0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < 4; ++i)
    {
        uint boneIdx = input.bone_indices[i];
        
        float maskWeight = 1.0f;
        if (maskID >= 0)
        {
            uint maskIdx = (uint) maskID * boneCount + boneIdx;
            maskWeight = gBoneMasks[maskIdx];
        }
        
        // 최종 가중치 = 레이어 자체의 블렌드 수치 * 본별 마스크 수치
        float finalWeight = alpha * maskWeight;
        
        float4x4 matA = gAnimBuffer[frameBaseA + boneIdx];
        float4x4 matB = gAnimBuffer[frameBaseB + boneIdx];
    
        float4x4 blendedMatrix = lerp(matA, matB, finalWeight);

        posL += weights[i] * mul(float4(input.position, 1.0f), blendedMatrix).xyz;
        normalL += weights[i] * mul(input.normal, (float3x3) blendedMatrix);
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
    MaterialData instMat = gInstanceData[input.instanceID].material;
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