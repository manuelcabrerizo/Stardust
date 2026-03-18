#include "D3D11Renderer.h"
#include "Platform.h"
#include "Config.h"

#include "Bindable.h"
#include "GraphicPipeline.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ConstBuffer.h"
#include "Texture2D.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>

// Important Resource for CONST BUFFERS
// Source: https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0#

struct D3D11Format
{
	DXGI_FORMAT Format;
	int Channels;
};

static D3D11Format GetFormat(const D3D11_SIGNATURE_PARAMETER_DESC& paramDesc);
static std::vector<D3D11_INPUT_ELEMENT_DESC> GetInputLayoutDesc(ID3D11ShaderReflection* reflection, const D3D11_SHADER_DESC& desc);

class D3D11GraphicPipelineID : public ResourceIdentifier
{
public:
	ID3D11InputLayout* Layout;
	ID3D11VertexShader* VertexID;
	ID3D11PixelShader* PixelID;
};

class D3D11VertexBufferID : public ResourceIdentifier
{
public:
	ID3D11Buffer* ID;
};

class D3D11IndexBufferID : public ResourceIdentifier
{
public:
	ID3D11Buffer* ID;
	DXGI_FORMAT Format;
};

class D3D11Texture2DID : public ResourceIdentifier
{
public:
	ID3D11Texture2D *Texture;
	ID3D11ShaderResourceView *ShaderResourceView;
};

D3D11Renderer::D3D11Renderer(const Config& config, Platform* platform)
{
	GetEventBus()->AddListener(EventType::WindowResizeEvent, this);

	mWindow = static_cast<HWND>(platform->GetWindowHandle());

	mDevice = nullptr;
	mDeviceContext = nullptr;
	mDeviceContext1 = nullptr;
	mMsaaQuality4x = 0;
	mSwapChain = nullptr;
	mSwapChain1 = nullptr;
	mRenderTargetView = nullptr;
	mDepthStencilView = nullptr;
	mPerFrameConstBuffer = nullptr;
	mGlobalConstBuffer = nullptr;
	mGlobalConstBufferSize = 0;
	mGlobalConstBufferUsed = 0;
	mLinearClampSamplerState = nullptr;
	mLinearWrapSamplerState = nullptr;
	mPointClampSamplerState = nullptr;
	mPointWrapSamplerState = nullptr;

	CreateDevice();
	CreateSwapChain(config);
	CreateRenderTargetView();
	CreateDepthStencilView(config);
	CreateGlobalConstBuffer();
	CreateSamplerStates();

	mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);

	// Set up the viewport.
	D3D11_VIEWPORT vp;
	vp.Width = static_cast<float>(config.ScreenWidth);
	vp.Height = static_cast<float>(config.ScreenHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	mDeviceContext->RSSetViewports( 1, &vp );
}

D3D11Renderer::~D3D11Renderer()
{
	if(mPerFrame)
	{
		delete mPerFrame;
	}
	if(mPerDraw)
	{
		delete mPerDraw;
	}

	mLinearClampSamplerState->Release();
	mLinearWrapSamplerState->Release();
	mPointClampSamplerState->Release();
	mPointWrapSamplerState->Release();

	mPerFrameConstBuffer->Release();
	mGlobalConstBuffer->Release();
	mDepthStencilView->Release();
	mRenderTargetView->Release();
	mSwapChain1->Release();
	mSwapChain->Release();
	mDeviceContext1->Release();
	mDeviceContext->Release();
	mDevice->Release();

	GetEventBus()->RemoveListener(EventType::WindowResizeEvent, this);
}

void D3D11Renderer::OnEvent(const Event& event)
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

void D3D11Renderer::OnWindowResizeEvent(const WindowResizeEvent& windowResizeEvent)
{
	mDeviceContext->OMSetRenderTargets(0, 0, 0);
	mRenderTargetView->Release();
	mDepthStencilView->Release();

	mSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	CreateRenderTargetView();

	Config config{};
	config.ScreenWidth = windowResizeEvent.Width;
	config.ScreenHeight = windowResizeEvent.Height;
	CreateDepthStencilView(config);

	mDeviceContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
	D3D11_VIEWPORT vp;
	vp.Width = static_cast<float>(windowResizeEvent.Width);
	vp.Height = static_cast<float>(windowResizeEvent.Height);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	mDeviceContext->RSSetViewports( 1, &vp );
}

