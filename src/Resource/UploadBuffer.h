#pragma once
#include "GpuBuffer.h"

// uploadbuffer 
// mapping the data to uploadbuffer which is GPU-readable buffer

class UploadBuffer : public GpuResource
{
public:
	virtual ~UploadBuffer() { Destroy(); }

	void Create(ID3D12Device* device, const std::wstring& name, size_t BufferSize);

	void* Map();

	void Unmap(size_t begin = 0, size_t end = -1);

	size_t GetBufferSize() const { return m_BufferSize; }
protected:
	
	size_t m_BufferSize;
};

