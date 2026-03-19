#include "VKRenderer.h"
#include "VKUtils.h"

#include "Platform.h"
#include "Config.h"

#include "ServiceProvider.h"

#include "Bindable.h"
#include "GraphicPipeline.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ConstBuffer.h"
#include "Texture2D.h"

#include "spirv_reflect.h"

// TODO: remove #include <windows.h>
#define NOMINMAX
#include <windows.h>

#include <set>

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

class VKIndexBufferID : public ResourceIdentifier
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

static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

VKRenderer::VKRenderer(const Config& config, Platform* platform)
{
	GetEventBus()->AddListener(EventType::WindowResizeEvent, this);

	CreateInstance(config, platform);
	SetUpDebugMessenger();
	CreateSurface(platform);
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageView();
	CreateDepthResource();
	CreateRenderPass();
	CreateFramebuffers();
	CreateCommandPool();
	CreateCommandBuffer();
	CreateSyncObjects();
	CreateSamplers();
	CreateDescriptorPool();
	CreateDescriptorSetLayout();

	mFramebufferResized = false;
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

	CleanupSwapChain();

	vkDestroyRenderPass(mDevice, mRenderPass, nullptr);

	vkDestroyDevice(mDevice, nullptr);
	if(ENABLE_VALIDATION_LAYERS)
	{
		VKUtils::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
	}
	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	vkDestroyInstance(mInstance, nullptr);

	if(mPerFrame) delete mPerFrame;
	if(mPerDraw) delete mPerDraw;

	GetEventBus()->RemoveListener(EventType::WindowResizeEvent, this);
}

void VKRenderer::CleanupSwapChain()
{
	vkDestroyImageView(mDevice, mDepthImageView, nullptr);
	vkDestroyImage(mDevice, mDepthImage, nullptr);
	vkFreeMemory(mDevice, mDepthImageMemory, nullptr);
    for (auto framebuffer : mSwapChainFramebuffers)
    {
        vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
    }
    for (auto imageView : mSwapChainImageViews)
    {
        vkDestroyImageView(mDevice, imageView, nullptr);
    }
	vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
}

void VKRenderer::RecreateSwapChain()
{
	vkDeviceWaitIdle(mDevice);
	CleanupSwapChain();

	CreateSwapChain();
	CreateImageView();
	CreateDepthResource();
	CreateFramebuffers();
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
	mFramebufferResized = true;
}

