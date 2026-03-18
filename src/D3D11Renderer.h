#ifndef D3D11_RENDERER_H
#define D3D11_RENDERER_H

#include "Renderer.h"

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <vector>

struct Config;
class Platform;

struct ConstBufferInfo
{
	unsigned int Slot;
	ConstBufferBindStage BindStage;
	unsigned int Offset;
	unsigned int Size;
};

struct Texture2DInfo
{
	unsigned int Slot;
	Texture2D* Texture;
};

struct IndexBufferInfo
{
	VertexBuffer* VBuffer;
	IndexBuffer* IBuffer;
};

enum class D3D11RenderItemType
{
	ConstBuffer,
	VertexBuffer,
	IndexBuffer,
	GraphicPipeline,
	Texture2D
};

struct D3D11RenderItem
{
	D3D11RenderItemType Type;
	union
	{
		ConstBufferInfo Info;
		VertexBuffer* VBuffer;
		IndexBufferInfo IBuffer;
		GraphicPipeline* Pipeline;
		Texture2DInfo Tex2DInfo;
	};
};

class D3D11Renderer : public Renderer
{
public:
	D3D11Renderer(const Config& config, Platform* platform);
	~D3D11Renderer();
	void BeginFrame() override;
	void EndFrame() override;
protected:
	void OnLoadGraphicPipeline(ResourceIdentifier*& id, GraphicPipeline* graphicPipeline) override;
	void OnReleaseGraphicPipeline(ResourceIdentifier* id) override;
	void OnLoadVertexBuffer(ResourceIdentifier*& id, VertexBuffer* vertexBuffer) override;
	void OnReleaseVertexBuffer(ResourceIdentifier* id) override;
	void OnLoadIndexBuffer(ResourceIdentifier*& id, IndexBuffer* indexBuffer) override;
	void OnReleaseIndexBuffer(ResourceIdentifier* id) override;
	void OnLoadTexture2D(ResourceIdentifier*& id, Texture2D* texture2d) override;
	void OnReleaseTexture2D(ResourceIdentifier* id) override;
	void PushGraphicPipeline(GraphicPipeline* graphicPipeline) override;
	void PushPerFrameVariables() override;
	void PushPerDrawVariables() override;	
	void PushVerteBuffer(VertexBuffer* vertexBuffer) override;
	void PushIndexBuffer(IndexBuffer* indexBuffer, VertexBuffer* vertexBuffer) override;
	void PushTexture(Texture2D* texture2d, int slot) override;
private:
	bool CreateDevice();
	bool CreateSwapChain(const Config& config);
	bool CreateRenderTargetView();
	bool CreateDepthStencilView(const Config& config);
	bool CreateGlobalConstBuffer();
	bool CreateSamplerStates();
	void LoadPerFrameConstBuffer(ID3D11ShaderReflection* reflection, const D3D11_SHADER_DESC& desc, ConstBufferBindStage bindStage);
	void LoadPerDrawConstBuffer(ID3D11ShaderReflection* reflection, const D3D11_SHADER_DESC& desc, ConstBufferBindStage bindStage);
	void BeginRenderingSession();
	void EndRenderingSession();

	HWND mWindow;
	ID3D11Device *mDevice;
	ID3D11DeviceContext *mDeviceContext;
	ID3D11DeviceContext1 *mDeviceContext1;
	unsigned int mMsaaQuality4x;
	IDXGISwapChain *mSwapChain;
	IDXGISwapChain1 *mSwapChain1;
	ID3D11RenderTargetView* mRenderTargetView;
	ID3D11DepthStencilView* mDepthStencilView;

	ID3D11Buffer* mPerFrameConstBuffer;
	// Global const Buffer
	// TODO: rename to mPerDrawConstBuffer
	ID3D11Buffer* mGlobalConstBuffer;
	unsigned int mGlobalConstBufferSize;
	unsigned int mGlobalConstBufferUsed;
	D3D11_MAPPED_SUBRESOURCE mMappedSubresource;
	std::vector<D3D11RenderItem> mRenderItems;

	// Sampler State
	ID3D11SamplerState* mLinearClampSamplerState;
	ID3D11SamplerState* mLinearWrapSamplerState;
	ID3D11SamplerState* mPointClampSamplerState;
	ID3D11SamplerState* mPointWrapSamplerState;
};

class ConstBufferMapException : public std::exception
{
public:
	ConstBufferMapException(const std::string& message)
	{
		mErrorMessage = message;
	}

	const char *what() const override
	{
		return mErrorMessage.c_str();
	}
private:
	std::string mErrorMessage;
};

#endif