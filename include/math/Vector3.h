#ifndef VECTOR3_H
#define VECTOR3_H

#include "Common.h"

struct Vector2;
struct Vector4;

struct SD_API Vector3
{
	union
	{
		struct
		{
			float X;
			float Y;
			float Z;
		};
		float V[3];
	};

    static constexpr int NumComponents = 3;

    static Vector3 ZeroVector;
    static Vector3 OneVector;
    static Vector3 XAxisVector;
    static Vector3 YAxisVector;
    static Vector3 ZAxisVector;

    Vector3() = default;
    explicit Vector3(float value);
    explicit Vector3(float x, float y, float z);
    explicit Vector3(const Vector2& xy, float z);
    explicit Vector3(const Vector4& v);

    float& operator[](int index);
    float operator[](int index) const;

    Vector3 operator+(const Vector3& rhs) const;
    void operator+=(const Vector3& rhs);
    Vector3 operator-(const Vector3& rhs) const;
    void operator-=(const Vector3& rhs);
    Vector3 operator*(float rhs) const;
    void operator*=(float rhs);
    Vector3 operator/(float rhs) const;
    void operator/=(float rhs);

    float Magnitude() const;
    float MagnitudeSq() const;
    void Normalize();
    Vector3 Normalized() const;

    float Dot(const Vector3& vector) const;
    static float Dot(const Vector3& a, const Vector3& b);

    Vector3 Cross(const Vector3& vector) const;
    static Vector3 Cross(const Vector3& a, const Vector3& b);

    Vector3 Lerp(const Vector3& vector, float t) const;
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t);
};

#endif