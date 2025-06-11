// texture
// Loads and stores a single texture.
// Handles .dds, .png and .jpg (probably).
#include "pch.h"
#include "TextureManager.h"


 //Attempt to load texture. If load fails use default texture.
 //Based on extension, uses slightly different loading function for different image types .dds vs .png/.jpg.
TextureManager::TextureManager(ID3D11Device* device)
{
	AddDefaultTexture(device);
}

void TextureManager::Load(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const wchar_t* uid, const wchar_t* filename, bool linear)
{
	HRESULT result;

	// check if file exists
	if (!filename)
	{
		MessageBox(NULL, L"Texture filename does not exist", L"ERROR", MB_OK);
		return;
	}
	// if not set default texture
	if (!FileExists(filename))
	{
		MessageBox(NULL, L"Texture filename does not exist", L"ERROR", MB_OK);
		return;
	}

	// check file extension for correct loading function.
	std::wstring fn(filename);
	std::string::size_type idx;
	std::wstring extension;

	idx = fn.rfind('.');

	if (idx != std::string::npos)
	{
		extension = fn.substr(idx + 1);
	}
	else
	{
		// No extension found
	}

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSrv;
	// Load the texture in.
	if (extension == L"dds")
	{
		//result = CreateDDSTextureFromFile(device, deviceContext, filename, NULL, textureSrv.ReleaseAndGetAddressOf());
		result = CreateDDSTextureFromFileEx(
			device, deviceContext, filename, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
			linear ? DDS_LOADER_DEFAULT : DDS_LOADER_FORCE_SRGB, nullptr, textureSrv.ReleaseAndGetAddressOf());
	}
	else
	{
		//result = CreateWICTextureFromFile(device, deviceContext, filename, NULL, textureSrv.ReleaseAndGetAddressOf(), 0);
		result = CreateWICTextureFromFileEx(
			device, deviceContext, filename, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
			linear ? WIC_LOADER_DEFAULT : WIC_LOADER_FORCE_SRGB, nullptr, textureSrv.ReleaseAndGetAddressOf());
	}

	if (FAILED(result))
	{
		MessageBox(NULL, L"Texture loading error", L"ERROR", MB_OK);
	}
	else
	{
		m_textureMap.insert(std::make_pair(const_cast<wchar_t*>(uid), textureSrv));
	}
}

// Return texture as a shader resource.
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureManager::Get(const wchar_t* uid)
{
	if (m_textureMap.find(const_cast<wchar_t*>(uid)) != m_textureMap.end())
	{
		// texture exists
		return m_textureMap.at(const_cast<wchar_t*>(uid));
	}
	else
	{
		return m_textureMap.at(L"default");
	}
}

bool TextureManager::FileExists(const wchar_t* fname)
{
	std::ifstream infile(fname);
	return infile.good();
}

void TextureManager::AddDefaultTexture(ID3D11Device* device)
{
	ID3D11Texture2D* texture;

	static const uint32_t s_pixel = 0xffffffff;

	D3D11_SUBRESOURCE_DATA initData = { &s_pixel, sizeof(uint32_t), 0 };

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	HRESULT hr = device->CreateTexture2D(&desc, &initData, &texture);

	if (SUCCEEDED(hr))
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureSrv;
		hr = device->CreateShaderResourceView(texture, &SRVDesc, textureSrv.ReleaseAndGetAddressOf());
		m_textureMap.insert(std::make_pair(const_cast <wchar_t*>(L"default"), textureSrv));
	}
}