void VKRenderer::BeginFrame(float r, float g, float b)
{
	mPerDrawConstBufferUsed = 0;

	vkWaitForFences(mDevice, 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &mImageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
    	RecreateSwapChain();
    	return;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
		throw VKException("Error: vkAcquireNextImageKHR failed");
    }

    vkResetFences(mDevice, 1, &mInFlightFences[mCurrentFrame]);
    vkResetCommandBuffer(mCommandBuffers[mCurrentFrame], 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;
	if (vkBeginCommandBuffer(mCommandBuffers[mCurrentFrame], &beginInfo) != VK_SUCCESS)
	{
		throw VKException("Error: vkBeginCommandBuffer failed");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = mRenderPass;
	renderPassInfo.framebuffer = mSwapChainFramebuffers[mImageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = mSwapChainExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {{r, g, b, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};
	renderPassInfo.clearValueCount = static_cast<unsigned int>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

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
 		throw VKException("Error: vkEndCommandBuffer failed");
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
 		throw VKException("Error: vkQueueSubmit failed");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &mImageIndex;
	presentInfo.pResults = nullptr; // Optional
	VkResult result =  vkQueuePresentKHR(mPresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFramebufferResized)
	{
		mFramebufferResized = false;
	    RecreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
 		throw VKException("Error: vkQueuePresentKHR failed");
	}

	mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VKRenderer::OnLoadGraphicPipeline(ResourceIdentifier*& id, GraphicPipeline* graphicPipeline)
{
	VKGraphicPipelineID* resource = new VKGraphicPipelineID();
	id = resource;

	SpvReflectResult reflectResult{};
	SpvReflectShaderModule vertexModule;
	SpvReflectShaderModule pixelModule;
	reflectResult = spvReflectCreateShaderModule(graphicPipeline->GetVertexShaderSize(), graphicPipeline->GetVertexShaderData(), &vertexModule);
	assert(reflectResult == SPV_REFLECT_RESULT_SUCCESS);
	reflectResult = spvReflectCreateShaderModule(graphicPipeline->GetPixelShaderSize(), graphicPipeline->GetPixelShaderData(), &pixelModule);
	assert(reflectResult == SPV_REFLECT_RESULT_SUCCESS);

	VkShaderModule vertShaderModule = VKUtils::CreateShaderModule(mDevice, graphicPipeline->GetVertexShaderData(), graphicPipeline->GetVertexShaderSize());
	VkShaderModule pixelShaderModule = VKUtils::CreateShaderModule(mDevice, graphicPipeline->GetPixelShaderData(), graphicPipeline->GetPixelShaderSize());
	if(mPerFrame == nullptr)
	{
		LoadPerFrameConstBuffer(vertexModule);
	}
	if(mPerDraw == nullptr)
	{
		LoadPerDrawConstBuffer(vertexModule);
	}
	if(mPerFrame && mPerDraw && mPerFrameDescriptorSets.size() == 0)
	{
		CreateDescriptorSet();
	}

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = vertexModule.entry_point_name;

	VkPipelineShaderStageCreateInfo pixelShaderStageInfo{};
	pixelShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pixelShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	pixelShaderStageInfo.module = pixelShaderModule;
	pixelShaderStageInfo.pName = pixelModule.entry_point_name;

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, pixelShaderStageInfo};

	unsigned int count = 0;
	spvReflectEnumerateInputVariables(&vertexModule, &count, nullptr);
	std::vector<SpvReflectInterfaceVariable*> inputVariables(count);
	spvReflectEnumerateInputVariables(&vertexModule, &count, inputVariables.data());
	int stride = 0;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(inputVariables.size());
	for(int i = 0; i < inputVariables.size(); i++)
	{
		SpvReflectInterfaceVariable* variable = inputVariables[i];
		attributeDescriptions[i].binding = 0;
		attributeDescriptions[i].location = variable->location;
		attributeDescriptions[i].format = static_cast<VkFormat>(variable->format);
		attributeDescriptions[i].offset = stride;
		stride += (variable->numeric.scalar.width >> 3) * variable->numeric.vector.component_count;
	}

	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = stride;
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = mLayout;
	pipelineInfo.renderPass = mRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &resource->Pipeline) != VK_SUCCESS)
	{
		throw VKException("Error: vkCreateGraphicsPipelines failed");
	}

    vkDestroyShaderModule(mDevice, pixelShaderModule, nullptr);
    vkDestroyShaderModule(mDevice, vertShaderModule, nullptr);

	spvReflectDestroyShaderModule(&vertexModule);
	spvReflectDestroyShaderModule(&pixelModule);
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
	VKUtils::CreateBuffer(mPhysicalDevice, mDevice,
		bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertexBuffer->GetData(), (size_t)bufferSize);
	vkUnmapMemory(mDevice, stagingBufferMemory);

	VKUtils::CreateBuffer(mPhysicalDevice, mDevice,
		bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	resource->Buffer, resource->Memory);

	VKUtils::CopyBuffer(
		mDevice, mCommandPool, mGraphicsQueue,
		stagingBuffer, resource->Buffer, bufferSize);

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
	VKIndexBufferID* resource = new VKIndexBufferID();
	id = resource;

	VkDeviceSize bufferSize = indexBuffer->GetIndexQuantity() * sizeof(int);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VKUtils::CreateBuffer(mPhysicalDevice, mDevice, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indexBuffer->GetData(), (size_t)bufferSize);
	vkUnmapMemory(mDevice, stagingBufferMemory);

	VKUtils::CreateBuffer(mPhysicalDevice, mDevice, bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		resource->Buffer, resource->Memory);

	VKUtils::CopyBuffer(mDevice, mCommandPool, mGraphicsQueue,
						stagingBuffer, resource->Buffer, bufferSize);

	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
	vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
}

void VKRenderer::OnReleaseIndexBuffer(ResourceIdentifier* id)
{
	VKIndexBufferID* resource = reinterpret_cast<VKIndexBufferID*>(id);
	vkDeviceWaitIdle(mDevice);
	vkDestroyBuffer(mDevice, resource->Buffer, nullptr);
	vkFreeMemory(mDevice, resource->Memory, nullptr);
	delete resource;
}

void VKRenderer::OnLoadTexture2D(ResourceIdentifier*& id, Texture2D* texture2d)
{
	VKTexture2DID* resource = new VKTexture2DID();
	id = resource;

	{
	    VkDeviceSize imageSize = texture2d->GetWidth() * texture2d->GetHeight() * texture2d->GetChannels();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		VKUtils::CreateBuffer(mPhysicalDevice,  mDevice, imageSize,
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
			throw VKException("Error: vkCreateImage failed");
		}

		// Alloc Memory for the Image
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(mDevice, resource->Image, &memRequirements);
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = VKUtils::FindMemoryType(mPhysicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &resource->Memory) != VK_SUCCESS)
		{
			throw VKException("Error: vkAllocateMemory failed");
		}
		vkBindImageMemory(mDevice, resource->Image, resource->Memory, 0);

		VKUtils::TransitionImageLayout(
			mDevice, mCommandPool, mGraphicsQueue,
			resource->Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VKUtils::CopyBufferToImage(
			mDevice, mCommandPool, mGraphicsQueue,
			stagingBuffer, resource->Image,
			static_cast<unsigned int>(texture2d->GetWidth()),
			static_cast<unsigned int>(texture2d->GetHeight()));
		VKUtils::TransitionImageLayout(
			mDevice, mCommandPool, mGraphicsQueue,
			resource->Image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		
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
			throw VKException("Error: vkCreateImageView failed");
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
			throw VKException("Error: vkAllocateDescriptorSets failed");
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

void VKRenderer::PushIndexBuffer(IndexBuffer* indexBuffer, VertexBuffer* vertexBuffer)
{
	VKVertexBufferID* vertexBufferResource = static_cast<VKVertexBufferID *>(vertexBuffer->GetIdentifier(this));
	VkBuffer vertexBuffers[] = {vertexBufferResource->Buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(mCommandBuffers[mCurrentFrame], 0, 1, vertexBuffers, offsets);

	VKIndexBufferID* indexBufferResource = static_cast<VKIndexBufferID *>(indexBuffer->GetIdentifier(this));
	vkCmdBindIndexBuffer(mCommandBuffers[mCurrentFrame], indexBufferResource->Buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(mCommandBuffers[mCurrentFrame], indexBuffer->GetIndexQuantity(), 1, 0, 0, 0);
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
			throw VKException("Error: mRequiredExtension not founded");
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
			throw VKException("Error: mRequiredLayers not founded");
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
		throw VKException("Error: vkCreateInstance failed");
	}
}

void VKRenderer::SetUpDebugMessenger()
{
	if(!ENABLE_VALIDATION_LAYERS)
	{
		return;
	}
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);
	if (VKUtils::CreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger) != VK_SUCCESS)
	{
    	throw VKException("Error: VKUtils::CreateDebugUtilsMessengerEXT failed");
	}
}

void VKRenderer::PickPhysicalDevice()
{
	mPhysicalDevice = VK_NULL_HANDLE;

	unsigned int deviceCount = 0;
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
	if(deviceCount == 0)
	{
    	throw VKException("Error: non GPU compatible with vulkan was founded");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());
	for(const VkPhysicalDevice& device : devices)
	{
		if(VKUtils::IsDeviceSiutable(device, mSurface, mDeviceExtensions))
		{
			mPhysicalDevice = device;
			break;
		}
	}

	if(mPhysicalDevice == VK_NULL_HANDLE)
	{
    	throw VKException("Error: non GPU compatible with vulkan was founded");
	}
}

void VKRenderer::CreateLogicalDevice()
{
	QueueFamilyIndices indices = VKUtils::FindQueueFamilies(mPhysicalDevice, mSurface);
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
		throw VKException("Error: vkCreateDevice failed");
	}

	vkGetDeviceQueue(mDevice, indices.GraphicsFamily.value(), 0, &mGraphicsQueue);
	vkGetDeviceQueue(mDevice, indices.PresentFamily.value(), 0, &mPresentQueue);
}

void VKRenderer::CreateSurface(Platform* platform)
{
	if(!platform->CreateVulkanPlatformSurface(mInstance, mSurface))
	{
		throw VKException("Error: CreateVulkanPlatformSurface failed");
	}
}

void VKRenderer::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = VKUtils::QuerySwapChainSupport(mPhysicalDevice, mSurface);
	VkSurfaceFormatKHR surfaceFormat = VKUtils::ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = VKUtils::ChooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = VKUtils::ChooseSwapExtent(swapChainSupport.Capabilities);

	unsigned int imageCount = swapChainSupport.Capabilities.minImageCount + 1;
	if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
	{
	    imageCount = swapChainSupport.Capabilities.maxImageCount;
	}

	QueueFamilyIndices indices = VKUtils::FindQueueFamilies(mPhysicalDevice, mSurface);
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
		throw VKException("Error: vkCreateSwapchainKHR failed");
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
			throw VKException("Error: vkCreateImageView failed");
		}
	}
}

