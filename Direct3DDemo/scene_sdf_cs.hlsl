RWTexture3D<float> gSDF : register(u0);

Texture3D<float> gSDF0 : register(t0);
//StructuredBuffer<float> gSDF1 : register(t1);

SamplerState samplerClamp : register(s0);

cbuffer SDFParams : register(b0)
{
    matrix simulationTransform;
    matrix sdfTransformInv0;
    //matrix sdfTransformInv1;
    float sdfScale0;
}

[numthreads(4, 4, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    int subdivision = 4;
    int dimension = 16;
    int simRes = dimension * subdivision;
    int3 gridSize = int3(simRes + 2, simRes + 2, simRes + 2); // account for ghost cells
    
    //int x = groupID.x * subdivision + groupThreadID.x;
    //int y = groupID.y * subdivision + groupThreadID.y;
    //int z = groupID.z * subdivision + groupThreadID.z;
    
    int x = dispatchThreadID.x;
    int y = dispatchThreadID.y;
    int z = dispatchThreadID.z;
    
    if (x > gridSize.x - 1 || y > gridSize.y - 1 || z > gridSize.z - 1)
        return;
    
    // ghost cells are boundaries
    if (x == 0 || x == gridSize.x - 1 || y == 0 || y == gridSize.y - 1 || z == 0 || z == gridSize.z - 1)
    {
        gSDF[float3(x, y, z)] = -(simRes + 2);
        
        return;
    }
    
    // Transform sim grid cell center from [0,1] space
    float3 simLocal = (float3(x, y, z)) / (simRes + 2);
    float3 worldPos = mul(float4(simLocal, 1.0f), simulationTransform).xyz;
    
    float minDist = simRes;

    {
        float3 sdfLocal0 = mul(float4(worldPos, 1.0f), sdfTransformInv0).xyz;
        bool inside0 = all(sdfLocal0 >= 0.0f) && all(sdfLocal0 <= 1.0f);
        if (inside0)
        {
            float d0 = gSDF0.SampleLevel(samplerClamp, sdfLocal0, 0) * sdfScale0;
            minDist = min(minDist, d0);
        }
    }

    //{
    //    float3 sdfLocal1 = mul(float4(worldPos, 1.0f), sdfTransformInv1).xyz;
    //    bool inside1 = all(sdfLocal1 >= 0.0f) && all(sdfLocal1 <= 1.0f);
    //    if (inside1)
    //    {
    //        float d1 = SampleSDF(gSDF1, int3(32, 32, 32), sdfLocal1);
    //        minDist = min(minDist, d1);
    //    }
    //}
    
    gSDF[float3(x, y, z)] = minDist;
}