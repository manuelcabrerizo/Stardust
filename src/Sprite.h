#ifndef SPRITE_H
#define SPRITE_H

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix4x4.h"

struct Sprite
{
	Vector3 Position;
	Vector3 Scale;
	Matrix4x4 Rotation;
	Vector4 Uvs;
};

#endif