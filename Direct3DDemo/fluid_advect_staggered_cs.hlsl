RWStructuredBuffer<float> gNewVelocityX : register(u0);
RWStructuredBuffer<float> gNewVelocityY : register(u1);
RWStructuredBuffer<float> gNewVelocityZ : register(u2);
StructuredBuffer<float> gVelocityX : register(t0);
StructuredBuffer<float> gVelocityY : register(t1);
StructuredBuffer<float> gVelocityZ : register(t2);
Texture3D<float> gSDF : register(t3);
Texture3D<float> gNoiseMap : register(t4);
StructuredBuffer<float> gDensity : register(t5);

SamplerState samplerClamp : register(s0);
SamplerState samplerWrap : register(s1);

cbuffer FluidParams : register(b0)
{
    float deltaTime;
    float elapsedTime;
}

int GridIndex(int x, int y, int z, int3 size)
{
    return (z * size.y * size.x) + (y * size.x) + x;
}

float TrilinearSample(StructuredBuffer<float> grid, int3 gridSize, float3 pos)
{
    pos = clamp(pos, float3(0.0, 0.0, 0.0), float3(gridSize - int3(1, 1, 1)));

    // Get integer cell corner indices
    int3 p0 = (int3) floor(pos);
    int3 p1 = p0 + int3(1, 1, 1);

    // Clamp indices to avoid out-of-bounds
    p0 = clamp(p0, int3(0, 0, 0), gridSize - 1);
    p1 = clamp(p1, int3(0, 0, 0), gridSize - 1);

    // Fractional offset within the cell
    float3 f = pos - (float3) p0;

    // Sample the 8 corners
    float c000 = grid[GridIndex(p0.x, p0.y, p0.z, gridSize)];
    float c100 = grid[GridIndex(p1.x, p0.y, p0.z, gridSize)];
    float c010 = grid[GridIndex(p0.x, p1.y, p0.z, gridSize)];
    float c110 = grid[GridIndex(p1.x, p1.y, p0.z, gridSize)];
    float c001 = grid[GridIndex(p0.x, p0.y, p1.z, gridSize)];
    float c101 = grid[GridIndex(p1.x, p0.y, p1.z, gridSize)];
    float c011 = grid[GridIndex(p0.x, p1.y, p1.z, gridSize)];
    float c111 = grid[GridIndex(p1.x, p1.y, p1.z, gridSize)];

    // Trilinear interpolation
    return lerp(
        lerp(
            lerp(c000, c100, f.x),
            lerp(c010, c110, f.x),
            f.y),
        lerp(
            lerp(c001, c101, f.x),
            lerp(c011, c111, f.x),
            f.y),
        f.z);
}

float3 SampleVelocity(float3 pos, int3 gridSizeX, int3 gridSizeY, int3 gridSizeZ)
{
    float u = TrilinearSample(gVelocityX, gridSizeX, pos - float3(0.0f, 0.5f, 0.5f)); // to X-face
    float v = TrilinearSample(gVelocityY, gridSizeY, pos - float3(0.5f, 0.0f, 0.5f)); // to Y-face
    float w = TrilinearSample(gVelocityZ, gridSizeZ, pos - float3(0.5f, 0.5f, 0.0f)); // to Z-face

    return float3(u, v, w);
}

float3 WindForce(float3 samplePos)
{
    float3 windDir = float3(0, 0, 0);
    
    // Wind field parameters
    float gustSpeed = 0.07f * 0.13f;
    float windStrength = 52.0f;
    
    // fBm scalar for wind strength modulation
    float strengthFreq = 1.2f * 0.11f;
    float strengthAmp = 0.5f;
    float gust = 0.0f;

    for (int j = 0; j < 3; j++)
    {
        float noise = gNoiseMap.SampleLevel(samplerWrap, samplePos * strengthFreq - gustSpeed * elapsedTime, 0).r;
        gust += (noise * 0.5f + 0.5f) * strengthAmp;
        strengthFreq *= 1.8;
        strengthAmp *= 0.5;
    }
            
    float threshold = 0.43f;
    gust = smoothstep(threshold, threshold + 0.15f, gust);

    
    if (gust > 0)
    {
        // fBm vector noise for wind direction
        float dirFreq = 1.462f * 0.13f;
        float dirAmp = 1.0f;
        float windSpeed = 0.1f * 0.09f;

        for (int i = 0; i < 4; i++)
        {               
            windDir.x += gNoiseMap.SampleLevel(samplerWrap, (samplePos + float3(0.92, 0, 0)) * dirFreq - windSpeed * elapsedTime, 0).r * dirAmp;
            windDir.y += gNoiseMap.SampleLevel(samplerWrap, (samplePos + float3(0, 1.679, 0)) * dirFreq - windSpeed * elapsedTime, 0).r * dirAmp;
            windDir.z += gNoiseMap.SampleLevel(samplerWrap, (samplePos + float3(0, 0, 2.697)) * dirFreq - windSpeed * elapsedTime, 0).r * dirAmp;

            dirFreq *= 1.8;
            dirAmp *= 0.6;
        }

        windDir = normalize(windDir); // direction only
    }
    
    return windDir * gust * windStrength;
}

