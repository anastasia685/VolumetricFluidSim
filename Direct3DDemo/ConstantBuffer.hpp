#pragma once
#include "pch.h"
#include "ConstantBufferTypes.h"


template <typename BufferType>
class ConstantBuffer
{
public:
	ConstantBuffer() {};

	void Initialize(ID3D11Device* device);
	void Apply(ID3D11DeviceContext* deviceContext, const BufferType& newData);

	ID3D11Buffer* const* GetAddressOf() const { return m_buffer.GetAddressOf(); }

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_buffer;
};


template <typename BufferType>
void ConstantBuffer<BufferType>::Initialize(ID3D11Device* device)
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(BufferType) + (16 - sizeof(BufferType) % 16);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_buffer.ReleaseAndGetAddressOf()));
}

template <typename BufferType>
void ConstantBuffer<BufferType>::Apply(ID3D11DeviceContext* deviceContext, const BufferType& newData)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	DX::ThrowIfFailed(deviceContext->Map(m_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

	CopyMemory(mappedResource.pData, &newData, sizeof(BufferType));

	deviceContext->Unmap(m_buffer.Get(), 0);
}