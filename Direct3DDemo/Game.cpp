//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr UINT MSAA_COUNT = 4;
    constexpr UINT MSAA_QUALITY = 0;
}

Game::Game() noexcept(false)
{
    //m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R10G10B10A2_UNORM);
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
    m_deviceResources->RegisterDeviceNotify(this);

    //m_mainSceneRT = std::make_unique<DX::RenderTexture>(m_deviceResources->GetBackBufferFormat());
    m_mainSceneRT = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT, true, true);
    m_volumetricSceneRT = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_postprocess0RT = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_postprocess1RT = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */

    m_ambient = std::make_unique<Light>();
    m_ambient->SetColor(XMFLOAT3(50/255.0f, 116/255.0f, 223/255.0f));
    m_ambient->SetIntensity(1.6f);

    m_sun = std::make_unique<Light>();
    m_sun->SetColor(XMFLOAT3(1.0f, 0.9f, 0.75f));
    m_sun->SetIntensity(15);
    m_sun->SetDirection(XMFLOAT3(0.09f, -0.04f, -0.23f));

    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_camera = std::make_unique<FPCamera>();
    m_camera->SetPosition(0, 5.f, -15.f);

    m_orthoCamera = std::make_unique<Camera>();
    m_orthoCamera->SetPosition(0, 0, -15);
    m_orthoCamera->Update();
    toneMap_effect->SetView(m_orthoCamera->GetViewMatrix());
    godRays_effect->SetView(m_orthoCamera->GetViewMatrix());
    blend_effect->SetView(m_orthoCamera->GetViewMatrix());

    
    ImGui_ImplDX11_Init(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    auto kb = m_keyboard->GetState();
    if (kb.Escape)
    {
        ExitGame();
    }
    auto mouse = m_mouse->GetState();
    m_mouse->SetMode(mouse.leftButton ? Mouse::MODE_RELATIVE : Mouse::MODE_ABSOLUTE);

    m_camera->Update(m_timer.GetElapsedSeconds(), kb, mouse, sceneSDF_effect->GetCPUSdf().data(), sceneSDF_effect->GetCPUSdfGradient().data());

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    static bool wireframeMode = false;
    auto context = m_deviceResources->GetD3DDeviceContext();
    
    m_view = m_camera->GetViewMatrix();

    auto size = m_deviceResources->GetOutputSize();
    auto width = static_cast<float>(size.right);
    auto height = static_cast<float>(size.bottom);


    m_deviceResources->PIXBeginEvent(L"Simulate Clouds");
    fluid_effect->SetDeltaTime(m_timer.GetElapsedSeconds());
    fluid_effect->SetElapsedTime(m_timer.GetTotalSeconds());
    fluid_effect->Compute(context, 8, 8, 8);
    m_deviceResources->PIXEndEvent();

    m_mainSceneRT->SetRenderTarget(context);

    m_deviceResources->PIXBeginEvent(L"Render Opaque");
    
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);

    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    context->RSSetState(m_states->CullNone());
    XMFLOAT3 cameraPos = m_camera->GetPosition();
    base_effect->SetWorld(XMMatrixTranslation(cameraPos.x, cameraPos.y, cameraPos.z));
    base_effect->SetView(m_view);
    base_effect->SetAmbientColor(m_ambient->GetColor());
    base_effect->SetSunColor(m_sun->GetColor());
    base_effect->SetSunIntensity(m_sun->GetIntensity());
    base_effect->SetSunDirection(m_sun->GetDirection());
    base_effect->Apply(context);
    skybox_mesh->Draw(context);

    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
    context->RSSetState(wireframeMode ? m_states->Wireframe() : m_states->CullCounterClockwise());

    terrain_effect->SetWorld(XMMatrixScaling(16, 16, 16) * XMMatrixTranslation(-8, -4, -8));
    terrain_effect->SetView(m_view);
    terrain_effect->SetAmbientColor(m_ambient->GetColor());
    terrain_effect->SetAmbientIntensity(m_ambient->GetIntensity());
    terrain_effect->SetSunColor(m_sun->GetColor());
    terrain_effect->SetSunIntensity(m_sun->GetIntensity());
    terrain_effect->SetSunDirection(m_sun->GetDirection());
    terrain_effect->SetCameraPosition(m_camera->GetPosition());
    terrain_effect->SetHeightmapSrv(displacement_effect->GetDisplacementSrv());
    terrain_effect->SetNormalmapSrv(displacement_effect->GetNormalSrv());
    terrain_effect->Apply(context);
    grid_mesh->Draw(context);
    terrain_effect->Unbind(context);

    m_deviceResources->PIXEndEvent();
    

    m_deviceResources->PIXBeginEvent(L"Render Volume");

    m_volumetricSceneRT->SetRenderTarget(context, Colors::Black);
    ID3D11RenderTargetView* rtvs[] = { m_volumetricSceneRT->GetRenderTargetView(), m_mainSceneRT->GetOcclusionRenderTargetView() };
    context->OMSetRenderTargets(2, rtvs, m_volumetricSceneRT->GetDepthStencilView());
    context->OMSetBlendState(m_mainSceneRT->GetOcclusionBlendState(), nullptr, 0xFFFFFFFF);

    // don't want wireframe of bounding volume
    context->RSSetState(m_states->CullNone());
    volume_effect->SetWorld(XMMatrixScaling(16, 16, 16) * XMMatrixTranslation(-8, -8, -8));
    volume_effect->SetView(m_view);
    volume_effect->SetMainCameraViewInv(XMMatrixInverse(nullptr, m_view));
    volume_effect->SetMainCameraProjInv(XMMatrixInverse(nullptr, m_proj));
    volume_effect->SetAmbientColor(m_ambient->GetColor());
    volume_effect->SetAmbientIntensity(m_ambient->GetIntensity());
    volume_effect->SetSunColor(m_sun->GetColor());
    volume_effect->SetSunIntensity(m_sun->GetIntensity());
    volume_effect->SetSunDirection(m_sun->GetDirection());
    volume_effect->SetCameraPosition(m_camera->GetPosition());
    volume_effect->SetDensityMapSrv(fluid_effect->GetDensitySrv());
    volume_effect->SetSceneColorSrv(m_mainSceneRT->GetShaderResourceView());
    volume_effect->SetSceneDepthSrv(m_mainSceneRT->GetLinearDepthShaderResourceView());
    volume_effect->Apply(context);

    volumeBound_mesh->Draw(context);
    volume_effect->Unbind(context);
    
    m_deviceResources->PIXEndEvent();

    // don't want wireframe of postprocessing orthomesh volume
    context->RSSetState(m_states->CullCounterClockwise());
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthDefault(), 0);


    m_deviceResources->PIXBeginEvent(L"Composite Volume");

    m_postprocess0RT->SetRenderTarget(context);
    // separate pass for compositing opaque and volumetric renders
    blend_effect->SetSrcSrv(m_volumetricSceneRT->GetShaderResourceView());
    blend_effect->SetDestSrv(m_mainSceneRT->GetShaderResourceView());
    blend_effect->SetWorld(XMMatrixScaling(width, height, 1) * XMMatrixTranslation(-width / 2, -height / 2, 0));
    blend_effect->Apply(context);
    postprocess_mesh->Draw(context);

    blend_effect->Unbind(context);

    m_deviceResources->PIXEndEvent();


    m_deviceResources->PIXBeginEvent(L"Postprocess - God Rays");

    m_postprocess1RT->SetRenderTarget(context);
    godRays_effect->SetSunDirection(m_sun->GetDirection());
    godRays_effect->SetCameraPosition(m_camera->GetPosition());
    godRays_effect->SetSceneViewMatrix(m_view);
    godRays_effect->SetSceneProjectionMatrix(m_proj);
    godRays_effect->SetColorSrv(m_postprocess0RT->GetShaderResourceView());
    godRays_effect->SetOcclusionSrv(m_mainSceneRT->GetOcclusionShaderResourceView());
    godRays_effect->SetWorld(XMMatrixScaling(width, height, 1)* XMMatrixTranslation(-width / 2, -height / 2, 0));
    godRays_effect->Apply(context);
    postprocess_mesh->Draw(context);

    godRays_effect->Unbind(context);

    m_deviceResources->PIXEndEvent();


    m_deviceResources->PIXBeginEvent(L"Postprocess - Tonemap");

    Clear();
    toneMap_effect->SetSceneColorSrv(m_postprocess1RT->GetShaderResourceView());
    toneMap_effect->SetWorld(XMMatrixScaling(width, height, 1) * XMMatrixTranslation(-width / 2, -height / 2, 0));
    toneMap_effect->Apply(context);
    postprocess_mesh->Draw(context);

    toneMap_effect->Unbind(context);

    m_deviceResources->PIXEndEvent();

    this->ImGui(wireframeMode);

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    XMVECTORF32 color;
    color.v = XMColorSRGBToRGB(Colors::CornflowerBlue);

    context->ClearRenderTargetView(renderTarget, color);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1200;
    height = 900;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto deviceContext = m_deviceResources->GetD3DDeviceContext();

    m_textureManager = std::make_unique<TextureManager>(device);
    m_textureManager->Load(device, deviceContext, L"ground", L"res/textures/ground.png");
    m_textureManager->Load(device, deviceContext, L"rock", L"res/textures/rock.png");

    m_mainSceneRT->SetDevice(device);
    m_volumetricSceneRT->SetDevice(device);
    m_postprocess0RT->SetDevice(device);
    m_postprocess1RT->SetDevice(device);

    // TODO: Initialize device dependent objects here (independent of window size).
    m_world = Matrix::Identity;

    m_states = std::make_unique<CommonStates>(device);

    m_effect = std::make_unique<BasicEffect>(device);
    m_effect->SetVertexColorEnabled(true);

    /*DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<VertexType>(device, m_effect.get(),
            m_inputLayout.ReleaseAndGetAddressOf())
    );

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_batch = std::make_unique<PrimitiveBatch<VertexType>>(context);*/


    displacement_effect = std::make_unique<CustomEffects::DisplacementEffect>(device, L"res/shaders/terrain_cs.cso", L"res/shaders/terrain_sdf_cs.cso", XMFLOAT3(136, 136, 1));
    sceneSDF_effect = std::make_unique<CustomEffects::SDFEffect>(device, L"res/shaders/scene_sdf_cs.cso", L"res/shaders/scene_sdf_gradient_cs.cso", XMFLOAT3(64, 64, 64));
    fluid_effect = std::make_unique<CustomEffects::FluidSimEffect>(device, deviceContext, XMFLOAT3(32, 32, 32));
    volume_effect = std::make_unique<CustomEffects::VolumetricEffect<VertexPosNormalTex>>(device, L"res/shaders/base_vs.cso", L"res/shaders/volume_ps.cso");
    base_effect = std::make_unique<CustomEffects::BaseEffect<VertexPosNormalTex>>(device, L"res/shaders/base_vs.cso", L"res/shaders/skybox_ps.cso");
    terrain_effect = std::make_unique<CustomEffects::TerrainEffect<VertexPosTex>>(device, L"res/shaders/tessellation_vs.cso", L"res/shaders/terrain_ps.cso", L"res/shaders/tessellation_hs.cso", L"res/shaders/tessellation_ds.cso");
    toneMap_effect = std::make_unique<CustomEffects::PostProcessEffect<VertexPosNormalTex>>(device, L"res/shaders/base_vs.cso", L"res/shaders/tonemap_ps.cso");
    godRays_effect = std::make_unique<CustomEffects::GodRaysEffect<VertexPosNormalTex>>(device, L"res/shaders/god_rays_vs.cso", L"res/shaders/god_rays_ps.cso");
    blend_effect = std::make_unique<CustomEffects::BlendEffect<VertexPosNormalTex>>(device, L"res/shaders/base_vs.cso", L"res/shaders/composite_volume_ps.cso");
    grid_mesh = std::make_unique<GridMesh<VertexPosTex>>(device, 17, 17);
    volumeBound_mesh = std::make_unique<CubeMesh<VertexPosNormalTex>>(device);
    skybox_mesh = std::make_unique<SphereMesh<VertexPosNormalTex>>(device);
    postprocess_mesh = std::make_unique<OrthoMesh<VertexPosNormalTex>>(device);


    terrain_effect->SetAlbedoTextureSrv(m_textureManager->Get(L"ground"));
    terrain_effect->SetAlbedoTextureSrv(m_textureManager->Get(L"rock"), 1);



    // run compute shaders that only need initialization, no updates
    m_deviceResources->PIXBeginEvent(L"Compute Terrain Displacement");
    displacement_effect->Compute(deviceContext, 17, 17, 1);
    m_deviceResources->PIXEndEvent();

    fluid_effect->SetSurfaceSRV(displacement_effect->GetDisplacementSrv());

    m_deviceResources->PIXBeginEvent(L"Compute Scene SDF");
    sceneSDF_effect->SetSimulationTransform(XMMatrixScaling(16, 16, 16) * XMMatrixTranslation(-8, -8, -8));
    sceneSDF_effect->AddSceneObject(device, displacement_effect->GetSDFSrv(), XMMatrixScaling(16, 16, 16) * XMMatrixTranslation(-8, -12, -8), 16);
    sceneSDF_effect->Compute(deviceContext, 16, 16, 16);
    m_deviceResources->PIXEndEvent();
    fluid_effect->SwapDensityBuffers();

    fluid_effect->SetSDFSRV(sceneSDF_effect->GetSrv());
    fluid_effect->SetSDFGradientSRV(sceneSDF_effect->GetGradientSrv());

    m_deviceResources->PIXBeginEvent(L"Compute Worley");
    volume_effect->Compute(deviceContext);
    m_deviceResources->PIXEndEvent();

    m_deviceResources->PIXBeginEvent(L"Compute Perlin");
    fluid_effect->ComputeNoise(deviceContext);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();


    m_mainSceneRT->SetWindow(size);
    m_volumetricSceneRT->SetWindow(size);
    m_postprocess0RT->SetWindow(size);
    m_postprocess1RT->SetWindow(size);

    // Create the projection matrix for 3D rendering.
    m_proj = XMMatrixPerspectiveFovLH(XM_PI / 4.f, float(size.right) / float(size.bottom), 0.1f, 100.f);

    m_effect->SetView(m_view);
    m_effect->SetProjection(m_proj);

    terrain_effect->SetView(m_view);
    terrain_effect->SetProjection(m_proj);

    // volumetrics are now rendered as full-screen effects, meaning they will require ortho camera and full-screen quad
    volume_effect->SetView(m_view);
    volume_effect->SetProjection(m_proj);

    base_effect->SetView(m_view);
    base_effect->SetProjection(m_proj);


    auto device = m_deviceResources->GetD3DDevice();
    auto width = static_cast<UINT>(size.right);
    auto height = static_cast<UINT>(size.bottom);


    auto ortho_matrix = XMMatrixOrthographicLH(width, height, 0.1f, 100.f);
    toneMap_effect->SetProjection(ortho_matrix);
    godRays_effect->SetProjection(ortho_matrix);
    blend_effect->SetProjection(ortho_matrix);
}

