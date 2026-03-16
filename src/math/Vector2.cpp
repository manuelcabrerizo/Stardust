#include "Vector2.h"
#include <cassert>
#include <cmath>
#include <cfloat>

Vector2 Vector2::ZeroVector = Vector2(0.0f, 0.0f);
Vector2 Vector2::OneVector = Vector2(1.0f, 1.0f);
Vector2 Vector2::XAxisVector = Vector2(1.0f, 0.0f);
Vector2 Vector2::YAxisVector = Vector2(0.0f, 1.0f);

Vector2::Vector2(float value)
	: X(value), Y(value)
{
}

Vector2::Vector2(float x, float y)
	: X(x), Y(y)
{
}


float& Vector2::operator[](int index)
{
	assert(index >= 0 & index < NumComponents);
	return V[index];
}

float Vector2::operator[](int index) const
{
	assert(index >= 0 & index < NumComponents);
	return V[index];
}

Vector2 Vector2::operator+(const Vector2& rhs) const
{
	return Vector2(X + rhs.X, Y + rhs.Y);
}

void Vector2::operator+=(const Vector2& rhs)
{
	X += rhs.X;
	Y += rhs.Y;
}

Vector2 Vector2::operator-(const Vector2& rhs) const
{
	return Vector2(X - rhs.X, Y - rhs.Y);
}

void Vector2::operator-=(const Vector2& rhs)
{
	X -= rhs.X;
	Y -= rhs.Y;
}

Vector2 Vector2::operator*(float rhs) const
{
	return Vector2(X * rhs, Y * rhs);
}

void Vector2::operator*=(float rhs)
{
	X *= rhs;
	Y *= rhs;
}

Vector2 Vector2::operator/(float rhs) const
{
	return Vector2(X / rhs, Y / rhs);
}

void Vector2::operator/=(float rhs)
{
	X /= rhs;
	Y /= rhs;
}

float Vector2::Magnitude() const
{
	return sqrtf(X * X + Y * Y);
}

float Vector2::MagnitudeSq() const
{
	return X * X + Y * Y;
}

void Vector2::Normalize()
{
	float magnitude = sqrtf(X * X + Y * Y);
	if(magnitude > FLT_EPSILON)
	{
		float invMagnitude = 1.0f / magnitude;
		X *= invMagnitude;
		Y *= invMagnitude;
	}
}

Vector2 Vector2::Normalized() const
{
	Vector2 normalized = *this;
	float magnitude = sqrtf(X * X + Y * Y);
	if(magnitude > FLT_EPSILON)
	{
		float invMagnitude = 1.0f / magnitude;
		normalized.X *= invMagnitude;
		normalized.Y *= invMagnitude;
	}
	return normalized;
}

float Vector2::Dot(const Vector2& vector) const
{
	return X * vector.X + Y * vector.Y;
}

float Vector2::Dot(const Vector2& a, const Vector2& b)
{
	return a.X * b.X + a.Y * b.Y;
}

Vector2 Vector2::Lerp(const Vector2& vector, float t) const
{
	return *this*(1.0f - t) + vector * t;
}

Vector2 Vector2::Lerp(const Vector2& a, const Vector2& b, float t)
{
	return a * (1.0f - t) + b * t;
}