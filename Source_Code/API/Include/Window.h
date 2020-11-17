/*
*
	File: Window.h
	Source: None

	Author: Garrett Courtney

	Description:
	Describes a generic window to render to. Platforms should override
	this window and pass it to the render context.

*/

#pragma once

namespace API
{
	class Window
	{
	public:
		/*
			Described the basic properties of a window.
			@param Width: The width in pixels of the window.
			@param Height: The height in pixels of the window.
		*/
		struct WindowProps
		{
			int Width;
			int Height;
		};
	public:
		virtual ~Window() = default;

		// Returns the platform window to render to.
		inline virtual void* GetNativeWindow() = 0;

		// Returns the width of the window in pixles.
		int GetWidth() const { return m_Props.Width; }
		// Returns the height of the window in pixles.
		int GetHeight() const { return m_Props.Height; }

		void OnResize(int Width, int Height) { m_Props.Width = Width; m_Props.Height = Height; }
		void SetIsMinimized(bool Minimized) { m_Minimized = Minimized; }
		bool GetIsMinimized() const { return m_Minimized; }
	protected:
		Window(const WindowProps& Props)
			: m_Props(Props) {}

	protected:
		WindowProps m_Props;
		bool m_Minimized = false;
	};
}
