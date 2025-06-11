#include "pch.h"
#include "FPCamera.h"


static float SampleSDF_CPU(float* sdf, XMFLOAT3 sdfUV, XMINT3 resolution)
{
	// sdfUV is in [0,1]^3
	XMFLOAT3 pos {
		sdfUV.x * (resolution.x - 1),
		sdfUV.y * (resolution.y - 1),
		sdfUV.z * (resolution.z - 1)
	};

	XMINT3 p0 {
		(int)floor(pos.x),
		(int)floor(pos.y),
		(int)floor(pos.z)
	};
	XMINT3 p1 {
		std::min(p0.x + 1, resolution.x - 1),
		std::min(p0.y + 1, resolution.y - 1),
		std::min(p0.z + 1, resolution.z - 1)
	};

	XMFLOAT3 f {
		pos.x - p0.x,
		pos.y - p0.y,
		pos.z - p0.z
	};

	auto idx = [&](int x, int y, int z) {
		return (z * resolution.y * resolution.x) + (y * resolution.x) + x;
		};

	float v000 = sdf[idx(p0.x, p0.y, p0.z)];
	float v100 = sdf[idx(p1.x, p0.y, p0.z)];
	float v010 = sdf[idx(p0.x, p1.y, p0.z)];
	float v110 = sdf[idx(p1.x, p1.y, p0.z)];
	float v001 = sdf[idx(p0.x, p0.y, p1.z)];
	float v101 = sdf[idx(p1.x, p0.y, p1.z)];
	float v011 = sdf[idx(p0.x, p1.y, p1.z)];
	float v111 = sdf[idx(p1.x, p1.y, p1.z)];

	float v00 = v000 * (1 - f.x) + v100 * f.x;
	float v10 = v010 * (1 - f.x) + v110 * f.x;
	float v01 = v001 * (1 - f.x) + v101 * f.x;
	float v11 = v011 * (1 - f.x) + v111 * f.x;

	float v0 = v00 * (1 - f.y) + v10 * f.y;
	float v1 = v01 * (1 - f.y) + v11 * f.y;

	return v0 * (1 - f.z) + v1 * f.z;
}
static XMFLOAT3 SampleSDFGradient_CPU(XMFLOAT3* sdfGradient, XMFLOAT3 sdfUV, XMINT3 resolution)
{
	XMFLOAT3 pos{
		sdfUV.x * (resolution.x - 1),
		sdfUV.y * (resolution.y - 1),
		sdfUV.z * (resolution.z - 1)
	};

	XMINT3 p0{
		(int)floor(pos.x),
		(int)floor(pos.y),
		(int)floor(pos.z)
	};
	XMINT3 p1{
		std::min(p0.x + 1, resolution.x - 1),
		std::min(p0.y + 1, resolution.y - 1),
		std::min(p0.z + 1, resolution.z - 1)
	};

	XMFLOAT3 f{
		pos.x - p0.x,
		pos.y - p0.y,
		pos.z - p0.z
	};

	auto idx = [&](int x, int y, int z) {
		return (z * resolution.y * resolution.x) + (y * resolution.x) + x;
		};

	// Load all 8 corners
	XMVECTOR g000 = XMLoadFloat3(&sdfGradient[idx(p0.x, p0.y, p0.z)]);
	XMVECTOR g100 = XMLoadFloat3(&sdfGradient[idx(p1.x, p0.y, p0.z)]);
	XMVECTOR g010 = XMLoadFloat3(&sdfGradient[idx(p0.x, p1.y, p0.z)]);
	XMVECTOR g110 = XMLoadFloat3(&sdfGradient[idx(p1.x, p1.y, p0.z)]);
	XMVECTOR g001 = XMLoadFloat3(&sdfGradient[idx(p0.x, p0.y, p1.z)]);
	XMVECTOR g101 = XMLoadFloat3(&sdfGradient[idx(p1.x, p0.y, p1.z)]);
	XMVECTOR g011 = XMLoadFloat3(&sdfGradient[idx(p0.x, p1.y, p1.z)]);
	XMVECTOR g111 = XMLoadFloat3(&sdfGradient[idx(p1.x, p1.y, p1.z)]);

	// Interpolate
	XMVECTOR g00 = XMVectorLerp(g000, g100, f.x);
	XMVECTOR g10 = XMVectorLerp(g010, g110, f.x);
	XMVECTOR g01 = XMVectorLerp(g001, g101, f.x);
	XMVECTOR g11 = XMVectorLerp(g011, g111, f.x);

	XMVECTOR g0 = XMVectorLerp(g00, g10, f.y);
	XMVECTOR g1 = XMVectorLerp(g01, g11, f.y);

	XMVECTOR result = XMVectorLerp(g0, g1, f.z);

	XMFLOAT3 out;
	XMStoreFloat3(&out, result);
	return out;
}

