#pragma once
#include "BaseEffect.hpp"

namespace CustomEffects
{
	template <typename VertexType>
	class BlendEffect : public BaseEffect<VertexType>
	{
	public:
		explicit BlendEffect(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath) : BaseEffect<VertexType>(device, vsPath, psPath)
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
			// unbind srvs for writing again
			ID3D11ShaderResourceView* nullSRV[] = { NULL, NULL };
			deviceContext->PSSetShaderResources(0, 2, nullSRV);
		}

		void SetSrcSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_srcSRV = srv; }
		void SetDestSrv(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_destSRV = srv; }

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

			ID3D11ShaderResourceView* srvs[] = { m_srcSRV.Get(), m_destSRV.Get() };
			deviceContext->PSSetShaderResources(0, 2, srvs);

			auto sampler = m_states->LinearClamp();
			deviceContext->PSSetSamplers(0, 1, &sampler);
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srcSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_destSRV;

		std::unique_ptr<DirectX::CommonStates> m_states;
	};
}