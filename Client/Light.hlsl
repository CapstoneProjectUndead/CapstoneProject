#define MaxLights 16

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

float3 ComputePointLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
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

float4 ComputeLighting(Light gLights[MaxLights], Material mat, float3 pos, float3 normal, float3 toEye, float3 shadowFactor)
{
    float3 result = 0.0f;
    
    int i = 0;
    
#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i) 
    {
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
}
#endif
#if (NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS+NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif

#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 

    return float4(result, 0.0f);
}