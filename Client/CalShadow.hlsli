#include "Common.hlsli"

Texture2D texDiffuse[AOMapIdx + 1] : register(t0);
TextureCubeArray pointShadowMap : register(t0, space2);

SamplerState sample : register(s0);
SamplerComparisonState gsamShadow : register(s1);

// PCF for shadow mapping
float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    texDiffuse[ShadowMapIdx].GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float) width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += texDiffuse[ShadowMapIdx].SampleCmpLevelZero(gsamShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}

float CalcPointShadowFactor(int cubeIndex, float3 fragPosWorld, float3 lightPosWorld)
{
    float3 fragToLight = fragPosWorld - lightPosWorld;

    // pixel 좌표 -> cubeMap view 좌표
    float n = gLights[cubeIndex + NUM_DIR_LIGHTS].falloff_start;
    float f = gLights[cubeIndex + NUM_DIR_LIGHTS].falloff_end;
    
    // 큐브맵의 각 면은 원근 투영이므로, 방향 벡터 중 가장 축방향 성분이 큰 절대값이 뷰 공간의 Z축 거리
    float maxAbsAxis = max(max(abs(fragToLight.x), abs(fragToLight.y)), abs(fragToLight.z));
    // NDC 깊이(0~1)로 변환
    float currentDepth = (f / (f - n)) - ((f * n) / (f - n)) / maxAbsAxis;

    uint width, height, numMips, element;
    pointShadowMap.GetDimensions(0, width, height, element, numMips);
    float dx = 1.0f / (float) width;

    const float3 sampleOffsetDirections[20] =
    {
        float3(1, 1, 1), float3(1, -1, 1), float3(-1, -1, 1), float3(-1, 1, 1),
        float3(1, 1, -1), float3(1, -1, -1), float3(-1, -1, -1), float3(-1, 1, -1),
        float3(1, 1, 0), float3(1, -1, 0), float3(-1, -1, 0), float3(-1, 1, 0),
        float3(1, 0, 1), float3(-1, 0, 1), float3(1, 0, -1), float3(-1, 0, -1),
        float3(0, 1, 1), float3(0, -1, 1), float3(0, -1, -1), float3(0, 1, -1)
    };

    float sumLit = 0.0f;
    
    [unroll]
    for (int i = 0; i < 20; ++i)
    {
        float3 offsetDir = fragToLight + sampleOffsetDirections[i] * dx * 2.0f; // 오프셋 반경 조절
        sumLit += pointShadowMap.SampleCmpLevelZero(gsamShadow, float4(offsetDir, cubeIndex), currentDepth);
    }

    return sumLit / 20.0f;
}