void D3D11Renderer::BeginFrame(float r, float g, float b)
{
	float clearColor[] = { r, g, b, 1.0f };
	mDeviceContext->ClearRenderTargetView(mRenderTargetView, clearColor);
	mDeviceContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	BeginRenderingSession();
}

void D3D11Renderer::EndFrame()
{
	EndRenderingSession();
	mSwapChain->Present(1, 0);
}

void D3D11Renderer::BeginRenderingSession()
{
	if(FAILED(mDeviceContext->Map(mGlobalConstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mMappedSubresource)))
	{
		throw ConstBufferMapException("Error mapping const buffer!");
	}
}

void D3D11Renderer::EndRenderingSession()
{
	mDeviceContext->Unmap(mGlobalConstBuffer, 0);
	for(int i = 0; i < mRenderItems.size(); i++)
	{
		const D3D11RenderItem& item = mRenderItems[i];
		switch(item.Type)
		{
			case D3D11RenderItemType::ConstBuffer:
			{
				const ConstBufferInfo& info  = item.Info;
				unsigned int offset = info.Offset / 16;
				unsigned int size = info.Size / 16;
				if(info.BindStage & CB_VERTEX_BIND_STAGE)
				{
					mDeviceContext1->VSSetConstantBuffers1(info.Slot, 1, &mGlobalConstBuffer, &offset, &size);
				}
				if(info.BindStage & CB_PIXEL_BIND_STAGE)
				{
					mDeviceContext1->PSSetConstantBuffers1(info.Slot, 1, &mGlobalConstBuffer, &offset, &size);
				}
			}break;
			case D3D11RenderItemType::VertexBuffer:
			{
				VertexBuffer *vertexBuffer = item.VBuffer;
				D3D11VertexBufferID* resource = static_cast<D3D11VertexBufferID *>(vertexBuffer->GetIdentifier(this));
				unsigned int vertexStride = vertexBuffer->GetVertexSize();
				unsigned int vertexOffset = 0;
				mDeviceContext->IASetVertexBuffers(0, 1, &resource->ID, &vertexStride, &vertexOffset);
				mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				mDeviceContext->Draw(vertexBuffer->GetVertexCount(), 0);
			}break;	
			case D3D11RenderItemType::IndexBuffer:
			{
				VertexBuffer *vertexBuffer = item.IBuffer.VBuffer;
				D3D11VertexBufferID* vResource = static_cast<D3D11VertexBufferID *>(vertexBuffer->GetIdentifier(this));
				unsigned int vertexStride = vertexBuffer->GetVertexSize();
				unsigned int vertexOffset = 0;
				mDeviceContext->IASetVertexBuffers(0, 1, &vResource->ID, &vertexStride, &vertexOffset);

				IndexBuffer *indexBuffer = item.IBuffer.IBuffer;
				D3D11IndexBufferID* iResource = static_cast<D3D11IndexBufferID *>(indexBuffer->GetIdentifier(this));
				mDeviceContext->IASetIndexBuffer(iResource->ID, iResource->Format, 0);
				mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				mDeviceContext->DrawIndexed(indexBuffer->GetIndexQuantity(), 0, 0);
			}break;
			case D3D11RenderItemType::GraphicPipeline:
			{
				GraphicPipeline* graphicPipeline = item.Pipeline;
				D3D11GraphicPipelineID* resource = reinterpret_cast<D3D11GraphicPipelineID*>(graphicPipeline->GetIdentifier(this));
				mDeviceContext->IASetInputLayout(resource->Layout);
				mDeviceContext->VSSetShader(resource->VertexID, 0, 0);
				mDeviceContext->PSSetShader(resource->PixelID, 0, 0);
			}break;
			case D3D11RenderItemType::Texture2D:
			{
				const Texture2DInfo& info = item.Tex2DInfo;
				D3D11Texture2DID* resource = reinterpret_cast<D3D11Texture2DID*>(info.Texture->GetIdentifier(this));
		     	mDeviceContext->PSSetShaderResources(info.Slot, 1, &resource->ShaderResourceView);
			}break; 
		}
	}
	mGlobalConstBufferUsed = 0;
	mRenderItems.clear();
}

