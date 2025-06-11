cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};
StructuredBuffer<float> heightBuffer : register(t0);
StructuredBuffer<float3> normalBuffer : register(t1);

struct DS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 wPosition : POSITION;
    float lHeight : TEXCOORD1;
    matrix worldMatrix : TEXCOORD2;
};

struct HS_CONTROL_POINT_OUTPUT
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]			: SV_TessFactor;
	float InsideTessFactor[2]		: SV_InsideTessFactor;
};


float sampleHeightBuffer(float2 localCoords)
{
    float resolution = 17;
    float quality = 8;
    
    // scale localCoords from [0, resolution] to match the (resolution*quality x resolution*quality) grid space
    float2 gridCoords = localCoords * quality;
    int dimension = resolution * quality;

    // split into whole and fractional parts
    float2 gridIndex = floor(gridCoords);
    float2 fracIndex = frac(gridCoords);

    // clamp gridIndex to bounds
    gridIndex = clamp(gridIndex, float2(0, 0), float2((dimension) - 2, (dimension) - 2));

    // calculate indices of neighbors to access 1d height buffer
    int indexBL = int(gridIndex.y) * (dimension) + int(gridIndex.x);
    int indexBR = int(gridIndex.y) * (dimension) + int(gridIndex.x + 1);
    int indexTL = int(gridIndex.y + 1) * (dimension) + int(gridIndex.x);
    int indexTR = int(gridIndex.y + 1) * (dimension) + int(gridIndex.x + 1);

    // sample heights
    float heightBL = heightBuffer[indexBL];
    float heightBR = heightBuffer[indexBR];
    float heightTL = heightBuffer[indexTL];
    float heightTR = heightBuffer[indexTR];

    // bilinear interpolation
    float topHeight = lerp(heightTL, heightTR, fracIndex.x);
    float bottomHeight = lerp(heightBL, heightBR, fracIndex.x);
    float interpolatedHeight = lerp(bottomHeight, topHeight, fracIndex.y);

    return interpolatedHeight;
}

#define NUM_CONTROL_POINTS 4
[domain("quad")]
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	//float3 domain : SV_DomainLocation,
    float2 uv : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT Output;

    //Output.position = patch[0].position * domain.x + patch[1].position * domain.y + patch[2].position * domain.z;
    Output.position = lerp(
        lerp(patch[0].position, patch[1].position, uv.x),
        lerp(patch[3].position, patch[2].position, uv.x),
        uv.y
    );
    //Output.normal = patch[0].normal * domain.x + patch[1].normal * domain.y + patch[2].normal * domain.z;
    //Output.normal = normalize(Output.normal);
    //Output.tex = patch[0].tex * domain.x + patch[1].tex * domain.y + patch[2].tex * domain.z;
    Output.tex = lerp(
        lerp(patch[0].tex, patch[1].tex, uv.x),
        lerp(patch[3].tex, patch[2].tex, uv.x),
        uv.y
    );
    
    float resolution = 17.f;
    float height = sampleHeightBuffer(Output.tex * (resolution - 1));
    Output.position.y += height;
    Output.lHeight = Output.position.y;
    
    //float3 normal = sampleNormalBuffer(Output.tex * (resolution - 1));
    //Output.normal = normalize(mul(normal, (float3x3) worldMatrix));;
    
    
    Output.position = mul(Output.position, worldMatrix);
    Output.wPosition = Output.position.xyz;
    Output.position = mul(Output.position, viewMatrix);
    Output.position = mul(Output.position, projectionMatrix);
    
    Output.worldMatrix = worldMatrix;

	return Output;
}
