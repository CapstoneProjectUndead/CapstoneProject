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
    float4x4 world_matrix; // UI의 위치, 크기, 피벗이 반영된 행렬
    MaterialData material;
};

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);

Texture2D texDiffuse[5] : register(t0);
SamplerState sample: register(s0);

VS_OUTPUT VSMain(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    InstanceData instData = gInstanceData[instanceID];

    float4 worldPos = mul(float4(input.position, 1.0f), instData.world_matrix);
    output.centerW = worldPos.xyz;
    
    output.size = float2(instData.world_matrix._11, instData.world_matrix._22);
    output.instanceID = instanceID;
    
    return (output);
}

[maxvertexcount(4)]
void GS(point VS_OUTPUT input[1], uint primID : SV_PrimitiveID, inout TriangleStream<GS_OUT> outStream)
{
    // GS 내부에서 좌표 계산 시
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
    MaterialData instMat = gInstanceData[input.instanceID].material;
    // texture
    float4 diffuseAlbedo = texDiffuse[instMat.tex_idx].Sample(sample, input.tex);

    // 알파 테스팅 (말풍선 등 투명 영역 처리)
    //clip(diffuseAlbedo.a - 0.1f);
    
    return diffuseAlbedo * instMat.albedo;
}