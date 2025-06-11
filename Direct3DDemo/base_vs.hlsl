cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
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
    float3 lPosition : POSITION0;
    float3 wPosition : POSITION1;
};

OutputType main(InputType input)
{
    OutputType output;
    
    output.lPosition = input.position;
    output.position = mul(input.position, worldMatrix);
    output.wPosition = output.position.xyz;
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    output.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
    output.tex = input.tex;
    
	return output;
}