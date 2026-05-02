#include "Common.hlsli"

struct VS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position_clip : SV_POSITION;
    float4 shadow_pos : POSITION0;
    float3 position_world : POSITION1;
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

struct InstanceData
{
    float4x4 world_matrix;
    MaterialData material;
};

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);

VS_OUTPUT VSMain(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    
    // load instData
    output.instanceID = instanceID;
    InstanceData instData = gInstanceData[instanceID];
    float4x4 finalWorld = instData.world_matrix;
    
    float4 posW = mul(float4(input.position, 1.0f), finalWorld);
    output.position_world = posW.xyz;

    output.normal = mul(input.normal, (float3x3) finalWorld);
    
    output.position_clip = mul(mul(posW, viewMatrix), projectionMatrix);

    output.tex = input.tex;
    
    output.shadow_pos = mul(posW, gShadowTransform);

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
    
    // Only the first light casts a shadow
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    shadowFactor[0] = CalcShadowFactor(input.shadow_pos);

    Material mat = { diffuseAlbedo, instMat.fresnel, instMat.glossiness };
    float4 directLight = ComputeLighting(gLights, mat, input.position_world, input.normal, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;
    
    litColor.a = diffuseAlbedo.a;

    return litColor;
}