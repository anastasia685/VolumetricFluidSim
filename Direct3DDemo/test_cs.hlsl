RWStructuredBuffer<float> gOutput : register(u0);

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
float PerlinFBm(float3 P)
{
    float init_frequency = 0.06f;
    float lacunarity = 1.8;
    float H = 0.25f;
    
    float3 vp = P * init_frequency;
    
    int octaves = 4;
    
    float res = 0;

    for (int i = 0; i < octaves; i++)
    {
        res += Perlin3D(vp) * pow(lacunarity, -H * i);
        vp *= lacunarity;
    }
    return max(0.f, res);
}

float Worley3D(float3 pos, float scale)
{
    pos *= scale;
    
    int3 cell = floor(pos);
    float3 localPos = frac(pos);
    
    float minDist = 1.0;

    // Check neighboring cells
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
            for (int z = -1; z <= 1; z++)
            {
                int3 neighborCell = cell + int3(x, y, z);
                float3 randomOffset = float3(
                    frac(sin(dot(float3(neighborCell), float3(127.1, 311.7, 74.7))) * 43758.5453),
                    frac(sin(dot(float3(neighborCell), float3(269.5, 183.3, 246.1))) * 43758.5453),
                    frac(sin(dot(float3(neighborCell), float3(113.5, 271.9, 124.6))) * 43758.5453)
                );

                float3 p = float3(neighborCell) + randomOffset;
                float dist = length(p - pos);

                minDist = min(minDist, dist);
            }

    return minDist;
}

float CloudDensity(float3 pos)
{
    float cloudMass = Worley3D(pos, 0.1);
    float threshold = 0.5; // Only keep the top 50% of Worley values
    cloudMass = cloudMass > threshold ? cloudMass : 0.0;
    
    float perlin = PerlinFBm(pos);
    float worley = Worley3D(pos, 0.1);
    
    return saturate(cloudMass * perlin);
    

    // Invert Worley to create cloudy gaps, and multiply with Perlin for soft variation
    return saturate((1.0 - worley) * perlin);
}

[numthreads(4, 4, 4)] // subdivision factor
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    int subdivision = 4;
    int dimension = 8;

    // this is for 3D
    int outputIndex = ((groupID.z * subdivision + groupThreadID.z) * dimension * subdivision * dimension * subdivision) +
                      ((groupID.y * subdivision + groupThreadID.y) * dimension * subdivision) +
                      (groupID.x * subdivision + groupThreadID.x);
    
    float3 positionCoords = float3(groupID.xyz) * subdivision + float3(groupThreadID.xyz);
    
    // this is for 2D
    /*int outputIndex = ((groupID.y * subdivision + groupThreadID.y) * dimension * subdivision) + (groupID.x * subdivision + groupThreadID.x);
    float2 positionCoords = float2(groupID.xy) + float2(groupThreadID.xy) / subdivision;*/
    
    float value = CloudDensity(positionCoords);
    //value = positionCoords.x * positionCoords.y * positionCoords.z;
    
    gOutput[outputIndex] = value;
}