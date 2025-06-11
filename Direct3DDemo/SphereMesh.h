#pragma once
#include "BaseMesh.hpp"

using namespace DirectX;

template <typename VertexType>
class SphereMesh : public BaseMesh<VertexType>
{
public:
	SphereMesh(ID3D11Device* device);
	virtual ~SphereMesh() = default;

protected:
	void InitBuffers(ID3D11Device* device);
};



template <typename T>
SphereMesh<T>::SphereMesh(ID3D11Device* device) : BaseMesh<T>()
{
    InitBuffers(device);
}

template <typename T>
void SphereMesh<T>::InitBuffers(ID3D11Device* device)
{
    
}