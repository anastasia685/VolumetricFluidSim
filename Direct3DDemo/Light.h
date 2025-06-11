#pragma once

using namespace DirectX;

class Light
{
public:
	Light();

public:
	XMFLOAT3 GetDirection() const { return m_direction; };
	XMFLOAT3 GetPosition() const { return m_position; };
	XMFLOAT3 GetColor() const { return m_color; };
	float GetIntensity() const { return m_intensity; };

	void SetDirection(const XMFLOAT3& direction) { m_direction = direction; };
	void SetPosition(const XMFLOAT3& position) { m_position = position; };
	void SetColor(const XMFLOAT3& color) { m_color = color; };
	void SetIntensity(float intensity) { m_intensity = intensity; };

protected:
	XMFLOAT3 m_direction;
	XMFLOAT3 m_position;
	XMFLOAT3 m_color;
	float m_intensity;
};
