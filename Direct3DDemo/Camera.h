#pragma once

using namespace DirectX;

class Camera
{
public:
	Camera();

	//static XMMATRIX GenerateProjectionMatrix(float fov, float aspectRatio, float nearZ, float farZ);

	const XMMATRIX& GetViewMatrix() const;

	const XMFLOAT3 GetPosition() const;
	const XMFLOAT3 GetRotation() const;

	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z);

	void Update();

protected:
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMMATRIX viewMatrix;
};
