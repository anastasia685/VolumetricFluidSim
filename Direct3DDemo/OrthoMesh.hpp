#pragma once
#include "BaseMesh.hpp"

template <typename VertexType>
class OrthoMesh : public BaseMesh<VertexType>
{
public:
	OrthoMesh(ID3D11Device* device);
	virtual ~OrthoMesh() = default;

protected:
	void InitBuffers(ID3D11Device* device);
};



template <typename T>
OrthoMesh<T>::OrthoMesh(ID3D11Device* device) : BaseMesh<T>()
{
	InitBuffers(device);
}

template <typename VertexType>
void OrthoMesh<VertexType>::InitBuffers(ID3D11Device* device)
{
	VertexType vertices[] =
	{
		{ {0.0f, 1.0f, 0.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 0.0f} }, // top left
		{ {1.0f, 1.0f, 0.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 0.0f} }, // top right
		{ {1.0f, 0.0f, 0.0f}, {0.0f,  0.0f, -1.0f}, {1.0f, 1.0f} },  // bottom right
		{ {0.0f, 0.0f, 0.0f}, {0.0f,  0.0f, -1.0f}, {0.0f, 1.0f} } // bottom left
	};

	UINT indices[] = { 0, 1, 2, 0, 2, 3 };

	BaseMesh<VertexType>::CreateBuffers(device, vertices, sizeof(VertexType), 4, indices, 6);
}
