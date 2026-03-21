#ifndef PLATFORM_H
#define PLATFORM_H

#include "Common.h"
#include <vulkan/vulkan.h>

struct Config;

class SD_API Platform
{
public:
	virtual ~Platform() {}
	static Platform* Create(const Config& config);

	virtual bool ShouldClose() = 0;
	virtual bool IsPaused() = 0;
	virtual void ProcessEvents() = 0;
	virtual void *GetWindowHandle() = 0;
	virtual double GetTime() = 0;

	virtual const char* GetVulkanPlatformExtension() = 0;
	virtual bool CreateVulkanPlatformSurface(const VkInstance& instance, VkSurfaceKHR& surface) = 0;
};

#endif