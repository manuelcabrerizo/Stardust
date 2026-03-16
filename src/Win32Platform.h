#ifndef WIN32_PLATFORM_H
#define WIN32_PLATFORM_H

#include "Platform.h"
#include <windows.h>

struct Config;

class Win32Platform : public Platform
{
public:
	Win32Platform(const Config& config);
	~Win32Platform();

	bool ShouldClose() override;
	bool ProcessEvents() override;
	void *GetWindowHandle() override;
	const char* GetVulkanPlatformExtension() override;
	bool CreateVulkanPlatformSurface(const VkInstance& instance, VkSurfaceKHR& surface) override;
private:
	HINSTANCE mInstance;
	HWND mWindow;
};

#endif