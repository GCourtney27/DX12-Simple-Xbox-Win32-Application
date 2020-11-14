#include "D3D12Renderer.h"

#include "d3dx12.h" // DX12 helpers
#include <dxgi1_2.h>
#include <stdexcept>
#if defined (_DEBUG)
#include <cassert>
#endif

// No matter which platform we are compiling on tell the 
// linker we NEED these libraries.
#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "DXGI.lib")

#define ThrowIfFailed(hr) if (FAILED(hr)) { __debugbreak(); }

namespace API
{

	D3D12Renderer::D3D12Renderer(Window* pWindow)
		:
		m_pWindow(pWindow),
		m_FrameIndex(0),
		m_ForceUseWarpAdapter(false)
	{
		CreateDevice();
		CreateSwapChain();
		CreateCommandAllocators();
		CreateResources();
		CreateSyncObjects();

		m_pCommandList->Close();
		ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		WaitForGPU();
	}

	D3D12Renderer::~D3D12Renderer()
	{

	}

	void D3D12Renderer::RenderFrame()
	{
		HRESULT hr = m_pCommandAllocators[m_FrameIndex]->Reset();
		ThrowIfFailed(hr);

		hr = m_pCommandList->Reset(m_pCommandAllocators[m_FrameIndex].Get(), nullptr);
		ThrowIfFailed(hr);

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_FrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
		
		m_pCommandList->OMSetRenderTargets(1, &GetNextSwapChainRTVHandle(), FALSE, nullptr);
		const float clearColor[] = { 0.1f, 0.1f, 0.3f, 1.0f };
		m_pCommandList->ClearRenderTargetView(GetNextSwapChainRTVHandle(), clearColor, 0, nullptr);
		m_pCommandList->RSSetViewports(1, &m_ClientViewPort);
		m_pCommandList->RSSetScissorRects(1, &m_ClientScissorRect);

		ID3D12Resource* TransitionResources[] = { m_pRenderTargets[m_FrameIndex].Get() };
		ResourceBarrier(m_pCommandList.Get(), 1, TransitionResources, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		//m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_FrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		hr = m_pCommandList->Close();
		ThrowIfFailed(hr);
		ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		WaitForGPU();

		hr = m_pSwapChain->Present(0, 0);
		ThrowIfFailed(hr);

		MoveToNextFrame();
	}

	void D3D12Renderer::WaitForGPU()
	{
		HRESULT hr;
		// Schedule a Signal command in the queue.
		hr = m_pCommandQueue->Signal(m_pFence.Get(), m_FenceValues[m_FrameIndex]);
		ThrowIfFailed(hr);

		// Wait until the fence has been processed.
		hr = m_pFence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_FenceEvent);
		ThrowIfFailed(hr);

		WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);

