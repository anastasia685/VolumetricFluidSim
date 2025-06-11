#pragma once

#include "Camera.h"
#include "SimpleMath.h"

using namespace DirectX;
using namespace SimpleMath;

class FPCamera : public Camera
{
public:
	FPCamera() : Camera() {};

	void Update(float dt, const Keyboard::State& kb, const Mouse::State& mouse, float* sdf = nullptr, XMFLOAT3* sdfGradient = nullptr);

	bool GetPlayerControls() const { return m_playerControls; };
	void SetPlayerControls(bool enableControls);

private:
	float m_speed = 8.f, m_lookSpeed = 0.2f, m_frameTime; // speed = 2.5f
	bool m_playerControls = false;

	XMFLOAT3 moveForward();
	XMFLOAT3 moveBackward();
	XMFLOAT3 moveLeft();
	XMFLOAT3 moveRight();
	XMFLOAT3 moveUp();
	XMFLOAT3 moveDown();
};
