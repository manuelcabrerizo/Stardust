#include "BatchRenderer.h"
#include "D3D11BatchRenderer.h"
#include "VKBatchRenderer.h"

BatchRenderer* BatchRenderer::Create(Renderer* renderer, int maxSpriteCount)
{
#if SD_D3D11
	return new D3D11BatchRenderer(renderer, maxSpriteCount);
#elif SD_VULKAN
	return new VKBatchRenderer(renderer, maxSpriteCount);
#endif
	return nullptr;
}
