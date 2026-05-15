#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
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
    float4x4 gShadowTransform;
    float4x4 gShadowViewProj;
    float4 ambientLight;
    float3 eyePosWorld;
	float pad;
    
    Light gLights[MaxLights];
};

Texture2D texDiffuse[50] : register(t0);

SamplerState sample : register(s0);
SamplerComparisonState gsamShadow : register(s1);

// PCF for shadow mapping
float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;
    float bias = 0.001f;
    depth += bias;

    uint width, height, numMips;
    texDiffuse[49].GetDimensions(0, width, height, numMips);

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
        percentLit += texDiffuse[49].SampleCmpLevelZero(gsamShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}