#pragma once
#include "BaseMesh.hpp"

using namespace DirectX;

template <typename VertexType>
class CubeMesh : public BaseMesh<VertexType>
{
public:
	CubeMesh(ID3D11Device* device);
	virtual ~CubeMesh() = default;

protected:
	void InitBuffers(ID3D11Device* device);
};



template <typename T>
CubeMesh<T>::CubeMesh(ID3D11Device* device) : BaseMesh<T>()
{
    InitBuffers(device);
}

template <typename VertexType>
void CubeMesh<VertexType>::InitBuffers(ID3D11Device* device)
{
    // Each face has 4 unique vertices, making 24 total
    VertexType vertices[] =
    {
        // Front Face
        { { 0,  1,  0 }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f } }, // top-left
        { { 1,  1,  0 }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f } }, // top-right
        { { 1,  0,  0 }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f } }, // bottom-right
        { { 0,  0,  0 }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f } }, // bottom-left

        // Back Face
        { { 1,  1,  1 }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f } },
        { { 0,  1,  1 }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f } },
        { { 0,  0,  1 }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f } },
        { { 1,  0,  1 }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f } },

        // Left Face
        { { 0,  1,  1 }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
        { { 0,  1,  0 }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
        { { 0,  0,  0 }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
        { { 0,  0,  1 }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },

        // Right Face
        { {  1,  1,  0 }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f } },
        { {  1,  1,  1 }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f } },
        { {  1,  0,  1 }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f } },
        { {  1,  0,  0 }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },

        // Top Face
        { { 0,  1,  1 }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
        { { 1,  1,  1 }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
        { { 1,  1,  0 }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f } },
        { { 0,  1,  0 }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f } },

        // Bottom Face
        { { 0, 0, 0 }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f } },
        { { 1, 0, 0 }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f } },
        { { 1, 0, 1 }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } },
        { { 0, 0, 1 }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } }
    };

    // Indices for 6 faces, each with 2 triangles
    UINT indices[] =
    {
        0, 1, 2, 0, 2, 3,    // Front
        4, 5, 6, 4, 6, 7,    // Back
        8, 9, 10, 8, 10, 11, // Left
        12, 13, 14, 12, 14, 15, // Right
        16, 17, 18, 16, 18, 19, // Top
        20, 21, 22, 20, 22, 23  // Bottom
    };

    BaseMesh<VertexType>::CreateBuffers(device, vertices, sizeof(VertexType), 24, indices, 36);
}