Texture2D volumeColor : register(t0); // src
Texture2D opaqueColor : register(t1); // dest

SamplerState samplerState : register(s0);

struct InputType
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
};


float4 main(InputType input) : SV_TARGET
{
    float4 destColor = opaqueColor.Sample(samplerState, input.tex);
    float4 srcColor = volumeColor.Sample(samplerState, input.tex);
    
    float3 result = destColor.rgb * srcColor.a + srcColor.rgb;
    
    return float4(result, 1.0f);
}