#include "Common.hlsli"

cbuffer CameraInverseInfo : register(b2)
{
    float4x4 gInvViewProj;
};

struct VS_OUT
{
    float4 position_clip : SV_POSITION;
    float2 screen_uv : TEXCOORD0;
};

Texture2D texDiffuse[GBufferNormalIdx + 1] : register(t0);
SamplerState sample : register(s0);

// 반구 탐색을 위한 무작위 샘플 커널 (Z축이 표면 법선 방향인 접선 공간 기준)
static const float3 gKernel[16] =
{
    float3(0.54, 0.18, 0.82), float3(-0.33, 0.71, 0.41),
    float3(0.11, -0.62, 0.77), float3(-0.72, -0.15, 0.48),
    float3(0.82, 0.44, 0.23), float3(-0.55, 0.31, 0.77),
    float3(0.39, -0.88, 0.12), float3(-0.14, -0.43, 0.89),
    float3(0.71, 0.58, 0.38), float3(-0.49, 0.67, 0.56),
    float3(0.28, -0.34, 0.89), float3(-0.83, -0.22, 0.34),
    float3(0.63, 0.12, 0.76), float3(-0.27, 0.91, 0.12),
    float3(0.44, -0.55, 0.71), float3(-0.61, -0.44, 0.65)
};

VS_OUT VSMain(uint vID : SV_VertexID)
{
    VS_OUT output;
    output.screen_uv = float2((vID << 1) & 2, vID & 2);
    output.position_clip = float4(output.screen_uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

// 화면 UV와 투영 깊이(Z) 값을 이용하여 월드 공간의 3차원 좌표를 복원하는 함수
float3 ReconstructWorldPos(float2 uv, float depth)
{
    float2 ndc;
    ndc.x = uv.x * 2.0f - 1.0f;
    ndc.y = (1.0f - uv.y) * 2.0f - 1.0f;

    float4 clipPos = float4(ndc, depth, 1.0f);
    float4 worldPos = mul(clipPos, gInvViewProj);

    return worldPos.xyz / worldPos.w;
}

float4 PSMain(VS_OUT input) : SV_TARGET
{
    int3 texCoord = int3(input.position_clip.xy, 0);

    float centerDepth = texDiffuse[MainDepthIdx].Load(texCoord).r;

    // 현재 픽셀의 월드 공간 좌표 및 월드 공간 법선 벡터를 복원
    float3 centerPosWorld = ReconstructWorldPos(input.screen_uv, centerDepth);
    float3 centerNormalWorld = normalize(texDiffuse[GBufferNormalIdx].Load(texCoord).xyz * 2.0f - 1.0f);

    // 월드 법선 방향을 기준으로 하는 직교 정규 기저(TBN) 행렬을 생성
    float3 up = abs(centerNormalWorld.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 tangent = normalize(cross(up, centerNormalWorld));
    float3 bitangent = cross(centerNormalWorld, tangent);
    
    float3x3 TBN = float3x3(tangent, bitangent, centerNormalWorld);

    float radius = 0.1f; // 주변광을 차폐할 탐색 반경 (미터 단위)
    float occlusion = 0.0f;

    [unroll]
    for (uint i = 0; i < 16; ++i)
    {
        // 커널 샘플을 월드 공간으로 회전
        float3 sampleDirWorld = mul(gKernel[i], TBN);
        float3 sampleWorldPos = centerPosWorld + sampleDirWorld * radius;

        // 샘플 좌표를 다시 화면 공간(Clip Space)으로 투영
        float4 sampleClip = mul(float4(sampleWorldPos, 1.0f), viewMatrix);
        sampleClip = mul(sampleClip, projectionMatrix);
        sampleClip.xyz /= sampleClip.w;

        // clip -> uv
        float2 sampleUV;
        sampleUV.x = sampleClip.x * 0.5f + 0.5f;
        sampleUV.y = -sampleClip.y * 0.5f + 0.5f;

        if (sampleUV.x < 0.0f || sampleUV.x > 1.0f || sampleUV.y < 0.0f || sampleUV.y > 1.0f)
            continue;

        float targetDepth = texDiffuse[MainDepthIdx].SampleLevel(sample, sampleUV, 0).r;
        float3 realWorldPos = ReconstructWorldPos(sampleUV, targetDepth);

        float sampleViewZ = mul(float4(sampleWorldPos, 1.0f), viewMatrix).z;
        float realViewZ = mul(float4(realWorldPos, 1.0f), viewMatrix).z;
        // 카메라 앞쪽 방향의 절댓값 거리를 비교하여, 샘플링한 포인트가 지형 속에 묻혔는지 확인
        if (abs(sampleViewZ) >= abs(realViewZ))
        {
            float dist = distance(realWorldPos, centerPosWorld);
            float rangeCheck = smoothstep(0.0f, 1.0f, radius / dist);

            float3 dir = normalize(realWorldPos - centerPosWorld);
            float occ = saturate(dot(centerNormalWorld, dir));

            occlusion += occ * rangeCheck;
        }
    }

    float ao = 1.0f - (occlusion / 16.0f);
    ao = saturate(ao);
    ao = pow(ao, 3.0f);

    return float4(ao, ao, ao, 1.0f);
}