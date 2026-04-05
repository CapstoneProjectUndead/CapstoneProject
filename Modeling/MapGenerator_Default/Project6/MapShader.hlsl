// 1. Camera matrix from C++ (Root Signature b0)
cbuffer CameraBuffer : register(b0)
{
    matrix viewProj;
};

// 2. Input data from C++ (Input Layout)
struct VS_INPUT
{
    float3 pos       : POSITION;       // Base cube vertex
    float3 instPos   : INSTANCE_POS;   // Real position on the map
    float3 instScale : INSTANCE_SCALE; // Box size
    float4 instColor : INSTANCE_COLOR; // Box color
};

// 3. Data passed from Vertex Shader to Pixel Shader
struct PS_INPUT
{
    float4 pos   : SV_POSITION; // Position on screen
    float4 color : COLOR;       // Color on screen
};

// =======================================================
// Vertex Shader (Calculate Positions)
// =======================================================
PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    
    // Calculate real world position: (Base Shape * Scale) + Position
    float3 worldPos = (input.pos * input.instScale) + input.instPos;
    
    // Convert world position to 2D screen position using camera matrix
    output.pos = mul(float4(worldPos, 1.0f), viewProj);
    
    // Pass color directly to pixel shader
    output.color = input.instColor;
    
    return output;
}

// =======================================================
// Pixel Shader (Draw Colors)
// =======================================================
float4 PSMain(PS_INPUT input) : SV_Target
{
    // Draw the color!
    return input.color;
}