void D3D11Renderer::OnLoadGraphicPipeline(ResourceIdentifier*& id, GraphicPipeline* graphicPipeline)
{
	D3D11GraphicPipelineID* resource = new D3D11GraphicPipelineID();
	id = resource;
	
	ConstBufferBindStage bindStage = (ConstBufferBindStage)((int)CB_VERTEX_BIND_STAGE|(int)CB_PIXEL_BIND_STAGE);
	// VERTEX SHADER
	{
		mDevice->CreateVertexShader(
			graphicPipeline->GetVertexShaderData(),
			graphicPipeline->GetVertexShaderSize(), 0,
			&resource->VertexID);

		ID3D11ShaderReflection* reflection = nullptr;
		D3DReflect(graphicPipeline->GetVertexShaderData(), graphicPipeline->GetVertexShaderSize(), IID_ID3D11ShaderReflection, (void**)&reflection);
		D3D11_SHADER_DESC desc;
		reflection->GetDesc(&desc);

		if(mPerFrame == nullptr)
		{
			LoadPerFrameConstBuffer(reflection, desc, bindStage);
		}
		if(mPerDraw == nullptr)
		{
			LoadPerDrawConstBuffer(reflection, desc, bindStage);
		}

		std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc = GetInputLayoutDesc(reflection, desc);
		mDevice->CreateInputLayout(
			inputLayoutDesc.data(), inputLayoutDesc.size(),
			graphicPipeline->GetVertexShaderData(), graphicPipeline->GetVertexShaderSize(),
			&resource->Layout);

		reflection->Release();
	}

	// PIXEL SHADER
	{
		mDevice->CreatePixelShader(
			graphicPipeline->GetPixelShaderData(),
			graphicPipeline->GetPixelShaderSize(), 0,
			&resource->PixelID);

		ID3D11ShaderReflection* reflection = nullptr;
		D3DReflect(graphicPipeline->GetPixelShaderData(), graphicPipeline->GetPixelShaderSize(), IID_ID3D11ShaderReflection, (void**)&reflection);
		D3D11_SHADER_DESC desc;
		reflection->GetDesc(&desc);
		
		if(mPerFrame == nullptr)
		{
			LoadPerFrameConstBuffer(reflection, desc, bindStage);
		}
		if(mPerDraw == nullptr)
		{
			LoadPerDrawConstBuffer(reflection, desc, bindStage);
		}

		reflection->Release();
	}

	// PER FRAME CONST BUFFER
	if(mPerFrame && mPerFrameConstBuffer == nullptr)
	{
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.ByteWidth = mPerFrame->GetSize();
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if(FAILED(mDevice->CreateBuffer(&bufferDesc, nullptr, &mPerFrameConstBuffer)))
		{
			assert(!"ERROR: failed to create mGlobalConstBuffer!");
		}

		if(mPerFrame->GetBindStage() & CB_VERTEX_BIND_STAGE)
		{
			mDeviceContext->VSSetConstantBuffers(mPerFrame->GetBinding(), 1, &mPerFrameConstBuffer);
		}
		if(mPerFrame->GetBindStage() & CB_PIXEL_BIND_STAGE)
		{
			mDeviceContext->VSSetConstantBuffers(mPerFrame->GetBinding(), 1, &mPerFrameConstBuffer);
		}
	}
}

void D3D11Renderer::OnReleaseGraphicPipeline(ResourceIdentifier* id)
{
	D3D11GraphicPipelineID* resource = reinterpret_cast<D3D11GraphicPipelineID*>(id);
	resource->PixelID->Release();
	resource->Layout->Release();
	resource->VertexID->Release();
	delete resource;
}

void D3D11Renderer::OnLoadVertexBuffer(ResourceIdentifier*& id, VertexBuffer* vertexBuffer)
{
	D3D11VertexBufferID* resource = new D3D11VertexBufferID();
	id = resource;
	// Create the vertex buffer
	D3D11_BUFFER_DESC vertexDesc = {};
	vertexDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexDesc.ByteWidth = vertexBuffer->GetVertexCount() * vertexBuffer->GetVertexSize();

	D3D11_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pSysMem = vertexBuffer->GetData();
	if(FAILED(mDevice->CreateBuffer(&vertexDesc, &subresourceData, &resource->ID)))
	{
		// TODO: handle error
	}
}

