RWStructuredBuffer<float> gNewDensity : register(u0);
StructuredBuffer<float> gVelocityX : register(t0);
StructuredBuffer<float> gVelocityY : register(t1);
StructuredBuffer<float> gVelocityZ : register(t2);
StructuredBuffer<float> gDensity : register(t3);
Texture3D<float> gSDF : register(t4);
StructuredBuffer<float> gSurface : register(t5);
Texture3D<float> gNoiseMap : register(t6);

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

// Trilinear interpolation for scalar fields
float TrilinearSample(StructuredBuffer<float> grid, int3 gridSize, float3 pos)
{
    int3 p0 = (int3) floor(pos);
    int3 p1 = p0 + int3(1, 1, 1);
    p0 = clamp(p0, int3(0, 0, 0), gridSize - 1);
    p1 = clamp(p1, int3(0, 0, 0), gridSize - 1);
    float3 f = pos - (float3) p0;

    float c000 = grid[GridIndex(p0.x, p0.y, p0.z, gridSize)];
    float c100 = grid[GridIndex(p1.x, p0.y, p0.z, gridSize)];
    float c010 = grid[GridIndex(p0.x, p1.y, p0.z, gridSize)];
    float c110 = grid[GridIndex(p1.x, p1.y, p0.z, gridSize)];
    float c001 = grid[GridIndex(p0.x, p0.y, p1.z, gridSize)];
    float c101 = grid[GridIndex(p1.x, p0.y, p1.z, gridSize)];
    float c011 = grid[GridIndex(p0.x, p1.y, p1.z, gridSize)];
    float c111 = grid[GridIndex(p1.x, p1.y, p1.z, gridSize)];

    return lerp(
        lerp(lerp(c000, c100, f.x), lerp(c010, c110, f.x), f.y),
        lerp(lerp(c001, c101, f.x), lerp(c011, c111, f.x), f.y),
        f.z);
}

// Samples staggered velocity components at a given position
float3 SampleVelocity(float3 pos, int3 gridSizeX, int3 gridSizeY, int3 gridSizeZ)
{
    float u = TrilinearSample(gVelocityX, gridSizeX, pos - float3(0.0f, 0.5f, 0.5f)); // to X-face
    float v = TrilinearSample(gVelocityY, gridSizeY, pos - float3(0.5f, 0.0f, 0.5f)); // to Y-face
    float w = TrilinearSample(gVelocityZ, gridSizeZ, pos - float3(0.5f, 0.5f, 0.0f)); // to Z-face

    return float3(u, v, w);
}

[numthreads(4, 4, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    int subdivision = 4;
    int dimension = 8;
    int simRes = dimension * subdivision;
    
    int x = groupID.x * subdivision + groupThreadID.x;
    int y = groupID.y * subdivision + groupThreadID.y;
    int z = groupID.z * subdivision + groupThreadID.z;

    int3 gridSize = int3(simRes + 2, simRes + 2, simRes + 2);
    int3 gridSizeX = int3(simRes + 3, simRes + 2, simRes + 2);
    int3 gridSizeY = int3(simRes + 2, simRes + 3, simRes + 2);
    int3 gridSizeZ = int3(simRes + 2, simRes + 2, simRes + 3);
    
    if (x >= gridSize.x || y >= gridSize.y || z >= gridSize.z)
        return;

    int index = GridIndex(x, y, z, gridSize);
    
    //if (gSDF.SampleLevel(samplerClamp, float3(x, y, z) / gridSize, 0) > 0.5f)
    //{
    //    gNewDensity[index] = 0;
    //    return;
    //}
    //else
    //{
    //    gNewDensity[index] = 0.4f;
    //    return;
    //}

    // Sample velocity field at this cell's center
    float3 velocity = SampleVelocity(float3(x, y, z), gridSizeX, gridSizeY, gridSizeZ);

    // Compute backtraced position
    float3 prevPosition = float3(x, y, z) - velocity * deltaTime;
    
    prevPosition = clamp(prevPosition, float3(1, 1, 1), float3(gridSize - 2));

    // Sample density at the backtraced position
    float newDensity = TrilinearSample(gDensity, gridSize, prevPosition);
    
    
    //--- DEBUGGING
    // set persistent density in small debug region
    //if (x > gridSize.x * 0.4f && x < gridSize.x * 0.6f &&
    //        z > gridSize.z * 0.4f && z < gridSize.z * 0.6f &&
    //        y > gridSize.y * 0.4f && y < gridSize.y * 0.6f)
    //{
    //    //newDensity += 0.03 * (sin(elapsedTime) * 0.5 + 0.5);
    //    newDensity = 0.4f;
    //}
    
    float surfaceRes = 17 * 8;
    // normalize simulation-space coordinates
    float2 _heightMapPos = float2((float) x / gridSize.x, (float) z / gridSize.z) * surfaceRes;
    int2 heightMapPos = (int2) _heightMapPos;
    // sample height
    // add world-space offset of grid_mesh (it's shifted down by 4 but after scaling by 16, so shift back up by 4/16=0.25)
    // convert to simulation space
    float height = (gSurface[heightMapPos.y * surfaceRes + heightMapPos.x] + 0.25f) * gridSize.y;
    
    
    if (x > 1 && x < gridSize.x - 2 && z > 1 && z < gridSize.z - 2 && y < gridSize.y * 0.35f && abs(y - height) <= 1.5)
    {
        float freq = 0.275f;
        float amp = 0.5;
        float speed = 0.7f;
        
        float injected = 0;
        for (int j = 0; j < 3; j++)
        {
            float3 samplePos = float3(x, elapsedTime * speed, z) / (simRes + 2);
            
            float noise = gNoiseMap.SampleLevel(samplerWrap, samplePos * freq, 0).r;
            injected += smoothstep(0.07, 0.6f, noise) * amp;
        
            freq *= 1.4;
            amp *= 0.7;
        }
        
        
        newDensity = saturate(injected * smoothstep(0, 0.2, 1 - y / (gridSize.y * 0.35f)));
    }
    else
    {   
        // Parameters
        float baseDecayRate = 0.003;
        float sharpness = 1.4; // higher = more resistance for high density
        float minDecay = 0.001;
        float decayRate = baseDecayRate / (1.0 + newDensity * sharpness);
        //decayRate = max(decayRate, minDecay); // ensure some decay always happens

        newDensity -= decayRate * deltaTime;
        newDensity = saturate(newDensity);
    }

    // Store new density
    gNewDensity[index] = newDensity;
}