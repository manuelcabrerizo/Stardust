#include "VKRenderer.h"
#include "Platform.h"
#include "Config.h"

#include "ServiceProvider.h"

#include "Bindable.h"
#include "GraphicPipeline.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ConstBuffer.h"
#include "Texture2D.h"

// TODO: remove #include <windows.h>
#define NOMINMAX
#include <windows.h>

#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>

#include "math/Matrix4x4.h"
#include "math/Vector3.h"

class VKGraphicPipelineID : public ResourceIdentifier
{
public:
	VkPipeline Pipeline;
};

class VKVertexBufferID : public ResourceIdentifier
{
public:
	VkBuffer Buffer;
	VkDeviceMemory Memory;
};

class VKTexture2DID : public ResourceIdentifier
{
public:
	VkImage Image;
	VkDeviceMemory Memory;
	VkImageView View;
	VkDescriptorSet DescriptorSet;
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
static QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

VKRenderer::VKRenderer(const Config& config, Platform* platform)
{
	mEventBus = ServiceProvider::Instance()->GetService<EventBus>();
	mEventBus->AddListener(EventType::WindowResizeEvent, this);

	CreateInstance(config, platform);
	SetUpDebugMessenger();
	CreateSurface(platform);
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageView();
	CreateRenderPass();
	CreateFramebuffers();
	CreateCommandPool();
	CreateCommandBuffer();
	CreateSyncObjects();
	CreatePerFrameConstBuffers();
	CreatePerDrawConstBuffer();
	CreateSamplers();
	CreateDescriptorPool();
	CreateDescriptorSetLayout();
	CreateDescriptorSet();

	mImageIndex = 0;
	mCurrentFrame = 0;
}

VKRenderer::~VKRenderer()
{
    vkDeviceWaitIdle(mDevice);

    for(int i = 0; i < mSamplers.size(); i++)
    {
		vkDestroySampler(mDevice, mSamplers[i], nullptr);
    }

	vkDestroyPipelineLayout(mDevice, mLayout, nullptr);
	vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout3, nullptr);
	vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout2, nullptr);
	vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout1, nullptr);
	vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout0, nullptr);

	vkDestroyDescriptorPool(mDevice, mPerDrawDescriptorPool, nullptr);
	vkDestroyDescriptorPool(mDevice, mPerFrameDescriptorPool, nullptr);

	vkDestroyBuffer(mDevice, mPerDrawConstBuffers, nullptr);
	vkFreeMemory(mDevice, mPerDrawConstBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(mDevice, mPerFrameConstBuffers[i], nullptr);
        vkFreeMemory(mDevice, mPerFrameConstBufferMemory[i], nullptr);
    }

    for(int i = 0; i < mSwapChainImages.size(); i++)
    {
    	vkDestroySemaphore(mDevice, mRenderFinishedSemaphores[i], nullptr);
	}

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
    	vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], nullptr);
    	vkDestroyFence(mDevice, mInFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
    for (auto framebuffer : mSwapChainFramebuffers)
    {
        vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
    }

	vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
    for (auto imageView : mSwapChainImageViews)
    {
        vkDestroyImageView(mDevice, imageView, nullptr);
    }
	vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
	vkDestroyDevice(mDevice, nullptr);
	if(ENABLE_VALIDATION_LAYERS)
	{
		DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
	}
	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	vkDestroyInstance(mInstance, nullptr);

	mEventBus->RemoveListener(EventType::WindowResizeEvent, this);
}

void VKRenderer::OnEvent(const Event& event)
{
	switch(event.Type)
	{
		case EventType::WindowResizeEvent:
		{
			OnWindowResizeEvent(reinterpret_cast<const WindowResizeEvent&>(event));
		}break;
		default:
		{
			assert(!"ERROR!");
		}
	};
}

void VKRenderer::OnWindowResizeEvent(const WindowResizeEvent& windowResizeEvent)
{

}

