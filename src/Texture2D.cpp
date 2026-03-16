#include "Texture2D.h"

#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture2D::Texture2D(const char* filepath, bool isSRGB)
{
	stbi_set_flip_vertically_on_load(true);

	int width, height, channels;
	void* data = (void*)stbi_load(filepath, &width, &height, &channels, 4);
	if (!data)
	{
	  assert(!"Error reading texture file");
	}

	mWidth = width;
	mHeight = height;
	mChannels = channels;
	mIsSRGB = isSRGB;

	size_t imageSize = mWidth * mHeight * mChannels;
	mData = new unsigned char[imageSize];
	memcpy(mData, data, imageSize);
	
	stbi_image_free(data);
}

Texture2D::~Texture2D()
{
	Release();
	mWidth = 0;
	mHeight = 0;
	if(mData)
	{
		delete[] mData;
	}
}

int Texture2D::GetWidth() const
{
	return mWidth;
}

int Texture2D::GetHeight() const
{
	return mHeight;
}

int Texture2D::GetChannels() const
{
	return mChannels;
}

unsigned char* Texture2D::GetData() const
{
	return mData;
}

bool Texture2D::IsSRGB() const
{
	return mIsSRGB;
}