void Game::ImGui(bool& wireframeMode)
{
    //ImGui::ShowDemoWindow();

    ImGui::Text("FPS: %u", m_timer.GetFramesPerSecond());

    ImGui::Checkbox("Wireframe mode", &wireframeMode);

    bool playerControls = m_camera->GetPlayerControls();
    ImGui::Checkbox("Player Controls", &playerControls);
    m_camera->SetPlayerControls(playerControls);

    /*if (ImGui::CollapsingHeader("_Displacement Config"))
    {
        const GridMeshUtil::DisplacementType items[] = { GridMeshUtil::DisplacementType::DiamondSquare, GridMeshUtil::DisplacementType::Perlin, GridMeshUtil::DisplacementType::Simplex };
        static int item_selected_idx = 1;
        std::string combo_preview_value = GridMeshUtil::DisplacementTypeString(items[item_selected_idx]);
        if (ImGui::BeginCombo("Method", combo_preview_value.c_str()))
        {
            for (int n = 0; n < IM_ARRAYSIZE(items); n++)
            {
                const bool is_selected = (item_selected_idx == n);
                if (ImGui::Selectable(GridMeshUtil::DisplacementTypeString(items[n]).c_str(), is_selected))
                    item_selected_idx = n;

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();

            grid_mesh->SetDisplacementType(items[item_selected_idx]);
        }

        float amplitude = grid_mesh->GetAmplitude();
        ImGui::SliderFloat("Amplitude", &amplitude, 0.1f, 10.0f);
        grid_mesh->SetAmplitude(amplitude);

        float frequency = grid_mesh->GetFrequency();
        ImGui::SliderFloat("Frequency", &frequency, 0.01f, 2.0f);
        grid_mesh->SetFrequency(frequency);

        float gain = grid_mesh->GetGain();
        ImGui::SliderFloat("Gain", &gain, 0.01f, 1.0f);
        grid_mesh->SetGain(gain);

        float lacunarity = grid_mesh->GetLacunarity();
        ImGui::SliderFloat("Lacunarity", &lacunarity, 1.0f, 4.0f);
        grid_mesh->SetLacunarity(lacunarity);

        int octaves = grid_mesh->GetOctaves();
        ImGui::SliderInt("fBm octaves", &octaves, 1, 10);
        grid_mesh->SetOctaves(octaves);


        if (ImGui::Button("Update Buffers"))
        {
            grid_mesh->InitBuffers(m_deviceResources->GetD3DDevice());
        }
    }*/

    if (ImGui::CollapsingHeader("Displacement Config"))
    {
        int octaves = displacement_effect->GetOctaves();
        ImGui::SliderInt("Octaves", &octaves, 1, 20);
        displacement_effect->SetOctaves(octaves);

        float frequency = displacement_effect->GetFrequency();
        ImGui::SliderFloat("Frequency", &frequency, 0.01f, 5.0f);
        displacement_effect->SetFrequency(frequency);

        float amplitude = displacement_effect->GetAmplitude();
        ImGui::SliderFloat("Amplitude", &amplitude, 0.1f, 10.0f);
        displacement_effect->SetAmplitude(amplitude);

        float lacunarity = displacement_effect->GetLacunarity();
        ImGui::SliderFloat("Lacunarity", &lacunarity, 1.0f, 4.0f);
        displacement_effect->SetLacunarity(lacunarity);

        float gain = displacement_effect->GetGain();
        ImGui::SliderFloat("Gain", &gain, 0.01f, 1.0f);
        displacement_effect->SetGain(gain);

        XMFLOAT3 offset = displacement_effect->GetOffset();
        ImGui::SliderFloat2("Offset", &offset.x, -1000, 1000);
        displacement_effect->SetOffset(offset);

        if (ImGui::Button("Update Buffers"))
        {
            m_deviceResources->PIXBeginEvent(L"Compute Terrain Displacement");
            displacement_effect->Compute(m_deviceResources->GetD3DDeviceContext(), 17, 17, 1);
            m_deviceResources->PIXEndEvent();

            fluid_effect->SetSurfaceSRV(displacement_effect->GetDisplacementSrv());

            m_deviceResources->PIXBeginEvent(L"Compute Solid Mask");
            sceneSDF_effect->SetSimulationTransform(XMMatrixScaling(16, 16, 16) * XMMatrixTranslation(-8, -8, -8));
            sceneSDF_effect->AddSceneObject(m_deviceResources->GetD3DDevice(), displacement_effect->GetSDFSrv(), XMMatrixScaling(16, 16, 16) * XMMatrixTranslation(-8, -12, -8), 16);
            sceneSDF_effect->Compute(m_deviceResources->GetD3DDeviceContext(), 16, 16, 16);
            m_deviceResources->PIXEndEvent();
            fluid_effect->SwapDensityBuffers();

            fluid_effect->SetSDFSRV(sceneSDF_effect->GetSrv());
            fluid_effect->SetSDFGradientSRV(sceneSDF_effect->GetGradientSrv());
        }
    }

    if (ImGui::CollapsingHeader("Volume Params"))
    {
        float absorptionCoeff = volume_effect->GetAbsorptionCoeff();
        ImGui::SliderFloat("Absorption", &absorptionCoeff, 0.05f, 2.0f);
        volume_effect->SetAbsorptionCoeff(absorptionCoeff);

        float scatterCoeff = volume_effect->GetScatterCoeff();
        ImGui::SliderFloat("Scatter", &scatterCoeff, 0.05f, 2.0f);
        volume_effect->SetScatterCoeff(scatterCoeff);
    }

    if (ImGui::CollapsingHeader("Light Params"))
    {
        XMFLOAT3 ambientColor = m_ambient->GetColor();
        ImGui::ColorEdit3("Ambient color", reinterpret_cast<float*>(&ambientColor));
        m_ambient->SetColor(ambientColor);

        float ambientIntensity = m_ambient->GetIntensity();
        ImGui::SliderFloat("Ambient intensity", &ambientIntensity, 0.5f, 5.0f);
        m_ambient->SetIntensity(ambientIntensity);

        XMFLOAT3 sunColor = m_sun->GetColor();
        ImGui::ColorEdit3("Sun color", reinterpret_cast<float*>(&sunColor));
        m_sun->SetColor(sunColor);

        float sunIntensity = m_sun->GetIntensity();
        ImGui::SliderFloat("Sun intensity", &sunIntensity, 10, 40);
        m_sun->SetIntensity(sunIntensity);

        XMFLOAT3 sunDirection = m_sun->GetDirection();
        ImGui::SliderFloat3("Sun direction", &sunDirection.x, -1, 1);
        m_sun->SetDirection(sunDirection);
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_states.reset();
    m_effect.reset();
    //m_batch.reset();
    m_inputLayout.Reset();

    m_mainSceneRT->ReleaseDevice();
    m_volumetricSceneRT->ReleaseDevice();
    m_postprocess0RT->ReleaseDevice();
    m_postprocess1RT->ReleaseDevice();
    m_textureManager.reset();


    displacement_effect.reset();
    sceneSDF_effect.reset();
    fluid_effect.reset();
    volume_effect.reset();
    base_effect.reset();
    terrain_effect.reset();
    toneMap_effect.reset();
    godRays_effect.reset();
    blend_effect.reset();
    grid_mesh.reset();
    volumeBound_mesh.reset();
    skybox_mesh.reset();
    postprocess_mesh.reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
