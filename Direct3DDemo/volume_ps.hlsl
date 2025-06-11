static const float PI = 3.14159265f;

StructuredBuffer<float> densityMap : register(t0);
StructuredBuffer<float> noiseMap : register(t1);
Texture2D sceneColor : register(t2);
Texture2D sceneDepth : register(t3);

SamplerState samplerState : register(s0);

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
cbuffer VolumeBuffer : register(b2)
{
    matrix mainCameraViewInv;
    matrix mainCameraProjInv;
    float sigma_a; // absorption coefficient
    float sigma_s; // scattering coefficient
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
    float4 occlusion : SV_TARGET1;
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
    float3 diffuseColor = float3(0.7f, 0.7f, 0.7f) * 10;
    float3 toLight = float3(0.4f, 1.0f, -0.8f);
    toLight = normalize(toLight);
    
    float intensity = saturate(dot(normal, toLight));
    return float4(diffuseColor * intensity, 1.0f);
}


bool intersectSphere(float3 ro, float3 rd, out float t0, out float t1)
{
    float3 sphere_center = float3(0, 0, 0);
    float sphere_radius = 2.f;
    
    
    float3 L = sphere_center - ro;
    float tca = dot(L, rd);
    
    float dSq = dot(L, L) - tca * tca;
    float rSq = sphere_radius * sphere_radius;
    
    if (dSq > rSq)
        return false;

    float thc = sqrt(rSq - dSq);
    
    
    t0 = tca - thc;
    t1 = tca + thc;
    
    if (dot(L, L) < rSq)
    {
        t0 = 0;
    }

    return true;
}
bool intersectBox(float3 ro, float3 rd, out float t0, out float t1)
{
    // bounds
    float3 boxMin = float3(-0.5, -0.5, -0.5) * 16;
    float3 boxMax = float3(0.5, 0.5, 0.5) * 16;
    
    float3 invRd = 1.0 / rd;

    // Compute intersections with the slabs on each axis.
    float3 t0s = (boxMin - ro) * invRd;
    float3 t1s = (boxMax - ro) * invRd;

    // For each axis, tmin is the entry distance and tmax is the exit distance.
    float3 tsmaller = min(t0s, t1s);
    float3 tbigger = max(t0s, t1s);

    // The overall tmin is the maximum of the per-axis tmins,
    // and the overall tmax is the minimum of the per-axis tmaxes.
    float tmin = max(tsmaller.x, max(tsmaller.y, tsmaller.z));
    float tmax = min(tbigger.x, min(tbigger.y, tbigger.z));

    // If there is no overlap, the ray misses the box.
    if (tmin > tmax)
        return false;

    // Optionally, if the intersection is entirely behind the ray origin, return false.
    if (tmax < 0.0)
        return false;

    // If the ray starts inside the box, clamp the entry distance to zero.
    t0 = tmin < 0.0 ? 0.0 : tmin;
    t1 = tmax;

    return true;
}

