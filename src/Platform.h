#ifndef PLATFORM_H
#define PLATFORM_H

#include <vulkan/vulkan.h>

struct Config;

class Platform
{
public:
	virtual ~Platform() {}
	static Platform* Create(const Config& config);

	virtual bool ShouldClose() = 0;
	virtual bool IsPaused() = 0;
	virtual bool ProcessEvents() = 0;
	virtual void *GetWindowHandle() = 0;

	virtual const char* GetVulkanPlatformExtension() = 0;
	virtual bool CreateVulkanPlatformSurface(const VkInstance& instance, VkSurfaceKHR& surface) = 0;
};

#endif