void VKRenderer::BeginFrame()
{
	mPerDrawConstBufferUsed = 0;

	vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrame]);

    vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &mImageIndex);

    vkResetCommandBuffer(mCommandBuffers[mCurrentFrame], 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;
	if (vkBeginCommandBuffer(mCommandBuffers[mCurrentFrame], &beginInfo) != VK_SUCCESS)
	{
		// TODO: Handle Error ...
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = mRenderPass;
	renderPassInfo.framebuffer = mSwapChainFramebuffers[mImageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = mSwapChainExtent;

	VkClearValue clearColor = {{{0.02f, 0.02f, 0.06f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(mCommandBuffers[mCurrentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(mSwapChainExtent.width);
	viewport.height = static_cast<float>(mSwapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(mCommandBuffers[mCurrentFrame], 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = mSwapChainExtent;
	vkCmdSetScissor(mCommandBuffers[mCurrentFrame], 0, 1, &scissor);

	unsigned int frameOffset = mCurrentFrame * PER_FRAME_SET_COUNT;
	vkCmdBindDescriptorSets(mCommandBuffers[mCurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, mLayout, 0, 1, &mPerFrameDescriptorSets[frameOffset + 0], 0, nullptr);
	vkCmdBindDescriptorSets(mCommandBuffers[mCurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, mLayout, 2, 1, &mPerFrameDescriptorSets[frameOffset + 2], 0, nullptr);
}

void VKRenderer::EndFrame()
{
	vkCmdEndRenderPass(mCommandBuffers[mCurrentFrame]);
	if (vkEndCommandBuffer(mCommandBuffers[mCurrentFrame]) != VK_SUCCESS)
	{
 		// TODO: Handle Error ...
	}

	VkSwapchainKHR swapChains[] = {mSwapChain};
	VkSemaphore waitSemaphores[] = {mImageAvailableSemaphores[mCurrentFrame]};
	VkSemaphore signalSemaphores[] = {mRenderFinishedSemaphores[mImageIndex]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffers[mCurrentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFences[mCurrentFrame]) != VK_SUCCESS)
	{
 		// TODO: Handle Error ...
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &mImageIndex;
	presentInfo.pResults = nullptr; // Optional
	vkQueuePresentKHR(mPresentQueue, &presentInfo);

	mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VKRenderer::OnLoadGraphicPipeline(ResourceIdentifier*& id, GraphicPipeline* graphicPipeline)
{
	VKGraphicPipelineID* resource = new VKGraphicPipelineID();
	id = resource;

	VkShaderModule vertShaderModule = CreateShaderModule(graphicPipeline->GetVertexShaderData(), graphicPipeline->GetVertexShaderSize());
	VkShaderModule pixelShaderModule = CreateShaderModule(graphicPipeline->GetPixelShaderData(), graphicPipeline->GetPixelShaderSize());

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo pixelShaderStageInfo{};
	pixelShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pixelShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	pixelShaderStageInfo.module = pixelShaderModule;
	pixelShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, pixelShaderStageInfo};

	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = 5 * sizeof(float);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = 0;
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = 3 * sizeof(float);

	// Vertex Input (this info should came from the shader reflection, Hard coded for now)
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<unsigned int>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Only draw triangle for now
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Dynamic Viewport and Sissor 
	std::vector<VkDynamicState> dynamicStates = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT ;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // VK_FRONT_FACE_COUNTER_CLOCKWISE
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// Desable Multisample for now
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = mLayout;
	pipelineInfo.renderPass = mRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &resource->Pipeline) != VK_SUCCESS)
	{
		// TODO: Handle Error ...
	}

    vkDestroyShaderModule(mDevice, pixelShaderModule, nullptr);
    vkDestroyShaderModule(mDevice, vertShaderModule, nullptr);
}

void VKRenderer::OnReleaseGraphicPipeline(ResourceIdentifier* id)
{
	VKGraphicPipelineID* resource = reinterpret_cast<VKGraphicPipelineID*>(id);
	vkDeviceWaitIdle(mDevice);
    vkDestroyPipeline(mDevice, resource->Pipeline, nullptr);
	delete resource;
}

void VKRenderer::OnLoadVertexBuffer(ResourceIdentifier*& id, VertexBuffer* vertexBuffer)
{
	VKVertexBufferID* resource = new VKVertexBufferID();
	id = resource;

	VkDeviceSize bufferSize = vertexBuffer->GetVertexSize() * vertexBuffer->GetVertexCount();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertexBuffer->GetData(), (size_t)bufferSize);
	vkUnmapMemory(mDevice, stagingBufferMemory);

	CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	resource->Buffer, resource->Memory);

	CopyBuffer(stagingBuffer, resource->Buffer, bufferSize);

	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void VKRenderer::OnReleaseVertexBuffer(ResourceIdentifier* id)
{
	VKVertexBufferID* resource = reinterpret_cast<VKVertexBufferID*>(id);
	vkDeviceWaitIdle(mDevice);
	vkDestroyBuffer(mDevice, resource->Buffer, nullptr);
	vkFreeMemory(mDevice, resource->Memory, nullptr);
	delete resource;
}

void VKRenderer::OnLoadIndexBuffer(ResourceIdentifier*& id, IndexBuffer* indexBuffer)
{

}

void VKRenderer::OnReleaseIndexBuffer(ResourceIdentifier* id)
{

}

void VKRenderer::OnLoadTexture2D(ResourceIdentifier*& id, Texture2D* texture2d)
{
	VKTexture2DID* resource = new VKTexture2DID();
	id = resource;

	{
	    VkDeviceSize imageSize = texture2d->GetWidth() * texture2d->GetHeight() * texture2d->GetChannels();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void *data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, texture2d->GetData(), static_cast<size_t>(imageSize));
		vkUnmapMemory(mDevice, stagingBufferMemory);

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = static_cast<unsigned int>(texture2d->GetWidth());
		imageInfo.extent.height = static_cast<unsigned int>(texture2d->GetHeight());
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0; // Optional
		if (vkCreateImage(mDevice, &imageInfo, nullptr, &resource->Image) != VK_SUCCESS)
		{
			// TODO: handle error ...
		}

		// Alloc Memory for the Image
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(mDevice, resource->Image, &memRequirements);
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &resource->Memory) != VK_SUCCESS)
		{
			// TODO: handle error ...
		}
		vkBindImageMemory(mDevice, resource->Image, resource->Memory, 0);

		TransitionImageLayout(resource->Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, resource->Image,
			static_cast<unsigned int>(texture2d->GetWidth()),
			static_cast<unsigned int>(texture2d->GetHeight()));
		TransitionImageLayout(resource->Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		
		vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
		vkFreeMemory(mDevice, stagingBufferMemory, nullptr);

		// Create Image View
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = resource->Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(mDevice, &viewInfo, nullptr, &resource->View) != VK_SUCCESS)
		{
			// TODO: handle error ...
		}
	}
	
	// Alloc Texture Descriptor Sets
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mPerDrawDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mDescriptorSetLayout3;
		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &resource->DescriptorSet) != VK_SUCCESS)
		{
			// TODO: handle error ...
		}
		// Set 2 is for Texture Data 4 bindings slots
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = resource->View;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write{};	
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = resource->DescriptorSet;
		write.dstBinding = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.descriptorCount = 1;
		write.pImageInfo = &imageInfo;
		
		vkUpdateDescriptorSets(mDevice, 1, &write, 0, nullptr);
	}
}

void VKRenderer::OnReleaseTexture2D(ResourceIdentifier* id)
{
	VKTexture2DID* resource = reinterpret_cast<VKTexture2DID*>(id);
	vkDeviceWaitIdle(mDevice);
	vkDestroyImageView(mDevice, resource->View, nullptr);
	vkDestroyImage(mDevice, resource->Image, nullptr);
	vkFreeMemory(mDevice, resource->Memory, nullptr);
	delete resource;
}

void VKRenderer::PushGraphicPipeline(GraphicPipeline* graphicPipeline)
{
	VKGraphicPipelineID* resource = static_cast<VKGraphicPipelineID *>(graphicPipeline->GetIdentifier(this));
	vkCmdBindPipeline(mCommandBuffers[mCurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, resource->Pipeline);
}

void VKRenderer::PushPerFrameVariables()
{
	memcpy(mPerFrameConstBuffersMapped[mCurrentFrame], mPerFrame->GetData(), mPerFrame->GetUsedSize());
}

void VKRenderer::PushPerDrawVariables()
{
	void* data;
	vkMapMemory(
	    mDevice, 
	    mPerDrawConstBufferMemory, 
	    mPerDrawConstBufferUsed,
	    mPerDraw->GetSize(),
	    0, &data);

	memcpy((unsigned char*)data, mPerDraw->GetData(), mPerDraw->GetUsedSize());

	VkMappedMemoryRange memoryRange;
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.pNext = nullptr;
	memoryRange.memory = mPerDrawConstBufferMemory;
	memoryRange.offset = mPerDrawConstBufferUsed;
	memoryRange.size = mPerDraw->GetSize();
	vkFlushMappedMemoryRanges(mDevice, 1, &memoryRange);

	vkUnmapMemory(mDevice, mPerDrawConstBufferMemory);

	// Bind it
	unsigned int frameOffset = mCurrentFrame * PER_FRAME_SET_COUNT;
  	unsigned int dynamicOffset = mPerDrawConstBufferUsed;
  	vkCmdBindDescriptorSets(mCommandBuffers[mCurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, mLayout, 1, 1, &mPerFrameDescriptorSets[frameOffset + 1], 1, &dynamicOffset);

	mPerDrawConstBufferUsed += mPerDraw->GetSize();
}

void VKRenderer::PushVerteBuffer(VertexBuffer* vertexBuffer)
{
	VKVertexBufferID* resource = static_cast<VKVertexBufferID *>(vertexBuffer->GetIdentifier(this));
	VkBuffer vertexBuffers[] = {resource->Buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(mCommandBuffers[mCurrentFrame], 0, 1, vertexBuffers, offsets);
	vkCmdDraw(mCommandBuffers[mCurrentFrame], vertexBuffer->GetVertexCount(), 1, 0, 0);
}

void VKRenderer::PushIndexBuffer(IndexBuffer* indexBuffer)
{

}

void VKRenderer::PushTexture(Texture2D* texture2d, int slot)
{
	VKTexture2DID* resource = static_cast<VKTexture2DID *>(texture2d->GetIdentifier(this));
	vkCmdBindDescriptorSets(mCommandBuffers[mCurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
		mLayout, 3, 1, &resource->DescriptorSet, 0, nullptr);
}

void VKRenderer::CreateInstance(const Config& config, Platform* platform)
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = config.Title;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Stardust Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Check for Required Rextensions
	mRequiredExtension.push_back(platform->GetVulkanPlatformExtension());

	unsigned int availableExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(0, &availableExtensionCount, 0);
	std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
	vkEnumerateInstanceExtensionProperties(0, &availableExtensionCount, availableExtensions.data());
	for(int i = 0; i < mRequiredExtension.size(); i++)
	{
		bool found = false;
		for(int j = 0; j < availableExtensions.size(); j++)
		{
			if(strcmp(mRequiredExtension[i], availableExtensions[j].extensionName) == 0)
			{
				found = true;
				break;
			}
		}
		if(!found)
		{
			// TODO: Handle Error ...
		}
	}

	// Check for Validation Layers
	unsigned int availableLayerCount = 0;
	vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(availableExtensionCount);
	vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());
	for(int i = 0; i < mRequiredLayers.size(); i++)
	{
		bool found = false;
		for(int j = 0; j < availableLayers.size(); j++)
		{
			if(strcmp(mRequiredLayers[i], availableLayers[j].layerName) == 0)
			{
				found = true;
				break;
			}
		}
		if(!found)
		{
			// TODO: Handle Error ...
		}
	}

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = mRequiredExtension.size();
	createInfo.ppEnabledExtensionNames = mRequiredExtension.data();
	if(ENABLE_VALIDATION_LAYERS)
	{
		createInfo.enabledLayerCount = mRequiredLayers.size();
		createInfo.ppEnabledLayerNames = mRequiredLayers.data();
		PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.pNext = nullptr;
	}
	if(vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
	{
		// TODO: handle error ...
	}
}


static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
	char buffer[2048];
	sprintf(buffer, "DEBUG Validation Layer: %s\n", pCallbackData->pMessage);
	OutputDebugString(buffer);
    return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
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

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}

void VKRenderer::SetUpDebugMessenger()
{
	if(!ENABLE_VALIDATION_LAYERS)
	{
		return;
	}
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);
	if (CreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS)
	{
    	// TODO: Handle Error
	}
}

bool VKRenderer::IsDeviceSiutable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = FindQueueFamilies(device, mSurface);
	bool extensionSupported = CheckDeviceExtensionSupport(device);
	bool swapChainAdequate = false;
	if (extensionSupported)
	{
	    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
	    swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
	}

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.IsComplete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

static QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
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

void VKRenderer::PickPhysicalDevice()
{
	mPhysicalDevice = VK_NULL_HANDLE;

	unsigned int deviceCount = 0;
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
	if(deviceCount == 0)
	{
		// TODO: Handle Error
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());
	for(const VkPhysicalDevice& device : devices)
	{
		if(IsDeviceSiutable(device))
		{
			mPhysicalDevice = device;
			break;
		}
	}

	if(mPhysicalDevice == VK_NULL_HANDLE)
	{
		// TODO: Handle Error
	}
}

void VKRenderer::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(mPhysicalDevice, mSurface);
	float queuePriority = 1.0f;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<unsigned int> uniqueQueueFamilies = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};
	for(unsigned int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<unsigned int>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<unsigned int>(mDeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = mDeviceExtensions.data();
	if(ENABLE_VALIDATION_LAYERS)
	{
		createInfo.enabledLayerCount = mRequiredLayers.size();
		createInfo.ppEnabledLayerNames = mRequiredLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	if(vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS)
	{
		// TODO: handle error ...
	}

	vkGetDeviceQueue(mDevice, indices.GraphicsFamily.value(), 0, &mGraphicsQueue);
	vkGetDeviceQueue(mDevice, indices.PresentFamily.value(), 0, &mPresentQueue);
}

void VKRenderer::CreateSurface(Platform* platform)
{
	if(!platform->CreateVulkanPlatformSurface(mInstance, mSurface))
	{
		// TODO: handle error ...
	}
}

bool VKRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	unsigned int extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	std::set<std::string> requiredExtensions(mDeviceExtensions.begin(), mDeviceExtensions.end());
	for (const auto& extension : availableExtensions)
	{
	    requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}

SwapChainSupportDetails VKRenderer::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &details.Capabilities);
	unsigned int formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, details.Formats.data());
	}
	unsigned int presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, details.PresentModes.data());
	}
	return details;
}

