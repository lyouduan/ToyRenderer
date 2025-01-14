#include "D3D12CommandContext.h"
#include "DXSamplerHelper.h"
#include "D3D12RHI.h"

TD3D12CommandContext::TD3D12CommandContext() 
{
}

TD3D12CommandContext::~TD3D12CommandContext()
{
	DestroyCommandContext();
}

void TD3D12CommandContext::CreateCommandContext(ID3D12Device* Device)
{
	DescriptorCache = std::make_unique<TD3D12DescriptorCache>(Device);

	// Create Fence
	ThrowIfFailed(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));

	// create Direct type CommandQueue
	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(Device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&CommandQueue)));

	// Create direct type CommandAllocator
	ThrowIfFailed(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CommandListAlloc.GetAddressOf())));

	// create direct type CommandList
	ThrowIfFailed(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandListAlloc.Get(), nullptr, IID_PPV_ARGS(CommandList.GetAddressOf())));

	// close commandlist
	// This is because the first time we refer to the command list we will Reset it,
	// and it needs to be closed before calling Reset.
	ThrowIfFailed(CommandList->Close());
}

void TD3D12CommandContext::DestroyCommandContext()
{
	// Don't need to do anything
	// ComPtr will destroy resource automatically

}

void TD3D12CommandContext::Transition(TD3D12Resource* resource, D3D12_RESOURCE_STATES afterState)
{
	if (resource->CurrentState == afterState)
		return;

	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource->D3DResource.Get(),
		resource->CurrentState,
		afterState);

	CommandList->ResourceBarrier(1, &barrier);

	resource->CurrentState = afterState;
}

void TD3D12CommandContext::ResetCommandAllocator()
{
	// command list allocators can only be reset when the associated command lists have finished execution on the GPU.
	// So we should use fences to determine GPU execution progress
	ThrowIfFailed(CommandListAlloc->Reset());
}

void TD3D12CommandContext::ResetCommandList()
{
	// Reusing the commandlist can reuses memory
	// A command list can be reset after it has been added to the command Queue via ExecuteCommandList.
	// Before an app calls reset, the commandlist must be in the "closed" state.
	// After Reset succeds, the command list is left in the "recording" state.
	ThrowIfFailed(CommandList->Reset(CommandListAlloc.Get(), nullptr));
}

void TD3D12CommandContext::ExecuteCommandLists()
{
	// Done recording commands
	ThrowIfFailed(CommandList->Close());

	// Add the commandlist to the queue for execution
	ID3D12CommandList* cmdLists[] = { CommandList.Get() };
	CommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

void TD3D12CommandContext::FlushCommandQueue()
{
	// advance the value to mark commands up to this fence point
	CurrentFenceValue++;

	// add an instruction to the command queue to set a new fence point.
	// Because we are on the GPU timeline, the new fence point won't be set untill the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(CommandQueue->Signal(Fence.Get(), CurrentFenceValue));

	// wait until the GPU has completed commands up to this fence point
	if (Fence->GetCompletedValue() < CurrentFenceValue)
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, nullptr);

		// fire event when GPU hits current fence
		ThrowIfFailed(Fence->SetEventOnCompletion(CurrentFenceValue, eventHandle));

		// wait until the GPU hits current fence event is fired
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void TD3D12CommandContext::EndFrame()
{
	DescriptorCache->Reset();
}


