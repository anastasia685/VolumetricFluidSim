#pragma once
#include "BaseMesh.hpp"

template <typename VertexType>
class TriangleMesh : public BaseMesh<VertexType>
{
public:
	TriangleMesh(ID3D11Device* device);
	virtual ~TriangleMesh() = default;

protected:
	void InitBuffers(ID3D11Device* device);
};



template <typename T>
TriangleMesh<T>::TriangleMesh(ID3D11Device* device) : BaseMesh<T>()
{
	InitBuffers(device);
}

template <typename VertexType>
void TriangleMesh<VertexType>::InitBuffers(ID3D11Device* device)
{
	VertexType vertices[] =
	{
		{ {0.0f,   0.5f, 0.0f}, {0.0f,  0.0f, -1.0f}, {0.5f, 0.0f} }, // top
		{ {0.5f,  -0.5f, 0.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f} },  // right
		{ {-0.5f, -0.5f, 0.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f} } // left
	};

	UINT indices[] = { 0, 1, 2 };

	BaseMesh<VertexType>::CreateBuffers(device, vertices, sizeof(VertexType), 3, indices, 3);
}