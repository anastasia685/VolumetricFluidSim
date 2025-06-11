RWStructuredBuffer<float> gDivergence : register(u0);

StructuredBuffer<float> gVelocityX : register(t0); // U-Velocity (Nx+1, Ny+2, Nz+2)
StructuredBuffer<float> gVelocityY : register(t1); // V-Velocity (Nx+2, Ny+1, Nz+2)
StructuredBuffer<float> gVelocityZ : register(t2); // W-Velocity (Nx+2, Ny+2, Nz+1)

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

    int3 gridSizeX = int3(simRes + 3, simRes + 2, simRes + 2);
    int3 gridSizeY = int3(simRes + 2, simRes + 3, simRes + 2);
    int3 gridSizeZ = int3(simRes + 2, simRes + 2, simRes + 3);
    int3 gridSize = int3(simRes + 2, simRes + 2, simRes + 2);

    int x = groupID.x * subdivision + groupThreadID.x;
    int y = groupID.y * subdivision + groupThreadID.y;
    int z = groupID.z * subdivision + groupThreadID.z;
    
    if (x >= gridSize.x || y >= gridSize.y || z >= gridSize.z)
        return;

    // **Compute Staggered Velocity Differences**
    float u_right = gVelocityX[GridIndex(x + 1, y, z, gridSizeX)];
    float u_left = gVelocityX[GridIndex(x, y, z, gridSizeX)];

    float v_up = gVelocityY[GridIndex(x, y + 1, z, gridSizeY)];
    float v_down = gVelocityY[GridIndex(x, y, z, gridSizeY)];

    float w_front = gVelocityZ[GridIndex(x, y, z + 1, gridSizeZ)];
    float w_back = gVelocityZ[GridIndex(x, y, z, gridSizeZ)];

    // **Compute divergence**
    float divergence = (u_right - u_left) + (v_up - v_down) + (w_front - w_back);
    
    int divergenceIndex = GridIndex(x, y, z, gridSize);
    gDivergence[divergenceIndex] = divergence;
}