VkSurfaceFormatKHR VKRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
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

VkPresentModeKHR VKRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
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

VkExtent2D VKRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
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

void VKRenderer::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(mPhysicalDevice);
	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

	unsigned int imageCount = swapChainSupport.Capabilities.minImageCount + 1;
	if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
	{
	    imageCount = swapChainSupport.Capabilities.maxImageCount;
	}

	QueueFamilyIndices indices = FindQueueFamilies(mPhysicalDevice, mSurface);
	unsigned int queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentFamily.value()};

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mSurface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (indices.GraphicsFamily != indices.PresentFamily)
	{
	    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	    createInfo.queueFamilyIndexCount = 2;
	    createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
	    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	    createInfo.queueFamilyIndexCount = 0; // Optional
	    createInfo.pQueueFamilyIndices = nullptr; // Optional
	}
	createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS)
	{
		// TODO: handle error ...
	}

	vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, nullptr);
	mSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, mSwapChainImages.data());
	mSwapChainImageFormat = surfaceFormat.format;
	mSwapChainExtent = extent;
}

void VKRenderer::CreateImageView()
{
	mSwapChainImageViews.resize(mSwapChainImages.size());
	for (size_t i = 0; i < mSwapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = mSwapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = mSwapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapChainImageViews[i]) != VK_SUCCESS)
		{
			// TODO: handle error ...
		}
	}
}