float3 CurlForce(float3 samplePos)
{
    float3 res = float3(0, 0, 0);
    float curlFreq = 0.07f * 0.1f;
    float curlSpeed = 0.03f * 0.1f;
    float curlAmp = 3.5f;
    for (int i = 0; i < 3; i++)
    {
        res.x += saturate(gNoiseMap.SampleLevel(samplerWrap, (samplePos + float3(3.862, 0, 0)) * curlFreq - curlSpeed * elapsedTime, 0).r) * curlAmp;
        res.y += saturate(gNoiseMap.SampleLevel(samplerWrap, (samplePos + float3(0, 4.621, 0)) * curlFreq - curlSpeed * elapsedTime, 0).r) * curlAmp;
        res.z += saturate(gNoiseMap.SampleLevel(samplerWrap, (samplePos + float3(0, 0, 5.638)) * curlFreq - curlSpeed * elapsedTime, 0).r) * curlAmp;

        curlFreq *= 1.7;
        curlAmp *= 0.4;
    }
    return res;
}
float3 BuoyancyForce(float3 samplePos, float3 densityGridSize)
{
    float density = gDensity[GridIndex(samplePos.x, samplePos.y, samplePos.z, densityGridSize)];
    
    float kDensity = 13.0f;
    float buoyancy = -kDensity * density;
    
    return float3(0, buoyancy, 0);
}

float3 Advect(float3 facePos, float3 gridSizeX, float3 gridSizeY, float3 gridSizeZ)
{   
    // Sample current full velocity field
    float3 velocity = SampleVelocity(facePos, gridSizeX, gridSizeY, gridSizeZ);
        
    float3 prevPos = facePos - deltaTime * velocity;
    float3 newVelocity = SampleVelocity(prevPos, gridSizeX, gridSizeY, gridSizeZ);
        
    return newVelocity;
}

[numthreads(4, 4, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    int subdivision = 4;
    int dimension = 8;
    int simRes = dimension * subdivision;
    
    int3 gridSizeX = int3(simRes + 3, simRes + 2, simRes + 2);
    int3 gridSizeY = int3(simRes + 2, simRes + 3, simRes + 2);
    int3 gridSizeZ = int3(simRes + 2, simRes + 2, simRes + 3);
    int3 gridSize = int3(simRes + 2, simRes + 2, simRes + 2);
    int3 simGridSize = int3(simRes, simRes, simRes);

    int x = groupID.x * subdivision + groupThreadID.x;
    int y = groupID.y * subdivision + groupThreadID.y;
    int z = groupID.z * subdivision + groupThreadID.z;
    
    if (x > gridSize.x - 1 || y > gridSize.y - 1 || z > gridSize.z - 1)
        return;
    
    
    if (gSDF.SampleLevel(samplerClamp, float3(x, y, z) / gridSize, 0) <= 0.0f)
    {
        return;
    }
    
    
    float3 windForce = WindForce(float3(x, y, z) / (simRes + 2));
    float3 curlForce = CurlForce(float3(x, y, z) / (simRes + 2));
    //float3 windForce = float3(0, 0, 0);
    //float3 curlForce = float3(0, 0, 0);
    float3 buoyancyForce = BuoyancyForce(float3(x, y, z), gridSize);
    float3 force = windForce + curlForce + buoyancyForce;
    
    int velocityIndex;
    float3 facePos, prevPos;
    float3 velocity, newVelocity;
    
    // U COMPONENT ADVECTION
    velocityIndex = GridIndex(x, y, z, gridSizeX);
    facePos = float3(x, y + 0.5f, z + 0.5f); // physical position of right face
    newVelocity.x = Advect(facePos, gridSizeX, gridSizeY, gridSizeZ).x;
    
    // U COMPONENT FORCE APPLICATION
    newVelocity.x += force.x * deltaTime;
        
    gNewVelocityX[velocityIndex] = newVelocity.x;
    
    
    // V COMPONENT ADVECTION
    velocityIndex = GridIndex(x, y, z, gridSizeY);
    facePos = float3(x + 0.5f, y, z + 0.5f);
    newVelocity.y = Advect(facePos, gridSizeX, gridSizeY, gridSizeZ).y;
    
    // V COMPONENT FORCE APPLICATION
    newVelocity.y += force.y * deltaTime;
        
    gNewVelocityY[velocityIndex] = newVelocity.y;
    
    
    // W COMPONENT ADVECTION
    velocityIndex = GridIndex(x, y, z, gridSizeZ);
    facePos = float3(x + 0.5f, y + 0.5f, z);
    newVelocity.z = Advect(facePos, gridSizeX, gridSizeY, gridSizeZ).z;
    
    // W COMPONENT FORCE APPLICATION
    newVelocity.z += force.z * deltaTime;
        
    gNewVelocityZ[velocityIndex] = newVelocity.z;
}