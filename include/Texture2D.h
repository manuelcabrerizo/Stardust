#ifndef TEXTURE_2D_H
#define TEXTURE_2D_H

#include "Bindable.h"

class Texture2D : public Bindable
{
public:
	Texture2D(const char* filepath, bool isSRGB);
	~Texture2D();

	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	int GetWidth() const;
	int GetHeight() const;
	int GetChannels() const;
	unsigned char* GetData() const;
	bool IsSRGB() const;
private:
	int mWidth;
	int mHeight;
	int mChannels;
	bool mIsSRGB;
	unsigned char* mData;
};

#endif