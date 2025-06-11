#pragma once
#include "pch.h"
#include "ReadData.h"
#include "ConstantBuffer.hpp"

using namespace DirectX;

namespace CustomEffects
{
	class SDFEffect
	{
	public:
		explicit SDFEffect(ID3D11Device* device, const std::wstring& csPath, const std::wstring& gradientCsPath, const XMFLOAT3& bufferDimensions)
		{
			m_bufferDimensions = bufferDimensions;
			m_sdfParamsBuffer = std::make_unique<ConstantBuffer<SolidMaskBufferType>>();
			m_sceneObjects = std::vector<SceneObjectData>();


			CreateConstantBuffers(device);

			CreateComputeShader(device, csPath, gradientCsPath);
		}

		void Compute(ID3D11DeviceContext* deviceContext, int x, int y, int z)
		{
			SetConstantBuffers(deviceContext);
			SetResourceViews(deviceContext);

			deviceContext->CSSetShader(m_cs.Get(), nullptr, 0);
			deviceContext->Dispatch(x + 1, y + 1, z + 1); // account for ghost cells

			Unbind(deviceContext, 2);

			// gradient calculation
			SetGradientResourceViews(deviceContext);
			deviceContext->CSSetShader(m_gradientCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x + 1, y + 1, z + 1); // account for ghost cells

			Unbind(deviceContext, 1);


			CopyToCPU(deviceContext);
		}

		void CopyToCPU(ID3D11DeviceContext* deviceContext)
		{
			deviceContext->CopyResource(m_stagingTexture.Get(), m_texture.Get());

			D3D11_MAPPED_SUBRESOURCE mapped;
			DX::ThrowIfFailed(deviceContext->Map(m_stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped));

			const uint8_t* data = reinterpret_cast<const uint8_t*>(mapped.pData);
			m_cpuSDF.resize((m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 2));
			for (UINT z = 0; z < m_bufferDimensions.z + 2; ++z)
			{
				for (UINT y = 0; y < m_bufferDimensions.y + 2; ++y)
				{
					const float* rowData = reinterpret_cast<const float*>(
						data + z * mapped.DepthPitch + y * mapped.RowPitch);

					for (UINT x = 0; x < m_bufferDimensions.x + 2; ++x)
					{
						size_t index = z * (m_bufferDimensions.y + 2) * (m_bufferDimensions.x + 2) + y * (m_bufferDimensions.x + 2) + x;
						m_cpuSDF[index] = rowData[x];
					}
				}
			}
			deviceContext->Unmap(m_stagingTexture.Get(), 0);



			// gradient
			deviceContext->CopyResource(m_stagingGradientTexture.Get(), m_gradientTexture.Get());

			DX::ThrowIfFailed(deviceContext->Map(m_stagingGradientTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped));

			data = reinterpret_cast<const uint8_t*>(mapped.pData);
			m_cpuSDFGradient.resize((m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 2));
			for (UINT z = 0; z < m_bufferDimensions.z + 2; ++z)
			{
				for (UINT y = 0; y < m_bufferDimensions.y + 2; ++y)
				{
					const XMFLOAT4* rowData = reinterpret_cast<const XMFLOAT4*>(
						data + z * mapped.DepthPitch + y * mapped.RowPitch);

					for (UINT x = 0; x < m_bufferDimensions.x + 2; ++x)
					{
						size_t index = z * (m_bufferDimensions.y + 2) * (m_bufferDimensions.x + 2) +
							y * (m_bufferDimensions.x + 2) +
							x;

						// You can store this however you want — e.g. XMFLOAT3, glm::vec3, or float[3]
						m_cpuSDFGradient[index] = XMFLOAT3(rowData[x].x, rowData[x].y, rowData[x].z);
					}
				}
			}
			deviceContext->Unmap(m_stagingGradientTexture.Get(), 0);
		}

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSrv() const { return m_srv; };
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetGradientSrv() const { return m_gradientSrv; };
		std::vector<float> GetCPUSdf() const { return m_cpuSDF; };
		std::vector<XMFLOAT3> GetCPUSdfGradient() const { return m_cpuSDFGradient; };

		void SetSimulationTransform(const XMMATRIX transform) { m_simulationTransform = transform; };
		void AddSceneObject(ID3D11Device* device, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sdfSRV, const XMMATRIX transform, float uniformScale) 
		{  
			m_sceneObjects.push_back({ sdfSRV , transform, uniformScale});
		}

	private:
		struct SceneObjectData
		{
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sdf;
			XMMATRIX transformMatrix;
			float uniformScale;
		};

		XMFLOAT3 m_bufferDimensions;

		Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_cs, m_gradientCs;

		Microsoft::WRL::ComPtr<ID3D11Texture3D> m_texture, m_stagingTexture;
		Microsoft::WRL::ComPtr<ID3D11Texture3D> m_gradientTexture, m_stagingGradientTexture;

		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_uav;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_gradientUav;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_gradientSrv;

		std::vector<SceneObjectData> m_sceneObjects;

		XMMATRIX m_simulationTransform;
		std::unique_ptr<ConstantBuffer<SolidMaskBufferType>> m_sdfParamsBuffer;


		std::vector<float> m_cpuSDF;
		std::vector<XMFLOAT3> m_cpuSDFGradient;


