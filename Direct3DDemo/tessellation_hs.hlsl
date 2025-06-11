cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};
cbuffer CameraBuffer : register(b1)
{
    float3 cameraPos;
};

struct VS_CONTROL_POINT_OUTPUT
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct HS_CONTROL_POINT_OUTPUT
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]			: SV_TessFactor;
	float InsideTessFactor[2]			: SV_InsideTessFactor;
};

#define NUM_CONTROL_POINTS 4

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;
    
    float2 distance = float2(2, 15);
    float2 factor = float2(2, 8);
    
    // edge0
    float4 edgeCenter = (ip[3].position + ip[0].position) * 0.5;
    float distRatio = smoothstep(distance.x, distance.y, length(mul(edgeCenter, worldMatrix).xyz - cameraPos));
    Output.EdgeTessFactor[0] = lerp(factor.y, factor.x, distRatio);

    // edge1
    edgeCenter = (ip[0].position + ip[1].position) * 0.5;
    distRatio = smoothstep(distance.x, distance.y, length(mul(edgeCenter, worldMatrix).xyz - cameraPos));
    Output.EdgeTessFactor[1] = lerp(factor.y, factor.x, distRatio);

    //edge2
    edgeCenter = (ip[1].position + ip[2].position) * 0.5;
    distRatio = smoothstep(distance.x, distance.y, length(mul(edgeCenter, worldMatrix).xyz - cameraPos));
    Output.EdgeTessFactor[2] = lerp(factor.y, factor.x, distRatio);
    
    //edge3
    edgeCenter = (ip[2].position + ip[3].position) * 0.5;
    distRatio = smoothstep(distance.x, distance.y, length(mul(edgeCenter, worldMatrix).xyz - cameraPos));
    Output.EdgeTessFactor[3] = lerp(factor.y, factor.x, distRatio);

    //avg
    Output.InsideTessFactor[0] = Output.InsideTessFactor[1] = (Output.EdgeTessFactor[0] + Output.EdgeTessFactor[1] + Output.EdgeTessFactor[2] + Output.EdgeTessFactor[3]) / 4.0;

	return Output;
}

[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main( 
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HS_CONTROL_POINT_OUTPUT Output;

	Output.position = ip[i].position;
	Output.tex = ip[i].tex;

	return Output;
}
