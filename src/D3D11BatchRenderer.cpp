#include "D3D11Renderer.h"
#include "D3D11BatchRenderer.h"

static D3D11Sprite gSprite =
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

D3D11BatchRenderer::D3D11BatchRenderer(Renderer* renderer, int maxSpriteCount)
{
	mRenderer = reinterpret_cast<D3D11Renderer*>(renderer);

	mDeviceContext = mRenderer->GetDeviceContext();
	mMaxSpriteCount = maxSpriteCount;

	AllocBuffer();
}

D3D11BatchRenderer::~D3D11BatchRenderer()
{
	FreeBuffer();
}

void D3D11BatchRenderer::DrawSprites(Sprite* sprites, int count)
{
	if(count > mMaxSpriteCount)
	{
		FreeBuffer();
		mMaxSpriteCount = count * 2;
		AllocBuffer();
	}

	mCpuBuffer.clear();
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

		D3D11Sprite d3dSprite;
		for(int i = 0; i < VERTEX_COUNT_PER_SPRITE; i++)
		{
			d3dSprite.Vertices[i].Position.X = gSprite.Vertices[i].Position.X * sprite->Scale.X;
			d3dSprite.Vertices[i].Position.Y = gSprite.Vertices[i].Position.Y * sprite->Scale.Y;
			d3dSprite.Vertices[i].Position.Z = gSprite.Vertices[i].Position.Z;
			d3dSprite.Vertices[i].Position = Matrix4x4::TransformPoint(sprite->Rotation, d3dSprite.Vertices[i].Position);
			d3dSprite.Vertices[i].Position += sprite->Position;
			d3dSprite.Vertices[i].Color = sprite->Color;
			d3dSprite.Vertices[i].TCoord = uvsArray[i];
		}
		mCpuBuffer.push_back(d3dSprite);
	}


	D3D11_MAPPED_SUBRESOURCE bufferData;
	ZeroMemory(&bufferData, sizeof(bufferData));
	mDeviceContext->Map(mGpuBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferData);
	memcpy(bufferData.pData, mCpuBuffer.data(), sizeof(D3D11Sprite) * mCpuBuffer.size());
	mDeviceContext->Unmap(mGpuBuffer, 0);
	mRenderer->PushBatchRenderer(this);
}

void D3D11BatchRenderer::DrawBuffer()
{
	unsigned int stride = sizeof(D3D11Sprite) / VERTEX_COUNT_PER_SPRITE;
	unsigned int offset = 0;
	mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mDeviceContext->IASetVertexBuffers(0, 1, &mGpuBuffer, &stride, &offset);
	mDeviceContext->Draw(mCpuBuffer.size() * VERTEX_COUNT_PER_SPRITE, 0);
}

void D3D11BatchRenderer::AllocBuffer()
{
	mCpuBuffer.reserve(mMaxSpriteCount);
	D3D11_BUFFER_DESC vertexDesc;
	ZeroMemory(&vertexDesc, sizeof(vertexDesc));
	vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexDesc.ByteWidth = sizeof(D3D11Sprite) * mMaxSpriteCount;
	if (FAILED(mRenderer->GetDevice()->CreateBuffer(&vertexDesc, 0, &mGpuBuffer)))
	{
		// TODO: error handling...	
	}
}

void D3D11BatchRenderer::FreeBuffer()
{
	mGpuBuffer->Release();
	mCpuBuffer.clear();
}
