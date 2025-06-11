#pragma once
#include "BaseEffect.hpp"

namespace CustomEffects
{
	template <typename VertexType>
	class VolumetricEffect : public BaseEffect<VertexType>
	{
	public:
		explicit VolumetricEffect(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath) : BaseEffect<VertexType>(device, vsPath, psPath)
		{
			m_cameraBuffer = std::make_unique<ConstantBuffer<CameraBufferType>>();
			m_volumeBuffer = std::make_unique<ConstantBuffer<VolumeBufferType>>();

			m_states = std::make_unique<CommonStates>(device);

			CreateBlendState(device);

			CreateConstantBuffers(device);

			CreateComputeShaders(device);
		}

		ID3D11BlendState* GetBlendState() const { return m_blendState.Get(); };
		DirectX::XMMATRIX GetMainCameraViewInv() const { return m_mainCameraViewInv; }
		DirectX::XMMATRIX GetMainCameraProjInv() const { return m_mainCameraProjInv; }
		float GetAbsorptionCoeff() const { return m_absorptionCoeff; }
		float GetScatterCoeff() const { return m_scatterCoeff; }

		void SetMainCameraViewInv(const DirectX::XMMATRIX& viewInv) { m_mainCameraViewInv = viewInv; }
		void SetMainCameraProjInv(const DirectX::XMMATRIX& projInv) { m_mainCameraProjInv = projInv; }
		void SetAbsorptionCoeff(float coeff) { m_absorptionCoeff = coeff; }
		void SetScatterCoeff(float coeff) { m_scatterCoeff = coeff; }
		void SetCameraPosition(const XMFLOAT3& cameraPos) { m_cameraPos = cameraPos; }
		void SetDensityMapSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_densityMapSrv = srv; }
		void SetSceneColorSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_sceneColorSrv = srv; }
		void SetSceneDepthSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_sceneDepthSrv = srv; }

		void Unbind(ID3D11DeviceContext* deviceContext)
		{
			// unbind density map, main render color & depth srvs for writing again
			ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL, NULL, NULL };
			deviceContext->PSSetShaderResources(0, 4, nullSRV);
		}

		void Compute(ID3D11DeviceContext* deviceContext)
		{
			SetComputeWorleyResourceViews(deviceContext);

			deviceContext->CSSetShader(m_worleyNoiseCs.Get(), nullptr, 0);
			deviceContext->Dispatch(16, 16, 16);

			ID3D11UnorderedAccessView* nullUAV[] = { NULL };
			deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
			deviceContext->CSSetShader(nullptr, nullptr, 0);

			//---

			/*SetComputePerlinResourceViews(deviceContext);

			deviceContext->CSSetShader(m_perlinNoiseCs.Get(), nullptr, 0);
			deviceContext->Dispatch(16, 16, 16);

			deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
			deviceContext->CSSetShader(nullptr, nullptr, 0);*/
		}


	protected:
		void CreateBlendState(ID3D11Device* device)
		{
			D3D11_BLEND_DESC blendDesc = {};
			blendDesc.RenderTarget[0].BlendEnable = TRUE;

			// srcRgb * 1 + destRgb * srcAlpha
			blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

			// srcAlpha * 0 + destAlpha * 1
			blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			device->CreateBlendState(&blendDesc, m_blendState.GetAddressOf());
		}
		virtual void CreateConstantBuffers(ID3D11Device* device) override
		{
			// create constant buffers for this shader's specific stuff
			m_cameraBuffer->Initialize(device);
			m_volumeBuffer->Initialize(device);
		}
		virtual void SetConstantBuffers(ID3D11DeviceContext* deviceContext) override
		{
			// call parent's set constant buffers
			BaseEffect<VertexType>::SetConstantBuffers(deviceContext);

			// set this instance's buffers
			// set
			m_cameraBuffer->Apply(deviceContext, { m_cameraPos });
			m_volumeBuffer->Apply(deviceContext, { 
				XMMatrixTranspose(m_mainCameraViewInv), XMMatrixTranspose(m_mainCameraProjInv),
				m_absorptionCoeff, m_scatterCoeff});
			// bind
			deviceContext->PSSetConstantBuffers(1, 1, m_cameraBuffer->GetAddressOf());
			deviceContext->PSSetConstantBuffers(2, 1, m_volumeBuffer->GetAddressOf());

			ID3D11ShaderResourceView* srvs[] = { m_densityMapSrv.Get(), m_worleyNoiseSRV.Get(), m_sceneColorSrv.Get(), m_sceneDepthSrv.Get()};
			deviceContext->PSSetShaderResources(0, 4, srvs);
			

			auto sampler = m_states->LinearClamp();
			deviceContext->PSSetSamplers(0, 1, &sampler);
		}
		void CreateComputeShaders(ID3D11Device* device)
		{
			auto csBlob = DX::ReadData(L"res/shaders/worley_cs.cso");
			DX::ThrowIfFailed(device->CreateComputeShader(csBlob.data(), csBlob.size(), nullptr, m_worleyNoiseCs.ReleaseAndGetAddressOf()));

			csBlob = DX::ReadData(L"res/shaders/perlin_cs.cso");
			DX::ThrowIfFailed(device->CreateComputeShader(csBlob.data(), csBlob.size(), nullptr, m_perlinNoiseCs.ReleaseAndGetAddressOf()));

			CreateComputeResourceViews(device);
		}
		void CreateComputeResourceViews(ID3D11Device* device)
		{
			int dimension = 128;
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(float) * (dimension) * (dimension) * (dimension);
			bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			bufferDesc.StructureByteStride = sizeof(float);
			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_worleyNoiseBuffer.GetAddressOf()));
			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_perlinNoiseBuffer.GetAddressOf()));


			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = (dimension) * (dimension) * (dimension);
			uavDesc.Buffer.Flags = 0;
			device->CreateUnorderedAccessView(m_worleyNoiseBuffer.Get(), &uavDesc, m_worleyNoiseUAV.GetAddressOf());
			device->CreateUnorderedAccessView(m_perlinNoiseBuffer.Get(), &uavDesc, m_perlinNoiseUAV.GetAddressOf());


			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = (dimension) * (dimension) * (dimension);
			device->CreateShaderResourceView(m_worleyNoiseBuffer.Get(), &srvDesc, m_worleyNoiseSRV.GetAddressOf());
			device->CreateShaderResourceView(m_perlinNoiseBuffer.Get(), &srvDesc, m_perlinNoiseSRV.GetAddressOf());
		}
		void SetComputeWorleyResourceViews(ID3D11DeviceContext* deviceContext)
		{
			deviceContext->CSSetUnorderedAccessViews(0, 1, m_worleyNoiseUAV.GetAddressOf(), nullptr);
		}
		void SetComputePerlinResourceViews(ID3D11DeviceContext* deviceContext)
		{
			deviceContext->CSSetUnorderedAccessViews(0, 1, m_perlinNoiseUAV.GetAddressOf(), nullptr);
		}

	private:
		float m_absorptionCoeff = 0.7f, m_scatterCoeff = 3.5f;
		DirectX::XMMATRIX m_mainCameraViewInv, m_mainCameraProjInv;

		Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;

		Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_worleyNoiseCs;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_worleyNoiseBuffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_worleyNoiseUAV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_worleyNoiseSRV;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_perlinNoiseCs;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_perlinNoiseBuffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_perlinNoiseUAV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_perlinNoiseSRV;

		std::unique_ptr<ConstantBuffer<CameraBufferType>> m_cameraBuffer;
		std::unique_ptr<ConstantBuffer<VolumeBufferType>> m_volumeBuffer;
		XMFLOAT3 m_cameraPos;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_densityMapSrv;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sceneColorSrv;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sceneDepthSrv;

		std::unique_ptr<DirectX::CommonStates> m_states;
	};
}