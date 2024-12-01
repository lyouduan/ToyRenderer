#include "Camera.h"
#include <DirectXMath.h>

using namespace DirectX;

Camera::Camera() :
	m_VerticalFOV(45.0), m_AspectRatio(780.0 / 1280.0), m_NearClip(0.1), m_FarClip(100.0)
{
	UpadteProjMat();
}

void Camera::UpadteProjMat()
{
	//m_ProjMat = XMMatrixPerspectiveFovLH(fov, m_AspectRatio, m_NearClip, m_FarClip);

	float fovRadians = XMConvertToRadians(m_VerticalFOV);
	float height = m_NearClip * tan(fovRadians / 2.0);
	float width = height * m_AspectRatio;

	XMVECTOR m0 = XMVectorSet(m_NearClip / width, 0, 0, 0);
	XMVECTOR m1 = XMVectorSet(0, m_NearClip / height, 0, 0);
	XMVECTOR m2 = XMVectorSet(0, 0, m_FarClip / (m_FarClip - m_NearClip), 1);
	XMVECTOR m3 = XMVectorSet(0, 0, -(m_FarClip * m_NearClip) / (m_FarClip - m_NearClip), 0);

	m_ProjMat.r[0] = m0;
	m_ProjMat.r[1] = m1;
	m_ProjMat.r[2] = m2;
	m_ProjMat.r[3] = m3;

	m_ViewProjMat = XMMatrixMultiply(m_ViewMat, m_ProjMat);
}

void Camera::SetEyeAtUp(DirectX::XMVECTOR eye, DirectX::XMVECTOR at, DirectX::XMVECTOR up)
{
	m_ViewMat = XMMatrixLookAtLH(eye, at, up);
}

void Camera::SetLookAt(DirectX::XMVECTOR eye, DirectX::XMVECTOR at, DirectX::XMVECTOR vup)
{
	// based on left hand coordinate
	// lookforward
	XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(at, eye));

	// right axis
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(vup, forward));

	// up axis
	XMVECTOR up = XMVector3Normalize(XMVector3Cross(forward, right));

	XMVECTOR x = XMVector3Dot(right, eye);
	XMVECTOR y = XMVector3Dot(up, eye);
	XMVECTOR z = XMVector3Dot(forward, eye);

	m_ViewMat.r[0] = right;
	m_ViewMat.r[1] = up;
	m_ViewMat.r[2] = forward;
	m_ViewMat.r[3] = XMVECTOR{ -XMVectorGetX(x),-XMVectorGetX(y),-XMVectorGetX(z), 1.0 };
}

void Camera::SetProjMatrix(float fov, float aspectRatio, float nearZ, float farZ)
{
	m_VerticalFOV = fov;
	m_AspectRatio = aspectRatio;
	m_NearClip = nearZ;
	m_FarClip = farZ;

	UpadteProjMat();
}
