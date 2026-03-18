#ifndef MODEL_H
#define MODEL_H

class Renderer;
class VertexBuffer;
class IndexBuffer;
class Texture2D;

class Model
{
public:
	Model(const char* modelFilepath, const char* textureFilePath);
	~Model();
	void Load(Renderer* renderer);
	void Release(Renderer* renderer);
	void Draw(Renderer* renderer);
private:
	VertexBuffer* mVertexBuffer;
	IndexBuffer* mIndexBuffer;
	Texture2D* mTexture;
};

#endif