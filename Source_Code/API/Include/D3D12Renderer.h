/*

	File: D3D12Renderer.h
	Source: API/D3D12Renderer.cpp

	Author: Garrett Courtney

	Description:
	A basic renderer that used the DirectX 12 API.
	
*/
#pragma once

#include "Window.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h> // For ComPtr

namespace API
{
	class D3D12Renderer
	{
	public:
		D3D12Renderer(Window* pWindow);
		~D3D12Renderer();


		void RenderFrame();
		void OnWindowResize();

		// Override the renderer and force the use of the less effiecient WARP adapter. Should be done at compile time.
		inline constexpr void SetForceUseWARPAdapter(bool ShouldUse) { m_ForceUseWarpAdapter = ShouldUse; }

	protected:
		// Create the D3D device context.
		void CreateDevice();
		// Create the swapchain to render into.
		void CreateSwapChain();
		// Create the textures and heaps
		void CreateResources();
		// Create GPU sync objects.
		void CreateSyncObjects();
		// Create the graphics memory for the commands to execute.
		void CreateCommandAllocators();

		// Helpers
		// Get the next available swapchain render target 
		inline D3D12_CPU_DESCRIPTOR_HANDLE GetNextSwapChainRTVHandle()
		{
			D3D12_CPU_DESCRIPTOR_HANDLE Handle = m_pRTVHeap->GetCPUDescriptorHandleForHeapStart();
			Handle.ptr += static_cast<SIZE_T>(m_FrameIndex * m_rtvDescriptorSize);
			return Handle;
		}
		void ResourceBarrier(ID3D12GraphicsCommandList* pCommandList, UINT NumBarriers, ID3D12Resource** pResources, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState);

		// Query the hardware adapter on the sytem.
		void GetHardwareAdapter(IDXGIAdapter1** ppAdapter);
		// Waits for the GPU to finish all pending work.
		void WaitForGPU();
		// Tell the render context to move to the next frame.
		void MoveToNextFrame();

		void UpdateSizeDependentResources();

	private:
		Window* m_pWindow;

		UINT m_FrameIndex;
		bool m_ForceUseWarpAdapter;
		static const unsigned int FRAME_BUFFER_COUNT = 3;
		bool m_WindowResizeComplete;

		// Sync objects.
		::Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
		UINT64 m_FenceValues[FRAME_BUFFER_COUNT];
		HANDLE m_FenceEvent;

		// Rasterizer resources.
		D3D12_VIEWPORT m_ClientViewPort;
		D3D12_RECT m_ClientScissorRect;

		// Device and creation.
		::Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
		::Microsoft::WRL::ComPtr<IDXGIFactory4> m_pDXGIFactory;

		// Command exeution values.
		::Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocators[FRAME_BUFFER_COUNT];
		::Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
		::Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;

		// SwapChain resoures.
		::Microsoft::WRL::ComPtr<IDXGISwapChain3> m_pSwapChain;
		::Microsoft::WRL::ComPtr<ID3D12Resource> m_pSwapChainRenderTargets[FRAME_BUFFER_COUNT];
		::Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pRTVHeap;
		UINT m_rtvDescriptorSize;


	};
}

