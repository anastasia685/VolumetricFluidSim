#pragma once
#include "pch.h"
#include <unordered_map>
#include "VertexTypes.h"


template <typename VertexType>
class BaseMesh
{
public:
	BaseMesh();
	virtual ~BaseMesh() = default;

	
	void Draw(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
	{
		deviceContext->IASetPrimitiveTopology(top);

		UINT stride = sizeof(VertexType);
		UINT offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

		if (indexBuffer)
		{
			deviceContext->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			deviceContext->DrawIndexed(indexCount, 0, 0);
		}
		else
		{
			deviceContext->Draw(indexCount, 0);
		}
	}
	int getIndexCount() const;

protected:
	//virtual void InitBuffers(ID3D11Device* device) = 0;
	void CreateBuffers(ID3D11Device* device, const void* vertices, UINT vertexSize, UINT vertexCount, const UINT* indices, UINT indexCount);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer, indexBuffer;
	int indexCount;
};


template <typename T>
BaseMesh<T>::BaseMesh()
{

}

template <typename T>
int BaseMesh<T>::getIndexCount() const
{
	return indexCount;
}

template <typename VertexType>
void BaseMesh<VertexType>::CreateBuffers(ID3D11Device* device, const void* vertices, UINT vertexSize, UINT vertexCount, const UINT* indices, UINT indexCount)
{
	this->indexCount = indexCount;

	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = vertexSize * vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vInitData = {};
	vInitData.pSysMem = vertices;

	DX::ThrowIfFailed(device->CreateBuffer(&vertexBufferDesc, &vInitData, vertexBuffer.ReleaseAndGetAddressOf()));

	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(UINT) * indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iInitData = {};
	iInitData.pSysMem = indices;

	DX::ThrowIfFailed(device->CreateBuffer(&indexBufferDesc, &iInitData, indexBuffer.ReleaseAndGetAddressOf()));
}
