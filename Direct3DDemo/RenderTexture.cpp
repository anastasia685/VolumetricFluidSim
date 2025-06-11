//--------------------------------------------------------------------------------------
// File: RenderTexture.cpp
//
// Helper for managing offscreen render targets
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "RenderTexture.h"

#include "DirectXHelpers.h"

#include <algorithm>
#include <cstdio>
#include <stdexcept>

using namespace DirectX;
using namespace DX;

using Microsoft::WRL::ComPtr;

RenderTexture::RenderTexture(DXGI_FORMAT format, bool storeLinearDepth, bool storeOcclusion) noexcept :
    m_format(format),
    m_width(0),
    m_height(0),
    m_storeLinearDepth(storeLinearDepth),
    m_storeOcclusion(storeOcclusion)
{
}

void RenderTexture::SetDevice(_In_ ID3D11Device* device)
{
    if (device == m_device.Get())
        return;

    if (m_device)
    {
        ReleaseDevice();
    }

    {
        UINT formatSupport = 0;
        if (FAILED(device->CheckFormatSupport(m_format, &formatSupport)))
        {
            throw std::runtime_error("CheckFormatSupport");
        }

        constexpr UINT32 required = D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_RENDER_TARGET;
        if ((formatSupport & required) != required)
        {
#ifdef _DEBUG
            char buff[128] = {};
            sprintf_s(buff, "RenderTexture: Device does not support the requested format (%d)!\n", m_format);
            OutputDebugStringA(buff);
#endif
            throw std::runtime_error("RenderTexture");
        }
    }

    m_device = device;

    CreateOcclusionBlendState();
}


void RenderTexture::SizeResources(size_t width, size_t height)
{
    if (width == m_width && height == m_height)
        return;

    if (m_width > UINT32_MAX || m_height > UINT32_MAX)
    {
        throw std::out_of_range("Invalid width/height");
    }

    if (!m_device)
        return;

    m_width = m_height = 0;

    // Create a render target
    CD3D11_TEXTURE2D_DESC renderTargetDesc(
        m_format,
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1, // The render target view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DEFAULT,
        0,
        1
    );

    ThrowIfFailed(m_device->CreateTexture2D(
        &renderTargetDesc,
        nullptr,
        m_renderTarget.ReleaseAndGetAddressOf()
    ));

    SetDebugObjectName(m_renderTarget.Get(), "RenderTexture RT");

    // Create RTV.
    CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2D, m_format);
    ThrowIfFailed(m_device->CreateRenderTargetView(
        m_renderTarget.Get(),
        &renderTargetViewDesc,
        m_renderTargetView.ReleaseAndGetAddressOf()
    ));
    SetDebugObjectName(m_renderTargetView.Get(), "RenderTexture RTV");

    // Create SRV.
    CD3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc(D3D11_SRV_DIMENSION_TEXTURE2D, m_format);
    ThrowIfFailed(m_device->CreateShaderResourceView(
        m_renderTarget.Get(),
        &shaderResourceViewDesc,
        m_shaderResourceView.ReleaseAndGetAddressOf()
    ));



    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_R24G8_TYPELESS,
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1, // The render target view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DEFAULT,
        0,
        1
    );

    ThrowIfFailed(m_device->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        m_depthStencilBuffer.ReleaseAndGetAddressOf()
    ));
    SetDebugObjectName(m_depthStencilBuffer.Get(), "RenderTexture DS");


    // Create DSV.
    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D24_UNORM_S8_UINT);
    ThrowIfFailed(m_device->CreateDepthStencilView(
        m_depthStencilBuffer.Get(),
        &depthStencilViewDesc,
        m_depthStencilView.ReleaseAndGetAddressOf()
    ));
    SetDebugObjectName(m_depthStencilView.Get(), "RenderTexture DSV");

    // Create depth SRV.
    /*CD3D11_SHADER_RESOURCE_VIEW_DESC depthShaderResourceViewDesc(D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
    ThrowIfFailed(m_device->CreateShaderResourceView(
        m_depthStencilBuffer.Get(),
        &depthShaderResourceViewDesc,
        m_depthShaderResourceView.ReleaseAndGetAddressOf()
    ));*/


    if(m_storeLinearDepth)
    {
        // Create a linear depth render target
        CD3D11_TEXTURE2D_DESC linearDepthRenderTargetDesc(
            DXGI_FORMAT_R32_FLOAT,
            static_cast<UINT>(width),
            static_cast<UINT>(height),
            1, // The render target view has only one texture.
            1, // Use a single mipmap level.
            D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
            D3D11_USAGE_DEFAULT,
            0,
            1
        );
        ThrowIfFailed(m_device->CreateTexture2D(
            &linearDepthRenderTargetDesc,
            nullptr,
            m_linearDepthRenderTarget.ReleaseAndGetAddressOf()
        ));

        SetDebugObjectName(m_linearDepthRenderTarget.Get(), "RenderTexture linear depth RT");
        // Create RTV.
        CD3D11_RENDER_TARGET_VIEW_DESC linearDepthRenderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R32_FLOAT);
        ThrowIfFailed(m_device->CreateRenderTargetView(
            m_linearDepthRenderTarget.Get(),
            &linearDepthRenderTargetViewDesc,
            m_linearDepthRenderTargetView.ReleaseAndGetAddressOf()
        ));
        SetDebugObjectName(m_linearDepthRenderTargetView.Get(), "RenderTexture linear depth RTV");

        // Create SRV.
        CD3D11_SHADER_RESOURCE_VIEW_DESC linearDepthShaderResourceViewDesc(D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R32_FLOAT);
        ThrowIfFailed(m_device->CreateShaderResourceView(
            m_linearDepthRenderTarget.Get(),
            &linearDepthShaderResourceViewDesc,
            m_linearDepthShaderResourceView.ReleaseAndGetAddressOf()
        ));
    }
    if (m_storeOcclusion)
    {
        // Create a linear depth render target
        CD3D11_TEXTURE2D_DESC occlusionRenderTargetDesc(
            DXGI_FORMAT_R32_FLOAT,
            static_cast<UINT>(width),
            static_cast<UINT>(height),
            1, // The render target view has only one texture.
            1, // Use a single mipmap level.
            D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
            D3D11_USAGE_DEFAULT,
            0,
            1
        );
        ThrowIfFailed(m_device->CreateTexture2D(
            &occlusionRenderTargetDesc,
            nullptr,
            m_occlusionRenderTarget.ReleaseAndGetAddressOf()
        ));

        SetDebugObjectName(m_occlusionRenderTarget.Get(), "RenderTexture occlusion RT");
        // Create RTV.
        CD3D11_RENDER_TARGET_VIEW_DESC occlusionRenderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R32_FLOAT);
        ThrowIfFailed(m_device->CreateRenderTargetView(
            m_occlusionRenderTarget.Get(),
            &occlusionRenderTargetViewDesc,
            m_occlusionRenderTargetView.ReleaseAndGetAddressOf()
        ));
        SetDebugObjectName(m_occlusionRenderTargetView.Get(), "RenderTexture occlusion RTV");

        // Create SRV.
        CD3D11_SHADER_RESOURCE_VIEW_DESC occlusionShaderResourceViewDesc(D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R32_FLOAT);
        ThrowIfFailed(m_device->CreateShaderResourceView(
            m_occlusionRenderTarget.Get(),
            &occlusionShaderResourceViewDesc,
            m_occlusionShaderResourceView.ReleaseAndGetAddressOf()
        ));
    }
    


    // Setup the viewport for rendering.
    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;

    SetDebugObjectName(m_shaderResourceView.Get(), "RenderTexture SRV");

    m_width = width;
    m_height = height;
}


