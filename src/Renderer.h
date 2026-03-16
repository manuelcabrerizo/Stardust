#ifndef RENDERER_H
#define RENDERER_H

struct Config;
class Platform;
class ResourceIdentifier;
class Bindable;
class GraphicPipeline;
class VertexBuffer;
class IndexBuffer;
class Texture2D;

#include "ConstBuffer.h"

#include <string>
#include <cassert>

class Renderer
{
public:
	virtual ~Renderer() {}
	static Renderer* Create(const Config& config, Platform* platform);

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

	// Function pointer types for binding and unbinding resources
	typedef void (Renderer::*ReleaseFunction)(Bindable*);

	void LoadGraphicPipeline(GraphicPipeline* graphicPipeline);
	void ReleaseGraphicPipeline(Bindable* graphicPipeline);

	void LoadVertexBuffer(VertexBuffer* vertexBuffer);
	void ReleaseVertexBuffer(Bindable* vertexBuffer);

	void LoadIndexBuffer(IndexBuffer* indexBuffer);
	void ReleaseIndexBuffer(Bindable* indexBuffer);

	void LoadTexture2D(Texture2D* texture2d);
	void ReleaseTexture2D(Bindable* texture2d);

	template <typename T>
	void SetPerFrameVariable(const std::string& variable, const T& value)
	{
		assert(mPerFrame != nullptr);
		mPerFrame->SetVariable<T>(variable, value);
	}

	template <typename T>
	void SetPerDrawVariable(const std::string& variable, const T& value)
	{
		assert(mPerDraw != nullptr);
		mPerDraw->SetVariable<T>(variable, value);
	}

	virtual void PushGraphicPipeline(GraphicPipeline* graphicPipeline) = 0;
	virtual void PushPerFrameVariables() = 0;
	virtual void PushPerDrawVariables() = 0;
	virtual void PushVerteBuffer(VertexBuffer* vertexBuffer) = 0;
	virtual void PushIndexBuffer(IndexBuffer* indexBuffer) = 0;
	virtual void PushTexture(Texture2D* texture2d, int slot) = 0;

protected:
	Renderer();

	virtual void OnLoadGraphicPipeline(ResourceIdentifier*& id, GraphicPipeline* graphicPipeline) = 0;
	virtual void OnReleaseGraphicPipeline(ResourceIdentifier* id) = 0;

	virtual void OnLoadVertexBuffer(ResourceIdentifier*& id, VertexBuffer* vertexBuffer) = 0;
	virtual void OnReleaseVertexBuffer(ResourceIdentifier* id) = 0;

	virtual void OnLoadIndexBuffer(ResourceIdentifier*& id, IndexBuffer* indexBuffer) = 0;
	virtual void OnReleaseIndexBuffer(ResourceIdentifier* id) = 0;

	virtual void OnLoadTexture2D(ResourceIdentifier*& id, Texture2D* texture2d) = 0;
	virtual void OnReleaseTexture2D(ResourceIdentifier* id) = 0;

	ConstBuffer* mPerFrame;
	ConstBuffer* mPerDraw;
};

#endif