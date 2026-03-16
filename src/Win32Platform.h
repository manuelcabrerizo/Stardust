#ifndef WIN32_PLATFORM_H
#define WIN32_PLATFORM_H

#include "Platform.h"
#include <windows.h>

#include "EventBus.h"

struct Config;

class Win32Platform : public Platform
{
public:
	Win32Platform(const Config& config);
	~Win32Platform();

	bool ShouldClose() override;
	bool IsPaused() override;
	bool ProcessEvents() override;
	void *GetWindowHandle() override;
	LRESULT MsgProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

	const char* GetVulkanPlatformExtension() override;
	bool CreateVulkanPlatformSurface(const VkInstance& instance, VkSurfaceKHR& surface) override;
private:
	HINSTANCE mInstance;
	HWND mWindow;
	bool mIsPaused;
};

#endif