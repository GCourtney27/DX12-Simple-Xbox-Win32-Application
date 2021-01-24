#include <pch.h>

#include "D3D12Renderer.h"
#include <d3dcompiler.h>

// No matter which platform we are compiling on tell the 
// linker we NEED these libraries.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define ThrowIfFailed(hr) if (FAILED(hr)) { __debugbreak(); }


namespace API
{

	D3D12Renderer::D3D12Renderer(Window* pWindow)
		:
		m_pWindow(pWindow),
		m_FrameIndex(0),
		m_ForceUseWarpAdapter(false),
		m_WindowResizeComplete(true)
	{
		CreateDevice();
		CreateSwapChain();
		CreateCommandAllocators();
		CreateResources();
		CreateSyncObjects();
		
		LoadAssets();

		// Submit resources to the GPU for creation.
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
		if (m_pWindow->GetIsMinimized() || !m_WindowResizeComplete) return;

		ThrowIfFailed(m_pCommandAllocators[m_FrameIndex]->Reset());
		ThrowIfFailed(m_pCommandList->Reset(m_pCommandAllocators[m_FrameIndex].Get(), nullptr));

		ID3D12Resource* TransitionResources[] = { m_pSwapChainRenderTargets[m_FrameIndex].Get() };
		ResourceBarrier(m_pCommandList.Get(), _countof(TransitionResources), TransitionResources, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		D3D12_CPU_DESCRIPTOR_HANDLE SwapChainCPUHandle = GetNextSwapChainRTVHandle();
		m_pCommandList->OMSetRenderTargets(1, &SwapChainCPUHandle, FALSE, nullptr);
		const float ClearColor[] = { 0.1f, 0.1f, 0.3f, 1.0f };
		m_pCommandList->ClearRenderTargetView(GetNextSwapChainRTVHandle(), ClearColor, 0, nullptr);
		m_pCommandList->RSSetViewports(1, &m_ClientViewPort);
		m_pCommandList->RSSetScissorRects(1, &m_ClientScissorRect);

		m_pCommandList->SetPipelineState(m_pPipelineState.Get());
		m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

		// Render the model.
		m_Model.Render(m_pCommandList.Get());

		ResourceBarrier(m_pCommandList.Get(), _countof(TransitionResources), TransitionResources, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		ThrowIfFailed(m_pCommandList->Close());
		ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		WaitForGPU();

		ThrowIfFailed(m_pSwapChain->Present(0, 0));

		MoveToNextFrame();
	}

	void D3D12Renderer::OnWindowResize()
	{
		if (!m_pWindow->GetIsMinimized())
		{
			if (m_WindowResizeComplete)
			{
				m_WindowResizeComplete = false;
				WaitForGPU();

				UpdateSizeDependentResources();
				m_FrameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
			}
		}
		m_WindowResizeComplete = true;
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
		if (!m_pRTVHeap)
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = c_FrameBufferCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap)));
			m_pRTVHeap->SetName(L"Render Target View Heap");
			m_rtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}


		// Create the render targets for each buffer in the swapchain.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
		for (int i = 0; i < c_FrameBufferCount; i++)
		{
			ThrowIfFailed(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pSwapChainRenderTargets[i])));
			m_pDevice->CreateRenderTargetView(m_pSwapChainRenderTargets[i].Get(), nullptr, rtvHandle);

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
		SwapChainDesc.BufferCount = c_FrameBufferCount;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// All Windows Universal apps must use _FLIP_ SwapEffects.
		SwapChainDesc.Flags = 0;
		SwapChainDesc.Scaling = DXGI_SCALING_NONE;
		SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		// Create the render target for the window. Depnding on the platform, compile in the correct creation method.
		HRESULT hr = S_OK;
		Microsoft::WRL::ComPtr<IDXGISwapChain1> TempSwapChain; // Swapchain creation needs DXGISwapChain1 base class. So make a temporary one.
#if defined (PLATFORM_WIN32)
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

#elif defined (PLATFORM_UWP)
		// Windows Store apps use CoreWindow objects to describe the window.
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
		for (unsigned int i = 0; i < c_FrameBufferCount; i++)
		{
			ThrowIfFailed(m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocators[i])));
		}
		// Create the main command list we will use to execute commands on.
		HRESULT hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocators[0].Get(), NULL, IID_PPV_ARGS(&m_pCommandList));
		ThrowIfFailed(hr);
	}

	void API::D3D12Renderer::LoadAssets()
	{
		m_Model.Create(m_pDevice.Get());

		// Create an empty root signature.
		{
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			::Microsoft::WRL::ComPtr<ID3DBlob> signature;
			::Microsoft::WRL::ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
			ThrowIfFailed(m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature)));
		}

		// Create the pipeline state, which includes compiling and loading shaders.
		{
			::Microsoft::WRL::ComPtr<ID3DBlob> vertexShader;
			::Microsoft::WRL::ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif

			ThrowIfFailed(D3DCompileFromFile(L"Shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
			ThrowIfFailed(D3DCompileFromFile(L"Shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

			// Define the vertex input layout.
			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.pRootSignature = m_pRootSignature.Get();
			psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineState)));
		}

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
		m_FrameIndex = (m_FrameIndex + 1) % c_FrameBufferCount;

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
	
	void D3D12Renderer::UpdateSizeDependentResources()
	{
		uint32_t WindowWidth = m_pWindow->GetWidth();
		uint32_t WindowHeight = m_pWindow->GetHeight();

		// Resize the swapchain
		{
			// Destroy the back buffer textures
			for (uint8_t i = 0; i < c_FrameBufferCount; ++i)
			{
				m_pSwapChainRenderTargets[i].Reset();
				m_FenceValues[i] = m_FenceValues[m_FrameIndex];
			}

			HRESULT hr;
			DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
			m_pSwapChain->GetDesc(&SwapChainDesc);
			hr = m_pSwapChain->ResizeBuffers(c_FrameBufferCount, WindowWidth, WindowHeight, SwapChainDesc.BufferDesc.Format, SwapChainDesc.Flags);
			ThrowIfFailed(hr);
		}
		
		CreateResources();
	}


	//
	// Model Class Implementation
	//

	void Model::Create(ID3D12Device* pDevice)
	{
		ScreenSpaceVertex TriangleVertices[] =
		{
			{ { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
		};

		const UINT VertexBufferSize = sizeof(TriangleVertices);
		m_DrawArgs.NumVerticies = VertexBufferSize / sizeof(ScreenSpaceVertex);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		auto HeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);
		ThrowIfFailed(pDevice->CreateCommittedResource(
			&HeapProps,
			D3D12_HEAP_FLAG_NONE,
			&ResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_VertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, TriangleVertices, sizeof(TriangleVertices));
		m_VertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
		m_VertexBufferView.StrideInBytes = sizeof(ScreenSpaceVertex);
		m_VertexBufferView.SizeInBytes = VertexBufferSize;

	}

	void Model::Render(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
		pCommandList->DrawInstanced(m_DrawArgs.NumVerticies, 1, 0, 0);
	}

	void Model::Destroy()
	{
	}

	bool Model::LoadResources()
	{
		return true;
	}

}
