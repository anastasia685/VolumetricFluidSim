#pragma once
#include "BaseEffect.hpp"

namespace CustomEffects
{
	template <typename VertexType>
	class PostProcessEffect : public BaseEffect<VertexType>
	{
	public:
		explicit PostProcessEffect(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath) : BaseEffect<VertexType>(device, vsPath, psPath)
		{
			m_states = std::make_unique<CommonStates>(device);

			CreateConstantBuffers(device);
		}

		virtual void Apply(ID3D11DeviceContext* deviceContext) override
		{
			BaseEffect<VertexType>::Apply(deviceContext);
		}

		void Unbind(ID3D11DeviceContext* deviceContext)
		{
			// unbind sceneColor srv for writing again
			ID3D11ShaderResourceView* nullSRV[] = { NULL };
			deviceContext->PSSetShaderResources(0, 1, nullSRV);
		}

		void SetSceneColorSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_sceneColorSRV = srv; }

	protected:
		virtual void CreateConstantBuffers(ID3D11Device* device) override
		{
			// create constant buffers for this shader's specific stuff
		}
		virtual void SetConstantBuffers(ID3D11DeviceContext* deviceContext) override
		{
			// call parent's set constant buffers
			BaseEffect<VertexType>::SetConstantBuffers(deviceContext);

			// set this instance's buffers
			// set

			// bind


			deviceContext->PSSetShaderResources(0, 1, m_sceneColorSRV.GetAddressOf());

			auto sampler = m_states->LinearClamp();
			deviceContext->PSSetSamplers(0, 1, &sampler);
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_sceneColorSRV;

		std::unique_ptr<DirectX::CommonStates> m_states;
	};
}