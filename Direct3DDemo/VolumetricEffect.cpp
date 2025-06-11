#include "pch.h"
#include "VolumetricEffect.h"

using namespace CustomEffects;

template <typename T>
VolumetricEffect<T>::VolumetricEffect(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath) : BaseEffect(device, vsPath, psPath)
{
	CreateConstantBuffers(device);
}

template <typename T>
void CustomEffects::VolumetricEffect<T>::CreateConstantBuffers(ID3D11Device* device)
{
	// create constant buffers for this shader's specific stuff
}

template <typename T>
void CustomEffects::VolumetricEffect<T>::SetConstantBuffers(ID3D11DeviceContext* deviceContext)
{
	// call parent's set constant buffers
	BaseEffect::SetConstantBuffers(deviceContext);

	// set this instance's buffers
}