VkShaderModule VKRenderer::CreateShaderModule(const char *data, size_t size)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = reinterpret_cast<const unsigned int*>(data);
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		// TODO: handle error ...
	}
	return shaderModule;
}

void VKRenderer::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = mSwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
	{
			// TODO: handle error ...
	}
}

void VKRenderer::CreateFramebuffers()
{
	mSwapChainFramebuffers.resize(mSwapChainImageViews.size());
	for (size_t i = 0; i < mSwapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = {
		    mSwapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = mSwapChainExtent.width;
		framebufferInfo.height = mSwapChainExtent.height;
		framebufferInfo.layers = 1;
		if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			// TODO: handle error ...
		}
	}
}

void VKRenderer::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(mPhysicalDevice, mSurface);
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();
	if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
	{
    	// TODO: handle error ...
	}
}

void VKRenderer::CreateCommandBuffer()
{
	mCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = mCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	if (vkAllocateCommandBuffers(mDevice, &allocInfo, mCommandBuffers.data()) != VK_SUCCESS)
	{
    	// TODO: handle error ...
	}
}

void VKRenderer::CreateSyncObjects()
{
	mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT); 
	mRenderFinishedSemaphores.resize(mSwapChainImages.size());


    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
	    if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
		    vkCreateFence(mDevice, &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS)
		{
	    	// TODO: handle error ...
		}
    }

    for(int i = 0; i < mSwapChainImages.size(); i++)
    {
	    if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS)
		{
	    	// TODO: handle error ...
		}
    }
}