void D3D11Renderer::OnReleaseVertexBuffer(ResourceIdentifier* id)
{
	D3D11VertexBufferID* resource = reinterpret_cast<D3D11VertexBufferID*>(id);
	resource->ID->Release();
	delete resource;
}

void D3D11Renderer::OnLoadIndexBuffer(ResourceIdentifier*& id, IndexBuffer* indexBuffer)
{
	D3D11IndexBufferID* resource = new D3D11IndexBufferID();
	id = resource;
	resource->Format = DXGI_FORMAT_R32_UINT;

	// Create the index buffer
	D3D11_BUFFER_DESC indexDesc = {};
	indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexDesc.ByteWidth = indexBuffer->GetIndexQuantity() * sizeof(unsigned int);

	D3D11_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pSysMem = indexBuffer->GetData();
	if(FAILED(mDevice->CreateBuffer(&indexDesc, &subresourceData, &resource->ID)))
	{
		// TODO: handle error
	}
}

void D3D11Renderer::OnReleaseIndexBuffer(ResourceIdentifier* id)
{
	D3D11IndexBufferID* resource = reinterpret_cast<D3D11IndexBufferID*>(id);
	resource->ID->Release();
	delete resource;
}

void D3D11Renderer::OnLoadTexture2D(ResourceIdentifier*& id, Texture2D* texture2d)
{
	D3D11Texture2DID* resource = new D3D11Texture2DID();
	id = resource;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = texture2d->GetWidth();
	texDesc.Height = texture2d->GetHeight();
	texDesc.MipLevels = 0;
	texDesc.ArraySize = 1;
	texDesc.Format = texture2d->IsSRGB() ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	if (FAILED(mDevice->CreateTexture2D(&texDesc, nullptr, &resource->Texture)))
	{
		// TODO: handle error
	}

	D3D11_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pSysMem = texture2d->GetData();
	subresourceData.SysMemPitch = texture2d->GetWidth() * texture2d->GetChannels();
	mDeviceContext->UpdateSubresource(resource->Texture, 0, 0, subresourceData.pSysMem, subresourceData.SysMemPitch, 0);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	texDesc.Format = texture2d->IsSRGB() ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	if (FAILED(mDevice->CreateShaderResourceView(resource->Texture, &srvDesc, &resource->ShaderResourceView)))
	{
		// TODO: handle error
	}

	mDeviceContext->GenerateMips(resource->ShaderResourceView);
}

void D3D11Renderer::OnReleaseTexture2D(ResourceIdentifier* id)
{
	D3D11Texture2DID* resource = reinterpret_cast<D3D11Texture2DID*>(id);
	resource->ShaderResourceView->Release();
	resource->Texture->Release();
	delete resource;
}

void D3D11Renderer::PushGraphicPipeline(GraphicPipeline* graphicPipeline)
{
	D3D11RenderItem item;
	item.Type = D3D11RenderItemType::GraphicPipeline;
	item.Pipeline = graphicPipeline;
	mRenderItems.push_back(item);
}

void  D3D11Renderer::PushPerFrameVariables()
{
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	if(FAILED(mDeviceContext->Map(mPerFrameConstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource)))
	{
		throw ConstBufferMapException("Error mapping const buffer!");
	}
	memcpy(mappedSubresource.pData, mPerFrame->GetData(), mPerFrame->GetUsedSize());
	mDeviceContext->Unmap(mPerFrameConstBuffer, 0);
}

void  D3D11Renderer::PushPerDrawVariables()
{
	if(mGlobalConstBufferUsed + mPerDraw->GetSize() > mGlobalConstBufferSize)
	{
		EndRenderingSession();
		BeginRenderingSession();
	}

	unsigned int alignSize = (mPerDraw->GetSize() + 255) & ~255;

	ConstBufferInfo info;
	info.Slot = mPerDraw->GetBinding();
	info.BindStage = mPerDraw->GetBindStage();
	info.Offset = mGlobalConstBufferUsed;
	info.Size = alignSize;

	D3D11RenderItem item;
	item.Type = D3D11RenderItemType::ConstBuffer;
	item.Info = info;
	mRenderItems.push_back(item);

	memcpy((unsigned char*)mMappedSubresource.pData + mGlobalConstBufferUsed, mPerDraw->GetData(), mPerDraw->GetUsedSize());
	mGlobalConstBufferUsed += alignSize;
}


