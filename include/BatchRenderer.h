#ifndef BATCH_RENDERER_H
#define BATCH_RENDERER_H

#include "Sprite.h"
#include "math/Vector2.h"

class Renderer;

struct QuadVertex
{
	Vector3 Position;
	Vector3 Color;
	Vector2 TCoord;
};

const int VERTEX_COUNT_PER_SPRITE = 6;

class BatchRenderer
{
public:
	static BatchRenderer* Create(Renderer* renderer, int maxSpriteCount);
	virtual  ~BatchRenderer() = default;
	virtual void DrawSprites(Sprite* sprites, int count) = 0;
protected:
	BatchRenderer() = default;
};

#endif