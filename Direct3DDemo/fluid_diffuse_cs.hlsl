RWStructuredBuffer<float> gNewDensity : register(u0);
StructuredBuffer<float> gInitDensity : register(t0);
StructuredBuffer<float> gDensity : register(t1);

cbuffer FluidParams : register(b0)
{
    float deltaTime;
}

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

    // don't solve for ghost cells
    if (x == 0 || x >= gridSize.x - 1 || y == 0 || y >= gridSize.y - 1 || z == 0 || z >= gridSize.z - 1)
        return;

    int index = GridIndex(x, y, z, gridSize);
    
    float diffusionRate = 0.03f;
    
    //float heightFactor = lerp(1.0, 1.5, saturate(y / (gridSize.y)));
    
    float a = deltaTime * diffusionRate;// * heightFactor;

    float sum =
        gDensity[GridIndex(x + 1, y, z, gridSize)] +
        gDensity[GridIndex(x - 1, y, z, gridSize)] +
        gDensity[GridIndex(x, y + 1, z, gridSize)] +
        gDensity[GridIndex(x, y - 1, z, gridSize)] +
        gDensity[GridIndex(x, y, z + 1, gridSize)] +
        gDensity[GridIndex(x, y, z - 1, gridSize)];
    
    gNewDensity[index] = (gInitDensity[index] + a * sum) / (1.0 + 6.0 * a);
}