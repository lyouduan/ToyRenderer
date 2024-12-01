#pragma once

#include <DirectXMath.h>

class Camera
{
public:
	Camera();

	void UpadteProjMat();

	// orient camera space
	void SetEyeAtUp(DirectX::XMVECTOR eye, DirectX::XMVECTOR at, DirectX::XMVECTOR up);
	void SetLookAt(DirectX::XMVECTOR eye, DirectX::XMVECTOR at, DirectX::XMVECTOR up);

	void SetProjMatrix(float fov, float aspectRatio, float nearZ, float farZ);

	void SetProjMatrix(const DirectX::XMMATRIX ProjMat) { m_ProjMat = ProjMat; }
	void SetZClip(float nearZ, float farZ) { m_NearClip = nearZ; m_FarClip = farZ; }
	void SetAspectRatio(float AspectRatio) 
	{ 
		m_AspectRatio = AspectRatio;
		UpadteProjMat();
	}

	const DirectX::XMMATRIX& GetViewMat() const { return m_ViewMat; }
	const DirectX::XMMATRIX& GetProjMat() const { return m_ProjMat; }
	const DirectX::XMMATRIX& GetViewProjMat() const { return m_ViewProjMat; }

private:

	float m_VerticalFOV;
	float m_AspectRatio;
	float m_NearClip;
	float m_FarClip;

	DirectX::XMMATRIX m_ViewMat;
	DirectX::XMMATRIX m_ProjMat;
	DirectX::XMMATRIX m_ViewProjMat;
};

