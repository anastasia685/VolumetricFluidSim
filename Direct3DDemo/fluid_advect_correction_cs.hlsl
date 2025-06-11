RWStructuredBuffer<float> gCorrectedDensity : register(u0);
StructuredBuffer<float> gDensityPred : register(t0); // phi_pred from first pass
StructuredBuffer<float> gDensity : register(t1); // phi_old (from previous frame)

StructuredBuffer<float> gVelocityX : register(t2);
StructuredBuffer<float> gVelocityY : register(t3);
StructuredBuffer<float> gVelocityZ : register(t4);

StructuredBuffer<float> gSurface : register(t5);

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

// Permutation table (256 values repeated for seamless lookup)
static const int perm[512] =
{
    151, 160, 137, 91, 90, 15,
		131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
		190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
		88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
		77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
		102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
		135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
		5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
		223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
		129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
		251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
		49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
		138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
    // Repeat the same 256 values to avoid needing modulo operations
    151, 160, 137, 91, 90, 15,
		131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
		190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
		88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
		77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
		102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
		135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
		5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
		223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
		129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
		251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
		49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
		138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

// Gradient function: Generates pseudo-random gradient vectors
float3 Gradient(int hash)
{
    hash = hash & 15;
    float3 grad = float3(
        (hash & 1) == 0 ? 1.0 : -1.0,
        (hash & 2) == 0 ? 1.0 : -1.0,
        (hash & 4) == 0 ? 1.0 : -1.0
    );
    return normalize(grad);
}

// Fade function: Smoothstep interpolation
float Fade(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

// Linear interpolation function
float Lerp(float t, float a, float b)
{
    return a + t * (b - a);
}
// 3D Perlin Noise function
float Perlin3D(float3 P)
{
    // Get grid cell coordinates
    int X = (int) floor(P.x) & 255;
    int Y = (int) floor(P.y) & 255;
    int Z = (int) floor(P.z) & 255;

    float3 Pf = P - floor(P);

    // Compute fade curves
    float3 f = float3(Fade(Pf.x), Fade(Pf.y), Fade(Pf.z));

    // Get hashed indices of cube corners
    int A = perm[X] + Y;
    int AA = perm[A] + Z;
    int AB = perm[A + 1] + Z;
    int B = perm[X + 1] + Y;
    int BA = perm[B] + Z;
    int BB = perm[B + 1] + Z;

    // Compute dot products with gradient vectors
    float res = Lerp(f.z,
        Lerp(f.y,
            Lerp(f.x, dot(Gradient(perm[AA]), Pf - float3(0, 0, 0)),
                      dot(Gradient(perm[BA]), Pf - float3(1, 0, 0))),
            Lerp(f.x, dot(Gradient(perm[AB]), Pf - float3(0, 1, 0)),
                      dot(Gradient(perm[BB]), Pf - float3(1, 1, 0)))),
        Lerp(f.y,
            Lerp(f.x, dot(Gradient(perm[AA + 1]), Pf - float3(0, 0, 1)),
                      dot(Gradient(perm[BA + 1]), Pf - float3(1, 0, 1))),
            Lerp(f.x, dot(Gradient(perm[AB + 1]), Pf - float3(0, 1, 1)),
                      dot(Gradient(perm[BB + 1]), Pf - float3(1, 1, 1))))
    );
    
    return res;
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
    float3 pos = float3(x, y, z);

    float3 vel = SampleVelocity(pos, gridSizeX, gridSizeY, gridSizeZ);
    float3 backtrace = pos - vel * deltaTime;
    backtrace = clamp(backtrace, float3(1, 1, 1), float3(gridSize - 2));

    float3 vel2 = SampleVelocity(backtrace, gridSizeX, gridSizeY, gridSizeZ);
    float3 forwardtrace = backtrace + vel2 * deltaTime;
    forwardtrace = clamp(forwardtrace, float3(1, 1, 1), float3(gridSize - 2));

    float phi_old = gDensity[index];
    float phi_pred = gDensityPred[index];
    float phi_double = TrilinearSample(gDensityPred, gridSize, forwardtrace);
    
    
    //--- DEBUGGING
    // set persistent density in small debug region
    //if (x > gridSize.x * 0.4f && x < gridSize.x * 0.6f &&
    //        z > gridSize.z * 0.4f && z < gridSize.z * 0.6f &&
    //        y > gridSize.y * 0.4f && y < gridSize.y * 0.6f)
    //{
    //    //phi_pred += 0.03 * (sin(elapsedTime) * 0.5 + 0.5);
    //    phi_pred = 0.4f;
    //}
    
    float surfaceRes = 17 * 8;
    // normalize simulation-space coordinates
    float2 _heightMapPos = float2((float) x / gridSize.x, (float)z / gridSize.z) * surfaceRes;
    int2 heightMapPos = (int2)_heightMapPos;
    // sample height
    // add world-space offset of grid_mesh (+3)
    // divide by world-space scale of grid_mesh
    // multiply by simulation resolution
    float height = (gSurface[heightMapPos.y * surfaceRes + heightMapPos.x] + 3) / 16 * gridSize.y;
    
    if (x > 1 && x < gridSize.x - 2 && z > 1 && z < gridSize.z - 2 && abs(y - height) <= 1.5)
    {   
        float freq = 0.045;
        float amp = 0.5;
        float speed = 1.1f;
        
        float injected;
        for (int j = 0; j < 3; j++)
        {
            injected += smoothstep(0.18, 0.6, Perlin3D(float3(x, elapsedTime * speed, z) * freq)) * amp;
        
            freq *= 1.7;
            amp *= 0.5;
        }
        injected = 1 - pow(1 - injected, 2); // ease-out function
        //injected = max(0, injected / maxAmp);
        
        phi_pred = injected;
    }
    else
    {
        float highDensityDissipation = 0.012f; // percentage of remaining density after 1 sec
        float dissipationPerStep = pow(highDensityDissipation, deltaTime);
        
        // Adaptive dissipation with smoother curve
        float densityFactor = saturate(phi_pred);
        float exponent = 0.6f;

        float dissipation = pow(dissipationPerStep, pow(densityFactor, exponent));
        phi_pred *= dissipation;
    }
    
    gCorrectedDensity[index] = phi_pred;
    return;
    

    float phi_corr = phi_pred + 0.5f * (phi_old - phi_double);

    // Extract 8 neighbor samples used for interpolation around forwardtrace
    float3 base = floor(forwardtrace);
    float3 frac = forwardtrace - base;

    int3 p0 = clamp(int3(base), int3(0, 0, 0), gridSize - 1);
    int3 p1 = clamp(p0 + int3(1, 1, 1), int3(0, 0, 0), gridSize - 1);

    // Sample the 8 neighbors
    float samples[8];
    samples[0] = gDensity[GridIndex(p0.x, p0.y, p0.z, gridSize)];
    samples[1] = gDensity[GridIndex(p1.x, p0.y, p0.z, gridSize)];
    samples[2] = gDensity[GridIndex(p0.x, p1.y, p0.z, gridSize)];
    samples[3] = gDensity[GridIndex(p1.x, p1.y, p0.z, gridSize)];
    samples[4] = gDensity[GridIndex(p0.x, p0.y, p1.z, gridSize)];
    samples[5] = gDensity[GridIndex(p1.x, p0.y, p1.z, gridSize)];
    samples[6] = gDensity[GridIndex(p0.x, p1.y, p1.z, gridSize)];
    samples[7] = gDensity[GridIndex(p1.x, p1.y, p1.z, gridSize)];

    float minVal = samples[0];
    float maxVal = samples[0];
    for (int i = 1; i < 8; ++i)
    {
        minVal = min(minVal, samples[i]);
        maxVal = max(maxVal, samples[i]);
    }

    // Final clamp
    phi_corr = clamp(phi_corr, minVal, maxVal);
    
    float reinjectionRate = 0.002f; // small and gentle, tweak as needed
    // Inject only where density is already present
    if (phi_corr > 0.005f && x > 1 && y > 1 && z > 1 && x < gridSize.x + 1 && y < gridSize.y + 1 && z < gridSize.z + 1)
    {
        phi_corr += reinjectionRate * deltaTime;
    }

    gCorrectedDensity[index] = phi_corr;
}