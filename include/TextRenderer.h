#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include "Common.h"
#include "Sprite.h"
#include <vector>

class Renderer;
class BatchRenderer;
class Texture2D;

struct FontInfo
{
	int Width;
	int Height;
	int SpriteCount;
	int IsFont;
	int FontSize;
	char ImagePath[256];
};

struct FontSpriteInfo
{
	char NameId[256];
	char Tag[256];
	int OriginX, OriginY;
	int PositionX, PositionY;
	int SourceWidth, SourceHeight;
	int Padding;
	bool Trimmed;
    int TrimRecX, TrimRecY, TrimRecWidth, TrimRecHeight;
    int ColliderType;
    int ColliderPosX, ColliderPosY, ColliderSizeX, ColliderSizeY;
    // Atlas contains font data
    int CharValue;
    int CharOffsetX, CharOffsetY;
    int CharAdvanceX;
};

class SD_API TextRenderer
{
public:
	TextRenderer(const char* fontFile);
	~TextRenderer();
	void Load(Renderer* renderer);
	void Release(Renderer* renderer);
	void DrawString(const char* text, const Vector3& position, float fontSize, const Vector3& color);
	void Present(Renderer* renderer);
private:
	void LoadFontInfo(const char* fontFile);

	Texture2D* mAtlas;
	std::vector<FontSpriteInfo> mSprites;
	FontInfo mInfo;

	BatchRenderer* mBatch;
	std::vector<Sprite> mLetters;
};

#endif