void VKRenderer::CreateDepthResource()
{
	VkFormat depthFormat = VKUtils::FindDepthFormat(mPhysicalDevice);

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = mSwapChainExtent.width;
	imageInfo.extent.height = mSwapChainExtent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = depthFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional
	if (vkCreateImage(mDevice, &imageInfo, nullptr, &mDepthImage) != VK_SUCCESS)
	{
		throw VKException("Error: vkCreateImage failed");
	}

	// Alloc Memory for the Image
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(mDevice, mDepthImage, &memRequirements);
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = VKUtils::FindMemoryType(mPhysicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &mDepthImageMemory) != VK_SUCCESS)
	{
		throw VKException("Error: vkAllocateMemory failed");
	}
	vkBindImageMemory(mDevice, mDepthImage, mDepthImageMemory, 0);


	// Create Image View
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = mDepthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = depthFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	if (vkCreateImageView(mDevice, &viewInfo, nullptr, &mDepthImageView) != VK_SUCCESS)
	{
		throw VKException("Error: vkCreateImageView failed");
	}
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

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = VKUtils::FindDepthFormat(mPhysicalDevice);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<unsigned int>(attachments.size());;
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
	{
		throw VKException("Error: vkCreateRenderPass failed");
	}
}

void VKRenderer::CreateFramebuffers()
{
	mSwapChainFramebuffers.resize(mSwapChainImageViews.size());
	for (size_t i = 0; i < mSwapChainImageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments = {
		    mSwapChainImageViews[i],
		    mDepthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mRenderPass;
		framebufferInfo.attachmentCount = static_cast<unsigned int>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = mSwapChainExtent.width;
		framebufferInfo.height = mSwapChainExtent.height;
		framebufferInfo.layers = 1;
		if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw VKException("Error: vkCreateFramebuffer failed");
		}
	}
}

