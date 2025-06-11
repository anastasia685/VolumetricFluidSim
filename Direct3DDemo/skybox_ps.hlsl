cbuffer LightBuffer : register(b0)
{
    float3 ambientColor;
    float ambientIntensity;
    float3 sunColor;
    float sunIntensity;
    float3 sunDirection;
};

struct InputType
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
    float3 lPosition : POSITION0;
    float3 wPosition : POSITION1;
};
struct OutputType
{
    float4 color : SV_TARGET0;
    float4 linearDepth : SV_TARGET1;
    float4 occlusion : SV_TARGET2;
};



OutputType main(InputType input)
{
    OutputType output;
    
    float3 skyColor = ambientColor;
    
    
    float3 sunDisc = sunColor * sunIntensity * 0.5 * pow(saturate(dot(normalize(input.lPosition), -normalize(sunDirection))), 3500.0f);
    skyColor += sunDisc;
    
    float3 occlusionMask = sunColor * 0.5 * pow(saturate(dot(normalize(input.lPosition), -normalize(sunDirection))), 200.0f);

    
    output.color = float4(skyColor, 1.0f);
    output.occlusion = float4(saturate(sunDisc.r + occlusionMask.r), 0, 0, 1);
    
    return output;
}