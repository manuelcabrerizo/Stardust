#ifndef QUATERNION_H
#define QUATERNION_H

#include "Common.h"

struct Vector3;
struct Matrix4x4;

struct SD_API Quaternion
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

    Quaternion();
    Quaternion(float w, float x, float y, float z);
    Quaternion(const Vector3& v);
    
    float& operator[](int index);
    float operator[](int index) const;
    Quaternion operator*(float f) const;
    Quaternion operator/(float f) const;
    Quaternion operator+(const Quaternion &q) const;
    Quaternion operator*(const Quaternion &q) const;
    Vector3 operator*(const Vector3 &v) const;

    float Magnitude() const;
    float MagnitudeSq() const;

    void Normalize();
    Quaternion Normalized() const;
    Matrix4x4 ToMatrix() const;
    
    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t);
    static Quaternion AngleAxis(float angle, const Vector3 &axis);
    static Quaternion Inverse(const Quaternion& q);
    static Quaternion FromMatrix(const Matrix4x4& m);
    static Quaternion FromEuler(float x, float y, float z);
};

#endif