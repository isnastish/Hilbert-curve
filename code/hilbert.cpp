#include "hilbert.h"

namespace hilbert
{
	static hilbert::HilbertApp* app_ptr = 0;

	static LRESULT CALLBACK win32MainWindowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		// Propagate all the argument to the member function, 
		// since we cannot assign mainWindowProc to lpfnWndProc, 
		// because it takes an instance of HilbertApp class as the first argument.
		return app_ptr->mainWindowProc(window, msg, wparam, lparam);
	}

	HilbertApp::HilbertApp(HINSTANCE instance)
		:
		m_is_running(false),
		m_instance(instance),
		m_window(0),
		m_client_width(1080),
		m_client_height(720)
	{
		app_ptr = this;
	}

	LRESULT CALLBACK HilbertApp::mainWindowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		LRESULT result = 0;

		switch (msg)
		{
			case WM_DESTROY:
			case WM_CLOSE:
			{
				m_is_running = false;
			}break;
			default:
			{
				result = DefWindowProc(window, msg, wparam, lparam);
			}break;
		}

		return result;
	}

	bool HilbertApp::initMainWindow()
	{
		WNDCLASSA wnd_class = {};
		wnd_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wnd_class.lpfnWndProc = win32MainWindowProc;
		wnd_class.cbClsExtra = 0;
		wnd_class.cbWndExtra = 0;
		wnd_class.hInstance = m_instance;
		wnd_class.hIcon = LoadIcon(0, IDI_APPLICATION);
		wnd_class.hCursor = LoadCursor(0, IDC_ARROW);
		wnd_class.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
		wnd_class.lpszMenuName = 0;
		wnd_class.lpszClassName = "HilbertCurveWindowClass";

		if (!RegisterClassA(&wnd_class))
		{
			// emit an error! (message box)
			return false;
		}

		// Compute an actual window dimensions based on desired(client) metrics.
		RECT window_rect{ 0, 0, m_client_width, m_client_height };
		AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

		int32_t width = window_rect.right - window_rect.left;
		int32_t height = window_rect.bottom - window_rect.top;

		m_window = CreateWindowExA(0, wnd_class.lpszClassName, "HilbertCurve", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, wnd_class.hInstance, 0);

		if (!m_window)
		{
			// emit an error! (message box)
			return false;
		}

		ShowWindow(m_window, SW_SHOW);
		UpdateWindow(m_window);

		return true;
	}

	void HilbertApp::initOffscreenBuffer()
	{
		m_offscreen_buffer.width = m_client_width;
		m_offscreen_buffer.height = m_client_height;
		m_offscreen_buffer.bpp = 4;
		m_offscreen_buffer.pitch = m_offscreen_buffer.width * m_offscreen_buffer.bpp;

		m_offscreen_buffer.info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		m_offscreen_buffer.info.bmiHeader.biWidth = m_client_width;
		m_offscreen_buffer.info.bmiHeader.biHeight = m_client_height; // bottom-up
		m_offscreen_buffer.info.bmiHeader.biPlanes = 1;
		m_offscreen_buffer.info.bmiHeader.biBitCount = 32;
		m_offscreen_buffer.info.bmiHeader.biCompression = BI_RGB;
		m_offscreen_buffer.info.bmiHeader.biSizeImage = 0;

		m_offscreen_buffer.memory = VirtualAlloc(0, (m_offscreen_buffer.width * m_offscreen_buffer.height * m_offscreen_buffer.bpp), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		assert(m_offscreen_buffer.memory);
	}


	void HilbertApp::displayOffscreenBufferInWindow() const
	{
		HDC ctx = GetDC(m_window);
		StretchDIBits(ctx, 0, 0, m_client_width, m_client_height, 0, 0, m_offscreen_buffer.width,
			m_offscreen_buffer.height, m_offscreen_buffer.memory, &m_offscreen_buffer.info, DIB_RGB_COLORS, SRCCOPY);
	}

	int32_t HilbertApp::randRange(int32_t range_min, int32_t range_max)
	{
		int32_t result = static_cast<int32_t>(static_cast<float>(rand()) / (RAND_MAX + 1) * (range_max - range_min) + range_min);
		return result;
	}

	uint32_t HilbertApp::roundReal32ToUint32(float value)
	{
		return static_cast<uint32_t>(value + 0.5f);
	}

	void HilbertApp::drawRectangle(OffscreenBuffer *buffer, int32_t rect_min_x, int32_t rect_min_y, int32_t rect_max_x, int32_t rect_max_y, Vec4 color, int32_t frame_width = 0)
	{
		if (rect_min_x < 0)               rect_min_x = 0;
		if (rect_min_x >= buffer->width)  rect_min_x = buffer->width;
		if (rect_min_y < 0)               rect_min_y = 0;
		if (rect_min_y >= buffer->height) rect_min_y = buffer->height;

		if (rect_max_x < 0)               rect_max_x = 0;
		if (rect_max_x >= buffer->width)  rect_max_x = buffer->width;
		if (rect_max_y < 0)               rect_max_y = 0;
		if (rect_max_y >= buffer->height) rect_max_y = buffer->height;

		uint32_t color_u32 = (HilbertApp::roundReal32ToUint32(color.b * 255.0f) |
			(HilbertApp::roundReal32ToUint32(color.g * 255.0f) << 8) |
			(HilbertApp::roundReal32ToUint32(color.r * 255.0f) << 16) |
			(HilbertApp::roundReal32ToUint32(color.r * 255.0f) << 24));

		bool frame_mode = (frame_width != 0);
		if (frame_mode)
		{
			assert(2 * frame_width <= (rect_max_x - rect_min_x));
			assert(2 * frame_width <= (rect_max_y - rect_min_y));
		}

		uint8_t* row = (static_cast<uint8_t*>(buffer->memory) + rect_min_y * buffer->pitch + rect_min_x * buffer->bpp);
		for (int32_t y = rect_min_y;
			y < rect_max_y;
			++y)
		{
			uint32_t* pixel = reinterpret_cast<uint32_t*>(row);
			for (int32_t x = rect_min_x;
				x < rect_max_x;
				++x)
			{
				if (frame_mode)
				{
					if ((x < (rect_min_x + frame_width) || (x > (rect_max_x - frame_width - 1))) ||
						(y < (rect_min_y + frame_width) || (y > (rect_max_y - frame_width - 1))))
					{
						*pixel = color_u32;
					}
				}
				else
				{
					*pixel = color_u32;
				}
				pixel++;
			}
			row += buffer->pitch;
		}
	}

	bool HilbertApp::init()
	{
		return initMainWindow();
	}

	int32_t HilbertApp::run()
	{
		initOffscreenBuffer();
		HilbertApp::drawRectangle(&m_offscreen_buffer, 0, 0, m_offscreen_buffer.width, m_offscreen_buffer.height, Vec4(0.54f, 0.52f, 0.47f));
		renderHilbertCurves();

		MSG msg = {};
		m_is_running = true;
		while (m_is_running)
		{
			while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}
			displayOffscreenBufferInWindow();
		}

		return msg.wParam;
	}

	void HilbertApp::renderHilbertCurves()
	{
		int32_t global_offset_x = 30;
		int32_t global_offset_y = 30;
		int32_t segment_dim_x = 20;
		int32_t segment_dim_y = 4;

		{
			// setup frame
			int32_t frame_width = 5;
			int32_t frame_minx = global_offset_x - 10;
			int32_t frame_miny = global_offset_y - 10;
			int32_t frame_maxx = frame_minx + 32 * segment_dim_x;
			int32_t frame_maxy = frame_miny + 32 * segment_dim_x;

			// draw frame
			HilbertApp::drawRectangle(&m_offscreen_buffer, frame_minx, frame_miny, frame_maxx, frame_maxy, Vec4(0.4f, 0.8f, 0.0f), frame_width);

			HilbertCurve curve = {};
			curve.offset_x = global_offset_x;
			curve.offset_y = global_offset_y;
			curve.segment_x = 0;
			curve.segment_y = 0;
			curve.segment_width = segment_dim_x;
			curve.segment_height = segment_dim_y;
			curve.direction = 0;
			curve.rotation = 1;
			curve.order = 5;

			std::thread thread(curve, &m_offscreen_buffer);
			thread.detach();
		}

		global_offset_x += 32*segment_dim_x + 50;

		{
			HilbertCurve curve = {};
			curve.offset_x = global_offset_x;
			curve.offset_y = global_offset_y;
			curve.segment_x = 0;
			curve.segment_y = 0;
			curve.segment_width = segment_dim_x;
			curve.segment_height = segment_dim_y;
			curve.direction = 0;
			curve.rotation = 1;
			curve.order = 4;

			std::thread thread(curve, &m_offscreen_buffer);
			thread.detach();
		}

		global_offset_y += 16*segment_dim_x + 50;

		{
			HilbertCurve curve = {};
			curve.offset_x = global_offset_x;
			curve.offset_y = global_offset_y;
			curve.segment_x = 0;
			curve.segment_y = 0;
			curve.segment_width = segment_dim_x;
			curve.segment_height = segment_dim_y;
			curve.direction = 0;
			curve.rotation = 1;
			curve.order = 3;

			std::thread thread(curve, &m_offscreen_buffer);
			thread.detach();
		}

		global_offset_x += 8*segment_dim_x + 50;

		{
			HilbertCurve curve = {};
			curve.offset_x = global_offset_x;
			curve.offset_y = global_offset_y;
			curve.segment_x = 0;
			curve.segment_y = 0;
			curve.segment_width = segment_dim_x;
			curve.segment_height = segment_dim_y;
			curve.direction = 0;
			curve.rotation = 1;
			curve.order = 2;

			std::thread thread(curve, &m_offscreen_buffer);
			thread.detach();
		}

		global_offset_y += 100;

		{
			HilbertCurve curve = {};
			curve.offset_x = global_offset_x;
			curve.offset_y = global_offset_y;
			curve.segment_x = 0;
			curve.segment_y = 0;
			curve.segment_width = segment_dim_x;
			curve.segment_height = segment_dim_y;
			curve.direction = 0;
			curve.rotation = 1;
			curve.order = 1;

			std::thread thread(curve, &m_offscreen_buffer);
			thread.detach();
		}
	}

	void HilbertCurve::makeStep(OffscreenBuffer *buffer, int32_t direction, Vec4 color)
	{
		switch (direction & 3)
		{
		case 0: // right
		{
			int32_t minx = (offset_x + segment_x * segment_width);
			int32_t miny = (offset_y + segment_y * segment_width);
			int32_t maxx = (minx + segment_width);
			int32_t maxy = (miny + segment_height);

			HilbertApp::drawRectangle(buffer, minx, miny, maxx, maxy, color);
			Sleep(20);

			segment_x += 1;
		}break;
		case 1: // up
		{
			int32_t minx = (offset_x + segment_x * segment_width);
			int32_t miny = (offset_y + segment_y * segment_width);
			int32_t maxx = (minx + segment_height);
			int32_t maxy = (miny + segment_width);

			HilbertApp::drawRectangle(buffer, minx, miny, maxx, maxy, color);
			Sleep(20);

			segment_y += 1;
		}break;
		case 2: // left
		{
			int32_t maxx = (offset_x + segment_x * segment_width);
			int32_t maxy = (offset_y + segment_y * segment_width);
			int32_t minx = maxx - segment_width;
			int32_t miny = maxy - segment_height;

			HilbertApp::drawRectangle(buffer, minx, miny, maxx, maxy, color);
			Sleep(20);

			segment_x -= 1;
		}break;
		case 3: // down
		{
			int32_t maxx = (offset_x + segment_x * segment_width);
			int32_t maxy = (offset_y + segment_y * segment_width);
			int32_t minx = (maxx - segment_height);
			int32_t miny = (maxy - segment_width);

			HilbertApp::drawRectangle(buffer, minx, miny, maxx, maxy, color);
			Sleep(20);

			segment_y -= 1;
		}break;
		};
	}

	void HilbertCurve::hilbert(OffscreenBuffer* buffer, int32_t direction, int32_t rotation, int32_t order)
	{
		if (order == 0)
		{
			return;
		}

		direction = (direction + rotation);
		hilbert(buffer, direction, -rotation, order - 1);

		Vec4 color0(HilbertApp::randRange(0, 255) / 255.0f,
			HilbertApp::randRange(0, 255) / 255.0f,
			HilbertApp::randRange(0, 255) / 255.0f);

		makeStep(buffer, direction, color0);

		direction = (direction - rotation);
		hilbert(buffer, direction, rotation, order - 1);

		Vec4 color1(HilbertApp::randRange(0, 255) / 255.0f,
			HilbertApp::randRange(0, 255) / 255.0f,
			HilbertApp::randRange(0, 255) / 255.0f);
		makeStep(buffer, direction, color1);

		hilbert(buffer, direction, rotation, order - 1);
		direction = (direction - rotation);

		Vec4 color2(HilbertApp::randRange(0, 255) / 255.0f,
			HilbertApp::randRange(0, 255) / 255.0f,
			HilbertApp::randRange(0, 255) / 255.0f);
		makeStep(buffer, direction, color2);

		hilbert(buffer, direction, -rotation, order - 1);
	}
}