#ifndef VK_UTILS_H
#define VK_UTILS_H

#include "ConstBuffer.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

struct QueueFamilyIndices
{
	std::optional<unsigned int> GraphicsFamily;
	std::optional<unsigned int> PresentFamily;
	bool IsComplete()
	{
		return GraphicsFamily.has_value() && PresentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;
};

class GraphicPipeline;
struct SpvReflectShaderModule;

class VKUtils
{
public:
	static bool IsDeviceSiutable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions);
	static bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions);
	static bool HasStencilComponent(VkFormat format);

	static QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
	static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	static unsigned int FindMemoryType(VkPhysicalDevice device, unsigned int typeFilter, VkMemoryPropertyFlags properties);
	
	static VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
	static void EndSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);

	static void CreateBuffer(
		VkPhysicalDevice physicalDevice,
		VkDevice device,
		VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	static void CopyBuffer(
		VkDevice device, VkCommandPool commandPool, VkQueue queue,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	static void CopyBufferToImage(
		VkDevice device, VkCommandPool commandPool, VkQueue queue,
		VkBuffer buffer, VkImage image, unsigned int width, unsigned int height);
	static void TransitionImageLayout(
		VkDevice device, VkCommandPool commandPool, VkQueue queue,
		VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	static VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	static VkFormat FindDepthFormat(VkPhysicalDevice device);

	static VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	static VkShaderModule CreateShaderModule(VkDevice device, const char *data, size_t size);
	static std::vector<ConstBuffer> CreateConstBufferPerSet(const SpvReflectShaderModule& vertexModule, VkPhysicalDevice device, int set, bool dynamic);
};

class VKException : public std::exception
{
public:
	VKException(const std::string& message)
	{
		mErrorMessage = message;
	}

	const char* what() const override
	{
		return mErrorMessage.c_str();
	}

private:
	std::string mErrorMessage;
};

#endif