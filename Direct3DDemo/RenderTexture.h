//--------------------------------------------------------------------------------------
// File: RenderTexture.h
//
// Helper for managing offscreen render targets
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

#include <cstddef>

#include <wrl/client.h>

#include <DirectXMath.h>

#include "pch.h"

namespace DX
{
    class RenderTexture
    {
    public:
        explicit RenderTexture(DXGI_FORMAT format, bool storeLinearDepth = false, bool storeOcclusion = false) noexcept;

        RenderTexture(RenderTexture&&) = default;
        RenderTexture& operator= (RenderTexture&&) = default;

        RenderTexture(RenderTexture const&) = delete;
        RenderTexture& operator= (RenderTexture const&) = delete;

        void SetDevice(_In_ ID3D11Device* device);

        void SizeResources(size_t width, size_t height);

        void ReleaseDevice() noexcept;

        void SetWindow(const RECT& rect);

        void SetRenderTarget(ID3D11DeviceContext* deviceContext, DirectX::XMVECTOR clearColor = DirectX::Colors::CornflowerBlue);

        ID3D11Texture2D* GetRenderTarget() const noexcept { return m_renderTarget.Get(); }
        ID3D11RenderTargetView* GetRenderTargetView() const noexcept { return m_renderTargetView.Get(); }
        ID3D11RenderTargetView* GetOcclusionRenderTargetView() const noexcept { return m_occlusionRenderTargetView.Get(); }
        ID3D11ShaderResourceView* GetShaderResourceView() const noexcept { return m_shaderResourceView.Get(); }
        ID3D11DepthStencilView* GetDepthStencilView() const noexcept { return m_depthStencilView.Get(); };
        //ID3D11ShaderResourceView* GetDepthShaderResourceView() const noexcept { return m_depthShaderResourceView.Get(); }
        ID3D11ShaderResourceView* GetLinearDepthShaderResourceView() const noexcept { return m_linearDepthShaderResourceView.Get(); }
        ID3D11ShaderResourceView* GetOcclusionShaderResourceView() const noexcept { return m_occlusionShaderResourceView.Get(); }

        DXGI_FORMAT GetFormat() const noexcept { return m_format; }

        ID3D11BlendState* GetOcclusionBlendState() const noexcept { return m_occlusionBlendState.Get(); };

    private:
        Microsoft::WRL::ComPtr<ID3D11Device> m_device;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_renderTarget;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shaderResourceView;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;
        //don't really need it since i have linear depth srv now
        //Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_depthShaderResourceView;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_linearDepthRenderTarget;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_linearDepthRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_linearDepthShaderResourceView;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_occlusionRenderTarget;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_occlusionRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_occlusionShaderResourceView;

        DXGI_FORMAT m_format;

        size_t m_width;
        size_t m_height;

        D3D11_VIEWPORT m_viewport;

        bool m_storeLinearDepth, m_storeOcclusion;

        Microsoft::WRL::ComPtr<ID3D11BlendState> m_occlusionBlendState;

        void CreateOcclusionBlendState();
    };
}