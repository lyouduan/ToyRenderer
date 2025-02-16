#pragma once

#include <DirectXMath.h>

class Camera
{
public:
	Camera();
	~Camera();

	void SetPosition(float x, float y, float z);
	void SetLen(float fov, float aspectRatio, float nearZ, float farZ);

	// orient camera space
	void SetEyeAtUp(DirectX::XMVECTOR eye, DirectX::XMVECTOR at, DirectX::XMVECTOR up);
	void SetLookAt(DirectX::XMVECTOR eye, DirectX::XMVECTOR at, DirectX::XMVECTOR up);

	void SetProjMatrix(const DirectX::XMMATRIX ProjMat) { m_ProjMat = ProjMat; }

	DirectX::XMMATRIX GetViewMat() const { return m_ViewMat; }
	DirectX::XMMATRIX GetProjMat() const { return m_ProjMat; }

	DirectX::XMFLOAT3 GetRight3f() const { return mRight; }
	DirectX::XMFLOAT3 GetUp3f() const { return mUp; }
	DirectX::XMFLOAT3 GetLook3f() const { return mLook; }

	DirectX::XMFLOAT3 GetPosition3f() const { return mPosition; }

	void CamerImGui();

	float GetRoll() { return roll; }
	float GetPitch() { return pitch; }
	float GetYaw() { return yaw; }
	float GetNearZ() { return mNearZ; }
	float GetFarZ() { return mFarZ; }
	float GetAspect() { return mAspect; }
	float GetFovY() { return mFovY; }

	void MoveCamera(float moveForward, float strafe, float vertical);
	// after modifying camera position/orientation, call to rebuild the view matrix

	void Rotate(DirectX::XMVECTOR axis, float degrees);
	void Pitch(float degrees);
	void Yaw(float degrees);
	void Roll(float degrees);

	void Reset();

	void UpdateViewMat();
	void UpdateProjMat();

private:

	// cache frustum properties
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0;

	// camera coordinate system with coordinates relative to world space
	DirectX::XMFLOAT3 homePosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	DirectX::XMMATRIX m_ViewMat = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX m_ProjMat = DirectX::XMMatrixIdentity();

	// Euler Angle
	bool viewDirty = true;

	float roll = 0.01;
	float pitch = 0.01;
	float yaw = 0.01;
	float lastRoll = 0.01;
	float lastPitch = 0.01;
	float lastYaw = 0.01;

	float moveForward = 0.0;
	float moveUp = 0.0;
	float moveRight = 0.0;
};

