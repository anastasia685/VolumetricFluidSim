#pragma once
#include "pch.h"
#include "ReadData.h"
#include "ConstantBuffer.hpp"
//#include "Perlin.h"
#include "Simplex.h"

using namespace DirectX;

namespace CustomEffects
{
	class FluidSimEffect
	{
	public:
		explicit FluidSimEffect(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const XMFLOAT3& bufferDimensions)
		{
			m_bufferDimensions = bufferDimensions;

			m_fluidBuffer = std::make_unique<ConstantBuffer<FluidBufferType>>();

			m_states = std::make_unique<CommonStates>(device);


			CreateConstantBuffers(device);

			CreateComputeShader(device, L"res/shaders/perlin_cs.cso", m_perlinNoiseCs.GetAddressOf());

			CreateComputeShader(device, L"res/shaders/fluid_bounds_cs.cso", m_boundsCs.GetAddressOf());
			CreateComputeShader(device, L"res/shaders/fluid_advect_staggered_cs.cso", m_advectStaggeredCs.GetAddressOf());
			CreateComputeShader(device, L"res/shaders/fluid_advect_cs.cso", m_advectCs.GetAddressOf());
			CreateComputeShader(device, L"res/shaders/fluid_curl_cs.cso", m_curlCs.GetAddressOf());
			CreateComputeShader(device, L"res/shaders/fluid_vorticity_cs.cso", m_vorticityCs.GetAddressOf());
			CreateComputeShader(device, L"res/shaders/fluid_divergence_cs.cso", m_divergenceCs.GetAddressOf());
			CreateComputeShader(device, L"res/shaders/fluid_jacobi_poisson_cs.cso", m_poissonCs.GetAddressOf());
			CreateComputeShader(device, L"res/shaders/fluid_gradient_cs.cso", m_gradientCs.GetAddressOf());
			CreateComputeShader(device, L"res/shaders/fluid_diffuse_cs.cso", m_diffuseCs.GetAddressOf());

			CreateResourceViews(device);

			InitializeBuffers(deviceContext);
		}

		void ComputeNoise(ID3D11DeviceContext* deviceContext)
		{
			SetPerlinResourceViews(deviceContext);

			deviceContext->CSSetShader(m_perlinNoiseCs.Get(), nullptr, 0);

			deviceContext->Dispatch(16, 16, 16);

			ID3D11UnorderedAccessView* nullUAV[] = { NULL };
			deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
			deviceContext->CSSetShader(nullptr, nullptr, 0);
		}

