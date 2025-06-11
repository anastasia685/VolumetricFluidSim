RWStructuredBuffer<float> gNewPressure : register(u0);

StructuredBuffer<float> gPressure : register(t0);
StructuredBuffer<float> gDivergence : register(t1);
Texture3D<float> gSDF : register(t2);

SamplerState samplerClamp : register(s0);

int GridIndex(int x, int y, int z, int3 size)
{
    return (z * size.y * size.x) + (y * size.x) + x;
}


[numthreads(4, 4, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    int subdivision = 4;
    int dimension = 8;
    int simRes = dimension * subdivision;
    int3 gridSize = int3(simRes + 2, simRes + 2, simRes + 2);
    

    int x = groupID.x * subdivision + groupThreadID.x;
    int y = groupID.y * subdivision + groupThreadID.y;
    int z = groupID.z * subdivision + groupThreadID.z;

    // don't solve for ghost pressure cells
    if (x == 0 || x >= gridSize.x - 1 || y == 0 || y >= gridSize.y - 1 || z == 0 || z >= gridSize.z - 1)
        return;

    int index = GridIndex(x, y, z, gridSize);
    
    if (gSDF.SampleLevel(samplerClamp, float3(x, y, z) / gridSize, 0) <= 0.0f)
        return;
    
    float pCenter = gPressure[index];

    // Check neighbors, apply solid boundary condition (use pCenter if solid)
    float P_right = gSDF.SampleLevel(samplerClamp, float3(x + 1, y, z) / gridSize, 0) <= 0.0f ? pCenter : gPressure[GridIndex(x + 1, y, z, gridSize)];
    float P_left = gSDF.SampleLevel(samplerClamp, float3(x - 1, y, z) / gridSize, 0) <= 0.0f ? pCenter : gPressure[GridIndex(x - 1, y, z, gridSize)];
    float P_up = gSDF.SampleLevel(samplerClamp, float3(x, y + 1, z) / gridSize, 0) <= 0.0f ? pCenter : gPressure[GridIndex(x, y + 1, z, gridSize)];
    float P_down = gSDF.SampleLevel(samplerClamp, float3(x, y - 1, z) / gridSize, 0) <= 0.0f ? pCenter : gPressure[GridIndex(x, y - 1, z, gridSize)];
    float P_front = gSDF.SampleLevel(samplerClamp, float3(x, y, z + 1) / gridSize, 0) <= 0.0f ? pCenter : gPressure[GridIndex(x, y, z + 1, gridSize)];
    float P_back = gSDF.SampleLevel(samplerClamp, float3(x, y, z - 1) / gridSize, 0) <= 0.0f ? pCenter : gPressure[GridIndex(x, y, z - 1, gridSize)];
    
    // compute new pressure using Jacobi iteration
    gNewPressure[index] = (P_right + P_left + P_up + P_down + P_front + P_back - gDivergence[index]) / 6;
}