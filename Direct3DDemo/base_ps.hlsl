struct InputType
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
};

float4 NormalColor(float3 normal)
{
    return float4(normal, 1.f);
}
float4 SlopeColor(float3 normal)
{
    float slope = 1.f - abs(normal.y);
    
    float3 red = float3(1.f, 0.f, 0.f);
    float3 green = float3(0.f, 1.f, 0.f);
    float3 blue = float3(0.f, 0.f, 1.f);
    
    if (slope < 0.2f)
        return float4(green, 1.f);
    if (slope < 0.5f)
        return float4(lerp(green, blue, (slope - 0.2f) / 0.3f), 1.f);
    return float4(lerp(blue, red, (slope - 0.5f) / 0.5f), 1.f);
}
float4 Phong(float3 normal)
{
    float3 diffuseColor = float3(0.7f, 0.7f, 0.7f);
    float3 toLight = float3(0.4f, 1.0f, -0.8f);
    toLight = normalize(toLight);
    
    float intensity = saturate(dot(normal, toLight));
    return float4(diffuseColor * intensity, 1.0f);
}


float4 main(InputType input) : SV_TARGET
{
    //return NormalColor(input.normal);
    //return SlopeColor(input.normal);
    return Phong(input.normal);
}