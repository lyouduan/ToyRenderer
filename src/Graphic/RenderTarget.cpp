#include "RenderTarget.h"

RenderTarget2D::RenderTarget2D(const std::wstring& name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format)
	: m_Width(Width), m_Height(Height), m_NumMips(NumMips), m_Format(Format)
{

	RenderMap.Create(name, Width, Height, NumMips, Format);
	DepthBuffer.Create(L"Depth buffer", Width, Height, DXGI_FORMAT_D32_FLOAT);

	SetViewportAndScissorRect();
}

RenderTarget2D::~RenderTarget2D()
{
}

void RenderTarget2D::SetViewportAndScissorRect()
{
	m_Viewport = { 0.0, 0.0, (float)m_Width, (float)m_Height, 0.0, 1.0 };
	m_ScissorRect = { 0, 0, (int)m_Viewport.Width, (int)m_Viewport.Height };
}