float rand(float seed)
{
    return frac(sin(seed * 12.9898) * 43758.5453);
}
float hash11(float3 p)
{
    uint3 q = (uint3) (int3(p)) * uint3(1597334673U, 3812015801U, 2798796415U);
    uint n = (q.x ^ q.y ^ q.z);

    // Bias away from zero:
    n = n | 1u; // ensure at least lowest bit set

    return float(n) / 4294967295.0;
}
// the Henyey-Greenstein phase function
float phase(float g, float cos_theta)
{
    return (1 / (4 * PI)) * (1 - g * g) / pow(1 + g * g - 2 * g * cos_theta, 1.5);
}
float SampleNoise(float3 uvw, int resolution)
{
    // uvw is expected to be in [0, 1] range
    float3 p = saturate(uvw) * (resolution - 1); // scale to voxel space
    int3 baseIndex = int3(p);
    float3 fracPart = frac(p);

    float result = 0.0;

    // Trilinear interpolation
    for (int i = 0; i <= 1; ++i)
    {
        float wx = (i == 0) ? (1.0 - fracPart.x) : fracPart.x;
        int xi = clamp(baseIndex.x + i, 0, resolution - 1);

        for (int j = 0; j <= 1; ++j)
        {
            float wy = (j == 0) ? (1.0 - fracPart.y) : fracPart.y;
            int yi = clamp(baseIndex.y + j, 0, resolution - 1);

            for (int k = 0; k <= 1; ++k)
            {
                float wz = (k == 0) ? (1.0 - fracPart.z) : fracPart.z;
                int zi = clamp(baseIndex.z + k, 0, resolution - 1);

                int index = xi + yi * resolution + zi * resolution * resolution;
                float sample = noiseMap[index];

                result += sample * wx * wy * wz;
            }
        }
    }

    return result;
}
float eval_density(float3 sample_pos)
{
    // bounds
    float3 boxMin = float3(-0.5, -0.5, -0.5) * 16;
    float3 boxMax = float3(0.5, 0.5, 0.5) * 16;
    
    float resolution = 32;
    float densityResolution = 34; // 32 + ghost cells
    
    float3 gridSize = boxMax - boxMin;
    float3 pLocal = (sample_pos - boxMin) / gridSize;
    float3 pVoxel = pLocal * resolution;
    
    //float3 pLattice = float3(pVoxel.x - 0.5, pVoxel.y - 0.5, pVoxel.z - 0.5);

    int xi = int(floor(pVoxel.x));
    int yi = int(floor(pVoxel.y));
    int zi = int(floor(pVoxel.z));
    
    float weight[3];
    float value = 0;

    // trilinear interpolation
    for (int i = 0; i < 2; ++i)
    {
        weight[0] = 1 - abs(pVoxel.x - (xi + i));
        for (int j = 0; j < 2; ++j)
        {
            weight[1] = 1 - abs(pVoxel.y - (yi + j));
            for (int k = 0; k < 2; ++k)
            {
                weight[2] = 1 - abs(pVoxel.z - (zi + k));
                int index = (clamp(xi + i, 0, resolution - 1) + 1) + (clamp(yi + j, 0, resolution - 1) + 1) * densityResolution + (clamp(zi + k, 0, resolution - 1) + 1) * densityResolution * densityResolution;
                value += weight[0] * weight[1] * weight[2] * densityMap[index];
            }
        }
    }
    
    int noiseResolution = 128;
    float noise = SampleNoise(pLocal, noiseResolution);
    
    return saturate(value * noise);
}
float4 traceVolume(float3 ro, float3 rd, float2 uv, float2 samplingPos)
{
    float3 light_color = sunColor * sunIntensity;
    float3 background_color = float3(0, 0, 0);
    
    float max_steps = 24;
    float max_light_steps = 6;
    float3 to_light = -normalize(sunDirection);
    
    float g = 0.2;
    float cos_theta = dot(-rd, to_light); // pixel_to_camera & pixel_to_light vectors
    
    float sigma_t = sigma_a + sigma_s;
    
    float zFar = 100;
    float sceneLinearDepth = sceneDepth.Sample(samplerState, samplingPos).r * zFar;
    
    float t0, t1;
    if (intersectBox(ro, rd, t0, t1))
    {
        t1 = min(t1, sceneLinearDepth);
        
        if (t0 >= t1)
            return float4(background_color, 1);
        
        
        float stride = (t1 - t0) / max_steps;
        
        float transparency = 1.0f;
        float3 result = float3(0, 0, 0);
        
        for (int n = 0; n < max_steps; n++)
        {
            // distance to middle of current section
            float jitter = (hash11(float3(samplingPos, n)) * 0.7) + 0.15; // instead of 0.5
            //jitter = 0.5f;
            float t = t0 + stride * (n + jitter);
            
            float3 sample_pos = ro + rd * t;
            
            float density = eval_density(sample_pos);
            
            float sample_attenuation = exp(-stride * sigma_t * density); // beer's law - light transmitted over the sample distance
            
            transparency *= sample_attenuation; // update overal volume light transmission with each step
            
            
            // AMBIENT LIGHT CONTRIBUTION
            result += ambientColor * ambientIntensity * 0.5f * density * stride * transparency;
            
            
            //---  in-scattering
            // t0 will be 0, cause we're casting from sample_pos, t1 will be distance light traveled through the volume
            float _t0_sample, t1_sample;
            if (density > 0 && intersectBox(sample_pos, to_light, _t0_sample, t1_sample)) 
            {   
                //int ns_light = ceil(t1_sample / step_size);
                float stride_light = t1_sample / max_light_steps;
                float tau = 0;
                
                
                for (int nl = 0; nl < max_light_steps; ++nl)
                {
                    float t_light = stride_light * (nl + 0.5);
                    float3 light_sample_pos = sample_pos + to_light * t_light;
                    tau += eval_density(light_sample_pos);
                }
                
                float light_ray_att = exp(-tau * stride_light * sigma_t);
                result += light_color * // light color
                      light_ray_att * // light ray transmission value
                      phase(g, cos_theta) * // phase function
                      sigma_s * // scattering coefficient
                      transparency * // ray current transmission value
                      stride * // dx in Riemann sum
                      density; // volume density at the sample location
            }
            
            if (transparency < 1e-3)
            {
                break;
            }
        }
        
        return float4(result, transparency);
    }
    else
    {
        return float4(background_color, 1);
    }
}


OutputType main(InputType input)
{
    OutputType output;
    //output.color = NormalColor(input.normal);
    //output.color = SlopeColor(input.normal);
    //output.color = Phong(input.normal);
    
    uint width, height;
    sceneColor.GetDimensions(width, height);
    
    float2 samplingPos = input.position.xy / float2(width, height);
    
     // clip-space
    float4 reconstructedWorldPos = float4(
        input.tex.x * 2.0f - 1.0f,
        (1 - input.tex.y) * 2.0f - 1.0f,
        1.0f, // any value for z component since i'm reconstructing the *ray*, not a specific point
        1.0f); 
    // view-space
    reconstructedWorldPos = mul(reconstructedWorldPos, mainCameraProjInv);
    reconstructedWorldPos /= reconstructedWorldPos.w; // perspective divide
    // world-space
    reconstructedWorldPos = mul(reconstructedWorldPos, mainCameraViewInv);
    
    
    output.color = traceVolume(cameraPos, normalize(input.wPosition.xyz - cameraPos), input.tex, samplingPos);
    output.occlusion = float4(output.color.a, 0, 0, 1);
    
    return output;
}