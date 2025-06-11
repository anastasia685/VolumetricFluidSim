#pragma once
// Texture
// Loads and stores a texture ready for rendering.
// Handles mipmap generation on load.

#include "pch.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include <string>
#include <fstream>
#include <vector>
#include <map>

using namespace DirectX;

class TextureManager
{
public:
	TextureManager(ID3D11Device* device);

	void Load(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const wchar_t* uid, const wchar_t* filename, bool linear = false);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Get(const wchar_t* uid);

private:
	bool FileExists(const wchar_t* fileName);
	void AddDefaultTexture(ID3D11Device* device);

	std::map<const wchar_t*, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_textureMap;
};