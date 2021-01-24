// WinRT

#include "pch.h"

#include "API/Core/D3D12Renderer.h"


using namespace winrt::Windows;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Numerics;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::ViewManagement;


class UWPWindow : public API::Window
{
public:
	UWPWindow(const WindowProps& Props, CoreWindow* pWindow)
		: Window(Props), m_Window(pWindow)
	{
	}

	inline virtual void* GetNativeWindow() override
	{
		return static_cast<void*>(winrt::get_abi(*m_Window));
	}

private:

	CoreWindow* m_Window;
};

struct App : public winrt::implements<App, IFrameworkView>
{
	CompositionTarget m_target{ nullptr };
	VisualCollection m_visuals{ nullptr };
	Visual m_selected{ nullptr };
	float2 m_offset{};

	bool m_exit = false;
	UWPWindow* m_Window;
	API::D3D12Renderer* m_Renderer;

	IFrameworkView CreateView()
	{
		return *this;
	}

	void Initialize(CoreApplicationView const& applicationView)
	{
		applicationView.Activated({ this, &App::OnActivated });
	}

	void Load(winrt::hstring const&)
	{
	}

	void Uninitialize()
	{
		delete m_Renderer;
	}

	void Run()
	{
		while (!m_exit)
		{
			CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

			m_Renderer->RenderFrame();
		}
	}

	void OnActivated(CoreApplicationView const& /*applicationView*/, IActivatedEventArgs const& args)
	{
		auto view = ApplicationView::GetForCurrentView();

		auto desiredSize = Size((float)m_Window->GetWidth(), (float)m_Window->GetHeight());
		view.SetPreferredMinSize(desiredSize);
		view.TryResizeView(desiredSize);
	}

	void SetWindow(CoreWindow const& window)
	{
		window.Activate();

		window.PointerPressed({ this, &App::OnPointerPressed });
		window.PointerMoved({ this, &App::OnPointerMoved });

		window.PointerReleased([&](auto && ...)
			{
				m_selected = nullptr;
			});
		window.Closed([this](auto&&, auto&&) { m_exit = true; });
		window.ResizeCompleted([this](auto&&, auto&&) {
			CoreWindow Window = CoreWindow::GetForCurrentThread();
			uint32_t Width = static_cast<uint32_t>(Window.Bounds().Width);
			uint32_t Height = static_cast<uint32_t>(Window.Bounds().Height);

			m_Window->OnResize(Width, Height);
			m_Renderer->OnWindowResize();
			});

		// UWP on Xbox One triggers a back request whenever the B button is pressed
		// which can result in the app being suspended if unhandled.
		auto navigation = SystemNavigationManager::GetForCurrentView();
		navigation.BackRequested([](const winrt::Windows::Foundation::IInspectable&, const BackRequestedEventArgs& args)
			{
				args.Handled(true);
			});

		static CoreWindow s_Window = CoreWindow::GetForCurrentThread();
		m_Window = new UWPWindow(API::Window::WindowProps{ 1280, 720 }, &s_Window);
		m_Renderer = new API::D3D12Renderer(m_Window);
	}

	void OnPointerPressed(IInspectable const&, PointerEventArgs const& args)
	{
		float2 const point = args.CurrentPoint().Position();

		for (Visual visual : m_visuals)
		{
			float3 const offset = visual.Offset();
			float2 const size = visual.Size();

			if (point.x >= offset.x &&
				point.x < offset.x + size.x &&
				point.y >= offset.y &&
				point.y < offset.y + size.y)
			{
				m_selected = visual;
				m_offset.x = offset.x - point.x;
				m_offset.y = offset.y - point.y;
			}
		}

		if (m_selected)
		{
			m_visuals.Remove(m_selected);
			m_visuals.InsertAtTop(m_selected);
		}
		else
		{
			AddVisual(point);
		}
	}

	void OnPointerMoved(IInspectable const&, PointerEventArgs const& args)
	{
		if (m_selected)
		{
			float2 const point = args.CurrentPoint().Position();

			m_selected.Offset(
				{
					point.x + m_offset.x,
					point.y + m_offset.y,
					0.0f
				});
		}
	}

	void AddVisual(float2 const point)
	{
		Compositor compositor = m_visuals.Compositor();
		SpriteVisual visual = compositor.CreateSpriteVisual();

		static Color colors[] =
		{
			{ 0xDC, 0x5B, 0x9B, 0xD5 },
			{ 0xDC, 0xED, 0x7D, 0x31 },
			{ 0xDC, 0x70, 0xAD, 0x47 },
			{ 0xDC, 0xFF, 0xC0, 0x00 }
		};

		static unsigned last = 0;
		unsigned const next = ++last % _countof(colors);
		visual.Brush(compositor.CreateColorBrush(colors[next]));

		float const BlockSize = 100.0f;

		visual.Size(
			{
				BlockSize,
				BlockSize
			});

		visual.Offset(
			{
				point.x - BlockSize / 2.0f,
				point.y - BlockSize / 2.0f,
				0.0f,
			});

		m_visuals.InsertAtTop(visual);

		m_selected = visual;
		m_offset.x = -BlockSize / 2.0f;
		m_offset.y = -BlockSize / 2.0f;
	}
};

class ViewProviderFactory : public winrt::implements<ViewProviderFactory, IFrameworkViewSource>
{
public:
	IFrameworkView CreateView()
	{
		return winrt::make<App>();
	}
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	auto viewProviderFactory = winrt::make<ViewProviderFactory>();
	CoreApplication::Run(viewProviderFactory);
}
