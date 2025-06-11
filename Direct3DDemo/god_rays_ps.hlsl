Texture2D sceneColor : register(t0); // Main rendered scene
Texture2D occlusion : register(t1); // Occlusion mask with sun & sky
SamplerState samplerState : register(s0); // Clamp sampler

struct InputType
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
    float2 sunPosition : TEXCOORD1;
};

float4 main(InputType input) : SV_TARGET
{
    const int NUM_SAMPLES = 80;
    const float density = 0.926;
    const float decay = 0.96815;
    const float exposure = 0.1f;
    const float weight = 0.58767f;

    float2 texCoord = input.tex;
    float2 deltaTexCoord = (texCoord - input.sunPosition) * (density / NUM_SAMPLES);
    
    float illuminationDecay = 1.0f;
    float occlusionValue = occlusion.Sample(samplerState, texCoord).r;
    
    float3 rayColor = occlusionValue.rrr;
    float2 sampleCoord = texCoord;

    [unroll]
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        sampleCoord -= deltaTexCoord; // step towards the sun

        float3 sampleLight = occlusion.Sample(samplerState, sampleCoord).rrr;
        sampleLight *= illuminationDecay * weight;

        rayColor += sampleLight;

        illuminationDecay *= decay;
    }

    // final brightness scale
    rayColor *= exposure;

    // composite over scene color
    float3 scene = sceneColor.Sample(samplerState, texCoord).rgb;
    float3 finalColor = scene + rayColor;

    return float4(finalColor, 1.0f);
}