unsigned int VKRenderer::FindMemoryType(unsigned int typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
	    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
	    {
	        return i;
	    }
	}
	// TODO: handle error ...
	return -1;
}

void VKRenderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(mDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
		// TODO: handle error ...
    }
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(mDevice, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
    if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
		// TODO: handle error ...
    }
    vkBindBufferMemory(mDevice, buffer, bufferMemory, 0);
}

VkCommandBuffer VKRenderer::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = mCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VKRenderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(mGraphicsQueue);

	vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);	
}


void VKRenderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void VKRenderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

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

    EndSingleTimeCommands(commandBuffer);
}

void VKRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

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

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
	    barrier.srcAccessMask = 0;
	    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
	    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
			// TODO: Handle Error ...
	}

	vkCmdPipelineBarrier(
	    commandBuffer,
	    sourceStage, destinationStage,
	    0,
	    0, nullptr,
	    0, nullptr,
	    1, &barrier
	);

	EndSingleTimeCommands(commandBuffer);
}

void VKRenderer::CreateDescriptorSetLayout()
{
	////////////////////////////////////////////////////////////////
	// Set 0
	////////////////////////////////////////////////////////////////
	{
		VkDescriptorSetLayoutBinding perFrameLayoutBinding{};
	    perFrameLayoutBinding.binding = 0;
	    perFrameLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    perFrameLayoutBinding.descriptorCount = 1;
	    perFrameLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
	    perFrameLayoutBinding.pImmutableSamplers = nullptr; // Optional

	    VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &perFrameLayoutBinding;
		if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mDescriptorSetLayout0) != VK_SUCCESS)
		{
			// TODO: Handle Error ...
		}
	}

	////////////////////////////////////////////////////////////////
	// Set 1
	////////////////////////////////////////////////////////////////
	{
		VkDescriptorSetLayoutBinding perDrawLayoutBinding{};
	    perDrawLayoutBinding.binding = 0;
	    perDrawLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	    perDrawLayoutBinding.descriptorCount = 1;
	    perDrawLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;
	    perDrawLayoutBinding.pImmutableSamplers = nullptr; // Optional

	    VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &perDrawLayoutBinding;
		if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mDescriptorSetLayout1) != VK_SUCCESS)
		{
			// TODO: Handle Error ...
		}

	}

	////////////////////////////////////////////////////////////////
	// Set 2
	////////////////////////////////////////////////////////////////
	{
		VkDescriptorSetLayoutBinding samplerBindings[SAMPLERS_BINDINGS_COUNT] = {};
		for(unsigned int i = 0; i < SAMPLERS_BINDINGS_COUNT; i++)
		{
			samplerBindings[i].binding = i;
			samplerBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			samplerBindings[i].descriptorCount = 1;
			samplerBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}
	    VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = SAMPLERS_BINDINGS_COUNT;
		layoutInfo.pBindings = samplerBindings;
		if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mDescriptorSetLayout2) != VK_SUCCESS)
		{
			// TODO: Handle Error ...
		}
	}

	////////////////////////////////////////////////////////////////
	// Set 3
	////////////////////////////////////////////////////////////////
	{
		VkDescriptorSetLayoutBinding textureBindings[TEXTURES_BINDINGS_COUNT] = {};
		for (unsigned int i = 0; i < TEXTURES_BINDINGS_COUNT; i++) 
		{
		    textureBindings[i].binding = i;
		    textureBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		    textureBindings[i].descriptorCount = 1;
		    textureBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		    textureBindings[i].pImmutableSamplers = nullptr;
		}

	    VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = TEXTURES_BINDINGS_COUNT;
		layoutInfo.pBindings = textureBindings;
		if (vkCreateDescriptorSetLayout(mDevice, &layoutInfo, nullptr, &mDescriptorSetLayout3) != VK_SUCCESS)
		{
			// TODO: Handle Error ...
		}
	}

	std::array<VkDescriptorSetLayout, 4> descriptorSetLayouts = {
		mDescriptorSetLayout0,
		mDescriptorSetLayout1,
		mDescriptorSetLayout2,
		mDescriptorSetLayout3
	};

	// Pipeline Layout (this info should came from the shader reflection, Hard coded for now)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<unsigned int>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mLayout) != VK_SUCCESS)
	{
		// TODO: Handle Error ...
	}
}

