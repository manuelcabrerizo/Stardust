#include "Renderer.h"
#include "Config.h"
#include "Bindable.h"
#include "GraphicPipeline.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture2D.h"

#include "D3D11Renderer.h"
#include "VKRenderer.h"

#include <cassert>

Renderer* Renderer::Create(const Config& config, Platform* platform)
{
#if SD_D3D11
	return new D3D11Renderer(config, platform);
#elif SD_VULKAN
	return new VKRenderer(config, platform);
#endif
	return nullptr;
}

Renderer::Renderer()
{
	mPerFrame = nullptr;
	mPerDraw = nullptr;
}

void Renderer::LoadGraphicPipeline(GraphicPipeline* graphicPipeline)
{
	if(!graphicPipeline)
	{
		return;
	}
	ResourceIdentifier* id = graphicPipeline->GetIdentifier(this);
	if(!id)
	{
		OnLoadGraphicPipeline(id, graphicPipeline);
		graphicPipeline->OnLoad(this, &Renderer::ReleaseGraphicPipeline, id);
	}
}

void Renderer::ReleaseGraphicPipeline(Bindable* graphicPipeline)
{
	if(!graphicPipeline)
	{
		return;
	}
	ResourceIdentifier* id = graphicPipeline->GetIdentifier(this);
	if(id)
	{
		OnReleaseGraphicPipeline(id);
		graphicPipeline->OnRelease(this, id);
	}
}

void Renderer::LoadVertexBuffer(VertexBuffer* vertexBuffer)
{
	if(!vertexBuffer)
	{
		return;
	}
	ResourceIdentifier* id = vertexBuffer->GetIdentifier(this);
	if(!id)
	{
		OnLoadVertexBuffer(id, vertexBuffer);
		vertexBuffer->OnLoad(this, &Renderer::ReleaseVertexBuffer, id);
	}
}

void Renderer::ReleaseVertexBuffer(Bindable* vertexBuffer)
{
	if(!vertexBuffer)
	{
		return;
	}

	ResourceIdentifier* id = vertexBuffer->GetIdentifier(this);
	if(id)
	{
		OnReleaseVertexBuffer(id);
		vertexBuffer->OnRelease(this, id);
	}
}

void Renderer::LoadIndexBuffer(IndexBuffer* indexBuffer)
{
	if(!indexBuffer)
	{
		return;
	}
	ResourceIdentifier* id = indexBuffer->GetIdentifier(this);
	if(!id)
	{
		OnLoadIndexBuffer(id, indexBuffer);
		indexBuffer->OnLoad(this, &Renderer::ReleaseIndexBuffer, id);
	}
}

void Renderer::ReleaseIndexBuffer(Bindable* indexBuffer)
{
	if(!indexBuffer)
	{
		return;
	}
	ResourceIdentifier* id = indexBuffer->GetIdentifier(this);
	if(id)
	{
		OnReleaseIndexBuffer(id);
		indexBuffer->OnRelease(this, id);
	}
}

void Renderer::LoadTexture2D(Texture2D* texture2d)
{
	if(!texture2d)
	{
		return;
	}
	ResourceIdentifier* id = texture2d->GetIdentifier(this);
	if(!id)
	{
		OnLoadTexture2D(id, texture2d);
		texture2d->OnLoad(this, &Renderer::ReleaseTexture2D, id);
	}
}

void Renderer::ReleaseTexture2D(Bindable* texture2d)
{
	if(!texture2d)
	{
		return;
	}
	ResourceIdentifier* id = texture2d->GetIdentifier(this);
	if(id)
	{
		OnReleaseTexture2D(id);
		texture2d->OnRelease(this, id);
	}
}