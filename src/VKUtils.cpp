#include "VKUtils.h"
#include "spirv_reflect.h"
#include "GraphicPipeline.h"

#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>

bool VKUtils::IsDeviceSiutable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions)
{
	QueueFamilyIndices indices = FindQueueFamilies(device, surface);
	bool extensionSupported = CheckDeviceExtensionSupport(device, deviceExtensions);
	bool swapChainAdequate = false;
	if (extensionSupported)
	{
	    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
	    swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
	}

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.IsComplete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool VKUtils::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions)
{
	unsigned int extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions)
	{
	    requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}

bool VKUtils::HasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

QueueFamilyIndices VKUtils::FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
{
	QueueFamilyIndices indices;
	unsigned int queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	int i = 0;
	for(const VkQueueFamilyProperties& queueFamily : queueFamilies)
	{
		if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.GraphicsFamily = i;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) 
		{
    		indices.PresentFamily = i;
		}

		if(indices.IsComplete())
		{
			break;
		}
		i++;
	}
	return indices;
}

SwapChainSupportDetails VKUtils::QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);
	unsigned int formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
	}
	unsigned int presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
	}
	return details;
}

VkSurfaceFormatKHR VKUtils::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for(const VkSurfaceFormatKHR& availableFormat : availableFormats)
	{
		if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
		   availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR VKUtils::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
	{
	    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
	    {
        	return availablePresentMode;
	    }
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VKUtils::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<unsigned int>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        unsigned int width = 0, height = 0;
        // TODO: glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actualExtent = {
            static_cast<unsigned int>(width),
            static_cast<unsigned int>(height)
        };
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

unsigned int VKUtils::FindMemoryType(VkPhysicalDevice device, unsigned int typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(device, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
	    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
	    {
	        return i;
	    }
	}
	throw VKException("Error: FindMemoryType failed");
	return -1;
}

VkCommandBuffer VKUtils::BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VKUtils::EndSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
	
void VKUtils::CreateBuffer(
	VkPhysicalDevice physicalDevice,
	VkDevice device,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
		throw VKException("Error: vkCreateBuffer failed");
    }
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
		throw VKException("Error: vkAllocateMemory failed");
    }
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VKUtils::CopyBuffer(
	VkDevice device, VkCommandPool commandPool, VkQueue queue,
	VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(device, commandPool, queue, commandBuffer);
}

void VKUtils::CopyBufferToImage(
	VkDevice device, VkCommandPool commandPool, VkQueue queue,
	VkBuffer buffer, VkImage image, unsigned int width, unsigned int height)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {
	    width,
	    height,
	    1
	};

	vkCmdCopyBufferToImage(
	    commandBuffer,
	    buffer,
	    image,
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	    1,
	    &region
	);

    EndSingleTimeCommands(device, commandPool, queue, commandBuffer);
}

void VKUtils::TransitionImageLayout(
	VkDevice device, VkCommandPool commandPool, VkQueue queue,
	VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands(device, commandPool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags sourceStage{};
	VkPipelineStageFlags destinationStage{};

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
	    barrier.srcAccessMask = 0;
	    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} 
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
	    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw VKException("Error: TransitionImageLayout failed");
	}

	vkCmdPipelineBarrier(
	    commandBuffer,
	    sourceStage, destinationStage,
	    0,
	    0, nullptr,
	    0, nullptr,
	    1, &barrier
	);

	EndSingleTimeCommands(device, commandPool, queue, commandBuffer);
}

VkFormat VKUtils::FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        } 
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
	throw VKException("Error: FindSupportedFormat failed");
	return {};
}

VkFormat VKUtils::FindDepthFormat(VkPhysicalDevice device)
{
    return FindSupportedFormat(
    	device,
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkResult VKUtils::CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) 
	{
	    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
	    return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VKUtils::DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}
	
VkShaderModule VKUtils::CreateShaderModule(VkDevice device, const char *data, size_t size)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = reinterpret_cast<const unsigned int*>(data);
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw VKException("Error: vkCreateShaderModule failed");
	}
	return shaderModule;
}

std::vector<ConstBuffer> VKUtils::CreateConstBufferPerSet(VkPhysicalDevice device, GraphicPipeline* graphicPipeline, int set, bool dynamic)
{
	SpvReflectShaderModule vertexModule;
	SpvReflectResult reflectResult = spvReflectCreateShaderModule(graphicPipeline->GetVertexShaderSize(), graphicPipeline->GetVertexShaderData(), &vertexModule);
	assert(reflectResult == SPV_REFLECT_RESULT_SUCCESS);

	unsigned int count = 0;
	reflectResult = spvReflectEnumerateDescriptorSets(&vertexModule, &count, nullptr);
	assert(reflectResult == SPV_REFLECT_RESULT_SUCCESS);
	std::vector<SpvReflectDescriptorSet*> sets(count);
	reflectResult = spvReflectEnumerateDescriptorSets(&vertexModule, &count, sets.data());
	assert(reflectResult == SPV_REFLECT_RESULT_SUCCESS);

	assert(set >= 0 && set < sets.size());
	SpvReflectDescriptorSet* descriptorSet = sets[set];

	std::vector<ConstBuffer> constBuffers;
	for(int i = 0; i < descriptorSet->binding_count; i++)
	{
		SpvReflectDescriptorBinding* binding = descriptorSet->bindings[i];
		if(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
		   binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
		{
		    size_t alignSize = binding->block.padded_size;
			if(dynamic)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(device, &properties);
				size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
			    if (minUboAlignment > 0)
			    {
					alignSize = (alignSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
				}
			}
			ConstBuffer constantBuffer = ConstBuffer(binding->name, set, binding->binding, alignSize);
			constantBuffer.AddBindStage(CB_VERTEX_BIND_STAGE);
			constantBuffer.AddBindStage(CB_PIXEL_BIND_STAGE);
			for(int j = 0; j < binding->block.member_count; j++)
			{
				const SpvReflectBlockVariable& variableDesc = binding->block.members[j];
				Variable constantBufferVariable;
				constantBufferVariable.Offset = variableDesc.absolute_offset; // use offset or absolute_offset?
				constantBufferVariable.Size = variableDesc.size;
				constantBuffer.AddVariable(variableDesc.name, constantBufferVariable);
			}
			constBuffers.push_back(constantBuffer);
		}
	}
	spvReflectDestroyShaderModule(&vertexModule);

	return constBuffers;
}