void VKRenderer::CreateDescriptorPool()
{
	// Create PerFramePool
	{
		std::array<VkDescriptorPoolSize, PER_FRAME_SET_COUNT> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<unsigned int>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		poolSizes[1].descriptorCount = static_cast<unsigned int>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSizes[2].descriptorCount = static_cast<unsigned int>(MAX_FRAMES_IN_FLIGHT) * SAMPLERS_COUNT;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<unsigned int>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT * PER_FRAME_SET_COUNT;
		if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mPerFrameDescriptorPool) != VK_SUCCESS)
		{
			// TODO: handle error ...
		}
	}

	// Create PerDrawPool
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSize.descriptorCount = static_cast<unsigned int>(MAX_FRAMES_IN_FLIGHT) * TEXTURES_COUNT;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = TEXTURES_COUNT * MAX_FRAMES_IN_FLIGHT;
		// Flag útil si vas a liberar/reusar materiales individualmente
    	// poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mPerDrawDescriptorPool) != VK_SUCCESS)
		{
			// TODO: handle error ...
		}
	}
}

void VKRenderer::CreateDescriptorSet()
{
	// Alloc PerFrame Descriptor Sets
	std::vector<VkDescriptorSetLayout> layouts;
	for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		layouts.push_back(mDescriptorSetLayout0);
		layouts.push_back(mDescriptorSetLayout1);
		layouts.push_back(mDescriptorSetLayout2);
	}

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = mPerFrameDescriptorPool;
	allocInfo.descriptorSetCount = static_cast<unsigned int>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();
	mPerFrameDescriptorSets.resize(layouts.size());
	if (vkAllocateDescriptorSets(mDevice, &allocInfo, mPerFrameDescriptorSets.data()) != VK_SUCCESS)
	{
		// TODO: handle error ...
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		int frameOffset = i * PER_FRAME_SET_COUNT;
		VkDescriptorSet set0 = mPerFrameDescriptorSets[frameOffset + 0];
		VkDescriptorSet set1 = mPerFrameDescriptorSets[frameOffset + 1];
		VkDescriptorSet set2 = mPerFrameDescriptorSets[frameOffset + 2];

        // SET 0: Per Frame Buffers
	    VkDescriptorBufferInfo bufferInfo{};
	    bufferInfo.buffer = mPerFrameConstBuffers[i];
	    bufferInfo.offset = 0;
	    bufferInfo.range =  VK_WHOLE_SIZE;

	    VkWriteDescriptorSet set0Writes{};
		set0Writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set0Writes.dstSet = set0;
		set0Writes.dstBinding = 0;
		set0Writes.dstArrayElement = 0;
		set0Writes.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		set0Writes.descriptorCount = 1;
		set0Writes.pBufferInfo = &bufferInfo;
		vkUpdateDescriptorSets(mDevice, 1, &set0Writes, 0, nullptr);

        // SET 1: Per Draw Buffers
    	VkDescriptorBufferInfo dynamicBufferInfo{};
	    dynamicBufferInfo.buffer = mPerDrawConstBuffers;
	    dynamicBufferInfo.offset = 0;
	    dynamicBufferInfo.range =  mDynamicAlignment;

	    VkWriteDescriptorSet set1Writes{};
		set1Writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set1Writes.dstSet = set1;
		set1Writes.dstBinding = 0;
		set1Writes.dstArrayElement = 0;
		set1Writes.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		set1Writes.descriptorCount = 1;
		set1Writes.pBufferInfo = &dynamicBufferInfo;
		vkUpdateDescriptorSets(mDevice, 1, &set1Writes, 0, nullptr);

		// SET 2: Samplers (4 bindings)
		std::array<VkDescriptorImageInfo, SAMPLERS_COUNT> samplerInfos{};
		std::array<VkWriteDescriptorSet, SAMPLERS_COUNT> set2Writes{};
		for (int j = 0; j < SAMPLERS_COUNT; j++)
		{
			samplerInfos[j].sampler = mSamplers[j];
			set2Writes[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set2Writes[j].dstSet = set2;
			set2Writes[j].dstBinding = j;
			set2Writes[j].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			set2Writes[j].descriptorCount = 1;
			set2Writes[j].pImageInfo = &samplerInfos[j];
		}
		vkUpdateDescriptorSets(mDevice, static_cast<unsigned int>(set2Writes.size()), set2Writes.data(), 0, nullptr);
    }
}


