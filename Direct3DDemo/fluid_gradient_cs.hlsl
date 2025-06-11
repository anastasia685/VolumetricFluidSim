StructuredBuffer<float> gPressure : register(t0);
StructuredBuffer<float> gVelocityX : register(t1);
StructuredBuffer<float> gVelocityY : register(t2);
StructuredBuffer<float> gVelocityZ : register(t3);
Texture3D<float> gSDF : register(t4);

RWStructuredBuffer<float> gNewVelocityX : register(u0);
RWStructuredBuffer<float> gNewVelocityY : register(u1);
RWStructuredBuffer<float> gNewVelocityZ : register(u2);

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
    
    int3 gridSize = int3(simRes + 2, simRes + 2, simRes + 2); // Pressure has ghost cells

    int x = groupID.x * subdivision + groupThreadID.x;
    int y = groupID.y * subdivision + groupThreadID.y;
    int z = groupID.z * subdivision + groupThreadID.z;
    
    if (x >= gridSize.x || y >= gridSize.y || z >= gridSize.z)
        return;
    
    int centerIndex = GridIndex(x, y, z, gridSize);
    
    float cellSolid = gSDF.SampleLevel(samplerClamp, float3(x, y, z) / gridSize, 0);
    if (cellSolid <= 0.0f)
        return;
    
    float pCenter = gPressure[centerIndex];
    
    
    // **U-Velocity Update (X-Staggered)**
    if (x > 0)
    {
        int3 gridSizeX = int3(simRes + 3, simRes + 2, simRes + 2);
        int U_index = GridIndex(x, y, z, gridSizeX); // Staggered in X
        
        int pLeftIndex = GridIndex(x - 1, y, z, gridSize);
        float pLeft = gSDF.SampleLevel(samplerClamp, float3(x - 1, y, z) / gridSize, 0) <= 0.0f ? pCenter : gPressure[pLeftIndex];
        float pRight = pCenter;

        gNewVelocityX[U_index] = gVelocityX[U_index] - (pRight - pLeft);
    }
    // **V-Velocity Update (Y-Staggered)**
    if (y > 0)
    {
        int3 gridSizeY = int3(simRes + 2, simRes + 3, simRes + 2);
        int V_index = GridIndex(x, y, z, gridSizeY); // Staggered in Y
        
        int pDownIndex = GridIndex(x, y - 1, z, gridSize);
        float pDown = gSDF.SampleLevel(samplerClamp, float3(x, y - 1, z) / gridSize, 0) <= 0.0f ? pCenter : gPressure[pDownIndex];
        float pUp = pCenter;

        gNewVelocityY[V_index] = gVelocityY[V_index] - (pUp - pDown);
    }

    // **W-Velocity Update (Z-Staggered)**
    if (z > 0)
    {
        int3 gridSizeZ = int3(simRes + 2, simRes + 2, simRes + 3);
        int W_index = GridIndex(x, y, z, gridSizeZ); // Staggered in Z
        
        int pBackIndex = GridIndex(x, y, z - 1, gridSize);
        float pBack = gSDF.SampleLevel(samplerClamp, float3(x, y, z - 1) / gridSize, 0) <= 0.0f ? pCenter : gPressure[pBackIndex];
        float pFront = pCenter;

        gNewVelocityZ[W_index] = gVelocityZ[W_index] - (pFront - pBack);
    }
}