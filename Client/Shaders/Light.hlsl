#include "Common.hlsli"

float CalcAttenuation(float distance, float falloffStart, float falloffEnd)
{
    return saturate((falloffEnd - distance) / (falloffEnd - falloffStart));
}

// 슐릭 근사. 프레넬 반사의 근삿값 제공
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncientAngle = saturate(dot(normal, lightVec));
    
    float f0 = 1.0f - cosIncientAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);
    
    return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    const float m = mat.glossiness * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);
    
    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    float3 fresnelFactor = SchlickFresnel(mat.fresnel, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;
    
    // LDR 렌더링 구현
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);
    
    return (mat.albedo.rgb + specAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light light, Material mat, float3 normal, float3 toEye)
{
    float3 lightVec = -light.direction;
    
    // 람베르트 코사인 법칙에 따라 빛의 세기를 줄인다
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.strength * ndotl;
    
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputeToonSpecular(float3 lightVec, float3 normal, float3 toEye, float glossiness)
{
    float3 halfVec = normalize(lightVec + toEye);
    float ndoth = saturate(dot(normal, halfVec));
    float specPower = max(1.0f, glossiness * 256.0f);
    float specRaw = pow(ndoth, specPower);
    float spec = smoothstep(0.5f, 0.55f, specRaw);

    return spec.xxx;
}

// 외각 light
float3 ComputeToonRim(float3 lightVec, float3 normal, float3 toEye, float ndotl)
{
    float rimAmount = 0.88f;
    float rimThreshold = 0.25f;
    
    float rim = 1.0f - saturate(dot(normal, toEye));
    rim *= ndotl;
    rim = smoothstep(rimAmount - rimThreshold, rimAmount + rimThreshold, rim);

    return rim.xxx;
}

float3 ToonBlinnPhong(float3 lightVec, float3 normal, float3 toEye, float3 lightColor, Material mat, float ndotl)
{
    float3 diffuse = mat.albedo.rgb;
    float3 rim = ComputeToonRim(lightVec, normal, toEye, ndotl);

    return (diffuse + rim) * lightColor;
}

float3 ComputeDirToon(Light light, Material mat, float3 normal, float3 toEye, float shadow)
{
    float3 lightVec = normalize(-light.direction);
    float ndotl = saturate(dot(normal, lightVec));  // 0~90: 1 ~ 0, 91~: -
    float toonFactor = smoothstep(0.0f, 0.01f, ndotl);

    float combinedShadow = toonFactor * shadow;
    float finalShadowFactor = lerp(0.2f, 1.0f, combinedShadow);
    // 조명 색상 벡터 생성
    float3 finalLightColor = light.strength * finalShadowFactor;

    return ToonBlinnPhong(lightVec, normal, toEye, finalLightColor, mat, ndotl);
}

float3 ComputePointToon(Light light, Material mat, float3 pos, float3 normal, float3 toEye, float shadow)
{
    float3 lightVec = light.position - pos;
    float distance = length(lightVec);
    
    if (distance > light.falloff_end)
        return float3(0.0, 0.0, 0.0);
    
    lightVec /= distance;
    
    float ndotl = saturate(dot(normal, lightVec));
    
    float att = CalcAttenuation(distance, light.falloff_start, light.falloff_end);
    float toonFactor = smoothstep(0.0f, 0.01f, ndotl);
    float combinedShadow = toonFactor * shadow;
    float finalShadowFactor = lerp(0.2f, 1.0f, combinedShadow);
    
    float3 finalLightColor = light.strength * finalShadowFactor * att;
    
    return ToonBlinnPhong(lightVec, normal, toEye, finalLightColor, mat, ndotl);
}

float3 ComputePointLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 lightVec = light.position - pos;
    
    float distance = length(lightVec);
    
    if(distance > light.falloff_end)
        return float3(0.0, 0.0, 0.0);
    
    lightVec /= distance;
    
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.strength * ndotl;
    
    float att = CalcAttenuation(distance, light.falloff_start, light.falloff_end);
    lightStrength *= att;
    
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float3 ComputeSpotLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 lightVec = light.position - pos;
    
    float distance = length(lightVec);
    
    if(distance > light.falloff_end)
        return 0.0f;
    
    lightVec /= distance;
    
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = light.strength * ndotl;
    
    float att = CalcAttenuation(distance, light.falloff_start, light.falloff_end);
    lightStrength *= att;
    
    float spotFactor = pow(max(dot(-lightVec, light.direction), 0.0f), light.spot_power);
    lightStrength *= spotFactor;
    
    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat, float3 pos, float3 normal, float3 toEye, float shadowFactor)
{
    float3 result = 0.0f;
    uint i = 0;
    
#if (NUM_DIR_LIGHTS > 0)
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif
#if (NUM_POINT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + activeDotNum; ++i)
    {
        result += shadowFactor * ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif
#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += shadowFactor * ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 
    return float4(result, 0.0f);
}