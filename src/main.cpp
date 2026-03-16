#include <windows.h>
#include "WizardEngine.h"

// Entry point of the Win32 Game Sample
int CALLBACK WinMain(HINSTANCE hInstance,
					 HINSTANCE hPrevInstance,
					 LPSTR lpCmdLine,
					 int nShowCmd)
{
	try
	{
		Config config;
		config.Title = "Wizard Game";
		config.ScreenWidth = 800;
		config.ScreenHeight = 600;
		config.Fullscreen = false;

		WizardEngine engine(config);
		engine.Run();
	}
	catch(const std::exception& e)
	{
		MessageBox(nullptr, e.what(), nullptr, MB_OK);
	}
	
	return 0;
}