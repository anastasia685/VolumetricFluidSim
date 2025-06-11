#pragma once
#include "pch.h"
#include "ReadData.h"
#include <vector>
#include <unordered_map>
#include "VertexTypes.h"
#include "ConstantBuffer.hpp"

using namespace DirectX;

namespace CustomEffects
{
	template <typename VertexType>
	class BaseEffect : public DirectX::IEffect
	{
	public:
		explicit BaseEffect(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath)
		{
			m_matrixBuffer = std::make_unique<ConstantBuffer<MatrixBufferType>>();
			m_lightBuffer = std::make_unique<ConstantBuffer<LightBufferType>>();

			worldMatrix = XMMatrixIdentity();
			viewMatrix = XMMatrixIdentity();
			projectionMatrix = XMMatrixIdentity();

			CreateConstantBuffers(device);


			CreateVertexShader(device, vsPath);
			CreatePixelShader(device, psPath);
		}

		virtual void Apply(ID3D11DeviceContext* deviceContext) override
		{
			SetConstantBuffers(deviceContext);


			deviceContext->VSSetShader(m_vs.Get(), nullptr, 0);
			deviceContext->PSSetShader(m_ps.Get(), nullptr, 0);

			// set all other shaders to null, they will be set by inherited effect classes per need basis
			deviceContext->HSSetShader(nullptr, nullptr, 0);
			deviceContext->DSSetShader(nullptr, nullptr, 0);
			deviceContext->GSSetShader(nullptr, nullptr, 0);

			deviceContext->IASetInputLayout(m_InputLayout.Get());
		}
		virtual void GetVertexShaderBytecode(void const** pShaderByteCode, size_t* pByteCodeLength) override
		{
			assert(!m_vsBlob.empty() && pShaderByteCode != nullptr && pByteCodeLength != nullptr);
			*pShaderByteCode = m_vsBlob.data();
			*pByteCodeLength = m_vsBlob.size();
		}

		/*XMFLOAT3 GetAmbientColor() const { return ambientColor; };
		float GetAmbientIntensity() const { return ambientIntensity; };
		XMFLOAT3 GetSunColor() const { return sunColor; };
		float GetSunIntensity() const { return sunIntensity; };
		XMFLOAT3 GetSunDirection() const { return sunDirection; };*/

		void SetWorld(const DirectX::XMMATRIX& world) { worldMatrix = world; }
		void SetView(const DirectX::XMMATRIX& view) { viewMatrix = view; }
		void SetProjection(const DirectX::XMMATRIX& projection) { projectionMatrix = projection; }

		void SetAmbientColor(const XMFLOAT3& color) { ambientColor = color; };
		void SetAmbientIntensity(float intensity) { ambientIntensity = intensity; };
		void SetSunColor(const XMFLOAT3& color) { sunColor = color; };
		void SetSunIntensity(float intensity) { sunIntensity = intensity; };
		void SetSunDirection(const XMFLOAT3& direction) { sunDirection = direction; };

	protected:
		void CreateVertexShader(ID3D11Device* device, const std::wstring& filepath)
		{
			m_vsBlob = DX::ReadData(filepath.c_str());
			DX::ThrowIfFailed(device->CreateVertexShader(m_vsBlob.data(), m_vsBlob.size(), nullptr, m_vs.ReleaseAndGetAddressOf()));

			auto layoutDesc = VertexType::GetLayoutDesc();
			CreateInputLayout(device, layoutDesc.data(), static_cast<UINT>(layoutDesc.size()));
		}
		void CreatePixelShader(ID3D11Device* device, const std::wstring& filepath)
		{
			auto psBlob = DX::ReadData(filepath.c_str());
			DX::ThrowIfFailed(device->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, m_ps.ReleaseAndGetAddressOf()));
		}

		void CreateInputLayout(ID3D11Device* device, const D3D11_INPUT_ELEMENT_DESC* layoutDesc, UINT layoutSize)
		{
			const void* shaderBytecode;
			size_t bytecodeLength;
			GetVertexShaderBytecode(&shaderBytecode, &bytecodeLength);

			device->CreateInputLayout(layoutDesc, layoutSize,
				shaderBytecode,
				bytecodeLength,
				m_InputLayout.ReleaseAndGetAddressOf());
		}

		virtual void CreateConstantBuffers(ID3D11Device* device)
		{
			m_matrixBuffer->Initialize(device);
			m_lightBuffer->Initialize(device);
		}
		virtual void SetConstantBuffers(ID3D11DeviceContext* deviceContext)
		{
			// set
			m_matrixBuffer->Apply(deviceContext, {
				XMMatrixTranspose(worldMatrix),
				XMMatrixTranspose(viewMatrix),
				XMMatrixTranspose(projectionMatrix)
				}
			);
			// bind
			deviceContext->VSSetConstantBuffers(0, 1, m_matrixBuffer->GetAddressOf());

			m_lightBuffer->Apply(deviceContext, {
				ambientColor, ambientIntensity,
				sunColor, sunIntensity, sunDirection
				}
			);
			// bind
			deviceContext->VSSetConstantBuffers(1, 1, m_lightBuffer->GetAddressOf());
			deviceContext->PSSetConstantBuffers(0, 1, m_lightBuffer->GetAddressOf());
		}

		std::unique_ptr<ConstantBuffer<MatrixBufferType>> m_matrixBuffer;
		std::unique_ptr<ConstantBuffer<LightBufferType>> m_lightBuffer;

	private:
		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> m_ps;

		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;

		std::vector<uint8_t> m_vsBlob;

		DirectX::XMMATRIX worldMatrix;
		DirectX::XMMATRIX viewMatrix;
		DirectX::XMMATRIX projectionMatrix;

		DirectX::XMFLOAT3 ambientColor;
		float ambientIntensity;
		DirectX::XMFLOAT3 sunColor;
		float sunIntensity;
		DirectX::XMFLOAT3 sunDirection;
	};
}
