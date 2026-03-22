#include "GraphicPipeline.h"

#include <string>
#include <fstream>
#include <cassert>

GraphicPipeline::GraphicPipeline(const char* vertexFileName, const char* pixelFileName)
{
#if SD_D3D11
	std::string extension = ".dxbc";
#elif SD_VULKAN
	std::string extension = ".spv";
#endif

	std::string vertexFilePath(vertexFileName);
	vertexFilePath.append(extension);
	std::ifstream vertexFile(vertexFilePath, std::ios::ate | std::ios::binary);
	if(!vertexFile.is_open())
	{
		assert(!"Error loading shader file!");
	}
	mVertexShaderSize = vertexFile.tellg();
	mVertexShaderData = new char[mVertexShaderSize];
	vertexFile.seekg(0, std::ios::beg);
	vertexFile.read(mVertexShaderData, static_cast<std::streamsize>(mVertexShaderSize));
	vertexFile.close();

	std::string pixelFilePath(pixelFileName);
	pixelFilePath.append(extension);
	std::ifstream pixelFile(pixelFilePath, std::ios::ate | std::ios::binary);
	if(!pixelFile.is_open())
	{
		assert(!"Error loading shader file!");
	}
	mPixelShaderSize = pixelFile.tellg();
	mPixelShaderData = new char[mPixelShaderSize];
	pixelFile.seekg(0, std::ios::beg);
	pixelFile.read(mPixelShaderData, static_cast<std::streamsize>(mPixelShaderSize));
	pixelFile.close();
}

GraphicPipeline::~GraphicPipeline()
{
	Release();
	mVertexShaderSize = 0;
	mPixelShaderSize = 0;
	delete[] mVertexShaderData;
	delete[] mPixelShaderData;
}

const char* GraphicPipeline::GetVertexShaderData() const
{
	return mVertexShaderData;
}

const size_t GraphicPipeline::GetVertexShaderSize() const
{
	return mVertexShaderSize;
}

const char* GraphicPipeline::GetPixelShaderData() const
{
	return mPixelShaderData;
}

const size_t GraphicPipeline::GetPixelShaderSize() const
{
	return mPixelShaderSize;
}