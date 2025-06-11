#pragma once
#include "BaseEffect.hpp"

namespace CustomEffects
{
	template <typename VertexType>
	class GodRaysEffect : public BaseEffect<VertexType>
	{
	public:
		explicit GodRaysEffect(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath) : BaseEffect<VertexType>(device, vsPath, psPath)
		{
			m_cameraBuffer = std::make_unique<ConstantBuffer<CameraBufferType>>();
			m_sceneMatrixBuffer = std::make_unique<ConstantBuffer<MatrixBufferType>>();
			m_states = std::make_unique<CommonStates>(device);

			CreateConstantBuffers(device);
		}

		virtual void Apply(ID3D11DeviceContext* deviceContext) override
		{
			BaseEffect<VertexType>::Apply(deviceContext);
		}

		void Unbind(ID3D11DeviceContext* deviceContext)
		{
			// unbind srvs for writing again
			ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL };
			deviceContext->PSSetShaderResources(0, 2, nullSRV);
		}

		void SetCameraPosition(const XMFLOAT3& cameraPos) { m_cameraPos = cameraPos; }
		void SetSceneViewMatrix(const XMMATRIX& view) { m_sceneView = view; }
		void SetSceneProjectionMatrix(const XMMATRIX& proj) { m_sceneProj = proj; }
		void SetColorSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_colorSRV = srv; }
		void SetOcclusionSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_occlusionSRV = srv; }

	protected:
		virtual void CreateConstantBuffers(ID3D11Device* device) override
		{
			// create constant buffers for this shader's specific stuff
			m_cameraBuffer->Initialize(device);
			m_sceneMatrixBuffer->Initialize(device);
		}
		virtual void SetConstantBuffers(ID3D11DeviceContext* deviceContext) override
		{
			// call parent's set constant buffers
			BaseEffect<VertexType>::SetConstantBuffers(deviceContext);

			// set this instance's buffers
			// set
			m_cameraBuffer->Apply(deviceContext, { m_cameraPos });
			// bind
			deviceContext->VSSetConstantBuffers(2, 1, m_cameraBuffer->GetAddressOf());

			// set
			m_sceneMatrixBuffer->Apply(deviceContext, { XMMatrixTranspose(XMMatrixIdentity()), XMMatrixTranspose(m_sceneView), XMMatrixTranspose(m_sceneProj)});
			// bind
			deviceContext->VSSetConstantBuffers(3, 1, m_sceneMatrixBuffer->GetAddressOf());

			ID3D11ShaderResourceView* srvs[] = { m_colorSRV.Get(), m_occlusionSRV.Get() };
			deviceContext->PSSetShaderResources(0, 2, srvs);

			auto sampler = m_states->LinearClamp();
			deviceContext->PSSetSamplers(0, 1, &sampler);
		}

	private:
		XMFLOAT3 m_cameraPos;
		XMMATRIX m_sceneView, m_sceneProj;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_colorSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_occlusionSRV;

		std::unique_ptr<ConstantBuffer<CameraBufferType>> m_cameraBuffer;
		std::unique_ptr<ConstantBuffer<MatrixBufferType>> m_sceneMatrixBuffer;

		std::unique_ptr<DirectX::CommonStates> m_states;
	};
}