		void Compute(ID3D11DeviceContext* deviceContext, int x, int y, int z)
		{
			//--- velocity advection
			SetConstantBuffers(deviceContext);
			SetStaggeredAdvectionResourceViews(deviceContext);
			deviceContext->CSSetShader(m_advectStaggeredCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x + 1, y + 1, z + 1);
			Unbind(deviceContext, 6);

			//--- boundary conditions
			SetConstantBuffers(deviceContext);
			SetBoundsResourceViews(deviceContext);
			deviceContext->CSSetShader(m_boundsCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x + 1, y + 1, z + 1);
			Unbind(deviceContext, 3);
			// swap velocity
			m_velocityBufferIndex = 1 - m_velocityBufferIndex;

			//--- vorticity confinement
			// velocity curl calculation
			SetCurlResourceViews(deviceContext);
			deviceContext->CSSetShader(m_curlCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x, y, z);
			Unbind(deviceContext, 3);

			// confinement force application
			SetVorticityResourceViews(deviceContext);
			deviceContext->CSSetShader(m_vorticityCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x, y, z);
			Unbind(deviceContext, 4);

			//--- boundary conditions
			SetConstantBuffers(deviceContext);
			SetBoundsResourceViews(deviceContext);
			deviceContext->CSSetShader(m_boundsCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x + 1, y + 1, z + 1);
			Unbind(deviceContext, 3);
			// swap velocity
			m_velocityBufferIndex = 1 - m_velocityBufferIndex;


			//--- velocity divergence calculation
			SetDivergenceResourceViews(deviceContext);
			deviceContext->CSSetShader(m_divergenceCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x + 1, y + 1, z + 1);
			Unbind(deviceContext, 3);

			//--- poisson equation with jacobi
			for (int i = 0; i < 70; i++)
			{
				SetPoissonResourceViews(deviceContext);
				deviceContext->CSSetShader(m_poissonCs.Get(), nullptr, 0);
				deviceContext->Dispatch(x + 1, y + 1, z + 1);
				Unbind(deviceContext, 3);
				// swap pressure
				m_pressureBufferIndex = 1 - m_pressureBufferIndex;
			}

			//--- gradient subtraction
			SetGradientResourceViews(deviceContext);
			deviceContext->CSSetShader(m_gradientCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x + 1, y + 1, z + 1);
			Unbind(deviceContext, 5);

			//--- velocity boundary conditions
			SetConstantBuffers(deviceContext);
			SetBoundsResourceViews(deviceContext);
			deviceContext->CSSetShader(m_boundsCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x + 1, y + 1, z + 1);
			Unbind(deviceContext, 3);
			// swap velocity
			m_velocityBufferIndex = 1 - m_velocityBufferIndex;

			// just for testing
			//--- velocity divergence calculation
			/*SetDivergenceResourceViews(deviceContext);
			deviceContext->CSSetShader(m_divergenceCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x + 1, y + 1, z + 1);
			Unbind(deviceContext, 3);*/

			//--- density diffusion
			//deviceContext->CopyResource(
			//	reinterpret_cast<ID3D11Resource*>(m_densityBuffer[(m_densityBufferIndex + 1) % 3].Get()),
			//	reinterpret_cast<ID3D11Resource*>(m_densityBuffer[m_densityBufferIndex].Get())
			//);
			//int numJacobiIterations = 20;
			//for (int i = 0; i < numJacobiIterations; i++)
			//{
			//	int readIndex = ((i % 2) + 1 + m_densityBufferIndex) % 3;
			//	int writeIndex = (((i + 1) % 2) + 1 + m_densityBufferIndex) % 3;

			//	//--- density diffusion
			//	SetDiffusionResourceViews(deviceContext, readIndex, writeIndex);
			//	deviceContext->CSSetShader(m_diffuseCs.Get(), nullptr, 0);
			//	deviceContext->Dispatch(x + 1, y + 1, z + 1);
			//	Unbind(deviceContext, 2);
			//}
			//m_densityBufferIndex = (((numJacobiIterations + 1) % 2) + 1 + m_densityBufferIndex) % 3;

			//--- density advection
			// semi-lagrangian for first pass
			SetConstantBuffers(deviceContext);
			SetAdvectionResourceViews(deviceContext);
			deviceContext->CSSetShader(m_advectCs.Get(), nullptr, 0);
			deviceContext->Dispatch(x + 1, y + 1, z + 1);
			Unbind(deviceContext, 7);
			// swap density
			m_densityBufferIndex = (m_densityBufferIndex + 1) % 3;
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

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetDensitySrv() const { return m_densitySRV[m_densityBufferIndex]; };
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> GetDensityUav() const { return m_densityUAV[1 - m_densityBufferIndex]; };

		void SwapDensityBuffers() { m_densityBufferIndex = (m_densityBufferIndex + 1) % 3; };
		void SetDeltaTime(float dt) { m_deltaTime = dt; };
		void SetElapsedTime(float t) { m_elapsedTime = t; };
		void SetSurfaceSRV(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_surfaceSRV = srv; };
		void SetSDFSRV(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_sdfSRV = srv; };
		void SetSDFGradientSRV(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv) { m_sdfGradientSRV = srv; };

	private:
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_perlinNoiseCs;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_boundsCs, m_advectStaggeredCs, m_advectCs, m_curlCs, m_vorticityCs, m_divergenceCs, m_poissonCs, m_gradientCs, m_diffuseCs;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_perlinNoiseBuffer;
		Microsoft::WRL::ComPtr<ID3D11Texture3D> m_perlinNoiseTexture;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_velocityXBuffer[2], m_velocityYBuffer[2], m_velocityZBuffer[2];
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_curlBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_divergenceBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pressureBuffer[2];
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_densityBuffer[3];


		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_perlinNoiseUAV;

		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_velocityXUAV[2], m_velocityYUAV[2], m_velocityZUAV[2];
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_curlUAV;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_divergenceUAV;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_pressureUAV[2];
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_densityUAV[3];


		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_perlinNoiseSRV;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_velocityXSRV[2], m_velocityYSRV[2], m_velocityZSRV[2];
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_curlSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_divergenceSRV;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pressureSRV[2];
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_densitySRV[3];

		int m_velocityBufferIndex = 0, m_densityBufferIndex = 0, m_pressureBufferIndex = 0;

		std::unique_ptr<ConstantBuffer<FluidBufferType>> m_fluidBuffer;

		XMFLOAT3 m_bufferDimensions;
		float m_deltaTime, m_elapsedTime;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_surfaceSRV, m_sdfSRV, m_sdfGradientSRV;

		std::unique_ptr<DirectX::CommonStates> m_states;


		void CreateComputeShader(ID3D11Device* device, const std::wstring& filepath, ID3D11ComputeShader** cs)
		{
			auto csBlob = DX::ReadData(filepath.c_str());
			DX::ThrowIfFailed(device->CreateComputeShader(csBlob.data(), csBlob.size(), nullptr, cs));
		}
		void CreateResourceViews(ID3D11Device* device)
		{
			int dimension = 128;
			D3D11_TEXTURE3D_DESC texDesc = {};
			texDesc.Width = dimension;
			texDesc.Height = dimension;
			texDesc.Depth = dimension;
			texDesc.MipLevels = 1;
			texDesc.Format = DXGI_FORMAT_R16_FLOAT;
			texDesc.Usage = D3D11_USAGE_DEFAULT;
			texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			texDesc.CPUAccessFlags = 0;
			texDesc.MiscFlags = 0;
			DX::ThrowIfFailed(device->CreateTexture3D(&texDesc, nullptr, m_perlinNoiseTexture.GetAddressOf()));

			D3D11_UNORDERED_ACCESS_VIEW_DESC noiseUavDesc = {};
			noiseUavDesc.Format = texDesc.Format;
			noiseUavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
			noiseUavDesc.Texture3D.MipSlice = 0;
			noiseUavDesc.Texture3D.FirstWSlice = 0;
			noiseUavDesc.Texture3D.WSize = dimension; // depth
			DX::ThrowIfFailed(device->CreateUnorderedAccessView(m_perlinNoiseTexture.Get(), &noiseUavDesc, m_perlinNoiseUAV.GetAddressOf()));

			D3D11_SHADER_RESOURCE_VIEW_DESC noiseSrvDesc = {};
			noiseSrvDesc.Format = texDesc.Format;
			noiseSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			noiseSrvDesc.Texture3D.MipLevels = 1;
			noiseSrvDesc.Texture3D.MostDetailedMip = 0;
			DX::ThrowIfFailed(device->CreateShaderResourceView(m_perlinNoiseTexture.Get(), &noiseSrvDesc, m_perlinNoiseSRV.GetAddressOf()));


			//--- SIM BUFFERS
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;
			bufferDesc.ByteWidth = sizeof(float) * (m_bufferDimensions.x + 3) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 2);
			bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
			bufferDesc.CPUAccessFlags = 0;
			bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			bufferDesc.StructureByteStride = sizeof(float);
			for (int i = 0; i < 2; i++)
			{
				DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_velocityXBuffer[i].GetAddressOf()));
			}
			bufferDesc.ByteWidth = sizeof(float) * (m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 3) * (m_bufferDimensions.z + 2);
			for (int i = 0; i < 2; i++)
			{
				DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_velocityYBuffer[i].GetAddressOf()));
			}
			bufferDesc.ByteWidth = sizeof(float) * (m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 3);
			for (int i = 0; i < 2; i++)
			{
				DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_velocityZBuffer[i].GetAddressOf()));
			}


			bufferDesc.ByteWidth = sizeof(float) * (m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 2);
			for (int i = 0; i < 2; i++)
			{
				DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_pressureBuffer[i].GetAddressOf()));
			}
			for (int i = 0; i < 3; i++)
			{
				DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_densityBuffer[i].GetAddressOf()));
			}
			//bufferDesc.ByteWidth = sizeof(float) * (m_bufferDimensions.x) * (m_bufferDimensions.y) * (m_bufferDimensions.z);
			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_divergenceBuffer.GetAddressOf()));

			bufferDesc.ByteWidth = 3 * sizeof(float) * (m_bufferDimensions.x) * (m_bufferDimensions.y) * (m_bufferDimensions.z);
			bufferDesc.StructureByteStride = 3 * sizeof(float);
			DX::ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, m_curlBuffer.GetAddressOf()));

			//--- UAV'S
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
			uavDesc.Format = DXGI_FORMAT_UNKNOWN;
			uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uavDesc.Buffer.FirstElement = 0;
			uavDesc.Buffer.NumElements = (m_bufferDimensions.x + 3) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 2);
			uavDesc.Buffer.Flags = 0;
			for (int i = 0; i < 2; i++)
			{
				device->CreateUnorderedAccessView(m_velocityXBuffer[i].Get(), &uavDesc, m_velocityXUAV[i].GetAddressOf());
			}
			uavDesc.Buffer.NumElements = (m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 3) * (m_bufferDimensions.z + 2);
			for (int i = 0; i < 2; i++)
			{
				device->CreateUnorderedAccessView(m_velocityYBuffer[i].Get(), &uavDesc, m_velocityYUAV[i].GetAddressOf());
			}
			uavDesc.Buffer.NumElements = (m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 3);
			for (int i = 0; i < 2; i++)
			{
				device->CreateUnorderedAccessView(m_velocityZBuffer[i].Get(), &uavDesc, m_velocityZUAV[i].GetAddressOf());
			}
			uavDesc.Buffer.NumElements = (m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 2);
			for (int i = 0; i < 2; i++)
			{
				device->CreateUnorderedAccessView(m_pressureBuffer[i].Get(), &uavDesc, m_pressureUAV[i].GetAddressOf());
			}
			for (int i = 0; i < 3; i++)
			{
				device->CreateUnorderedAccessView(m_densityBuffer[i].Get(), &uavDesc, m_densityUAV[i].GetAddressOf());
			}
			device->CreateUnorderedAccessView(m_divergenceBuffer.Get(), &uavDesc, m_divergenceUAV.GetAddressOf());
			
			uavDesc.Buffer.NumElements = (m_bufferDimensions.x) * (m_bufferDimensions.y) * (m_bufferDimensions.z);
			device->CreateUnorderedAccessView(m_curlBuffer.Get(), &uavDesc, m_curlUAV.GetAddressOf());

			//--- SRV
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = (m_bufferDimensions.x + 3) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 2);
			for (int i = 0; i < 2; i++)
			{
				device->CreateShaderResourceView(m_velocityXBuffer[i].Get(), &srvDesc, m_velocityXSRV[i].GetAddressOf());
			}
			srvDesc.Buffer.NumElements = (m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 3) * (m_bufferDimensions.z + 2);
			for (int i = 0; i < 2; i++)
			{
				device->CreateShaderResourceView(m_velocityYBuffer[i].Get(), &srvDesc, m_velocityYSRV[i].GetAddressOf());
			}
			srvDesc.Buffer.NumElements = (m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 3);
			for (int i = 0; i < 2; i++)
			{
				device->CreateShaderResourceView(m_velocityZBuffer[i].Get(), &srvDesc, m_velocityZSRV[i].GetAddressOf());
			}

			srvDesc.Buffer.NumElements = (m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 2);
			for (int i = 0; i < 2; i++)
			{
				device->CreateShaderResourceView(m_pressureBuffer[i].Get(), &srvDesc, m_pressureSRV[i].GetAddressOf());
			}
			for (int i = 0; i < 3; i++)
			{
				device->CreateShaderResourceView(m_densityBuffer[i].Get(), &srvDesc, m_densitySRV[i].GetAddressOf());
			}
			device->CreateShaderResourceView(m_divergenceBuffer.Get(), &srvDesc, m_divergenceSRV.GetAddressOf());
			
			srvDesc.Buffer.NumElements = (m_bufferDimensions.x) * (m_bufferDimensions.y) * (m_bufferDimensions.z);
			device->CreateShaderResourceView(m_curlBuffer.Get(), &srvDesc, m_curlSRV.GetAddressOf());
		}
		void CreateConstantBuffers(ID3D11Device* device) 
		{
			m_fluidBuffer->Initialize(device);
		};
		void SetConstantBuffers(ID3D11DeviceContext* deviceContext)
		{
			// set
			m_fluidBuffer->Apply(deviceContext, { m_deltaTime, m_elapsedTime});
			// bind
			deviceContext->CSSetConstantBuffers(0, 1, m_fluidBuffer->GetAddressOf());
		}
		void SetPerlinResourceViews(ID3D11DeviceContext* deviceContext)
		{
			deviceContext->CSSetUnorderedAccessViews(0, 1, m_perlinNoiseUAV.GetAddressOf(), nullptr);
		}
		void SetStaggeredAdvectionResourceViews(ID3D11DeviceContext* deviceContext)
		{
			// velocity
			int readIndex = m_velocityBufferIndex;
			int writeIndex = 1 - m_velocityBufferIndex;
			
			ID3D11UnorderedAccessView* uavs[] = { m_velocityXUAV[writeIndex].Get(), m_velocityYUAV[writeIndex].Get(), m_velocityZUAV[writeIndex].Get() };
			ID3D11ShaderResourceView* srvs[] = { m_velocityXSRV[readIndex].Get(), m_velocityYSRV[readIndex].Get(), m_velocityZSRV[readIndex].Get(), m_sdfSRV.Get(), m_perlinNoiseSRV.Get(), m_densitySRV[m_densityBufferIndex].Get()};
			
			deviceContext->CSSetUnorderedAccessViews(0, 3, uavs, nullptr);
			deviceContext->CSSetShaderResources(0, 6, srvs);

			ID3D11SamplerState* states[] = {m_states->LinearClamp(), m_states->LinearWrap()};
			deviceContext->CSSetSamplers(0, 2, states);
		}
		void SetDiffusionResourceViews(ID3D11DeviceContext* deviceContext, int readIndex, int writeIndex)
		{
			// density
			//int readIndex = m_densityBufferIndex;
			//int writeIndex = (m_densityBufferIndex + 1) % 3;

			// first always holds initial density values before diffusion step. second holds intermediary values - output of previous diffusion iteration
			ID3D11ShaderResourceView* srvs[] = { m_densitySRV[m_densityBufferIndex].Get(), m_densitySRV[readIndex].Get() };
			deviceContext->CSSetUnorderedAccessViews(0, 1, m_densityUAV[writeIndex].GetAddressOf(), nullptr);
			deviceContext->CSSetShaderResources(0, 2, srvs);
		}
		void SetAdvectionResourceViews(ID3D11DeviceContext* deviceContext)
		{
			// velocity
			int readIndex0 = m_velocityBufferIndex;

			// density
			int readIndex1 = m_densityBufferIndex;
			int writeIndex1 = (m_densityBufferIndex + 1) % 3;

			deviceContext->CSSetUnorderedAccessViews(0, 1, m_densityUAV[writeIndex1].GetAddressOf(), nullptr);

			ID3D11ShaderResourceView* srvs[] = { m_velocityXSRV[readIndex0].Get(), m_velocityYSRV[readIndex0].Get(), m_velocityZSRV[readIndex0].Get(), m_densitySRV[readIndex1].Get(), m_sdfSRV.Get(), m_surfaceSRV.Get(), m_perlinNoiseSRV.Get()};
			deviceContext->CSSetShaderResources(0, 7, srvs);


			ID3D11SamplerState* states[] = { m_states->LinearClamp(), m_states->LinearWrap() };
			deviceContext->CSSetSamplers(0, 2, states);
		}
		void SetBoundsResourceViews(ID3D11DeviceContext* deviceContext)
		{
			// velocity
			int writeIndex = 1 - m_velocityBufferIndex;

			ID3D11UnorderedAccessView* uavs[] = { m_velocityXUAV[writeIndex].Get(), m_velocityYUAV[writeIndex].Get(), m_velocityZUAV[writeIndex].Get() };

			deviceContext->CSSetUnorderedAccessViews(0, 3, uavs, nullptr);
			deviceContext->CSSetShaderResources(0, 1, m_sdfSRV.GetAddressOf());

			auto sampler = m_states->LinearClamp();
			deviceContext->CSSetSamplers(0, 1, &sampler);
		}
		void SetCurlResourceViews(ID3D11DeviceContext* deviceContext)
		{
			// velocity
			int readIndex = m_velocityBufferIndex;

			ID3D11ShaderResourceView* srvs[] = { m_velocityXSRV[readIndex].Get(), m_velocityYSRV[readIndex].Get(), m_velocityZSRV[readIndex].Get() };

			deviceContext->CSSetUnorderedAccessViews(0, 1, m_curlUAV.GetAddressOf(), nullptr);
			deviceContext->CSSetShaderResources(0, 3, srvs);
		}
		void SetVorticityResourceViews(ID3D11DeviceContext* deviceContext)
		{
			// velocity
			int readIndex = m_velocityBufferIndex;
			int writeIndex = 1 - m_velocityBufferIndex;

			ID3D11UnorderedAccessView* uavs[] = { m_velocityXUAV[writeIndex].Get(), m_velocityYUAV[writeIndex].Get(), m_velocityZUAV[writeIndex].Get() };
			ID3D11ShaderResourceView* srvs[] = { m_curlSRV.Get(), m_velocityXSRV[readIndex].Get(), m_velocityYSRV[readIndex].Get(), m_velocityZSRV[readIndex].Get() };

			deviceContext->CSSetUnorderedAccessViews(0, 3, uavs, nullptr);
			deviceContext->CSSetShaderResources(0, 4, srvs);
		}
		void SetDivergenceResourceViews(ID3D11DeviceContext* deviceContext)
		{
			// velocity
			int readIndex = m_velocityBufferIndex;
			
			ID3D11ShaderResourceView* srvs[] = { m_velocityXSRV[readIndex].Get(), m_velocityYSRV[readIndex].Get(), m_velocityZSRV[readIndex].Get()};

			deviceContext->CSSetUnorderedAccessViews(0, 1, m_divergenceUAV.GetAddressOf(), nullptr);
			deviceContext->CSSetShaderResources(0, 3, srvs);
		}
		void SetPoissonResourceViews(ID3D11DeviceContext* deviceContext)
		{
			// pressure
			int readIndex = m_pressureBufferIndex;
			int writeIndex = 1 - m_pressureBufferIndex;

			ID3D11ShaderResourceView* srvs[] = { m_pressureSRV[readIndex].Get(), m_divergenceSRV.Get(), m_sdfSRV.Get()};

			deviceContext->CSSetUnorderedAccessViews(0, 1, m_pressureUAV[writeIndex].GetAddressOf(), nullptr);
			deviceContext->CSSetShaderResources(0, 3, srvs);

			auto sampler = m_states->LinearClamp();
			deviceContext->CSSetSamplers(0, 1, &sampler);
		}
		void SetGradientResourceViews(ID3D11DeviceContext* deviceContext)
		{
			int readIndex0 = m_pressureBufferIndex;
			int readIndex1 = m_velocityBufferIndex;
			int writeIndex1 = 1 - m_velocityBufferIndex;

			ID3D11UnorderedAccessView* uavs[] = { m_velocityXUAV[writeIndex1].Get(), m_velocityYUAV[writeIndex1].Get(), m_velocityZUAV[writeIndex1].Get()};
			ID3D11ShaderResourceView* srvs[] = { m_pressureSRV[readIndex0].Get(), m_velocityXSRV[readIndex1].Get(), m_velocityYSRV[readIndex1].Get(), m_velocityZSRV[readIndex1].Get(), m_sdfSRV.Get()};

			deviceContext->CSSetUnorderedAccessViews(0, 3, uavs, nullptr);
			deviceContext->CSSetShaderResources(0, 5, srvs);

			auto sampler = m_states->LinearClamp();
			deviceContext->CSSetSamplers(0, 1, &sampler);
		}
		void InitializeBuffers(ID3D11DeviceContext* deviceContext)
		{
			// zero-initialize the buffers before simulation starts
			std::vector<float> zeroVelocityX((m_bufferDimensions.x + 3) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 2), 0);
			std::vector<float> zeroVelocityY((m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 3) * (m_bufferDimensions.z + 2), 0);
			std::vector<float> zeroVelocityZ((m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 3), 0);
			std::vector<float> zeroScalar((m_bufferDimensions.x + 2) * (m_bufferDimensions.y + 2) * (m_bufferDimensions.z + 2), 0.0f);

			deviceContext->UpdateSubresource(m_velocityXBuffer[m_velocityBufferIndex].Get(), 0, nullptr, zeroVelocityX.data(), 0, 0);
			deviceContext->UpdateSubresource(m_velocityYBuffer[m_velocityBufferIndex].Get(), 0, nullptr, zeroVelocityY.data(), 0, 0);
			deviceContext->UpdateSubresource(m_velocityZBuffer[m_velocityBufferIndex].Get(), 0, nullptr, zeroVelocityZ.data(), 0, 0);
			deviceContext->UpdateSubresource(m_densityBuffer[m_densityBufferIndex].Get(), 0, nullptr, zeroScalar.data(), 0, 0);
			deviceContext->UpdateSubresource(m_pressureBuffer[m_pressureBufferIndex].Get(), 0, nullptr, zeroScalar.data(), 0, 0);
		}
	};
}
