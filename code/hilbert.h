#ifndef HILBERT_H
#define HILBERT_H

#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <thread>

namespace hilbert
{
	union Vec2
	{
		Vec2() : x(0.0f), y(0.0f) {}
		Vec2(float xx, float yy) : x(xx), y(yy) {}

		struct
		{
			float x, y;
		};
		float e[2];
	};

	union Vec4
	{
		Vec4() : x(0.0f), y(0.0), z(0.0f), w(0.0f) {}
		Vec4(float xx, float yy, float zz, float ww=0.0f)
			: x(xx), y(yy), z(zz), w(ww) {}

		struct
		{
			float x, y, z, w;
		};

		struct
		{
			float r, g, b, a;
		};
		float e[4];
	};

	struct OffscreenBuffer
	{
		BITMAPINFO info;
		void* memory;
		int32_t width;
		int32_t height;
		int32_t pitch;
		int32_t bpp; // bytes per pixel
	};

	class HilbertCurve
	{
		void makeStep(OffscreenBuffer* buffer, int32_t direction, Vec4 color);
		void hilbert(OffscreenBuffer* buffer, int32_t direction, int32_t rotation, int32_t order);

	public:
		// Overloaded operator() to be able to pass an instance of this class to std::thread.
		void operator()(OffscreenBuffer* buffer)
		{
			hilbert(buffer, direction, rotation, order);
		}
	
		int32_t offset_x;
		int32_t offset_y;

		int32_t segment_x;
		int32_t segment_y;

		int32_t segment_width;
		int32_t segment_height;

		int32_t direction;
		int32_t rotation;
		int32_t order;
	};

	class HilbertApp
	{
	public:
		HilbertApp(HINSTANCE instance);
		~HilbertApp() {}

		LRESULT CALLBACK mainWindowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);

		bool init();
		int32_t run();

		// helper functions
		static int32_t randRange(int32_t range_min, int32_t range_max);
		static uint32_t roundReal32ToUint32(float value);
		static void drawRectangle(OffscreenBuffer* buffer, int32_t rect_min_x, int32_t rect_min_y, int32_t rect_max_x, int32_t rect_max_y, Vec4 color, int32_t frame_width);
	private:
		bool initMainWindow();
		void initOffscreenBuffer();
		void displayOffscreenBufferInWindow() const;

		// render hilbert curves of different orders, up to 5. Not programmable!
		void renderHilbertCurves();
	private:
		bool m_is_running;
		HINSTANCE m_instance;
		HWND m_window;
		OffscreenBuffer m_offscreen_buffer{};
		int32_t m_client_width;
		int32_t m_client_height;
	};
}

#endif // HILBERT_H