RWStructuredBuffer<float3> gCurl : register(u0); // Output vorticity at cell centers

StructuredBuffer<float> gVelocityX : register(t0);
StructuredBuffer<float> gVelocityY : register(t1);
StructuredBuffer<float> gVelocityZ : register(t2);

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

    int3 gridSize = int3(simRes, simRes, simRes); // no ghost cells
    int x = groupID.x * subdivision + groupThreadID.x;
    int y = groupID.y * subdivision + groupThreadID.y;
    int z = groupID.z * subdivision + groupThreadID.z;

    // **Do not solve for ghost cells **
    if (x >= gridSize.x || y >= gridSize.y || z >= gridSize.z)
        return;

    int3 gridSizeX = int3(simRes + 3, simRes + 2, simRes + 2);
    int3 gridSizeY = int3(simRes + 2, simRes + 3, simRes + 2);
    int3 gridSizeZ = int3(simRes + 2, simRes + 2, simRes + 3);

    
    float dw_dy = (gVelocityZ[GridIndex(x + 1, y + 2, z + 1, gridSizeZ)] - gVelocityZ[GridIndex(x + 1, y, z + 1, gridSizeZ)]) * 0.5f;
    float dv_dz = (gVelocityY[GridIndex(x + 1, y + 1, z + 2, gridSizeY)] - gVelocityY[GridIndex(x + 1, y + 1, z, gridSizeY)]) * 0.5f;

    float du_dz = (gVelocityX[GridIndex(x + 1, y + 1, z + 2, gridSizeX)] - gVelocityX[GridIndex(x + 1, y + 1, z, gridSizeX)]) * 0.5f;
    float dw_dx = (gVelocityZ[GridIndex(x + 2, y + 1, z + 1, gridSizeZ)] - gVelocityZ[GridIndex(x, y + 1, z + 1, gridSizeZ)]) * 0.5f;

    float dv_dx = (gVelocityY[GridIndex(x + 2, y + 1, z + 1, gridSizeY)] - gVelocityY[GridIndex(x, y + 1, z + 1, gridSizeY)]) * 0.5f;
    float du_dy = (gVelocityX[GridIndex(x + 1, y + 2, z + 1, gridSizeX)] - gVelocityX[GridIndex(x + 1, y, z + 1, gridSizeX)]) * 0.5f;

    float3 omega;
    omega.x = dw_dy - dv_dz;
    omega.y = du_dz - dw_dx;
    omega.z = dv_dx - du_dy;

    gCurl[GridIndex(x, y, z, gridSize)] = omega;
}