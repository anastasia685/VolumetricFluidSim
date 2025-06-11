//
// Game.h
//

#pragma once

#include "pch.h"
#include "DeviceResources.h"
#include "PostProcess.h"
#include "StepTimer.h"
#include "RenderTexture.h"
#include "TextureManager.h"
#include "VolumetricEffect.hpp"
#include "TerrainEffect.hpp"
#include "PostProcessEffect.hpp"
#include "BlendEffect.hpp"
#include "GodRaysEffect.hpp"
#include "DisplacementEffect.hpp"
#include "SDFEffect.hpp"
#include "FluidSimEffect.hpp"
#include "GridMesh.hpp"
#include "CubeMesh.hpp"
#include "SphereMesh.hpp"
#include "OrthoMesh.hpp"
#include "FPCamera.h"
#include "Light.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;


// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game() = default;

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void ImGui(bool& wireframeMode);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

    std::unique_ptr<DirectX::CommonStates> m_states;
    std::unique_ptr<DirectX::BasicEffect> m_effect;
    //std::unique_ptr<DirectX::PrimitiveBatch<VertexType>> m_batch;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    std::unique_ptr<FPCamera> m_camera;
    std::unique_ptr<Camera> m_orthoCamera;

    std::unique_ptr<Light> m_ambient;
    std::unique_ptr<Light> m_sun;

    std::unique_ptr<DirectX::Keyboard> m_keyboard;
    std::unique_ptr<DirectX::Mouse> m_mouse;

    Matrix m_world;
    Matrix m_view;
    Matrix m_proj;


    std::unique_ptr<CustomEffects::DisplacementEffect> displacement_effect;
    std::unique_ptr<CustomEffects::SDFEffect> sceneSDF_effect;
    std::unique_ptr<CustomEffects::FluidSimEffect> fluid_effect;
    std::unique_ptr<CustomEffects::VolumetricEffect<VertexPosNormalTex>> volume_effect;
    std::unique_ptr<CustomEffects::BaseEffect<VertexPosNormalTex>> base_effect;
    std::unique_ptr<CustomEffects::TerrainEffect<VertexPosTex>> terrain_effect;
    std::unique_ptr<CustomEffects::PostProcessEffect<VertexPosNormalTex>> toneMap_effect;
    std::unique_ptr<CustomEffects::GodRaysEffect<VertexPosNormalTex>> godRays_effect;
    std::unique_ptr<CustomEffects::BlendEffect<VertexPosNormalTex>> blend_effect;

    std::unique_ptr<GridMesh<VertexPosTex>> grid_mesh;
    std::unique_ptr<CubeMesh<VertexPosNormalTex>> volumeBound_mesh;
    std::unique_ptr<OrthoMesh<VertexPosNormalTex>> postprocess_mesh;
    std::unique_ptr<SphereMesh<VertexPosNormalTex>> skybox_mesh;

    std::unique_ptr<TextureManager> m_textureManager;
    std::unique_ptr<DX::RenderTexture> m_mainSceneRT;
    std::unique_ptr<DX::RenderTexture> m_volumetricSceneRT;
    std::unique_ptr<DX::RenderTexture> m_postprocess0RT;
    std::unique_ptr<DX::RenderTexture> m_postprocess1RT;
};
