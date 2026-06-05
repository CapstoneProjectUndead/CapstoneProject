#include "Light.hlsl"
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
    float3 position_world : POSITION0;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
    float3 tangent_world : TANGENT;

    nointerpolation uint instanceID : INSTANCEID;
};

struct InstanceData
{
    float4x4 world_matrix;
    MaterialData material;
};

struct PS_GBUFFER_OUT
{
    float4 color : SV_Target0; // GBufferColorIdx 에 연결됨
    float4 normal : SV_Target1; // GBufferNormalIdx 에 연결됨
};

Texture2D texDiffuse[DiffuseMapCount] : register(t0);
StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
SamplerState sample : register(s0);

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
    output.tangent_world = mul(input.tangent_local, (float3x3) finalWorld);

    return output;
}

PS_GBUFFER_OUT PSMain(VS_OUTPUT input)
{
    PS_GBUFFER_OUT output;

    MaterialData instMat = gInstanceData[input.instanceID].material;
    
    float4 diffuseAlbedo = texDiffuse[instMat.tex_idx].Sample(sample, input.tex) * instMat.albedo;
    output.color = diffuseAlbedo;

    input.normal = normalize(input.normal);
    input.tangent_world = normalize(input.tangent_world);

    float3 bumpedNormalW = input.normal;
    if (instMat.normal_idx != 0xffffffff)
    {
        float4 normalMapSample = texDiffuse[instMat.normal_idx].Sample(sample, input.tex);
        bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, bumpedNormalW, input.tangent_world);
    }
    
    // 노말 벡터(-1.0 ~ 1.0)를 텍스처에 안전하게 저장하기 위해 (0.0 ~ 1.0) 범위로 압축
    output.normal = float4(bumpedNormalW * 0.5f + 0.5f, instMat.glossiness);

    return output;
}