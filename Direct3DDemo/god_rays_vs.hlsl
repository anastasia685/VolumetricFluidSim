cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};
cbuffer LightBuffer : register(b1)
{
    float3 ambientColor;
    float ambientIntensity;
    float3 sunColor;
    float sunIntensity;
    float3 sunDirection;
};
cbuffer CameraBuffer : register(b2)
{
    float3 cameraPos;
};
cbuffer MatrixBuffer : register(b3)
{
    matrix _sceneWorldMatrix;
    matrix sceneViewMatrix;
    matrix sceneProjectionMatrix;
};

struct InputType
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
    float2 sunPosition : TEXCOORD1;
};

OutputType main(InputType input)
{
    OutputType output;
    
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    output.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
    output.tex = input.tex;
    
    float3 sunWorldPos = cameraPos - normalize(sunDirection);
    float4 sunViewPos = mul(float4(sunWorldPos, 1.0f), sceneViewMatrix);
    float4 sunClipPos = mul(sunViewPos, sceneProjectionMatrix);

    float2 sunNDC = sunClipPos.xy / sunClipPos.w;
    output.sunPosition = float2(0.5f * sunNDC.x + 0.5f, -0.5f * sunNDC.y + 0.5f);
    
    return output;
}