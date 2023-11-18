#include "hilbert.h"

INT WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR cmd_line, int show_code)
{
	hilbert::HilbertApp app(instance);
	if (app.init())
	{
		app.run();
	}

	return 0;
}