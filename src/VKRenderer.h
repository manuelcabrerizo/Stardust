#ifndef VK_RENDERER_H
#define VK_RENDERER_H

#include "Renderer.h"
#include "EventBus.h"

#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <array>

struct Config;
class Platform;

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

const bool ENABLE_VALIDATION_LAYERS = true;
const int MAX_FRAMES_IN_FLIGHT = 2;
const int SAMPLERS_BINDINGS_COUNT = 4;
const int TEXTURES_BINDINGS_COUNT = 4;
const int SAMPLERS_COUNT = 4;
const int TEXTURES_COUNT = 100;
const int DYNAMIC_CONST_BUFFER_BLOCK_COUNT = 100;
const int PER_FRAME_SET_COUNT = 3;

class VKRenderer : public Renderer, EventListener
{
public:
	VKRenderer(const Config& config, Platform* platform);
	~VKRenderer();

	void OnEvent(const Event& event) override;
	void OnWindowResizeEvent(const WindowResizeEvent& windowResizeEvent);

	void BeginFrame() override;
	void EndFrame() override;
protected:
	void OnLoadGraphicPipeline(ResourceIdentifier*& id, GraphicPipeline* graphicPipeline) override;
	void OnReleaseGraphicPipeline(ResourceIdentifier* id) override;
	void OnLoadVertexBuffer(ResourceIdentifier*& id, VertexBuffer* vertexBuffer) override;
	void OnReleaseVertexBuffer(ResourceIdentifier* id) override;
	void OnLoadIndexBuffer(ResourceIdentifier*& id, IndexBuffer* indexBuffer) override;
	void OnReleaseIndexBuffer(ResourceIdentifier* id) override;
	void OnLoadTexture2D(ResourceIdentifier*& id, Texture2D* texture2d) override;
	void OnReleaseTexture2D(ResourceIdentifier* id) override;
	void PushGraphicPipeline(GraphicPipeline* graphicPipeline) override;
	void PushPerFrameVariables() override;
	void PushPerDrawVariables() override;	
	void PushVerteBuffer(VertexBuffer* vertexBuffer) override;
	void PushIndexBuffer(IndexBuffer* indexBuffer) override;
	void PushTexture(Texture2D* texture2d, int slot) override;
private:
	void CreateInstance(const Config& config, Platform* platform);
	void SetUpDebugMessenger();
	void CreateSurface(Platform* platform);
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateImageView();
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void CreateSyncObjects();
	void CreateDescriptorPool();
	void CreateDescriptorSetLayout();
	void CreateDescriptorSet();
	void CreatePerFrameConstBuffers();
	void CreatePerDrawConstBuffer();
	void CreateSamplers();

	bool IsDeviceSiutable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
	VkShaderModule CreateShaderModule(const char *data, size_t size);
	unsigned int FindMemoryType(unsigned int typeFilter, VkMemoryPropertyFlags properties);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory); 
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	EventBus* mEventBus;

	VkInstance mInstance;
	VkPhysicalDevice mPhysicalDevice;
	VkDevice mDevice;
	VkQueue mGraphicsQueue;
	VkQueue mPresentQueue;
	VkSurfaceKHR mSurface;
	VkSwapchainKHR mSwapChain;
	std::vector<VkImage> mSwapChainImages;
	std::vector<VkImageView> mSwapChainImageViews;
	VkFormat mSwapChainImageFormat;
	VkExtent2D mSwapChainExtent;
	VkRenderPass mRenderPass;
	std::vector<VkFramebuffer> mSwapChainFramebuffers;
	VkCommandPool mCommandPool;
	std::vector<VkCommandBuffer> mCommandBuffers;

	std::vector<VkSemaphore> mImageAvailableSemaphores;
	std::vector<VkSemaphore> mRenderFinishedSemaphores;
	std::vector<VkFence> mInFlightFences; 

	unsigned int mImageIndex;
	unsigned int mCurrentFrame;

	VkPipelineLayout mLayout;
	VkDescriptorSetLayout mDescriptorSetLayout0;
	VkDescriptorSetLayout mDescriptorSetLayout1;
	VkDescriptorSetLayout mDescriptorSetLayout2;
	VkDescriptorSetLayout mDescriptorSetLayout3;

	VkDescriptorPool mPerFrameDescriptorPool;
	VkDescriptorPool mPerDrawDescriptorPool;
	std::vector<VkDescriptorSet> mPerFrameDescriptorSets;

	size_t mPerFrameConstBufferSize;
	std::vector<VkBuffer> mPerFrameConstBuffers;
	std::vector<VkDeviceMemory> mPerFrameConstBufferMemory;
	std::vector<void*> mPerFrameConstBuffersMapped;

	VkBuffer mPerDrawConstBuffers;
	VkDeviceMemory mPerDrawConstBufferMemory;
	size_t mDynamicAlignment;
	unsigned int mPerDrawConstBufferUsed;

	std::array<VkSampler, SAMPLERS_COUNT> mSamplers;

	VkDebugUtilsMessengerEXT mDebugMessenger;

	std::vector<const char*> mRequiredExtension = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME // Debug Utilities
	};
	std::vector<const char*> mRequiredLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	std::vector<const char*> mDeviceExtensions = {
    	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
};

#endif