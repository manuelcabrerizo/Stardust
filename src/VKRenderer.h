#ifndef VK_RENDERER_H
#define VK_RENDERER_H

#include "Renderer.h"
#include "EventBus.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <array>

struct Config;
class Platform;

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
	void CleanupSwapChain();
	void RecreateSwapChain();
	void CreateInstance(const Config& config, Platform* platform);
	void SetUpDebugMessenger();
	void CreateSurface(Platform* platform);
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain();
	void CreateImageView();
	void CreateDepthResource();
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void CreateSyncObjects();
	void CreateDescriptorPool();
	void CreateDescriptorSetLayout();
	void CreateSamplers();
	void CreateDescriptorSet(GraphicPipeline* graphicPipeline);
	void LoadPerFrameConstBuffer(GraphicPipeline* graphicPipeline);
	void LoadPerDrawConstBuffer(GraphicPipeline* graphicPipeline);

	VkInstance mInstance;
	VkPhysicalDevice mPhysicalDevice;
	VkDevice mDevice;
	VkQueue mGraphicsQueue;
	VkQueue mPresentQueue;
	VkSurfaceKHR mSurface;
	VkSwapchainKHR mSwapChain;
	
	std::vector<VkImage> mSwapChainImages;
	std::vector<VkImageView> mSwapChainImageViews;

	VkImage mDepthImage;
	VkDeviceMemory mDepthImageMemory;
	VkImageView mDepthImageView;

	VkFormat mSwapChainImageFormat;
	VkExtent2D mSwapChainExtent;

	VkRenderPass mRenderPass;
	std::vector<VkFramebuffer> mSwapChainFramebuffers;
	VkCommandPool mCommandPool;
	std::vector<VkCommandBuffer> mCommandBuffers;

	std::vector<VkSemaphore> mImageAvailableSemaphores;
	std::vector<VkSemaphore> mRenderFinishedSemaphores;
	std::vector<VkFence> mInFlightFences; 

	bool mFramebufferResized;
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