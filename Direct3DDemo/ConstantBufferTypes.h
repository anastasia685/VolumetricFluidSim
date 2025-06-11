#pragma once
#include "pch.h"

struct MatrixBufferType
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
};

struct LightBufferType
{
	DirectX::XMFLOAT3 ambientColor;
	float ambientIntensity;
	DirectX::XMFLOAT3 sunColor;
	float sunIntensity;
	DirectX::XMFLOAT3 sunDirection;
};

struct CameraBufferType
{
	DirectX::XMFLOAT3 cameraPos;
};

struct VolumeBufferType
{
	DirectX::XMMATRIX mainCameraViewInv;
	DirectX::XMMATRIX mainCameraProjInv;
	float absorption;
	float scatter;
};

struct FluidBufferType
{
	float deltaTime;
	float elapsedTime;
};

struct FractalNoiseBufferType
{
	float frequency;
	float amplitude;
	float lacunarity;
	float gain;
	DirectX::XMFLOAT3 offset;
	int octaves;
};

struct SDFBufferType
{
	float minY;
	float maxY;
};

struct SolidMaskBufferType
{
	DirectX::XMMATRIX simulationTransform;
	DirectX::XMMATRIX sdfTransformInv0;
	float uniformScale0;
};