#include "TextRenderer.h"
#include "BatchRenderer.h"
#include "Texture2D.h"

#include "Sprite.h"

#include <fstream>
#include <cassert>
#include <string>
#include <sstream>

const int INITIAL_SPRITE_COUNT = 100;

TextRenderer::TextRenderer(const char* fontFile)
{
	LoadFontInfo(fontFile);
	std::string fontFilePath(fontFile);
	size_t prevIndex = fontFilePath.find_last_of('/');
	std::string atlasFilePath = fontFilePath.substr(0, prevIndex + 1) + mInfo.ImagePath;
	mAtlas = new Texture2D(atlasFilePath.c_str(), false);
}

TextRenderer::~TextRenderer()
{
	mSprites.clear();
	delete mAtlas;
	if(mBatch)
	{
		delete mBatch;
	}
}

void TextRenderer::Load(Renderer* renderer)
{
	mBatch = BatchRenderer::Create(renderer, INITIAL_SPRITE_COUNT);
	renderer->LoadTexture2D(mAtlas);
}

void TextRenderer::Release(Renderer* renderer)
{
	renderer->ReleaseTexture2D(mAtlas);
	delete mBatch;
}

void TextRenderer::DrawString(const char* text, const Vector3& position, float size, const Vector3& color)
{
	float cursorX = 0;
	for (int i = 0; i < strlen(text); i++)
	{
		char letter = text[i];
		assert(letter >= 32);
		FontSpriteInfo& sprite = mSprites[letter - 32];

		float startX = sprite.PositionX;
		float startY = sprite.PositionY;
		float endX =  sprite.PositionX + sprite.SourceWidth;
		float endY = sprite.PositionY + sprite.SourceHeight;
		startX /= static_cast<float>(mInfo.Width);
		startY /= static_cast<float>(mInfo.Height);
		endX /= static_cast<float>(mInfo.Width);
		endY /= static_cast<float>(mInfo.Height);

		float baseY = (sprite.SourceHeight / 2) * size;
		float baseX = (sprite.SourceWidth / 2) * size;

		float offsetY = ((mInfo.FontSize - sprite.SourceHeight) - sprite.CharOffsetY) * size;
		float offsetX = (sprite.CharOffsetX) * size;

		Sprite letterSprite;
		letterSprite.Position = Vector3(baseX + position.X + cursorX + offsetX, baseY + position.Y + offsetY, 0);
		letterSprite.Scale = Vector3(sprite.SourceWidth * size, sprite.SourceHeight * size, 1);
		letterSprite.Rotation = Matrix4x4::Identity;
		letterSprite.Uvs = Vector4(endX, 1-endY, startX, 1-startY);
		letterSprite.Color = color;
		mLetters.push_back(letterSprite);

		cursorX += static_cast<float>(sprite.CharAdvanceX) * size;
	}
}

void TextRenderer::Present(Renderer* renderer)
{
	renderer->PushTexture(mAtlas, 0);
	mBatch->DrawSprites(mLetters.data(), mLetters.size());
	mLetters.clear();
}

void TextRenderer::LoadFontInfo(const char* fontFile)
{
	mSprites.clear();
	std::ifstream file(fontFile);
	if(!file.is_open())
	{
		assert(!"Error loading shader file!");
	}

	std::string line;
	while(std::getline(file, line))
	{
		char id;
	    std::istringstream stream(line);
	    stream >> id;
		switch(id)
		{
			case 'a':
			{
				memset(&mInfo, 0, sizeof(FontInfo));
				stream >> mInfo.ImagePath >> mInfo.Width >> mInfo.Height >> mInfo.SpriteCount >> mInfo.IsFont >> mInfo.FontSize;
			} break;
			case 's':
			{
				FontSpriteInfo spriteInfo;
				memset(&spriteInfo, 0, sizeof(FontSpriteInfo));
      			stream >> spriteInfo.NameId >> spriteInfo.Tag >> spriteInfo.OriginX >> spriteInfo.OriginY;
                stream >> spriteInfo.PositionX >> spriteInfo.PositionY >> spriteInfo.SourceWidth >> spriteInfo.SourceHeight;
                stream >> spriteInfo.Padding >> spriteInfo.Trimmed;
                stream >> spriteInfo.TrimRecX >> spriteInfo.TrimRecY >> spriteInfo.TrimRecWidth >> spriteInfo.TrimRecHeight;
                stream >> spriteInfo.ColliderType >> spriteInfo.ColliderPosX >> spriteInfo.ColliderPosY;
                stream >> spriteInfo.ColliderSizeX >> spriteInfo.ColliderSizeY;
                stream >> spriteInfo.CharValue >> spriteInfo.CharOffsetX >> spriteInfo.CharOffsetY >> spriteInfo.CharAdvanceX;
                mSprites.push_back(spriteInfo);
			} break;
		}
	}
}