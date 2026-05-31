#ifndef __COMMON_HLSLI__
#define __COMMON_HLSLI__
#define MaxLights 16

#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 1
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

struct Light
{
    float3 strength;
    float falloff_start;
    float3 direction;
    float falloff_end;
    float3 position;
    float spot_power;
};

struct Material
{
    float4 albedo;
    float3 fresnel;
    float glossiness;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
    float3 tangent_local : TANGENT;
};

struct VS_OUTPUT
{
    float4 position_clip : SV_POSITION;
    float4 shadow_pos : POSITION0;
    float3 position_world : POSITION1;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
    float3 tangent_world : TANGENT;

    nointerpolation uint instanceID : INSTANCEID;
};

struct MaterialData
{
    float4 albedo;
    float3 fresnel;
    float glossiness;
    uint tex_idx;
    uint normal_idx;
};

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
    uint activeDotNum;
    
    float4x4 gCubeShadowTransforms[MaxLights - NUM_DIR_LIGHTS][6];
    Light gLights[MaxLights];
};

Texture2D texDiffuse[70] : register(t0);
TextureCubeArray pointShadowMap : register(t0, space3);

static uint ShadowMapIdx = 69;
static uint SkyboxMapIdx = 68;

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

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
	// Uncompress each component from [0,1] to [-1,1].
    float3 normalT = 2.0f * normalMapSample - 1.0f;

	// Build orthonormal basis.
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);

	// Transform from tangent space to world space.
    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}
#endif // __COMMON_HLSLI__