void FPCamera::Update(float dt, const Keyboard::State& kb, const Mouse::State& mouse, float* sdf, XMFLOAT3* sdfGradient)
{
	m_frameTime = dt;

	XMFLOAT3 increment = XMFLOAT3(0, 0, 0);

	if (kb.Up || kb.W)
		increment = increment + moveForward();

	if (kb.Down || kb.S)
		increment = increment + moveBackward();

	if (kb.Left || kb.A)
		increment = increment + moveLeft();

	if (kb.Right || kb.D)
		increment = increment + moveRight();
	
	if (kb.E)
		increment = increment + moveUp();

	if (kb.Q)
		increment = increment + moveDown();

	XMFLOAT3 newPosition = position + increment;


	//--- COLLISION CHECK
	if (m_playerControls)
	{
		XMMATRIX sdfTransform = XMMatrixScaling(16, 16, 16) * XMMatrixTranslation(-8, -8, -8);
		XMMATRIX sdfTransformInv = XMMatrixInverse(nullptr, sdfTransform);
		XMVECTOR wp = XMLoadFloat3(&newPosition);

		XMFLOAT3 sdfSpace;
		XMStoreFloat3(&sdfSpace, XMVector3Transform(wp, sdfTransformInv));


		XMFLOAT3 grad = SampleSDFGradient_CPU(sdfGradient, sdfSpace, XMINT3(66, 66, 66));
		XMVECTOR normal = XMVector3Normalize(XMLoadFloat3(&grad));

		float slopeCos = XMVectorGetX(XMVector3Dot(normal, XMVectorSet(0, 1, 0, 0))); // cos(theta)
		float maxSlopeAngle = 75.0f; // max incline in degrees
		float minAllowedCos = cosf(XMConvertToRadians(maxSlopeAngle));

		if (sdfSpace.x >= 0 && sdfSpace.y >= 0 && sdfSpace.z >= 0 && slopeCos >= minAllowedCos) // inside simulation bounds, check collisions
		{
			float val = SampleSDF_CPU(sdf, sdfSpace, XMINT3(66, 66, 66));

			/*if (val < 0.35)
			{
				newPosition = position;
			}*/

			// instead of stopping, project camera onto surface
			wp -= normal * (val - 2.f); // exact surface point

			// values in sdf field are in world-space already, so just set newPosition to that
			XMStoreFloat3(&newPosition, wp);
		}
		else
		{
			newPosition = position;
		}
	}

	position = newPosition;


	if (mouse.positionMode == Mouse::MODE_RELATIVE)
	{
		Vector3 delta = Vector3(float(mouse.x), float(mouse.y), 0.f) * m_lookSpeed;

		rotation.x += delta.y;
		rotation.y += delta.x;
	}

	

	Camera::Update();
}

void FPCamera::SetPlayerControls(bool enableControls)
{
	m_playerControls = enableControls;

	m_speed = enableControls ? 2.5f : 8.f;
}

XMFLOAT3 FPCamera::moveForward()
{
	float speed = m_frameTime * m_speed;

	// Convert degrees to radians.
	float radians = rotation.y * 0.0174532f;

	return XMFLOAT3(sinf(radians), 0, cosf(radians)) * speed;
}


XMFLOAT3 FPCamera::moveBackward()
{
	// Update the backward movement based on the frame time
	float speed = m_frameTime * m_speed;

	// Convert degrees to radians.
	float radians = rotation.y * 0.0174532f;

	return XMFLOAT3(-sinf(radians), 0, -cosf(radians)) * speed;
}

XMFLOAT3 FPCamera::moveLeft()
{
	// Update the forward movement based on the frame time
	float speed = m_frameTime * m_speed;

	// Convert degrees to radians.
	float radians = rotation.y * 0.0174532f;

	return XMFLOAT3(-cosf(radians), 0, sinf(radians)) * speed;
}

XMFLOAT3 FPCamera::moveRight()
{
	// Update the forward movement based on the frame time
	float speed = m_frameTime * m_speed;

	// Convert degrees to radians.
	float radians = rotation.y * 0.0174532f;

	// Update the position.
	/*position.x += cosf(radians) * speed;
	position.z -= sinf(radians) * speed;*/

	return XMFLOAT3(cosf(radians), 0, -sinf(radians)) * speed;
}

XMFLOAT3 FPCamera::moveUp()
{
	// Update the upward movement based on the frame time
	float speed = m_frameTime * m_speed;// *0.5f;

	// Update the height position.
	//position.y += speed;

	return XMFLOAT3(0, 1, 0) * speed;
}


XMFLOAT3 FPCamera::moveDown()
{
	// Update the downward movement based on the frame time
	float speed = m_frameTime * m_speed;// *0.5f;

	// Update the height position.
	//position.y -= speed;

	return XMFLOAT3(0, -1, 0) * speed;
}