		// Increment the fence value for the current frame.
		m_FenceValues[m_FrameIndex]++;
	}

	void D3D12Renderer::CreateDevice()
	{
		UINT DxgiFactoryFlags = 0u;
#if defined(_DEBUG)
		// If the project is in a debug build, enable debugging via SDK Layers.
		{
			Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				DxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}
#endif

		ThrowIfFailed(CreateDXGIFactory2(DxgiFactoryFlags, IID_PPV_ARGS(&m_pDXGIFactory)));

		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
		GetHardwareAdapter(&adapter);


		// Create the Direct3D 12 API device object
		HRESULT hr = D3D12CreateDevice(
			adapter.Get(),					// The hardware adapter.
			D3D_FEATURE_LEVEL_12_0,			// Make sure the device can support DirectX 12
			IID_PPV_ARGS(&m_pDevice)		// Returns the Direct3D device created.
		);

		/*
			Important:
			If application is not set to "Game" under "App type" in the " View details" page on the xbox dev home
			the adapter creation will fail and the WARP adapter will be used by default. This is not desirable for games.

			If you are making a App than you can ignore this.
		*/

		if (FAILED(hr) || m_ForceUseWarpAdapter)
		{
			// If the initialization fails, or we explicitly ask for it (usually we dont want it), fall back to the WARP device.
			// For more information on WARP, see: 
			// https://go.microsoft.com/fwlink/?LinkId=286690
			

			Microsoft::WRL::ComPtr<IDXGIAdapter> warpAdapter;
			ThrowIfFailed(m_pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

			hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice));
		}

		ThrowIfFailed(hr);

		// Create the command queue.
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)));
		m_pCommandQueue->SetName(L"Graphics Command Queue");
	}

	void D3D12Renderer::CreateResources()
	{
		// Create descriptor heaps for render target views and depth stencil views.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FRAME_BUFFER_COUNT;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap)));
		m_pRTVHeap->SetName(L"Render Target View Heap");
		m_rtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);


		// Create the render targets for each buffer in the swapchain.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
		for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			ThrowIfFailed(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pRenderTargets[i])));
			m_pDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);

			rtvHandle.Offset(m_rtvDescriptorSize);
		}

		// Fill out the Viewport.
		m_ClientViewPort.TopLeftX = 0;
		m_ClientViewPort.TopLeftY = 0;
		m_ClientViewPort.Width = static_cast<FLOAT>(m_pWindow->GetWidth());
		m_ClientViewPort.Height = static_cast<FLOAT>(m_pWindow->GetHeight());
		m_ClientViewPort.MinDepth = 0.0f;
		m_ClientViewPort.MaxDepth = 1.0f;

		// Fill out Scissor Rect.
		m_ClientScissorRect.left = 0;
		m_ClientScissorRect.top = 0;
		m_ClientScissorRect.right = m_pWindow->GetWidth();
		m_ClientScissorRect.bottom = m_pWindow->GetHeight();
	}

	void D3D12Renderer::ResourceBarrier(ID3D12GraphicsCommandList* pCommandList, UINT NumBarriers, ID3D12Resource** pResources, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
	{
		constexpr int MaxBarrierTransitions = 8;
#if defined (_DEBUG)
		assert(NumBarriers <= MaxBarrierTransitions);
#endif
		// Batching transitions is much faster for the GPU than one at a time.
		D3D12_RESOURCE_BARRIER Barriers[MaxBarrierTransitions];
		for (UINT i = 0; i < NumBarriers; ++i)
		{
			Barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			Barriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			Barriers[i].Transition.pResource = pResources[i];
			Barriers[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			Barriers[i].Transition.StateBefore = BeforeState;
			Barriers[i].Transition.StateAfter = AfterState;
		}
		pCommandList->ResourceBarrier(NumBarriers, Barriers);
	}

	void D3D12Renderer::GetHardwareAdapter(IDXGIAdapter1** ppAdapter)
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
		*ppAdapter = nullptr;

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_pDXGIFactory->EnumAdapters1(adapterIndex, &adapter); adapterIndex++)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
			if (SUCCEEDED(hr))
			{
				break;
			}
		}

		*ppAdapter = adapter.Detach();
	}

	void D3D12Renderer::CreateSwapChain()
	{
		DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
		SwapChainDesc.Width = m_pWindow->GetWidth();				// Match the size of the window.
		SwapChainDesc.Height = m_pWindow->GetHeight();
		SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SwapChainDesc.Stereo = false;
		SwapChainDesc.SampleDesc.Count = 1;							// Don't use multi-sampling.
		SwapChainDesc.SampleDesc.Quality = 0;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// All Windows Universal apps must use _FLIP_ SwapEffects.
		SwapChainDesc.Flags = 0;
		SwapChainDesc.Scaling = DXGI_SCALING_NONE;
		SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		// Create the render target for the window. Depnding on the platform, compile in the correct creation method.
		HRESULT hr = S_OK;
		Microsoft::WRL::ComPtr<IDXGISwapChain1> TempSwapChain; // Swapchain creation needs DXGISwapChain1 base class. So make a temporary one.
#if defined (WINDOWS)
		// Win32 applications use HWND objects to describe the window.
		hr = m_pDXGIFactory->CreateSwapChainForHwnd(
			m_pCommandQueue.Get(),
			*(reinterpret_cast<HWND*>(m_pWindow->GetNativeWindow())),
			&SwapChainDesc,
			nullptr,
			nullptr,
			&TempSwapChain
		);
		ThrowIfFailed(hr);

		hr = m_pDXGIFactory->MakeWindowAssociation(*(reinterpret_cast<HWND*>(m_pWindow->GetNativeWindow())), DXGI_MWA_NO_ALT_ENTER);
		ThrowIfFailed(hr);

#elif defined (UWP)
		// Windows Store apps use Code Window objects to describe the window.
		hr = m_pDXGIFactory->CreateSwapChainForCoreWindow(
			m_pCommandQueue.Get(),
			reinterpret_cast<IUnknown*>(m_pWindow->GetNativeWindow()),
			&SwapChainDesc,
			nullptr,
			&TempSwapChain
		);
		ThrowIfFailed(hr);
#endif
		TempSwapChain.As(&m_pSwapChain);							// Save the created swapchain and detach/destroy the temporary one.
		m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();	// Get the first available buffer in the swapchain to render to.
	}

	void D3D12Renderer::CreateCommandAllocators()
	{
		// Create the graphics memory for the command buffers.
		for (unsigned int i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			ThrowIfFailed(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocators[i])));
		}
		// Create the main command list we will use to execute commands on.
		HRESULT hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocators[0].Get(), NULL, IID_PPV_ARGS(&m_pCommandList));
		ThrowIfFailed(hr);
	}

	void D3D12Renderer::CreateSyncObjects()
	{
		// Create the syncronization objects so we dont corrupt on other frames processing.
		HRESULT hr;
		hr = m_pDevice->CreateFence(m_FenceValues[m_FrameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
		ThrowIfFailed(hr);
		m_FenceValues[m_FrameIndex]++;

		m_FenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (m_FenceEvent == nullptr)
		{
			throw std::runtime_error("Failed to create fence for graphics operations.");
		}
	}

	void D3D12Renderer::MoveToNextFrame()
	{
		// Let the render context know we are finished processing a frame and to move to the next one.

		HRESULT hr;
		// Schedule a Signal command in the queue.
		const UINT64 currentFenceValue = m_FenceValues[m_FrameIndex];
		hr = m_pCommandQueue->Signal(m_pFence.Get(), currentFenceValue);
		ThrowIfFailed(hr);

		// Advance the frame index.
		m_FrameIndex = (m_FrameIndex + 1) % FRAME_BUFFER_COUNT;

		// Check to see if the next frame is ready to start.
		if (m_pFence->GetCompletedValue() < m_FenceValues[m_FrameIndex])
		{
			hr = m_pFence->SetEventOnCompletion(m_FenceValues[m_FrameIndex], m_FenceEvent);
			ThrowIfFailed(hr);
			WaitForSingleObjectEx(m_FenceEvent, INFINITE, FALSE);
		}

		// Set the fence value for the next frame.
		m_FenceValues[m_FrameIndex] = currentFenceValue + 1;
	}
	
	void D3D12Renderer::InternalShutdown()
	{

	}
}