		void CreateComputeShader(ID3D11Device* device, const std::wstring& filepath, const std::wstring& gradientFilepath)
		{
			auto csBlob = DX::ReadData(filepath.c_str());
			DX::ThrowIfFailed(device->CreateComputeShader(csBlob.data(), csBlob.size(), nullptr, m_cs.ReleaseAndGetAddressOf()));

			auto gradientCsBlob = DX::ReadData(gradientFilepath.c_str());
			DX::ThrowIfFailed(device->CreateComputeShader(gradientCsBlob.data(), gradientCsBlob.size(), nullptr, m_gradientCs.ReleaseAndGetAddressOf()));

			CreateResourceViews(device);
		}
		void CreateConstantBuffers(ID3D11Device* device)
		{
			m_sdfParamsBuffer->Initialize(device);
		}
		void CreateResourceViews(ID3D11Device* device)
		{
			D3D11_TEXTURE3D_DESC textureDesc;
			textureDesc.Width = m_bufferDimensions.x + 2;
			textureDesc.Height = m_bufferDimensions.y + 2;
			textureDesc.Depth = m_bufferDimensions.z + 2;
			textureDesc.MipLevels = 1;
			textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
			textureDesc.Usage = D3D11_USAGE_DEFAULT;
			textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			textureDesc.CPUAccessFlags = 0;
			textureDesc.MiscFlags = 0;
			DX::ThrowIfFailed(device->CreateTexture3D(&textureDesc, nullptr, m_texture.GetAddressOf()));

			D3D11_TEXTURE3D_DESC stagingTextureDesc;
			stagingTextureDesc.Width = m_bufferDimensions.x + 2;
			stagingTextureDesc.Height = m_bufferDimensions.y + 2;
			stagingTextureDesc.Depth = m_bufferDimensions.z + 2;
			stagingTextureDesc.MipLevels = 1;
			stagingTextureDesc.Format = DXGI_FORMAT_R32_FLOAT;
			stagingTextureDesc.Usage = D3D11_USAGE_STAGING;
			stagingTextureDesc.BindFlags = 0;
			stagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			stagingTextureDesc.MiscFlags = 0;
			DX::ThrowIfFailed(device->CreateTexture3D(&stagingTextureDesc, nullptr, m_stagingTexture.GetAddressOf()));

			D3D11_UNORDERED_ACCESS_VIEW_DESC textureUavDesc = {};
			textureUavDesc.Format = textureDesc.Format;
			textureUavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
			textureUavDesc.Texture3D.MipSlice = 0;
			textureUavDesc.Texture3D.FirstWSlice = 0;
			textureUavDesc.Texture3D.WSize = m_bufferDimensions.z + 2; // depth
			DX::ThrowIfFailed(device->CreateUnorderedAccessView(m_texture.Get(), &textureUavDesc, m_uav.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC textureSrvDesc = {};
			textureSrvDesc.Format = textureDesc.Format;
			textureSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			textureSrvDesc.Texture3D.MipLevels = 1;
			textureSrvDesc.Texture3D.MostDetailedMip = 0;
			DX::ThrowIfFailed(device->CreateShaderResourceView(m_texture.Get(), &textureSrvDesc, m_srv.GetAddressOf()));



			textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			DX::ThrowIfFailed(device->CreateTexture3D(&textureDesc, nullptr, m_gradientTexture.GetAddressOf()));

			stagingTextureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			DX::ThrowIfFailed(device->CreateTexture3D(&stagingTextureDesc, nullptr, m_stagingGradientTexture.GetAddressOf()));

			textureUavDesc.Format = textureDesc.Format;
			DX::ThrowIfFailed(device->CreateUnorderedAccessView(m_gradientTexture.Get(), &textureUavDesc, m_gradientUav.GetAddressOf()));

			textureSrvDesc.Format = textureDesc.Format;
			DX::ThrowIfFailed(device->CreateShaderResourceView(m_gradientTexture.Get(), &textureSrvDesc, m_gradientSrv.GetAddressOf()));
		}
		void SetConstantBuffers(ID3D11DeviceContext* deviceContext)
		{
			// set
			m_sdfParamsBuffer->Apply(deviceContext, { XMMatrixTranspose(m_simulationTransform), XMMatrixTranspose(XMMatrixInverse(nullptr, m_sceneObjects[0].transformMatrix)), m_sceneObjects[0].uniformScale});
			// bind
			deviceContext->CSSetConstantBuffers(0, 1, m_sdfParamsBuffer->GetAddressOf());
		}
		void SetResourceViews(ID3D11DeviceContext* deviceContext)
		{
			deviceContext->CSSetUnorderedAccessViews(0, 1, m_uav.GetAddressOf(), nullptr);

			for (int i = 0; i < m_sceneObjects.size(); i++)
			{
				deviceContext->CSSetShaderResources(0, 1, m_sceneObjects[i].sdf.GetAddressOf());
			}
		}

		void SetGradientResourceViews(ID3D11DeviceContext* deviceContext)
		{
			deviceContext->CSSetUnorderedAccessViews(0, 1, m_gradientUav.GetAddressOf(), nullptr);
			deviceContext->CSSetShaderResources(0, 1, m_srv.GetAddressOf());
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
