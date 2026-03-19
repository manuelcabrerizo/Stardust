#include "VKBatchRenderer.h"
#include "VKUtils.h"

static VKSprite gSprite =
{
	{
		{Vector3(-0.5, -0.5, 0), Vector3(0,0,1), Vector2(1, 0)},
		{Vector3(-0.5,  0.5, 0), Vector3(0,0,1), Vector2(1, 1)},
		{Vector3( 0.5,  0.5, 0), Vector3(0,0,1), Vector2(0, 1)},
		{Vector3( 0.5,  0.5, 0), Vector3(0,0,1), Vector2(0, 1)},
		{Vector3( 0.5, -0.5, 0), Vector3(0,0,1), Vector2(0, 0)},
		{Vector3(-0.5, -0.5, 0), Vector3(0,0,1), Vector2(1, 0)},
	}
};

VKBatchRenderer::VKBatchRenderer(Renderer* renderer, int maxSpriteCount)
{
	mRenderer = reinterpret_cast<VKRenderer*>(renderer);
	mPhysicalDevice = mRenderer->GetPhysicalDevice();
	mDevice = mRenderer->GetDevice();
	for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		mMaxSpriteCount[i] = maxSpriteCount;
		AllocBuffers(i);
	}
}

VKBatchRenderer::~VKBatchRenderer()
{
	for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		FreeBuffers(i);
	}
}

void VKBatchRenderer::DrawSprites(Sprite* sprites, int count)
{
	unsigned int frameIndex = mRenderer->GetCurrentFrame();

	if(count > mMaxSpriteCount[frameIndex])
	{
		FreeBuffers(frameIndex);
		mMaxSpriteCount[frameIndex] = count * 2;
		AllocBuffers(frameIndex);
	}

	mCpuBuffers[frameIndex].clear();
	for(int j = 0; j < count; j++)
	{
		Sprite* sprite = sprites + j;

		Vector2 minUv = Vector2(sprite->Uvs.X, sprite->Uvs.Y);
		Vector2 maxUv = Vector2(sprite->Uvs.Z, sprite->Uvs.W);
		Vector2 uvsArray[VERTEX_COUNT_PER_SPRITE] =
		{
			Vector2(maxUv.X, minUv.Y),
			Vector2(maxUv.X, maxUv.Y),
			Vector2(minUv.X, maxUv.Y),
			Vector2(minUv.X, maxUv.Y),
			Vector2(minUv.X, minUv.Y),
			Vector2(maxUv.X, minUv.Y),
		};

		VKSprite vkSprite;
		for(int i = 0; i < VERTEX_COUNT_PER_SPRITE; i++)
		{
			vkSprite.Vertices[i].Position.X = gSprite.Vertices[i].Position.X * sprite->Scale.X;
			vkSprite.Vertices[i].Position.Y = gSprite.Vertices[i].Position.Y * sprite->Scale.Y;
			vkSprite.Vertices[i].Position.Z = gSprite.Vertices[i].Position.Z;
			vkSprite.Vertices[i].Position = Matrix4x4::TransformPoint(sprite->Rotation, vkSprite.Vertices[i].Position);
			vkSprite.Vertices[i].Position += sprite->Position;
			vkSprite.Vertices[i].Normal = gSprite.Vertices[i].Normal;
			vkSprite.Vertices[i].TCoord = uvsArray[i];
		}
		mCpuBuffers[frameIndex].push_back(vkSprite);
	}

	VkDeviceSize size = sizeof(VKSprite) * mMaxSpriteCount[frameIndex];
	void* data;
	vkMapMemory(mDevice, mGpuBufferMemorys[frameIndex], 0, size, 0, &data);
    memcpy(data, mCpuBuffers[frameIndex].data(), size);
	vkUnmapMemory(mDevice, mGpuBufferMemorys[frameIndex]);

	VkBuffer vertexBuffers[] = {mGpuBuffers[frameIndex]};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(mRenderer->GetCommandBuffer(frameIndex), 0, 1, vertexBuffers, offsets);
	vkCmdDraw(mRenderer->GetCommandBuffer(frameIndex), mCpuBuffers[frameIndex].size() * VERTEX_COUNT_PER_SPRITE, 1, 0, 0);
}

void VKBatchRenderer::AllocBuffers(unsigned int frameIndex)
{
	mCpuBuffers[frameIndex].reserve(mMaxSpriteCount[frameIndex]);
	VkDeviceSize size = sizeof(VKSprite) * mMaxSpriteCount[frameIndex];
	VKUtils::CreateBuffer(mPhysicalDevice, mDevice,
						  size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
						  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						  mGpuBuffers[frameIndex], mGpuBufferMemorys[frameIndex]);
}

void VKBatchRenderer::FreeBuffers(unsigned int frameIndex)
{
	//vkDeviceWaitIdle(mDevice);
	vkDestroyBuffer(mDevice, mGpuBuffers[frameIndex], nullptr);
	vkFreeMemory(mDevice, mGpuBufferMemorys[frameIndex], nullptr);
	mCpuBuffers[frameIndex].clear();
}