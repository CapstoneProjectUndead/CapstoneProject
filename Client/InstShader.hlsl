#include "Light.hlsl"

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
    output.tangent_world = mul(input.tangent_local, (float3x3) finalWorld);
    
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
    input.tangent_world = normalize(input.tangent_world);

    float3 bumpedNormalW = input.normal;
    float4 normalMapSample = float4(0.5f, 0.5f, 1.0f, 1.0f);
    if (instMat.normal_idx != 0xffffffff)   // 나중에 수정(오버헤드 큼)
    {
        normalMapSample = texDiffuse[instMat.normal_idx].Sample(sample, input.tex);
        bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, bumpedNormalW, input.tangent_world);
    }
    
    float3 toEyeW = normalize(eyePosWorld - input.position_world);
    float4 ambient = ambientLight * diffuseAlbedo;
    
    const float shininess = instMat.glossiness * normalMapSample.a;
    Material mat = { diffuseAlbedo, instMat.fresnel, shininess };
    float4 directLight = ComputeLighting(gLights, mat, input.position_world, bumpedNormalW, toEyeW, input.shadow_pos);

    float4 litColor = ambient + directLight;
    
	// Add in specular reflections.
    float3 r = reflect(-toEyeW, bumpedNormalW);
    float4 reflectionColor = texDiffuse[SkyboxMapIdx].Sample(sample, r);
    float3 fresnelFactor = SchlickFresnel(instMat.fresnel, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;
    
    litColor.a = diffuseAlbedo.a;
    return litColor;
    
    // debugging
    //float4 shadowPos = input.shadow_pos;
    //shadowPos.xyz /= shadowPos.w;

    //return float4(shadowPos.xyz, 1.0f);
}