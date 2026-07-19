struct VS_INPUT
{
    float3 position : POSITION;
};

struct VS_OUTPUT
{
    float3 centerW : POSITION;
    float2 size : SIZE;
    nointerpolation uint instanceID : INSTANCEID;
};

struct GS_OUT
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float2 tex : TEXCOORD;
    nointerpolation uint instanceID : INSTANCEID;
};

cbuffer CameraInfo : register(b0)
{
    float4x4 viewMatrix : packoffset(c0);
    float4x4 projectionMatrix : packoffset(c4);
    float3 cameraPos : packoffset(c8);
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

struct InstanceData
{
    float4x4 world_matrix;
    MaterialData material;
    float3 velocity; // 파티클의 이동 속도/방향
    float spawn_time; // 생성 이후 흐른 시간 (Age)
    float life_time; // 수명
};

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);

Texture2D texDiffuse[5] : register(t0);
SamplerState sample : register(s0);

// 상수
const static float3 gravity = float3(0.0f, 1.0f, 0.0f);
const static float PI = 3.1415926535f;

VS_OUTPUT VSMain(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    InstanceData instData = gInstanceData[instanceID];

    float age = instData.spawn_time;
    float lifeRatio = age / instData.life_time; // 0(탄생) ~ 1(소멸)

    // 가속도 계산
    float3 particleOffset = (instData.velocity * age) + (0.5f * gravity * age * age);
    float3 finalLocalPos = input.position + particleOffset;

    float amp = 2.0f;
    float speed = 8.0f;
    float sinInput = (lifeRatio * PI * 3.0f) - (age * speed);
    float flameWave = sin(sinInput) * amp * lifeRatio;
    finalLocalPos.x += flameWave;

    float sizeScale = saturate(1.0f - lifeRatio);
    
    float4 worldPos = mul(float4(finalLocalPos, 1.0f), instData.world_matrix);
    
    output.centerW = worldPos.xyz;
    output.size = float2(instData.world_matrix._11, instData.world_matrix._22) * sizeScale;
    output.instanceID = instanceID;
    
    return output;
}

[maxvertexcount(4)]
void GS(point VS_OUTPUT input[1], uint primID : SV_PrimitiveID, inout TriangleStream<GS_OUT> outStream)
{
    // 카메라를 바라보는 빌보드 좌표 계산
    float3 look = normalize(cameraPos - input[0].centerW);
    float3 right = normalize(cross(float3(0, 1, 0), look));
    float3 up = cross(look, right);

    float halfW = input[0].size.x * 0.5f;
    float halfH = input[0].size.y * 0.5f;
    
    float4 v[4];
    v[0] = float4(input[0].centerW + halfW * right - halfH * up, 1.0f);
    v[1] = float4(input[0].centerW + halfW * right + halfH * up, 1.0f);
    v[2] = float4(input[0].centerW - halfW * right - halfH * up, 1.0f);
    v[3] = float4(input[0].centerW - halfW * right + halfH * up, 1.0f);
    
    float2 uv[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        GS_OUT output;
        output.posW = v[i].xyz;
        output.posH = mul(mul(v[i], viewMatrix), projectionMatrix);
        output.tex = uv[i];
        output.instanceID = input[0].instanceID;
        outStream.Append(output);
    }
    outStream.RestartStrip();
}

float4 PSMain(GS_OUT input) : SV_TARGET
{
    float dist = distance(input.tex, float2(0.5f, 0.5f));
    
    clip(0.5f - dist);


    InstanceData instData = gInstanceData[input.instanceID];
    MaterialData instMat = instData.material;
    
    float age = instData.spawn_time;
    float lifeRatio = age / instData.life_time;

    float4 diffuseAlbedo = texDiffuse[instMat.tex_idx].Sample(sample, input.tex);
    
    float3 fireColor = lerp(float3(2.0f, 0.15f, 0.05f), instMat.albedo.rgb, lifeRatio);
    float4 finalColor;
    finalColor.rgb = fireColor * diffuseAlbedo.rgb;
    
    float3 dynamicEmissive = lerp(float3(3.0f, 0.1f, 0.0f), instMat.emissive_color.rgb, lifeRatio);
    finalColor.rgb += dynamicEmissive * diffuseAlbedo.a;
    
    // 지날 수록 투명하게
    float finalAlpha = diffuseAlbedo.a * (1.0f - lifeRatio) * instMat.albedo.a;
    finalColor.a = finalAlpha;
    
    return finalColor;
}