void VKRenderer::CreatePerFrameConstBuffers()
{
	// TODO: implement this using reflection
	struct VkPerFrameConstBuffer
	{
		Matrix4x4 View;
		Matrix4x4 Proj;
		float Time;
	};

    VkDeviceSize bufferSize = sizeof(VkPerFrameConstBuffer);

    mPerFrameConstBufferSize = bufferSize;
	mPerFrameConstBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	mPerFrameConstBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
	mPerFrameConstBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		CreateBuffer(bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			mPerFrameConstBuffers[i], mPerFrameConstBufferMemory[i]);
	  	vkMapMemory(mDevice, mPerFrameConstBufferMemory[i], 0, bufferSize, 0, &mPerFrameConstBuffersMapped[i]);
	}

	mPerFrame = new ConstBuffer("PerFrame", 0, bufferSize);
	mPerFrame->AddBindStage(CB_VERTEX_BIND_STAGE);
	mPerFrame->AddBindStage(CB_PIXEL_BIND_STAGE);
	Variable variable;
	variable.Offset = offsetof(VkPerFrameConstBuffer, View);
	variable.Size = sizeof(Matrix4x4); 
	mPerFrame->AddVariable("View", variable);
	variable.Offset = offsetof(VkPerFrameConstBuffer, Proj);
	variable.Size = sizeof(Matrix4x4); 
	mPerFrame->AddVariable("Proj", variable);
	variable.Offset = offsetof(VkPerFrameConstBuffer, Time);
	variable.Size = sizeof(float); 
	mPerFrame->AddVariable("Time", variable);
}

