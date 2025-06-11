StructuredBuffer<float3> gVorticity : register(t0);
StructuredBuffer<float> gVelocityX : register(t1);
StructuredBuffer<float> gVelocityY : register(t2);
StructuredBuffer<float> gVelocityZ : register(t3);

RWStructuredBuffer<float> gNewVelocityX : register(u0);
RWStructuredBuffer<float> gNewVelocityY : register(u1);
RWStructuredBuffer<float> gNewVelocityZ : register(u2);

cbuffer FluidParams : register(b0)
{
    float deltaTime;
};

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
    int3 gridSize = int3(simRes, simRes, simRes);

    int x = groupID.x * subdivision + groupThreadID.x;
    int y = groupID.y * subdivision + groupThreadID.y;
    int z = groupID.z * subdivision + groupThreadID.z;

    // do nothing on edge cells, idk gradient there
    if (x == 0 || y == 0 || z == 0 || x >= gridSize.x - 1 || y >= gridSize.y - 1 || z >= gridSize.z - 1)
        return;

    // Staggered velocity buffer sizes
    int3 gridSizeX = int3(simRes + 3, simRes + 2, simRes + 2);
    int3 gridSizeY = int3(simRes + 2, simRes + 3, simRes + 2);
    int3 gridSizeZ = int3(simRes + 2, simRes + 2, simRes + 3);

    // Compute omega(curl) gradient
    float magXp = length(gVorticity[GridIndex(x + 1, y, z, gridSize)]);
    float magXm = length(gVorticity[GridIndex(x - 1, y, z, gridSize)]);
    float magYp = length(gVorticity[GridIndex(x, y + 1, z, gridSize)]);
    float magYm = length(gVorticity[GridIndex(x, y - 1, z, gridSize)]);
    float magZp = length(gVorticity[GridIndex(x, y, z + 1, gridSize)]);
    float magZm = length(gVorticity[GridIndex(x, y, z - 1, gridSize)]);

    float3 gradMag = 0.5f * float3(magXp - magXm, magYp - magYm, magZp - magZm);
    float3 N = normalize(gradMag + 1e-5f); // Add epsilon to prevent divide-by-zero

    
    float confinementScale = 1.5f;
    float3 omega = gVorticity[GridIndex(x, y, z, gridSize)];
    float3 fConf = confinementScale * cross(N, omega);
    
    
    int3 samplePosStaggered = int3(x + 1, y + 1, z + 1);
    int indexX = GridIndex(samplePosStaggered.x, samplePosStaggered.y, samplePosStaggered.z, gridSizeX);
    int indexY = GridIndex(samplePosStaggered.x, samplePosStaggered.y, samplePosStaggered.z, gridSizeY);
    int indexZ = GridIndex(samplePosStaggered.x, samplePosStaggered.y, samplePosStaggered.z, gridSizeZ);
    
    float3 f0 = fConf, f1;
    
    // U component
    f1 = confinementScale * cross(N, gVorticity[GridIndex(x - 1, y, z, gridSize)]);
    gNewVelocityX[indexX] = gVelocityX[indexX] + 0.5f * (f0.x + f1.x) * deltaTime;
    
    // V component
    f1 = confinementScale * cross(N, gVorticity[GridIndex(x, y - 1, z, gridSize)]);
    gNewVelocityY[indexY] = gVelocityY[indexY] + 0.5f * (f0.y + f1.y) * deltaTime;
    
    // W component
    f1 = confinementScale * cross(N, gVorticity[GridIndex(x, y, z - 1, gridSize)]);
    gNewVelocityZ[indexZ] = gVelocityZ[indexZ] + 0.5f * (f0.z + f1.z) * deltaTime;
}