RWTexture3D<float4> gSDFGradient : register(u0);
Texture3D<float> gSDF : register(t0);

SamplerState samplerClamp : register(s0);

[numthreads(4, 4, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    int subdivision = 4;
    int dimension = 16;
    int simRes = dimension * subdivision;
    int3 gridSize = int3(simRes + 2, simRes + 2, simRes + 2); // account for ghost cells
    
    int x = groupID.x * subdivision + groupThreadID.x;
    int y = groupID.y * subdivision + groupThreadID.y;
    int z = groupID.z * subdivision + groupThreadID.z;
    
    
    // clamp to avoid out-of-bounds reads
    int x0 = max(x - 1, 0);
    int x1 = min(x + 1, gridSize.x - 1);

    int y0 = max(y - 1, 0);
    int y1 = min(y + 1, gridSize.y - 1);

    int z0 = max(z - 1, 0);
    int z1 = min(z + 1, gridSize.z - 1);
    
    
    float3 worldSize = float3(16, 16, 16); // total span in world units
    float3 voxelSize = worldSize / float3(gridSize); // delta per axis

    // Compute central differences
    float dx = float(gSDF.SampleLevel(samplerClamp, float3(x1, y, z) / gridSize, 0) - gSDF.SampleLevel(samplerClamp, float3(x0, y, z) / gridSize, 0)) / (2.0f * voxelSize.x);
    float dy = float(gSDF.SampleLevel(samplerClamp, float3(x, y1, z) / gridSize, 0) - gSDF.SampleLevel(samplerClamp, float3(x, y0, z) / gridSize, 0)) / (2.0f * voxelSize.y);
    float dz = float(gSDF.SampleLevel(samplerClamp, float3(x, y, z1) / gridSize, 0) - gSDF.SampleLevel(samplerClamp, float3(x, y, z0) / gridSize, 0)) / (2.0f * voxelSize.z);

    float3 gradient = float3(dx, dy, dz);
    
    gSDFGradient[float3(x, y, z)] = float4(gradient, 0);
}