void VKRenderer::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = VKUtils::FindQueueFamilies(mPhysicalDevice, mSurface);
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();
	if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS)
	{
    	throw VKException("Error: vkCreateCommandPool failed");
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
    	throw VKException("Error: vkAllocateCommandBuffers failed");
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
	    	throw VKException("Error: CreateSyncObjects failed");
		}
    }

    for(int i = 0; i < mSwapChainImages.size(); i++)
    {
	    if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS)
		{
	    	throw VKException("Error: CreateSyncObjects failed");
		}
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
			throw VKException("Error: vkCreateDescriptorPool failed");
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
			throw VKException("Error: vkCreateDescriptorPool failed");
		}
	}
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
			throw VKException("Error: vkCreateDescriptorSetLayout failed");
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
			throw VKException("Error: vkCreateDescriptorSetLayout failed");
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
			throw VKException("Error: vkCreateDescriptorSetLayout failed");
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
			throw VKException("Error: vkCreateDescriptorSetLayout failed");
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
		throw VKException("Error: vkCreatePipelineLayout failed");
	}
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
		throw VKException("Error: vkCreateSampler failed");
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
		throw VKException("Error: vkCreateSampler failed");
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
		throw VKException("Error: vkCreateSampler failed");
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
		throw VKException("Error: vkCreateSampler failed");
    }
}

void VKRenderer::LoadPerFrameConstBuffer(const SpvReflectShaderModule& vertexModule)
{
	std::vector<ConstBuffer> set0ConstBuffers = VKUtils::CreateConstBufferPerSet(vertexModule,mPhysicalDevice, 0, false);
	assert(set0ConstBuffers.size() > 0);
	mPerFrame = new ConstBuffer(set0ConstBuffers[0]);
    mPerFrameConstBufferSize = mPerFrame->GetSize();
	mPerFrameConstBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	mPerFrameConstBufferMemory.resize(MAX_FRAMES_IN_FLIGHT);
	mPerFrameConstBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
	for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VKUtils::CreateBuffer(mPhysicalDevice, mDevice,
			mPerFrame->GetSize(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			mPerFrameConstBuffers[i], mPerFrameConstBufferMemory[i]);
	  	vkMapMemory(mDevice, mPerFrameConstBufferMemory[i], 0, mPerFrame->GetSize(), 0, &mPerFrameConstBuffersMapped[i]);
	}
}

void VKRenderer::LoadPerDrawConstBuffer(const SpvReflectShaderModule& vertexModule)
{
	std::vector<ConstBuffer> set1ConstBuffers = VKUtils::CreateConstBufferPerSet(vertexModule, mPhysicalDevice, 1, true);
	assert(set1ConstBuffers.size() > 0);
	mPerDraw = new ConstBuffer(set1ConstBuffers[0]);
	size_t bufferSize = DYNAMIC_CONST_BUFFER_BLOCK_COUNT * mPerDraw->GetSize();
	VKUtils::CreateBuffer(mPhysicalDevice, mDevice,
		bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		mPerDrawConstBuffers, mPerDrawConstBufferMemory);
	mPerDrawConstBufferUsed = 0;
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
		throw VKException("Error: vkAllocateDescriptorSets failed");
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
	    dynamicBufferInfo.range =  mPerDraw->GetSize();

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


static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}