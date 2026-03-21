#ifndef D3D11_BATCH_RENDERER_h
#define D3D11_BATCH_RENDERER_h

#include "BatchRenderer.h"
#include <vector>

struct D3D11Sprite
{
	QuadVertex Vertices[VERTEX_COUNT_PER_SPRITE];
};

class D3D11Renderer;
class ID3D11DeviceContext;
class ID3D11Buffer;

class D3D11BatchRenderer : public BatchRenderer
{
public:
	D3D11BatchRenderer(Renderer* renderer, int maxSpriteCount);
	~D3D11BatchRenderer();
	
	void DrawSprites(Sprite* sprites, int count) override;
	void DrawBuffer();
private:
	void AllocBuffer();
	void FreeBuffer();

	D3D11Renderer* mRenderer;
    ID3D11DeviceContext* mDeviceContext;
    ID3D11Buffer* mGpuBuffer;
    std::vector<D3D11Sprite> mCpuBuffer;
    int mMaxSpriteCount;
};

#endif