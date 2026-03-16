#include "Vector3.h"
#include "Vector2.h"
#include "Vector4.h"
#include <cassert>
#include <cmath>
#include <cfloat>

Vector3 Vector3::ZeroVector = Vector3(0.0f, 0.0f, 0.0f);
Vector3 Vector3::OneVector = Vector3(1.0f, 1.0f, 1.0f);
Vector3 Vector3::XAxisVector = Vector3(1.0f, 0.0f, 0.0f);
Vector3 Vector3::YAxisVector = Vector3(0.0f, 1.0f, 0.0f);
Vector3 Vector3::ZAxisVector = Vector3(0.0f, 0.0f, 1.0f);

Vector3::Vector3(float value)
	: X(value), Y(value), Z(value)
{
}

Vector3::Vector3(float x, float y, float z)
	: X(x), Y(y), Z(z)
{
}

Vector3::Vector3(const Vector2& xy, float z)
	: X(xy.X), Y(xy.Y), Z(z)
{	
}

Vector3::Vector3(const Vector4& v)
	: X(v.X), Y(v.Y), Z(v.Z)
{	
}

float& Vector3::operator[](int index)
{
	assert(index >= 0 & index < NumComponents);
	return V[index];
}

float Vector3::operator[](int index) const
{
	assert(index >= 0 & index < NumComponents);
	return V[index];
}

Vector3 Vector3::operator+(const Vector3& rhs) const
{
	return Vector3(X + rhs.X, Y + rhs.Y, Z + rhs.Z);
}

void Vector3::operator+=(const Vector3& rhs)
{
	X += rhs.X;
	Y += rhs.Y;
	Z += rhs.Z;
}

Vector3 Vector3::operator-(const Vector3& rhs) const
{
	return Vector3(X - rhs.X, Y - rhs.Y, Z - rhs.Z);
}

void Vector3::operator-=(const Vector3& rhs)
{
	X -= rhs.X;
	Y -= rhs.Y;
	Z -= rhs.Z;
}

Vector3 Vector3::operator*(float rhs) const
{
	return Vector3(X * rhs, Y * rhs, Z * rhs);
}

void Vector3::operator*=(float rhs)
{
	X *= rhs;
	Y *= rhs;
	Z *= rhs;
}

Vector3 Vector3::operator/(float rhs) const
{
	return Vector3(X / rhs, Y / rhs, Z / rhs);
}

void Vector3::operator/=(float rhs)
{
	X /= rhs;
	Y /= rhs;
	Z /= rhs;
}

float Vector3::Magnitude() const
{
	return sqrtf(X * X + Y * Y + Z * Z);
}

float Vector3::MagnitudeSq() const
{
	return X * X + Y * Y + Z * Z;
}

void Vector3::Normalize()
{
	float magnitude = sqrtf(X * X + Y * Y + Z * Z);
	if(magnitude > FLT_EPSILON)
	{
		float invMagnitude = 1.0f / magnitude;
		X *= invMagnitude;
		Y *= invMagnitude;
		Z *= invMagnitude;
	}
}

Vector3 Vector3::Normalized() const
{
	Vector3 normalized = *this;
	float magnitude = sqrtf(X * X + Y * Y + Z * Z);
	if(magnitude > FLT_EPSILON)
	{
		float invMagnitude = 1.0f / magnitude;
		normalized.X *= invMagnitude;
		normalized.Y *= invMagnitude;
		normalized.Z *= invMagnitude;
	}
	return normalized;
}

float Vector3::Dot(const Vector3& vector) const
{
	return X * vector.X + Y * vector.Y + Z * vector.Z;
}

float Vector3::Dot(const Vector3& a, const Vector3& b)
{
	return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}

Vector3 Vector3::Cross(const Vector3& vector) const
{    
	return Vector3(
        Y * vector.Z - Z * vector.Y, 
        Z * vector.X - X * vector.Z, 
        X * vector.Y - Y * vector.X 
    );
}

Vector3 Vector3::Cross(const Vector3& a, const Vector3& b)
{
	return Vector3(
        a.Y * b.Z - a.Z * b.Y, 
        a.Z * b.X - a.X * b.Z, 
        a.X * b.Y - a.Y * b.X 
    );
}

Vector3 Vector3::Lerp(const Vector3& vector, float t) const
{
	return *this*(1.0f - t) + vector * t;
}

Vector3 Vector3::Lerp(const Vector3& a, const Vector3& b, float t)
{
	return a * (1.0f - t) + b * t;
}