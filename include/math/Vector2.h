#ifndef VECTOR2_H
#define VECTOR2_H

#include "Common.h"

struct SD_API Vector2
{
    union
    {
        struct
        {
            float X;
            float Y;
        };
        float V[2];
    };

    static constexpr int NumComponents = 2;

    static Vector2 ZeroVector;
    static Vector2 OneVector;
    static Vector2 XAxisVector;
    static Vector2 YAxisVector;

    static inline Vector2 Zero() { return ZeroVector; }
    static inline Vector2 One() { return OneVector; }
    static inline Vector2 UnitX() { return XAxisVector; }
    static inline Vector2 UnitY() { return YAxisVector; }

    Vector2() = default;
    explicit Vector2(float value);
    explicit Vector2(float x, float y);

    float& operator[](int index);
    float operator[](int index) const;

    Vector2 operator+(const Vector2& rhs) const;
    void operator+=(const Vector2& rhs);
    Vector2 operator-(const Vector2& rhs) const;
    void operator-=(const Vector2& rhs);
    Vector2 operator*(float rhs) const;
    void operator*=(float rhs);
    Vector2 operator/(float rhs) const;
    void operator/=(float rhs);

    float Magnitude() const;
    float MagnitudeSq() const;
    void Normalize();
    Vector2 Normalized() const;

    float Dot(const Vector2& vector) const;
    static float Dot(const Vector2& a, const Vector2& b);

    Vector2 Lerp(const Vector2& vector, float t) const;
    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t);
};

#endif