#pragma once

#include "pch.h"
#include "Common\DeviceResources.h"
#include "Application_UWP_CXMain.h"

#include "API/Core/D3D12Renderer.h"

class UWPWindow : public API::Window
{
public:
	UWPWindow(const WindowProps& Props, Windows::UI::Core::CoreWindow^ Window)
		: Window(Props), m_Window(Window)
	{
		//m_Window = Windows::UI::Core::CoreWindow::GetForCurrentThread();
	}

	inline virtual void* GetNativeWindow() override
	{
		return (void*)m_Window.Get();
	}

private:
	Platform::Agile<Windows::UI::Core::CoreWindow> m_Window;
};

namespace Application_UWP_CX
{
	// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
	ref class App sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	public:
		App();

		// IFrameworkView methods.
		virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
		virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
		virtual void Load(Platform::String^ entryPoint);
		virtual void Run();
		virtual void Uninitialize();

	protected:
		// Application lifecycle event handlers.
		void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
		void OnResuming(Platform::Object^ sender, Platform::Object^ args);

		// Window event handlers.
		void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
		void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

		// DisplayInformation event handlers.
		void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

	private:
		API::D3D12Renderer* m_pRenderer;
		UWPWindow* m_pWindow;

		bool m_windowClosed;
		bool m_windowVisible;
	};
}

ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};
