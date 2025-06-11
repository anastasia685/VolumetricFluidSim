#pragma once
#include "BaseEffect.hpp"

namespace CustomEffects
{
	template <typename VertexType>
	class TerrainEffect : public BaseEffect<VertexType>
	{
	public:
		explicit TerrainEffect(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath, const std::wstring& hsPath, const std::wstring& dsPath) : BaseEffect<VertexType>(device, vsPath, psPath)
		{
			m_cameraBuffer = std::make_unique<ConstantBuffer<CameraBufferType>>();
			m_states = std::make_unique<CommonStates>(device);
			m_albedoTextureSrvs = std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>();
			m_albedoTextureSrvs.resize(5);

			CreateConstantBuffers(device);

			CreateHullShader(device, hsPath);
			CreateDomainShader(device, dsPath);
		}

		virtual void Apply(ID3D11DeviceContext* deviceContext) override
		{
			BaseEffect<VertexType>::Apply(deviceContext);


			deviceContext->HSSetShader(m_hs.Get(), nullptr, 0);
			deviceContext->DSSetShader(m_ds.Get(), nullptr, 0);
		}

		void Unbind(ID3D11DeviceContext* deviceContext)
		{
			// unbind heightmap srv for writing again
			ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL };
			deviceContext->DSSetShaderResources(0, 2, nullSRV);
		}

		void SetCameraPosition(const XMFLOAT3& cameraPos) { m_cameraPos = cameraPos; }
		void SetHeightmapSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_heightmapSrv = srv; }
		void SetNormalmapSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_normalmapSrv = srv; }
		void SetAlbedoTextureSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv, int index = 0) { m_albedoTextureSrvs[index] = srv; }

	protected:
		void CreateHullShader(ID3D11Device* device, const std::wstring& filepath)
		{
			auto hsBlob = DX::ReadData(filepath.c_str());
			DX::ThrowIfFailed(device->CreateHullShader(hsBlob.data(), hsBlob.size(), nullptr, m_hs.ReleaseAndGetAddressOf()));
		}
		void CreateDomainShader(ID3D11Device* device, const std::wstring& filepath)
		{
			auto dsBlob = DX::ReadData(filepath.c_str());
			DX::ThrowIfFailed(device->CreateDomainShader(dsBlob.data(), dsBlob.size(), nullptr, m_ds.ReleaseAndGetAddressOf()));
		}

		virtual void CreateConstantBuffers(ID3D11Device* device) override
		{
			// create constant buffers for this shader's specific stuff
			m_cameraBuffer->Initialize(device);
		}
		virtual void SetConstantBuffers(ID3D11DeviceContext* deviceContext) override
		{
			// call parent's set constant buffers
			BaseEffect<VertexType>::SetConstantBuffers(deviceContext);

			// set this instance's buffers
			// set
			m_cameraBuffer->Apply(deviceContext, { m_cameraPos });
			
			// bind
			deviceContext->HSSetConstantBuffers(0, 1, BaseEffect<VertexType>::m_matrixBuffer->GetAddressOf());
			deviceContext->HSSetConstantBuffers(1, 1, m_cameraBuffer->GetAddressOf());

			deviceContext->DSSetConstantBuffers(0, 1, BaseEffect<VertexType>::m_matrixBuffer->GetAddressOf());
			deviceContext->DSSetShaderResources(0, 1, m_heightmapSrv.GetAddressOf());
			deviceContext->DSSetShaderResources(1, 1, m_normalmapSrv.GetAddressOf());

			deviceContext->PSSetConstantBuffers(1, 1, m_cameraBuffer->GetAddressOf());
			deviceContext->PSSetShaderResources(0, 1, m_normalmapSrv.GetAddressOf());
			deviceContext->PSSetShaderResources(1, 1, m_albedoTextureSrvs[0].GetAddressOf());
			deviceContext->PSSetShaderResources(2, 1, m_albedoTextureSrvs[1].GetAddressOf());

			auto sampler = m_states->LinearWrap();
			deviceContext->PSSetSamplers(0, 1, &sampler);
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11HullShader> m_hs;
		Microsoft::WRL::ComPtr<ID3D11DomainShader> m_ds;

		std::unique_ptr<ConstantBuffer<CameraBufferType>> m_cameraBuffer;
		XMFLOAT3 m_cameraPos;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_heightmapSrv, m_normalmapSrv;
		std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_albedoTextureSrvs;

		std::unique_ptr<DirectX::CommonStates> m_states;
	};
}