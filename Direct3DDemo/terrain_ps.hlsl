StructuredBuffer<float3> normalBuffer : register(t0);

Texture2D albedoTexture0 : register(t1);
Texture2D albedoTexture1 : register(t2);
SamplerState samplerState : register(s0);

static const float PI = 3.14159265f;

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 wPosition : POSITION;
    float lHeight : TEXCOORD1;
    matrix worldMatrix : TEXCOORD2;
};
struct OutputType
{
    float4 color : SV_TARGET0;
    float4 linearDepth : SV_TARGET1;
    float4 occlusion : SV_TARGET2;
};

cbuffer LightBuffer : register(b0)
{
    float3 ambientColor;
    float ambientIntensity;
    float3 sunColor;
    float sunIntensity;
    float3 sunDirection;
};

cbuffer CameraBuffer : register(b1)
{
    float3 cameraPos;
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
    float3 albedo = float3(0.18f, 0.17f, 0.16f);
    
    float3 diffuseColor = sunColor * sunIntensity;
    float3 toLight = normalize(-sunDirection);
    
    float intensity = saturate(dot(normal, toLight));
    return float4(albedo * diffuseColor * intensity, 1.0f);
}


float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Adapted from https://learnopengl.com/PBR/Lighting
float4 PBR(float3 N, float3 V, float3 albedo)
{
    float3 lightColor = sunColor * sunIntensity;
    float metallic = 0.0f, roughness = 0.9f;
    
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedo, metallic);
	           
    // reflectance equation
    float3 Lo = float3(0.0, 0.0, 0.0);
    
    float3 L = normalize(-sunDirection);
    float3 H = normalize(V + L);

    //float distance = length(lightPositions[i] - WorldPos);
    //float attenuation = 1.0 / (distance * distance);
    float attenuation = 1.0f;
    float3 radiance = lightColor * attenuation;
        
    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0 - metallic;
        
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
            
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    
    float3 ambient = ambientColor * ambientIntensity * albedo;
    
    return float4(Lo + ambient, 1.0f);
}

float3 sampleNormalBuffer(float2 localCoords)
{
    float resolution = 17;
    float quality = 8;
    
    // scale localCoords from [0, resolution] to match the (resolution*quality x resolution*quality) grid space
    float2 gridCoords = localCoords * quality;
    int dimension = resolution * quality;

    // split into whole and fractional parts
    float2 gridIndex = floor(gridCoords);
    float2 fracIndex = frac(gridCoords);

    // clamp gridIndex to bounds
    gridIndex = clamp(gridIndex, float2(0, 0), float2((dimension) - 2, (dimension) - 2));

    // calculate indices of neighbors to access 1d height buffer
    int indexBL = int(gridIndex.y) * (dimension) + int(gridIndex.x);
    int indexBR = int(gridIndex.y) * (dimension) + int(gridIndex.x + 1);
    int indexTL = int(gridIndex.y + 1) * (dimension) + int(gridIndex.x);
    int indexTR = int(gridIndex.y + 1) * (dimension) + int(gridIndex.x + 1);

    // sample normals
    float3 normalBL = normalBuffer[indexBL];
    float3 normalBR = normalBuffer[indexBR];
    float3 normalTL = normalBuffer[indexTL];
    float3 normalTR = normalBuffer[indexTR];

    // bilinear interpolation
    float3 topNormal = lerp(normalTL, normalTR, fracIndex.x);
    float3 bottomNormal = lerp(normalBL, normalBR, fracIndex.x);
    float3 interpolatedNormal = lerp(bottomNormal, topNormal, fracIndex.y);

    return interpolatedNormal;
}


OutputType main(InputType input)
{
    OutputType o;
    
    float resolution = 17.f;
    float3 normal = sampleNormalBuffer(input.tex * (resolution - 1));
    normal = normalize(mul(normal, (float3x3) input.worldMatrix));
    
    //float3 albedo = float3(0.045f, 0.0425f, 0.04f);
    //float3 albedo = albedoTexture0.Sample(samplerState, input.tex * 3);
    float blendFactor = smoothstep(0.45, 0.55, input.lHeight * 0.5f + 0.5f);
    float3 tex0 = albedoTexture0.Sample(samplerState, input.tex * 2.5).rgb; // ground
    float3 tex1 = albedoTexture1.Sample(samplerState, input.tex * 2.5).rgb; // rock
    float3 albedo = lerp(tex0, tex1, blendFactor);
    
    //o.color = NormalColor(input.normal);
    //o.color = SlopeColor(input.normal);
    //o.color = Phong(normal);// * albedoTexture.Sample(samplerState, input.tex);
    o.color = PBR(normal, normalize(cameraPos - input.wPosition), albedo);
    
    float zFar = 100;
    o.linearDepth = float4(length(input.wPosition.xyz - cameraPos) / zFar, 0, 0, 1);
    
    o.occlusion = float4(0, 0, 0, 1);
    
    return o;
}