void D3D11Renderer::PushVerteBuffer(VertexBuffer *vertexBuffer)
{
	D3D11RenderItem item;
	item.Type = D3D11RenderItemType::VertexBuffer;
	item.VBuffer = vertexBuffer;
	mRenderItems.push_back(item);
}

void D3D11Renderer::PushIndexBuffer(IndexBuffer* indexBuffer, VertexBuffer* vertexBuffer)
{
	D3D11RenderItem item;
	item.Type = D3D11RenderItemType::IndexBuffer;
	item.IBuffer.VBuffer = vertexBuffer;
	item.IBuffer.IBuffer = indexBuffer;
	mRenderItems.push_back(item);
}

void D3D11Renderer::PushTexture(Texture2D* texture2d, int slot)
{
	Texture2DInfo info;
	info.Slot = slot;
	info.Texture = texture2d;

	D3D11RenderItem item;
	item.Type = D3D11RenderItemType::Texture2D;
	item.Tex2DInfo = info;
	mRenderItems.push_back(item);
}

bool D3D11Renderer::CreateDevice()
{
	// TODO: create a debug FLAG
	int deviceFlags = D3D11_CREATE_DEVICE_DEBUG;

	const unsigned int driverTypesCount = 3;
	D3D_DRIVER_TYPE driverTypes[driverTypesCount] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_SOFTWARE
	};
	
	const unsigned int featureLevelsCount = 4;
	D3D_FEATURE_LEVEL featureLevels[featureLevelsCount] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
     
    bool deviceFound = false;
	D3D_FEATURE_LEVEL featureLevel;
	for(unsigned int driver = 0; driver < driverTypesCount; ++driver)
	{
		if (SUCCEEDED(D3D11CreateDevice(nullptr, driverTypes[driver], 0, deviceFlags,
		 	featureLevels, featureLevelsCount,
		 	D3D11_SDK_VERSION, &mDevice, &featureLevel, &mDeviceContext)))
		{
			deviceFound = true;
			break;
		}
		else
		{
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			if (SUCCEEDED(D3D11CreateDevice(nullptr, driverTypes[driver], 0, deviceFlags,
				&featureLevels[1], featureLevelsCount - 1,
				D3D11_SDK_VERSION, &mDevice, &featureLevel, &mDeviceContext)))
			{
				deviceFound = true;
				break;
			}
		}
	}

	if(!deviceFound)
	{
		return false;
	}

	// DirectX 11.1 or later
	if(FAILED(mDeviceContext->QueryInterface(IID_PPV_ARGS(&mDeviceContext1))))
	{
		return false;
	}

	// Check for msaa
	mDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &mMsaaQuality4x);
	if(mMsaaQuality4x <= 0)
	{
		// TODO: this is not necesary a limiation to play the game, return false for now
		return false;
	}
	return true;
}

bool D3D11Renderer::CreateSwapChain(const Config& config)
{
	// Obtain DXGIFactory from device (since we use nullptr for pAdapter above)
	IDXGIFactory1* dxgiFactory = nullptr;
	IDXGIDevice* dxgiDevice = nullptr;
	if(SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice))))
	{
		IDXGIAdapter* dxgiAdapter = nullptr;
		if(SUCCEEDED(dxgiDevice->GetAdapter(&dxgiAdapter)))
		{
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
			dxgiAdapter->Release();
		}
		else
		{
			return false;
		}
		dxgiDevice->Release();
	}
	else
	{
		return false;
	}

	IDXGIFactory2* dxgiFactory2 = nullptr;
	if(SUCCEEDED(dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory2))))
	{
		// TODO: implement fullscreen
		DXGI_SWAP_CHAIN_DESC1 sd = {};
        	sd.Width = config.ScreenWidth;
		sd.Height = config.ScreenHeight;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = mMsaaQuality4x - 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 2;
		if(SUCCEEDED(dxgiFactory2->CreateSwapChainForHwnd(mDevice, mWindow, &sd, nullptr, nullptr, &mSwapChain1)))
		{
			mSwapChain1->QueryInterface(IID_PPV_ARGS(&mSwapChain));
		}
		dxgiFactory2->Release();
	}
	else
	{
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 2;
		sd.BufferDesc.Width = config.ScreenWidth;
		sd.BufferDesc.Height = config.ScreenHeight;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = mWindow;
		sd.SampleDesc.Count = 4;
		sd.SampleDesc.Quality = mMsaaQuality4x - 1;
		sd.Windowed = !config.Fullscreen;
		dxgiFactory->CreateSwapChain(mDevice, &sd, &mSwapChain);
	}
	dxgiFactory->Release();
	return true;
}

