#ifndef VECTOR4_H
#define VECTOR4_H

struct Vector2;
struct Vector3;

struct alignas(16) Vector4
{
    union
    {
        struct
        {
            float X;
            float Y;
            float Z;
            float W;
        };
        float V[4];
    };

    static constexpr int NumComponents = 4;

    Vector4() = default;
    explicit Vector4(float value);
    explicit Vector4(float x, float y, float z, float w);
    explicit Vector4(const Vector2& xy, float z, float w);
    explicit Vector4(const Vector3& xyz, float w);
};

#endif