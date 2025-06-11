RWStructuredBuffer<float> gOutput : register(u0);

// Hash function: simple 3D integer hash to float3 in [0, 1]
float3 hash33(float3 p)
{
    uint3 q = (uint3) (int3(p)) * uint3(1597334673U, 3812015801U, 2798796415U);
    q = (q.x ^ q.y ^ q.z) * uint3(1597334673U, 3812015801U, 2798796415U);
    return frac(float3(q) / 4294967295.0);
}

// Worley noise (returns distance to closest feature point)
float Worley3D(float3 p)
{

    // Get cell coordinates and local position
    float3 cell = floor(p);
    float3 localPos = frac(p);

    float minDist = 1e10;

    // Check neighboring cells (3󫢫)
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            [unroll]
            for (int z = -1; z <= 1; ++z)
            {
                float3 neighbor = float3(x, y, z);
                
                // Random point in neighbor cell
                float3 _point = hash33(cell + neighbor);
                
                // Compute distance to feature point
                float3 diff = neighbor + _point - localPos;
                float dist = dot(diff, diff); // squared distance

                minDist = min(minDist, dist);
            }
        }
    }

    // Optional: return inverted distance for "solid" appearance
    return 1.0 - sqrt(minDist); // sqrt for actual distance
}
float WorleyFBm(float3 P)
{
    float lacunarity = 1.6f;
    float gain = 0.6f;
    
    int octaves = 4;
    
    float res = 0;

    float _f = 0.07f, _a = 1;
    float maxAmp = 0;
    for (int i = 0; i < octaves; i++)
    {
        res += Worley3D(P * _f) * _a;
        maxAmp += _a;
        
        _f *= lacunarity;
        _a *= gain;
    }
    res /= maxAmp; // [0,1] range
    
    // remap to center around 1.0
    float contrast = 0.4;
    float center = 0.8;
    res = lerp(center - contrast, center + contrast, res);
    
    return res;
}

[numthreads(8, 8, 8)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    int subdivision = 8;
    int dimension = 16;
    
    int outputIndex = ((groupID.z * subdivision + groupThreadID.z) * dimension * subdivision * dimension * subdivision) +
                      ((groupID.y * subdivision + groupThreadID.y) * dimension * subdivision) +
                      (groupID.x * subdivision + groupThreadID.x);
    
    float3 positionCoords = float3(groupID.xyz) * subdivision + float3(groupThreadID.xyz);
    
    gOutput[outputIndex] = WorleyFBm(positionCoords);
}