StructuredBuffer<float> gDisplacement : register(t0); // 2D terrain heightmap
RWTexture3D<float> gSDF : register(u0); // 3D output

cbuffer SDFParams : register(b0)
{
    float _minY;
    float _maxY;
}

int GridIndex(int x, int y, int z, int3 size)
{
    return (z * size.y * size.x) + (y * size.x) + x;
}

[numthreads(4, 4, 4)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
    int subdivision = 4;
    int dimension = 16;
    int sdfRes = dimension * subdivision; //32
    
    int terrainSubdivision = 8;
    int terrainDimension = 17;
    int terrainRes = terrainDimension * terrainSubdivision; // 136
    
    int x = groupID.x * subdivision + groupThreadID.x;
    int y = groupID.y * subdivision + groupThreadID.y;
    int z = groupID.z * subdivision + groupThreadID.z;
    
    // Normalize voxel position [0,1]
    float3 voxelNorm = (float3(x, y, z) + float3(0.5f, 0.5f, 0.5f)) / sdfRes; // take voxel centers for sampling displacement buffer
    
    // Convert normalized voxel XZ to terrain local space
    int2 terrainPos = int2(voxelNorm.xz * (terrainRes - 1));
    terrainPos = clamp(terrainPos, int2(0, 0), int2(terrainRes - 1, terrainRes - 1));
    
    // Get terrain height at that XZ
    int terrainIndex = terrainPos.y * terrainRes + terrainPos.x;
    float terrainHeight = gDisplacement[terrainIndex];
    
    // Map normalized Y to terrain height range
    float minY = -0.5, maxY = 0.5;
    float normY = lerp(minY, maxY, voxelNorm.y);
    
    // Compute signed distance: positive = above terrain, negative = below
    float sdf = normY - terrainHeight;
    
    // Store in SDF buffer
    //int sdfIndex = GridIndex(x, y, z, int3(sdfRes, sdfRes, sdfRes));
    gSDF[float3(x, y, z)] = sdf;
}