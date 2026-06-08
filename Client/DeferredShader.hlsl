#include "Light.hlsl"
#include "CalShadow.hlsli"

cbuffer CameraInverseInfo : register(b2)
{
    float4x4 gInvViewProj;
};

struct VS_OUT
{
    float4 position_clip : SV_POSITION;
    float2 screen_uv : TEXCOORD0;
};

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

    float4 albedo = texDiffuse[GBufferColorIdx].Load(texCoord);
    float4 packedNormal = texDiffuse[GBufferNormalIdx].Load(texCoord);
    float depth = texDiffuse[MainDepthIdx].Load(texCoord).r;
    float4 emissiveColor = texDiffuse[EmissiveMapIdx].Load(texCoord);
    
    float3 worldNormal = normalize(packedNormal.xyz * 2.0f - 1.0f);
    float glossiness = packedNormal.w;

    float ndcX = input.screen_uv.x * 2.0f - 1.0f;
    float ndcY = (1.0f - input.screen_uv.y) * 2.0f - 1.0f;

    float4 clipPos = float4(ndcX, ndcY, depth, 1.0f);
    float4 worldPosW = mul(clipPos, gInvViewProj);
    float3 fragPosWorld = worldPosW.xyz / worldPosW.w;

    float3 toEyeW = normalize(eyePosWorld - fragPosWorld);

    Material mat = { albedo, float3(0.05f, 0.05f, 0.05f), glossiness };
    
    float ao = texDiffuse[AOMapIdx].Load(texCoord).r;
    
    // 툰 스타일 전구(Emissive) 라이팅 계산
    float3 finalEmissive = float3(0.0f, 0.0f, 0.0f);
    float ndotv = saturate(dot(worldNormal, toEyeW));
    float toonEmissiveGlow = smoothstep(0.3f, 0.7f, ndotv); // 외곽선 쪽은 어두워지게 컷오프
    // 중심부는 원본 에미시브 색상보다 더 밝게(전구 필라멘트 느낌), 외곽은 살짝 알베도가 묻어나게 믹스
    finalEmissive = lerp(emissiveColor.rgb * 0.5f, emissiveColor.rgb * 1.8f, toonEmissiveGlow);
    float3 selfSpecular = ComputeToonSpecular(toEyeW, worldNormal, toEyeW, glossiness);
    finalEmissive += selfSpecular * emissiveColor.rgb * 1.2f;

    // 기존 앰비언트 계산에서 emissiveColor를 빼고 finalEmissive를 나중에 더해줍니다.
    float4 ambient = ambientLight * albedo * ao;
    float3 directLighting = 0.0f;
#if (NUM_DIR_LIGHTS > 0)
    float4 shadowPosH = mul(float4(fragPosWorld, 1.0f), gShadowTransform);
    float dirShadow = CalcShadowFactor(shadowPosH);
    directLighting += ComputeDirToon(gLights[0], mat, worldNormal, toEyeW) * dirShadow;
#endif
#if (NUM_POINT_LIGHTS > 0)
    [loop]
    for (uint i = 0; i < activeDotNum; ++i)
    {
        float pointShadow = CalcPointShadowFactor(i, fragPosWorld, gLights[i + NUM_DIR_LIGHTS].position);
        directLighting += ComputePointToon(gLights[i + NUM_DIR_LIGHTS], mat, fragPosWorld, worldNormal, toEyeW) * pointShadow;
    }
#endif
#if (NUM_SPOT_LIGHTS > 0)
    [loop]
    for (uint i = NUM_DIR_LIGHTS + activeDotNum; i < NUM_DIR_LIGHTS + activeDotNum + NUM_SPOT_LIGHTS; ++i)
    {
        directLighting += ComputeSpotLight(gLights[i], mat, fragPosWorld, worldNormal, toEyeW);
    }
#endif
    float4 litColor = ambient + float4(directLighting, 0.0f) + float4(finalEmissive, 0.0f);
    
    float3 r = reflect(-toEyeW, worldNormal);
    float4 reflectionColor = texDiffuse[SkyboxMapIdx].Sample(sample, r.xy);
    litColor.rgb += glossiness * reflectionColor.rgb;
    litColor.a = albedo.a;
    
    return litColor;
}