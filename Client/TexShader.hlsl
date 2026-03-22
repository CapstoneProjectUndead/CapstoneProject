struct VS_INPUT
{
    float3 position : POSITION;
    float2 size : SIZE;
};

struct VS_OUTPUT
{
    float3 position : POSITION;
    float2 size : SIZE;
    nointerpolation uint instanceID : INSTANCEID;
};

struct GS_OUT
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD;
    uint primID : SV_PrimitiveID;
    //float3 normal : NORMAL;
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
    uint tex_idx;
};

struct InstanceData
{
    float4x4 world_matrix; // UI의 위치, 크기, 피벗이 반영된 행렬
    MaterialData material;
};

StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);

Texture2D texDiffuse : register(t0);
SamplerState sample: register(s0);

VS_OUTPUT VSMain(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    output.instanceID = instanceID;
    InstanceData instData = gInstanceData[instanceID];
    
    float4 posW = mul(float4(input.position, 1.0f), instData.world_matrix);
    
    output.position = posW;
    output.size = input.size;
    
    return (output);
}

float4 PSMain(GS_OUT input) : SV_TARGET
{
    //float3 uvw = float3(input.tex, input.primID % 4);
    return texDiffuse.Sample(sample, input.tex);
}

[maxvertexcount(4)]
void GS(point VS_OUTPUT input[1], uint primID : SV_PrimitiveID, inout TriangleStream<GS_OUT> outStream)
{
    // GS 내부에서 좌표 계산 시
    float3 look = normalize(cameraPos - input[0].position);
    float3 right = normalize(cross(float3(0, 1, 0), look));
    float3 up = cross(look, right); // 카메라 기준 위쪽 방향 재계산
    float halfW = input[0].size.x * 0.5f;
    float halfH = input[0].size.y * 0.5f;
    
    float4 vertices[4];
    vertices[0] = float4(input[0].position + halfW * right - halfH * up, 1.0f);
    vertices[1] = float4(input[0].position + halfW * right + halfH * up, 1.0f);
    vertices[2] = float4(input[0].position - halfW * right - halfH * up, 1.0f);
    vertices[3] = float4(input[0].position - halfW * right + halfH * up, 1.0f);
    float2 uv[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };

    GS_OUT output;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        GS_OUT output = (GS_OUT) 0;
        output.posW = vertices[i].xyz;
        output.posH = mul(mul(vertices[i], viewMatrix), projectionMatrix);
        //output.normal = look;
        output.tex = uv[i];
        output.primID = primID;
        outStream.Append(output);
    }
}
