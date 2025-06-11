RWStructuredBuffer<float> gNewVelocityX : register(u0);
RWStructuredBuffer<float> gNewVelocityY : register(u1);
RWStructuredBuffer<float> gNewVelocityZ : register(u2);

Texture3D<float> gSDF : register(t0);
//StructuredBuffer<float3> gSDFGradient : register(t1);

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
    
    int3 gridSizeX = int3(simRes + 3, simRes + 2, simRes + 2);
    int3 gridSizeY = int3(simRes + 2, simRes + 3, simRes + 2);
    int3 gridSizeZ = int3(simRes + 2, simRes + 2, simRes + 3);
    
    int3 gridSize = int3(simRes + 2, simRes + 2, simRes + 2);

    int x = groupID.x * subdivision + groupThreadID.x;
    int y = groupID.y * subdivision + groupThreadID.y;
    int z = groupID.z * subdivision + groupThreadID.z;
    
    if (x > gridSize.x - 1 || y > gridSize.y - 1 || z > gridSize.z - 1)
    {   
        return;
    }
    
    // not really necessary, but eh
    if (x == gridSize.x - 1)
    {
        gNewVelocityX[GridIndex(x + 1, y, z, gridSizeX)] = 0.0f;
    }
    if (y == gridSize.y - 1)
    {
        gNewVelocityY[GridIndex(x, y + 1, z, gridSizeY)] = 0.0f;
    }
    if (z == gridSize.z - 1)
    {
        gNewVelocityZ[GridIndex(x, y, z + 1, gridSizeZ)] = 0.0f;
    }
    
    
    float cellSolid = gSDF.SampleLevel(samplerClamp, float3(x, y, z) / gridSize, 0);
    if (cellSolid > 0.0f) // Only process fluid cells
    {
        //float3 cellNormal = gSDFGradient[GridIndex(x, y, z, gridSize)];
        
        // --- X-Velocity: Left face ---
        if (x == 0 || (x > 0 && gSDF.SampleLevel(samplerClamp, float3(x - 1, y, z) / gridSize, 0) <= 0.0f))
        {
            int uIndex = GridIndex(x, y, z, gridSizeX);
            gNewVelocityX[uIndex] = 0.0f;
        }

        // --- Y-Velocity: Bottom face ---
        if (y == 0 || (y > 0 && gSDF.SampleLevel(samplerClamp, float3(x, y - 1, z) / gridSize, 0) <= 0.0f))
        {
            int vIndex = GridIndex(x, y, z, gridSizeY);
            gNewVelocityY[vIndex] = 0.0f;
        }

        // --- Z-Velocity: Front face ---
        if (z == 0 || (z > 0 && gSDF.SampleLevel(samplerClamp, float3(x, y, z - 1) / gridSize, 0) <= 0.0f))
        {
            int wIndex = GridIndex(x, y, z, gridSizeZ);
            gNewVelocityZ[wIndex] = 0.0f;
        }
    }
    else // not really necessary either
    {
        // set every component to zero
        int uIndex = GridIndex(x, y, z, gridSizeX);
        int vIndex = GridIndex(x, y, z, gridSizeY);
        int wIndex = GridIndex(x, y, z, gridSizeZ);

        gNewVelocityX[uIndex] = 0.0f;
        gNewVelocityY[vIndex] = 0.0f;
        gNewVelocityZ[wIndex] = 0.0f;
    }
}