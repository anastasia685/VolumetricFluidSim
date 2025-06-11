#pragma once
#include "pch.h"
#include "ReadData.h"
#include "ConstantBuffer.hpp"

using namespace DirectX;

namespace CustomEffects
{
	class DisplacementEffect
	{
	public:
		explicit DisplacementEffect(ID3D11Device* device, const std::wstring& csPath, const std::wstring& sdfCsPath, const XMFLOAT3& bufferDimensions)
		{
			m_displacementPropsBuffer = std::make_unique<ConstantBuffer<FractalNoiseBufferType>>();
			m_sdfPropsBuffer = std::make_unique<ConstantBuffer<SDFBufferType>>();

			CreateConstantBuffers(device);

			CreateComputeShader(device, csPath, sdfCsPath, bufferDimensions);
		}

		void Compute(ID3D11DeviceContext* deviceContext, int x, int y, int z)
		{
			SetConstantBuffers(deviceContext);
			SetResourceViews(deviceContext);

			deviceContext->CSSetShader(m_cs.Get(), nullptr, 0);
			deviceContext->Dispatch(x, y, z);

			Unbind(deviceContext, 2);

			// sdf
			SetSDFConstantBuffers(deviceContext);
			SetSDFResourceViews(deviceContext);

			deviceContext->CSSetShader(m_sdfCs.Get(), nullptr, 0);
			deviceContext->Dispatch(16, 16, 16); // for 64x64x64 field

			Unbind(deviceContext, 1);
		}

		Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> GetDisplacementSrv() const { return m_displacementSRV; };
		Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> GetNormalSrv() const { return m_normalSRV; };
		Microsoft::WRL::ComPtr <ID3D11ShaderResourceView> GetSDFSrv() const { return m_sdfSRV; };

		float GetFrequency() const { return m_frequency; };
		float GetAmplitude() const { return m_amplitude; };
		float GetLacunarity() const { return m_lacunarity; };
		float GetGain() const { return m_gain; };
		XMFLOAT3 GetOffset() const { return m_offset; };
		int GetOctaves() const { return m_octaves; };

		void SetFrequency(float frequency) { m_frequency = frequency; };
		void SetAmplitude(float amplitude) { m_amplitude = amplitude; };
		void SetLacunarity(float lacunarity) { m_lacunarity = lacunarity; };
		void SetGain(float gain) { m_gain = gain; };
		void SetOffset(const XMFLOAT3 offset) { m_offset = offset; };
		void SetOctaves(int octaves) { m_octaves = octaves; };

		float EstimateMax()
		{
			float maxVal = 0;
			float amplitude = m_amplitude;
			for (int i = 0; i < m_octaves; i++)
			{
				maxVal += amplitude;
				amplitude *= m_gain;
			}
			return maxVal;
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_cs, m_sdfCs;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_displacementBuffer, m_normalBuffer, m_sdfBuffer;
		Microsoft::WRL::ComPtr<ID3D11Texture3D> m_sdfTexture;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_displacementUAV;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_normalUAV;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_sdfUAV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_displacementSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_normalSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sdfSRV;

		std::unique_ptr<ConstantBuffer<FractalNoiseBufferType>> m_displacementPropsBuffer;
		std::unique_ptr<ConstantBuffer<SDFBufferType>> m_sdfPropsBuffer;

		float m_frequency = 3.28f, m_amplitude = 0.24f, m_lacunarity = 1.6f, m_gain = 0.6f;
		XMFLOAT3 m_offset{ 2, 180, 0 };
		int m_octaves = 8;

		void CreateComputeShader(ID3D11Device* device, const std::wstring& filepath, const std::wstring& sdfFilepath, const XMFLOAT3& bufferDimensions)
		{
			auto csBlob = DX::ReadData(filepath.c_str());
			DX::ThrowIfFailed(device->CreateComputeShader(csBlob.data(), csBlob.size(), nullptr, m_cs.ReleaseAndGetAddressOf()));

			auto sdfCsBlob = DX::ReadData(sdfFilepath.c_str());
			DX::ThrowIfFailed(device->CreateComputeShader(sdfCsBlob.data(), sdfCsBlob.size(), nullptr, m_sdfCs.ReleaseAndGetAddressOf()));

			CreateResourceViews(device, bufferDimensions);
		}
		void CreateConstantBuffers(ID3D11Device* device)
		{
			m_displacementPropsBuffer->Initialize(device);
			m_sdfPropsBuffer->Initialize(device);
		}
		void CreateResourceViews(ID3D11Device* device, const XMFLOAT3& bufferDimensions)
		{
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(float) * (bufferDimensions.x) * (bufferDimensions.y) * (bufferDimensions.z);
			bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			bufferDesc.StructureByteStride = sizeof(float);
			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_displacementBuffer.GetAddressOf()));

