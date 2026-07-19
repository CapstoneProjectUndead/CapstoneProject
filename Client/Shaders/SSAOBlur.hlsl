#include "Common.hlsli"

cbuffer CB_BlurInfo : register(b2)
{
    // 가로 블러인 경우 float2(1, 0), 세로 블러인 경우 float2(0, 1)
    float2 gBlurDirection;
    float2 gTexelSize;
};

struct VS_OUT
{
    float4 position_clip : SV_POSITION;
    float2 screen_uv : TEXCOORD0;
};

// 0: main depth, 1: ssao original/final, 2: blur temp
Texture2D texDepth : register(t0);
Texture2D texSSAO[2] : register(t1);
SamplerState sample : register(s0);

// 9탭 가우시안 가중치
static const float gWeights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

VS_OUT VSMain(uint vID : SV_VertexID)
{
    VS_OUT output;
    output.screen_uv = float2((vID << 1) & 2, vID & 2);
    output.position_clip = float4(output.screen_uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    int3 texCoord = int3(input.position_clip.xy, 0);

    uint aoTexIdx = gBlurDirection.y;
    float centerAO = texSSAO[aoTexIdx].Load(texCoord).r;
    float centerDepth = texDepth.Load(texCoord).r;

    // 초기 가중치 설정
    float totalAO = centerAO * gWeights[0];
    float totalWeight = gWeights[0];
    float depthSharpness = 200.0f;
    [unroll]
    for (int i = 1; i <= 4; ++i)
    {
        float2 offset = gBlurDirection * gTexelSize * (float) i;

        // + 방향 샘플링 및 가중치 계산
        float2 uvPlus = input.screen_uv + offset;
        float aoPlus = texSSAO[aoTexIdx].SampleLevel(sample, uvPlus, 0).r;
        float depthPlus = texDepth.SampleLevel(sample, uvPlus, 0).r;

        float weightPlus = gWeights[i] * exp(-abs(depthPlus - centerDepth) * depthSharpness);
        totalAO += aoPlus * weightPlus;
        totalWeight += weightPlus;

        // - 방향 샘플링 및 가중치 계산
        float2 uvMinus = input.screen_uv - offset;
        float aoMinus = texSSAO[aoTexIdx].SampleLevel(sample, uvMinus, 0).r;
        float depthMinus = texDepth.SampleLevel(sample, uvMinus, 0).r;

        float weightMinus = gWeights[i] * exp(-abs(depthMinus - centerDepth) * depthSharpness);
        totalAO += aoMinus * weightMinus;
        totalWeight += weightMinus;
    }

    // totalWeight는 최소 gWeights[0] 이상이 보장
    float finalAO = totalAO / totalWeight;

    return float4(finalAO, finalAO, finalAO, 1.0f);
}