bool D3D11Renderer::CreateRenderTargetView()
{
	ID3D11Texture2D* backBufferTexture = nullptr;
	mSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferTexture));
	if(backBufferTexture && FAILED(mDevice->CreateRenderTargetView(backBufferTexture, 0, &mRenderTargetView)))
	{
		return false;
	}
	backBufferTexture->Release();
	return true;
}

bool D3D11Renderer::CreateDepthStencilView(const Config& config)
{
     ID3D11Texture2D *depthStencilTexture;
     D3D11_TEXTURE2D_DESC depthStencilTextureDesc;
     depthStencilTextureDesc.Width = config.ScreenWidth;
     depthStencilTextureDesc.Height = config.ScreenHeight;
     depthStencilTextureDesc.MipLevels = 1;
     depthStencilTextureDesc.ArraySize = 1;
     depthStencilTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
     depthStencilTextureDesc.SampleDesc.Count = 4;
     depthStencilTextureDesc.SampleDesc.Quality = mMsaaQuality4x - 1;
     depthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
     depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
     depthStencilTextureDesc.CPUAccessFlags = 0;
     depthStencilTextureDesc.MiscFlags = 0;
     if (FAILED(mDevice->CreateTexture2D(&depthStencilTextureDesc, 0, &depthStencilTexture)))
     {
     	return false;
     }
     if (depthStencilTexture && FAILED(mDevice->CreateDepthStencilView(depthStencilTexture, 0, &mDepthStencilView)))
     {
     	return false;
     }
     depthStencilTexture->Release();
     return true;
}

bool D3D11Renderer::CreateGlobalConstBuffer()
{
	mGlobalConstBufferSize = MB(10);
	mGlobalConstBufferUsed = 0;

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.ByteWidth = mGlobalConstBufferSize;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if(FAILED(mDevice->CreateBuffer(&bufferDesc, nullptr, &mGlobalConstBuffer)))
	{
		assert(!"ERROR: failed to create mGlobalConstBuffer!");
	}
	return true;
}

bool D3D11Renderer::CreateSamplerStates()
{
     D3D11_SAMPLER_DESC samplerDesc = {};
     samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
     samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
     samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
     samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
     samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
     samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
     if (FAILED(mDevice->CreateSamplerState(&samplerDesc, &mLinearClampSamplerState)))
     {
          return false;
     }
     samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
     if (FAILED(mDevice->CreateSamplerState(&samplerDesc, &mPointClampSamplerState)))
     {
          return false;
     }
     samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
     samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
     samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
     samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
     if (FAILED(mDevice->CreateSamplerState(&samplerDesc, &mLinearWrapSamplerState)))
     {
          return false;
     }
     samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
     if (FAILED(mDevice->CreateSamplerState(&samplerDesc, &mPointWrapSamplerState)))
     {
          return false;
     }
     mDeviceContext->PSSetSamplers(0, 1, &mLinearClampSamplerState);
     mDeviceContext->PSSetSamplers(1, 1, &mLinearWrapSamplerState);
     mDeviceContext->PSSetSamplers(2, 1, &mPointClampSamplerState);
     mDeviceContext->PSSetSamplers(3, 1, &mPointWrapSamplerState);
	return true;
}

void D3D11Renderer::LoadPerFrameConstBuffer(ID3D11ShaderReflection* reflection, const D3D11_SHADER_DESC& desc, ConstBufferBindStage bindStage)
{
	for(int i = 0; i < desc.ConstantBuffers; i++)
	{
		ID3D11ShaderReflectionConstantBuffer *constBuffer = reflection->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC  bufferDesc;
		constBuffer->GetDesc(&bufferDesc);

		D3D11_SHADER_INPUT_BIND_DESC bindDesc;
		reflection->GetResourceBindingDescByName(bufferDesc.Name, &bindDesc);

		if(bindDesc.BindPoint != 0)
		{
			continue;
		}
		
		ConstBuffer constantBuffer = ConstBuffer(bufferDesc.Name, 0, bindDesc.BindPoint, bufferDesc.Size);
		for(int j = 0; j < bufferDesc.Variables; j++)
		{
			ID3D11ShaderReflectionVariable* variable = constBuffer->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC variableDesc;
			variable->GetDesc(&variableDesc);

			Variable constantBufferVariable;
			constantBufferVariable.Offset = variableDesc.StartOffset;
			constantBufferVariable.Size = variableDesc.Size;
			constantBuffer.AddVariable(variableDesc.Name, constantBufferVariable);
		}

		constantBuffer.AddBindStage(bindStage);
		mPerFrame = new ConstBuffer(constantBuffer);
	}
}

