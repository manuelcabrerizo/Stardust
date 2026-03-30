#ifndef MODEL_H
#define MODEL_H

#include "Common.h"
#include "math/Vector3.h"
#include "math/Vector2.h"
#include "math/Matrix4x4.h"

class Renderer;
class VertexBuffer;
class IndexBuffer;
class Texture2D;
class Skeleton;

#define MAX_BONE_INFLUENCE 4
struct ModelVertex
{
	Vector3 Position;
	Vector3 Normal;
	Vector2 TCoord;
	int BoneId[MAX_BONE_INFLUENCE];
	float Weights[MAX_BONE_INFLUENCE];
};

class SD_API Model
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