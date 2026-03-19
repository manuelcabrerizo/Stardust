#ifndef VK_BATCH_RENDERER_H
#define VK_BATCH_RENDERER_H

#include "BatchRenderer.h"
#include "VKRenderer.h"

#include "math/Vector2.h"
#include <array>
#include <vector>

struct VKSprite
{
	QuadVertex Vertices[VERTEX_COUNT_PER_SPRITE];
};

class VKBatchRenderer : public BatchRenderer
{
public:
	VKBatchRenderer(Renderer* renderer, int maxSpriteCount);
	~VKBatchRenderer();

	void DrawSprites(Sprite* sprites, int count) override;
private:
	void AllocBuffers(unsigned int frameIndex);
	void FreeBuffers(unsigned int frameIndex);

	VKRenderer* mRenderer;
	VkPhysicalDevice mPhysicalDevice;
	VkDevice mDevice;
	std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> mGpuBuffers;
	std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> mGpuBufferMemorys;
    std::array<std::vector<VKSprite>, MAX_FRAMES_IN_FLIGHT> mCpuBuffers;
	std::array<int, MAX_FRAMES_IN_FLIGHT> mMaxSpriteCount;
};

#endif