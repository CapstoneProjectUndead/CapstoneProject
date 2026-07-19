#include "Common.hlsli"
#define SKINNED

struct BONE_INPUT
{
    float3 position : POSITION;
#ifdef SKINNED
    uint4 bone_indices : BLENDINDICES;
    float4 bone_weights : BLENDWEIGHT;
#endif
};

struct VS_SHADOW_OUTPUT
{
#ifdef CUBE_SHADOW
    float4 position_world : TEXCOORD0;
#else
    float4 position_clip : SV_POSITION;
#endif
};

struct GS_SHADOW_OUTPUT
{
    uint layer_index : SV_RenderTargetArrayIndex;
    float4 position_clip : SV_POSITION; // 하드웨어 래스터라이저용
};

struct AnimationData
{
    uint start_offset_A;
    uint cur_frame_A;
    uint start_offset_B;
    uint cur_frame_B;
    uint bone_count;
    int mask_id;
    float blend_weight;
};

struct InstanceData
{
    float4x4 world_matrix;
    MaterialData material;
    AnimationData animation;
};

cbuffer UpdateLightIndex : register(b2)
{
    uint gCurrentLightIndex;
};

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<float4x4> gAnimBuffer : register(t1, space1);

VS_SHADOW_OUTPUT VSMain(BONE_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_SHADOW_OUTPUT output;
    
    InstanceData instData = gInstanceData[instanceID];
#ifdef SKINNED
    bool isAni = !(instData.animation.bone_count == 0);
    if (isAni)
    {
        uint frameBaseA = instData.animation.start_offset_A + (instData.animation.cur_frame_A * instData.animation.bone_count);
        uint frameBaseB = instData.animation.start_offset_B + (instData.animation.cur_frame_B * instData.animation.bone_count);

        float alpha = instData.animation.blend_weight;

        float weights[4];
        weights[0] = input.bone_weights.x;
        weights[1] = input.bone_weights.y;
        weights[2] = input.bone_weights.z;
        weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

        float3 posL = float3(0, 0, 0);

        for (int i = 0; i < 4; ++i)
        {
            uint boneIdx = input.bone_indices[i];

            float4x4 matA = gAnimBuffer[frameBaseA + boneIdx];
            float4x4 matB = gAnimBuffer[frameBaseB + boneIdx];

            float4x4 blendedMatrix = lerp(matA, matB, alpha);

            posL += weights[i] * mul(float4(input.position, 1.0f), blendedMatrix).xyz;
        }
        input.position = posL;
    }
#endif
    float4 posW = mul(float4(input.position, 1.0f), instData.world_matrix);

#ifdef CUBE_SHADOW
    output.position_world = posW;
#else
    output.position_clip = mul(posW, gShadowViewProj);
#endif

    return output;
}

#ifdef CUBE_SHADOW
[maxvertexcount(18)]
void GSMain(triangle VS_SHADOW_OUTPUT input[3], inout TriangleStream<GS_SHADOW_OUTPUT> outStream)
{
    for (int face = 0; face < 6; ++face)
    {
        GS_SHADOW_OUTPUT output;
        output.layer_index = (gCurrentLightIndex * 6) + face;

        for (int v = 0; v < 3; ++v)
        {
            output.position_clip = mul(float4(input[v].position_world.xyz, 1.0f), gCubeShadowTransforms[gCurrentLightIndex][face]);
            outStream.Append(output);
        }
        outStream.RestartStrip();
    }
}

#endif