void D3D11Renderer::LoadPerDrawConstBuffer(ID3D11ShaderReflection* reflection, const D3D11_SHADER_DESC& desc, ConstBufferBindStage bindStage)
{
	for(int i = 0; i < desc.ConstantBuffers; i++)
	{
		ID3D11ShaderReflectionConstantBuffer *constBuffer = reflection->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC  bufferDesc;
		constBuffer->GetDesc(&bufferDesc);

		D3D11_SHADER_INPUT_BIND_DESC bindDesc;
		reflection->GetResourceBindingDescByName(bufferDesc.Name, &bindDesc);

		if(bindDesc.BindPoint != 1)
		{
			continue;
		}
		
		ConstBuffer constantBuffer = ConstBuffer(bufferDesc.Name, 0, bindDesc.BindPoint, bufferDesc.Size);
		for(int j = 0; j < bufferDesc.Variables; j++)
		{
			ID3D11ShaderReflectionVariable* variable = constBuffer->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC variableDesc;
			variable->GetDesc(&variableDesc);

			Variable constantBufferVariable;
			constantBufferVariable.Offset = variableDesc.StartOffset;
			constantBufferVariable.Size = variableDesc.Size;
			constantBuffer.AddVariable(variableDesc.Name, constantBufferVariable);
		}

		constantBuffer.AddBindStage(bindStage);
		mPerDraw = new ConstBuffer(constantBuffer);
	}
}

static D3D11Format GetFormat(const D3D11_SIGNATURE_PARAMETER_DESC& paramDesc)
{
     D3D11Format format = {};
     if ( paramDesc.Mask == 1 )
     {
     	format.Channels = 1;
          if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) format.Format = DXGI_FORMAT_R32_UINT; 
          else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) format.Format = DXGI_FORMAT_R32_SINT;
          else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) format.Format = DXGI_FORMAT_R32_FLOAT;
     }
     else if ( paramDesc.Mask <= 3 )
     {
     	format.Channels = 2;
          if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) format.Format = DXGI_FORMAT_R32G32_UINT;
          else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) format.Format = DXGI_FORMAT_R32G32_SINT;
          else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) format.Format = DXGI_FORMAT_R32G32_FLOAT;
     }
     else if ( paramDesc.Mask <= 7 )
     {
     	format.Channels = 3;
          if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) format.Format = DXGI_FORMAT_R32G32B32_UINT;
          else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) format.Format = DXGI_FORMAT_R32G32B32_SINT;
          else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) format.Format = DXGI_FORMAT_R32G32B32_FLOAT;
     }
     else if ( paramDesc.Mask <= 15 )
     {
     	format.Channels = 4;
          if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 ) format.Format = DXGI_FORMAT_R32G32B32A32_UINT;
          else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 ) format.Format = DXGI_FORMAT_R32G32B32A32_SINT;
          else if ( paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 ) format.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
     }
     return format;
}

static std::vector<D3D11_INPUT_ELEMENT_DESC> GetInputLayoutDesc(ID3D11ShaderReflection* reflection, const D3D11_SHADER_DESC& desc)
{
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayoutDesc;
	for(int i = 0; i < desc.InputParameters; i++)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
          reflection->GetInputParameterDesc(i, &paramDesc );
          D3D11_INPUT_ELEMENT_DESC elementDesc;
          elementDesc.SemanticName = paramDesc.SemanticName;
          elementDesc.SemanticIndex = paramDesc.SemanticIndex;
          elementDesc.InputSlot = 0;
          elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
          elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
          elementDesc.InstanceDataStepRate = 0;
          elementDesc.Format = GetFormat(paramDesc).Format;
          inputLayoutDesc.push_back(elementDesc);
	}
	return inputLayoutDesc;
}