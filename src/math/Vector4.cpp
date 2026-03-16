#include "Vector4.h"
#include "Vector2.h"
#include "Vector3.h"

Vector4::Vector4(float value)
	: X(value), Y(value), Z(value), W(value)
{
}

Vector4::Vector4(float x, float y, float z, float w)
	: X(x), Y(y), Z(z), W(w)
{
}

Vector4::Vector4(const Vector2& xy, float z, float w)
	: X(xy.X), Y(xy.Y), Z(z), W(w)
{
}

Vector4::Vector4(const Vector3& xyz, float w)
	: X(xyz.X), Y(xyz.Y), Z(xyz.Z), W(w)
{
}