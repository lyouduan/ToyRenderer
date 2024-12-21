#include "Camera.h"
#include <DirectXMath.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

using namespace DirectX;

Camera::Camera() :
	m_VerticalFOV(45.0), m_AspectRatio(780.0 / 1280.0), m_NearClip(0.1), m_FarClip(100.0)
{
	UpadteProjMat();
}

void Camera::UpadteViewMat()
{
	XMVECTOR x = XMVector3Dot(m_right, m_pos);
	XMVECTOR y = XMVector3Dot(m_up, m_pos);
	XMVECTOR z = XMVector3Dot(m_look, m_pos);

	m_ViewMat.r[0] = m_right;
	m_ViewMat.r[1] = m_up;
	m_ViewMat.r[2] = m_look;
	m_ViewMat.r[3] = XMVECTOR{ -XMVectorGetX(x),-XMVectorGetX(y),-XMVectorGetX(z), 1.0 };

	m_ViewProjMat = XMMatrixMultiply(m_ViewMat, m_ProjMat);
}

void Camera::UpadteProjMat()
{
	//m_ProjMat = XMMatrixPerspectiveFovLH(m_VerticalFOV, m_AspectRatio, m_NearClip, m_FarClip);

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
	m_pos = eye;
	m_look = at;
	m_up = up;
	m_ViewMat = XMMatrixLookAtLH(eye, at, up);
}

void Camera::SetLookAt(DirectX::XMVECTOR eye, DirectX::XMVECTOR at, DirectX::XMVECTOR vup)
{
	// based on left hand coordinate
	// lookforward
	m_pos = eye;

	m_look = XMVector3Normalize(XMVectorSubtract(at, eye));

	// right axis
	m_right = XMVector3Normalize(XMVector3Cross(vup, m_look));

	// up axis
	m_up = XMVector3Normalize(XMVector3Cross(m_look, m_right));

	UpadteViewMat();
}

void Camera::SetProjMatrix(float fov, float aspectRatio, float nearZ, float farZ)
{
	m_VerticalFOV = fov;
	m_AspectRatio = aspectRatio;
	m_NearClip = nearZ;
	m_FarClip = farZ;

	UpadteProjMat();
}

void Camera::CamerImGui()
{
	bool rotDirty = false;
	const auto dcheck = [](bool d, bool& carry) { carry = carry || d; };

	ImGui::Text("Orientation");
	dcheck(ImGui::SliderFloat("Position", &distance, -50.0f, 50.0f, "%.1f"), rotDirty);
	dcheck(ImGui::SliderFloat("FOV", &m_VerticalFOV, 0.1f, 89.0f, "%.1f"), rotDirty);
	dcheck(ImGui::SliderFloat("Pitch", &pitch, -45.0f, 45.0f, "%.1f"), rotDirty);
	dcheck(ImGui::SliderFloat("Yaw", &yaw, -89.0f, 89.0f, "%.1f"), rotDirty);
	dcheck(ImGui::SliderFloat("Roll", &roll, -180.0f, 180.0f, "%.1f"), rotDirty);

	{
		m_pos[2] += distance;

		// Rotate up and look vector about the right vector.
		XMMATRIX R = XMMatrixRotationAxis(m_look, XMConvertToRadians(-roll));
		m_up = XMVector3TransformNormal(m_up, R);
		m_right = XMVector3TransformNormal(m_right, R);

		R = XMMatrixRotationAxis(m_up, XMConvertToRadians(-yaw));
		m_right = XMVector3TransformNormal(m_right, R);
		m_look = XMVector3TransformNormal(m_look, R);

		R = XMMatrixRotationAxis(m_right, XMConvertToRadians(pitch));
		m_look = XMVector3TransformNormal(m_look, R);
		m_up = XMVector3TransformNormal(m_up, R);

		UpadteViewMat();

	}
}