			bufferDesc.ByteWidth = 3 * sizeof(float) * (bufferDimensions.x) * (bufferDimensions.y) * (bufferDimensions.z);
			bufferDesc.StructureByteStride = 3 * sizeof(float);
			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_normalBuffer.GetAddressOf()));

			/*int sdfDimension = 32;
			bufferDesc.ByteWidth = sizeof(float) * (sdfDimension) * (sdfDimension) * (sdfDimension);
			bufferDesc.StructureByteStride = sizeof(float);
			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_sdfBuffer.GetAddressOf()));*/


			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = (bufferDimensions.x) * (bufferDimensions.y) * (bufferDimensions.z);
			uavDesc.Buffer.Flags = 0;
			device->CreateUnorderedAccessView(m_displacementBuffer.Get(), &uavDesc, m_displacementUAV.GetAddressOf());
			device->CreateUnorderedAccessView(m_normalBuffer.Get(), &uavDesc, m_normalUAV.GetAddressOf());

			/*uavDesc.Buffer.NumElements = (sdfDimension) * (sdfDimension) * (sdfDimension);
			device->CreateUnorderedAccessView(m_sdfBuffer.Get(), &uavDesc, m_sdfUAV.GetAddressOf());*/


			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = (bufferDimensions.x) * (bufferDimensions.y) * (bufferDimensions.z);
			device->CreateShaderResourceView(m_displacementBuffer.Get(), &srvDesc, m_displacementSRV.GetAddressOf());
			device->CreateShaderResourceView(m_normalBuffer.Get(), &srvDesc, m_normalSRV.GetAddressOf());

			/*srvDesc.Buffer.NumElements = (sdfDimension) * (sdfDimension) * (sdfDimension);
			device->CreateShaderResourceView(m_sdfBuffer.Get(), &srvDesc, m_sdfSRV.GetAddressOf());*/


			int sdfDimension = 64;
			D3D11_TEXTURE3D_DESC texDesc;
			texDesc.Width = sdfDimension;
			texDesc.Height = sdfDimension;
			texDesc.Depth = sdfDimension;
			texDesc.MipLevels = 1;
			texDesc.Format = DXGI_FORMAT_R32_FLOAT;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;
			DX::ThrowIfFailed(device->CreateTexture3D(&texDesc, nullptr, m_sdfTexture.GetAddressOf()));

			D3D11_UNORDERED_ACCESS_VIEW_DESC textureUavDesc = {};
			textureUavDesc.Format = texDesc.Format;
			textureUavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
			textureUavDesc.Texture3D.MipSlice = 0;
			textureUavDesc.Texture3D.FirstWSlice = 0;
			textureUavDesc.Texture3D.WSize = sdfDimension; // depth
			DX::ThrowIfFailed(device->CreateUnorderedAccessView(m_sdfTexture.Get(), &textureUavDesc, m_sdfUAV.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC textureSrvDesc = {};
			textureSrvDesc.Format = texDesc.Format;
			textureSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			textureSrvDesc.Texture3D.MipLevels = 1;
			textureSrvDesc.Texture3D.MostDetailedMip = 0;
			DX::ThrowIfFailed(device->CreateShaderResourceView(m_sdfTexture.Get(), &textureSrvDesc, m_sdfSRV.GetAddressOf()));
		}
		void SetConstantBuffers(ID3D11DeviceContext* deviceContext)
		{
			// set
			m_displacementPropsBuffer->Apply(deviceContext, { m_frequency, m_amplitude, m_lacunarity, m_gain, m_offset, m_octaves });
			// bind
			deviceContext->CSSetConstantBuffers(0, 1, m_displacementPropsBuffer->GetAddressOf());
		}
		void SetSDFConstantBuffers(ID3D11DeviceContext* deviceContext)
		{
			// max displacement estimation funciton
			float max = EstimateMax();

			// set
			m_sdfPropsBuffer->Apply(deviceContext, { -max, max });
			// bind
			deviceContext->CSSetConstantBuffers(0, 1, m_sdfPropsBuffer->GetAddressOf());
		}
		void SetResourceViews(ID3D11DeviceContext* deviceContext)
		{
			ID3D11UnorderedAccessView* uavs[] = {m_displacementUAV.Get(), m_normalUAV.Get()};
			deviceContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);
		}
		void SetSDFResourceViews(ID3D11DeviceContext* deviceContext)
		{
			deviceContext->CSSetUnorderedAccessViews(0, 1, m_sdfUAV.GetAddressOf(), nullptr);
			deviceContext->CSSetShaderResources(0, 1, m_displacementSRV.GetAddressOf());
		}

		void Unbind(ID3D11DeviceContext* deviceContext, UINT count)
		{
			std::vector<ID3D11ShaderResourceView*> nullSRVs(count, nullptr);
			deviceContext->CSSetShaderResources(0, count, nullSRVs.data());

			std::vector<ID3D11UnorderedAccessView*> nullUAVs(count, nullptr);
			deviceContext->CSSetUnorderedAccessViews(0, count, nullUAVs.data(), nullptr);

			// Disable Compute Shader
			deviceContext->CSSetShader(nullptr, nullptr, 0);
		}
	};
}