void RenderTexture::ReleaseDevice() noexcept
{
    m_renderTargetView.Reset();
    m_shaderResourceView.Reset();
    m_renderTarget.Reset();

    m_depthStencilBuffer.Reset();
    m_depthStencilView.Reset();
    //m_depthShaderResourceView.Reset();

    m_linearDepthRenderTargetView.Reset();
    m_linearDepthShaderResourceView.Reset();
    m_linearDepthRenderTarget.Reset();

    m_occlusionRenderTargetView.Reset();
    m_occlusionShaderResourceView.Reset();
    m_occlusionRenderTarget.Reset();

    m_device.Reset();

    m_width = m_height = 0;
}

void RenderTexture::SetWindow(const RECT& output)
{
    // Determine the render target size in pixels.
    const auto width = size_t(std::max<LONG>(output.right - output.left, 1));
    const auto height = size_t(std::max<LONG>(output.bottom - output.top, 1));

    SizeResources(width, height);
}

void RenderTexture::SetRenderTarget(ID3D11DeviceContext* deviceContext, XMVECTOR clearColor)
{
    XMVECTORF32 color;
    color.v = XMColorSRGBToRGB(clearColor);

    deviceContext->ClearRenderTargetView(m_renderTargetView.Get(), color);
    if (m_storeLinearDepth)
    {
        deviceContext->ClearRenderTargetView(m_linearDepthRenderTargetView.Get(), Colors::White);
    }
    if (m_storeOcclusion)
    {
        deviceContext->ClearRenderTargetView(m_occlusionRenderTargetView.Get(), Colors::Black);
    }
    deviceContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    if (m_storeLinearDepth && m_storeOcclusion)
    {
        ID3D11RenderTargetView* rtvs[] = { m_renderTargetView.Get(), m_linearDepthRenderTargetView.Get(), m_occlusionRenderTargetView.Get()};
        deviceContext->OMSetRenderTargets(3, rtvs, m_depthStencilView.Get());
    }
    else if (m_storeLinearDepth)
    {
        ID3D11RenderTargetView* rtvs[] = { m_renderTargetView.Get(), m_linearDepthRenderTargetView.Get() };
        deviceContext->OMSetRenderTargets(2, rtvs, m_depthStencilView.Get());
    }
    else if (m_storeOcclusion)
    {
        ID3D11RenderTargetView* rtvs[] = { m_renderTargetView.Get(), m_occlusionRenderTargetView.Get() };
        deviceContext->OMSetRenderTargets(2, rtvs, m_depthStencilView.Get());
    }
    else
    {
        deviceContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
    }
    deviceContext->RSSetViewports(1, &m_viewport);
}

void RenderTexture::CreateOcclusionBlendState()
{
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = TRUE;

    // RT 0: color buffer — opaque blend
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    // RT 1: occlusion buffer — multiplicative blend (Src * Dest)
    blendDesc.RenderTarget[1].BlendEnable = TRUE;
    blendDesc.RenderTarget[1].SrcBlend = D3D11_BLEND_DEST_COLOR;
    blendDesc.RenderTarget[1].DestBlend = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    m_device->CreateBlendState(&blendDesc, m_occlusionBlendState.ReleaseAndGetAddressOf());
}
