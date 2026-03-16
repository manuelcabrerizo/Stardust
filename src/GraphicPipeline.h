#ifndef GRAPHIC_PIPELINE_H
#define GRAPHIC_PIPELINE_H

#include "Bindable.h"

class GraphicPipeline : public Bindable
{
public:
	GraphicPipeline(const char* vertexFilePath, const char* pixelFilePath);
	~GraphicPipeline();
	const char* GetVertexShaderData() const;
	const size_t GetVertexShaderSize() const;
	const char* GetPixelShaderData() const;
	const size_t GetPixelShaderSize() const;
private:
	char* mVertexShaderData;
	size_t mVertexShaderSize;
	char* mPixelShaderData;
	size_t mPixelShaderSize;
};

#endif