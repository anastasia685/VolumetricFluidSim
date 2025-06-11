RWTexture3D<float> gOutput : register(u0);

static const int perm[16] =
{
    3, 6, 1, 0, 5, 7, 4, 2,
    
    // repeat
    3, 6, 1, 0, 5, 7, 4, 2
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

// 3D Perlin Noise function
float Perlin3D(float3 P, int period)
{
    int X = (int) floor(P.x) % period;
    int Y = (int) floor(P.y) % period;
    int Z = (int) floor(P.z) % period;
    if (X < 0) X += period;
    if (Y < 0) Y += period;
    if (Z < 0) Z += period;

    float3 Pf = P - floor(P);
    float u = Fade(Pf.x);
    float v = Fade(Pf.y);
    float w = Fade(Pf.z);

    int A = (perm[X] + Y) % period;
    int AA = (perm[A] + Z) % period;
    int AB = (perm[(A + 1) % period] + Z) % period;
    int B = (perm[(X + 1) % period] + Y) % period;
    int BA = (perm[B] + Z) % period;
    int BB = (perm[(B + 1) % period] + Z) % period;

    float res = lerp(
        lerp(
            lerp(dot(Gradient(perm[AA]), Pf - float3(0, 0, 0)),
                 dot(Gradient(perm[BA]), Pf - float3(1, 0, 0)), u),
            lerp(dot(Gradient(perm[AB]), Pf - float3(0, 1, 0)),
                 dot(Gradient(perm[BB]), Pf - float3(1, 1, 0)), u), v),
        lerp(
            lerp(dot(Gradient(perm[(AA + 1) % period]), Pf - float3(0, 0, 1)),
                 dot(Gradient(perm[(BA + 1) % period]), Pf - float3(1, 0, 1)), u),
            lerp(dot(Gradient(perm[(AB + 1) % period]), Pf - float3(0, 1, 1)),
                 dot(Gradient(perm[(BB + 1) % period]), Pf - float3(1, 1, 1)), u), v),
        w
    );

    return res;
}



[numthreads(8, 8, 8)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    int subdivision = 8;
    int dimension = 16;
    
    int noisePeriod = 8; // has to match permutation table length
    
    float3 positionCoords = float3(dispatchThreadID) / (dimension * subdivision);
    positionCoords = positionCoords * noisePeriod;

    gOutput[dispatchThreadID] = Perlin3D(positionCoords, noisePeriod);
}