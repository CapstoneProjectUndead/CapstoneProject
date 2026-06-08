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

struct MaterialData
{
    float4 albedo;
    float3 fresnel;
    float glossiness;
    float4 emissive_color;
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

static const uint MainDepthIdx = 0;
static const uint GBufferColorIdx = 1;
static const uint GBufferNormalIdx = 2;
static const uint ShadowMapIdx = 3;
static const uint SkyboxMapIdx = 4;
static const uint AOMapIdx = 5;
static const uint EmissiveMapIdx = 6;

static const uint DiffuseMapCount = 70;

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