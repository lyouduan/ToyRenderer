#pragma once
#include "GpuResource.h"

class UploadBuffer : public GpuResource
{
public:
	virtual ~UploadBuffer() { Destroy(); }

	void Create(const std::wstring& name, size_t BufferSize);

	void* Map();

	void Unmap(size_t Begin = 0, size_t end = 0);

	size_t GetBufferSize() const { return m_BufferSize; }

protected:
	size_t m_BufferSize;
};

