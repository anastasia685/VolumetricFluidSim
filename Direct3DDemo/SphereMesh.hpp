#pragma once
#include "BaseMesh.hpp"

using namespace DirectX;

template <typename VertexType>
class SphereMesh : public BaseMesh<VertexType>
{
public:
	SphereMesh(ID3D11Device* device, int slices = 16, int stacks = 16);
	virtual ~SphereMesh() = default;

protected:
	void InitBuffers(ID3D11Device* device, int slices, int stacks);
};


template <typename T>
SphereMesh<T>::SphereMesh(ID3D11Device* device, int slices, int stacks) : BaseMesh<T>()
{
    InitBuffers(device, slices, stacks);
}

template <typename VertexType>
void SphereMesh<VertexType>::InitBuffers(ID3D11Device* device, int slices, int stacks)
{
    std::vector<VertexType> vertices;
    std::vector<UINT> indices;

    float radius = 0.5f;

    // Generate vertices
    for (int stack = 0; stack <= stacks; ++stack)
    {
        float phi = XM_PI * stack / stacks; // Latitude angle from 0 to pi
        float y = radius * cosf(phi);
        float r = radius * sinf(phi); // Radius of the slice circle

        for (int slice = 0; slice <= slices; ++slice)
        {
            float theta = 2.0f * XM_PI * slice / slices; // Longitude angle from 0 to 2pi
            float x = r * cosf(theta);
            float z = r * sinf(theta);

            XMFLOAT3 position = XMFLOAT3(x, y, z);
            XMFLOAT3 normal = XMFLOAT3(x / radius, y / radius, z / radius); // Normalized
            XMFLOAT2 texcoord = XMFLOAT2((float)slice / slices, (float)stack / stacks);

            vertices.push_back({ position, normal, texcoord });
        }
    }

    // Generate indices
    for (int stack = 0; stack < stacks; ++stack)
    {
        for (int slice = 0; slice < slices; ++slice)
        {
            int current = stack * (slices + 1) + slice;
            int next = current + slices + 1;

            // First triangle
            indices.push_back(current);
            indices.push_back(current + 1);
            indices.push_back(next);

            // Second triangle
            indices.push_back(current + 1);
            indices.push_back(next + 1);
            indices.push_back(next);
        }
    }

    // Create buffers
    BaseMesh<VertexType>::CreateBuffers(device, vertices.data(), sizeof(VertexType), vertices.size(), indices.data(), indices.size());
}