void VKRenderer::CreatePerDrawConstBuffer()
{
	// TODO: implement this using reflection
	struct VkPerDrawConstBuffer
	{
		Matrix4x4 World;
		Vector3 Tint;
	};

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(mPhysicalDevice, &properties);
	size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
    mDynamicAlignment = sizeof(VkPerDrawConstBuffer);
    if (minUboAlignment > 0)
    {
		mDynamicAlignment = (mDynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	size_t bufferSize = DYNAMIC_CONST_BUFFER_BLOCK_COUNT * mDynamicAlignment;

	CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		mPerDrawConstBuffers, mPerDrawConstBufferMemory);

	mPerDrawConstBufferUsed = 0;

	mPerDraw = new ConstBuffer("mPerDraw", 0, mDynamicAlignment);
	mPerDraw->AddBindStage(CB_VERTEX_BIND_STAGE);
	mPerDraw->AddBindStage(CB_PIXEL_BIND_STAGE);
	Variable variable;
	variable.Offset = offsetof(VkPerDrawConstBuffer, World);
	variable.Size = sizeof(Matrix4x4); 
	mPerDraw->AddVariable("World", variable);
	variable.Offset = offsetof(VkPerDrawConstBuffer, Tint);
	variable.Size = sizeof(Vector3); 
	mPerDraw->AddVariable("Tint", variable);
}

void VKRenderer::CreateSamplers()
{

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(mPhysicalDevice, &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &mSamplers[0]) != VK_SUCCESS)
	{
		// TODO: handle error ...
    }

    samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &mSamplers[1]) != VK_SUCCESS)
	{
		// TODO: handle error ...
    }

    samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &mSamplers[2]) != VK_SUCCESS)
	{
		// TODO: handle error ...
    }

    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	if (vkCreateSampler(mDevice, &samplerInfo, nullptr, &mSamplers[3]) != VK_SUCCESS)
	{
		// TODO: handle error ...
    }
}