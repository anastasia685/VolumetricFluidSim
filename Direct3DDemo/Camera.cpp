#include "pch.h"
#include "Camera.h"

Camera::Camera()
{
	position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);

	Update();
}

const XMMATRIX& Camera::GetViewMatrix() const
{
	return viewMatrix;
}

const XMFLOAT3 Camera::GetPosition() const
{
	return position;
}

const XMFLOAT3 Camera::GetRotation() const
{
	return rotation;
}

void Camera::SetPosition(float x, float y, float z)
{
	position = XMFLOAT3(x, y, z);

	//Update();
}

void Camera::SetRotation(float x, float y, float z)
{
	rotation = XMFLOAT3(x, y, z);

	//Update();
}

void Camera::Update()
{
	XMVECTOR upVec, positionVec, lookAtVec;
	float yaw, pitch, roll;
	XMMATRIX rotationMatrix;

	upVec = XMVectorSet(0.0f, 1.0, 0.0, 1.0f); // up
	positionVec = XMLoadFloat3(&position);
	lookAtVec = XMVectorSet(0.0, 0.0, 1.0f, 1.0f); // forward

	// rotations in radians
	pitch = rotation.x * 0.0174532f;
	yaw = rotation.y * 0.0174532f;
	roll = rotation.z * 0.0174532f;

	// rotation matrix
	rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	// transform up & forward vectors
	lookAtVec = XMVector3TransformCoord(lookAtVec, rotationMatrix);
	upVec = XMVector3TransformCoord(upVec, rotationMatrix);

	// translate target vector from origin to camera position
	lookAtVec = positionVec + lookAtVec;

	viewMatrix = XMMatrixLookAtLH(positionVec, lookAtVec, upVec);
}
