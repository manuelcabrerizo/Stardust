#include "Quaternion.h" 

#include "Vector3.h"
#include "Matrix4x4.h"

#include <cassert>
#include <cmath>
#include <cfloat>

Quaternion::Quaternion()
    : X(0), Y(0), Z(0), W(1)
{
}

Quaternion::Quaternion(float w, float x, float y, float z)
    : X(x), Y(y), Z(z), W(w)
{
}

Quaternion::Quaternion(const Vector3& v)
    : X(v.X), Y(v.Y), Z(v.Z), W(1)
{
}

float& Quaternion::operator[](int index)
{
    assert(index >= 0 && index < NumComponents);
    return V[index];
}

float Quaternion::operator[](int index) const
{
    assert(index >= 0 && index < NumComponents);
    return V[index];
}

Quaternion Quaternion::operator*(float f) const
{
    return Quaternion(W * f, X * f, Y * f, Z * f);
}

Quaternion Quaternion::operator/(float f) const
{
    return *this * (1.0f/f);
}

Quaternion Quaternion::operator+(const Quaternion &q) const
{
    return Quaternion(W + q.W, X + q.X, Y + q.Y, Z + q.Z);
}

Quaternion Quaternion::operator*(const Quaternion &q) const
{
    return Quaternion(
        W*q.W - X*q.X - Y*q.Y - Z*q.Z,
        W*q.X + X*q.W + Y*q.Z - Z*q.Y,
        W*q.Y + Y*q.W + Z*q.X - X*q.Z,
        W*q.Z + Z*q.W + X*q.Y - Y*q.X
    );
}

Vector3 Quaternion::operator*(const Vector3 &v) const
{
    Quaternion p = Quaternion(0, v.X, v.Y, v.Z);
    p = *this * p * Inverse(*this);
    return Vector3(p.X, p.Y, p.Z);
}

float Quaternion::Magnitude() const
{
    float magnitudeSq = W * W + X * X + Y * Y + Z * Z;
    return sqrtf(magnitudeSq);
}

float Quaternion::MagnitudeSq() const
{
    return W * W + X * X + Y * Y + Z * Z;
}

void Quaternion::Normalize()
{
    float magnitude = sqrtf(W * W + X * X + Y * Y + Z * Z);
    if(magnitude > FLT_EPSILON)
    {
        float invMagnitude = 1.0f / magnitude;
        X *= invMagnitude;
        Y *= invMagnitude;
        Z *= invMagnitude;
        W *= invMagnitude;
    }
}

Quaternion Quaternion::Normalized() const
{
    Quaternion normalized = *this;
    float magnitude = sqrtf(W * W + X * X + Y * Y + Z * Z);
    if(magnitude > FLT_EPSILON)
    {
        float invMagnitude = 1.0f / magnitude;
        normalized.X *= invMagnitude;
        normalized.Y *= invMagnitude;
        normalized.Z *= invMagnitude;
        normalized.W *= invMagnitude;
    }
    return normalized;
}

Matrix4x4 Quaternion::ToMatrix() const
{
    Matrix4x4 result;

    result.M[0][0] = 1 - 2*Y*Y - 2*Z*Z;
    result.M[0][1] = 2 *  (X*Y -   W*Z);
    result.M[0][2] = 2 *  (X*Z +   W*Y);
    result.M[0][3] = 0;

    result.M[1][0] = 2 *  (X*Y +   W*Z);
    result.M[1][1] = 1 - 2*X*X - 2*Z*Z;
    result.M[1][2] = 2 *  (Y*Z -   W*X);
    result.M[1][3] = 0;

    result.M[2][0] = 2 *  (X*Z -   W*Y);
    result.M[2][1] = 2 *  (Y*Z +   W*X);
    result.M[2][2] = 1 - 2*X*X - 2*Y*Y;
    result.M[2][3] = 0;

    result.M[3][0] = 0;
    result.M[3][1] = 0;
    result.M[3][2] = 0;
    result.M[3][3] = 1;

    return Matrix4x4::Transposed(result);
}

Quaternion Quaternion::Slerp(const Quaternion& a, const Quaternion& b, float t)
{
    float w0 = a.W, x0 = a.X, y0 = a.Y, z0 = a.Z;
    float w1 = b.W, x1 = b.X, y1 = b.Y, z1 = b.Z;

    // Compute the cosine of the angle betwiin the quaternions
    // using the dot product
    float cosOmega = w0 * w1 + x0 * x1 + y0 * y1 + z0 * z1;

    // If negative dot, negate one of the input 
    // quaternions, to take the shorter 4D arc
    if (cosOmega < 0.0f)
    {
        w1 = -w1;
        x1 = -x1;
        y1 = -y1;
        z1 = -z1;
        cosOmega = -cosOmega;
    }

    // Check if they are very close together, to protect
    // against divide by zero
    float k0, k1;
    if (cosOmega > 0.9999f)
    {
        // Very close, just use linear interpolation
        k0 = 1.0f - t;
        k1 = t;
    }
    else
    {
        // Compute the sin of the angle using the
        // trig identity sin^2(omega) + cos^2(omega) = 1
        float sinOmega = sqrtf(1.0f - cosOmega * cosOmega);
        // Compute the angle from its sine and cosine
        float omega = atan2f(sinOmega, cosOmega);
        // Compute inverse of denominator, so we only
        // have to divide once
        float oneOverSinOmega = 1.0f / sinOmega;
        // Compute the interpolation parameters
        k0 = sinf((1.0f - t) * omega) * oneOverSinOmega;
        k1 = sinf(t * omega) * oneOverSinOmega;
    }

    // Interpolate
    return Quaternion(
        w0*k0 + w1*k1,
        x0*k0 + x1*k1,
        y0*k0 + y1*k1,
        z0*k0 + z1*k1
    );
}

Quaternion Quaternion::AngleAxis(float angle, const Vector3 &axis)
{
    Vector3 norm = axis.Normalized();
    float s = sinf(angle * 0.5f);
    return  Quaternion(cosf(angle * 0.5f), norm.X * s, norm.Y * s, norm.Z * s);
}

Quaternion Quaternion::Inverse(const Quaternion& q)
{
    // in quaternions that represent rotations the conjugate is
    // the inverse quaternion, so the division by the magnitude can be omited
    float magnitude = q.Magnitude();
    if (magnitude < FLT_EPSILON) {
        return Quaternion();
    }
    float recip = 1.0f / magnitude;
    return Quaternion(q.W * recip , -q.X * recip, -q.Y * recip, -q.Z * recip);
}

Quaternion Quaternion::FromMatrix(const Matrix4x4& m)
{
    float m11 = m.M[0][0], m12 = m.M[1][0], m13 = m.M[2][0];
    float m21 = m.M[0][1], m22 = m.M[1][1], m23 = m.M[2][1];
    float m31 = m.M[0][2], m32 = m.M[1][2], m33 = m.M[2][2];

    // Determine whitch of w, x, y, or z has the largets absolute value
    float fourWSquaredMinus1 = m11 + m22 + m33;
    float fourXSquaredMinus1 = m11 - m22 - m33;
    float fourYSquaredMinus1 = m22 - m11 - m33;
    float fourZSquaredMinus1 = m33 - m11 - m22;

    int biggestIndex = 0;
    float fourBiggestSquaredMinus1 = fourWSquaredMinus1;
    if (fourXSquaredMinus1 > fourBiggestSquaredMinus1)
    {
        fourBiggestSquaredMinus1 = fourXSquaredMinus1;
        biggestIndex = 1;
    }
    if (fourYSquaredMinus1 > fourBiggestSquaredMinus1)
    {
        fourBiggestSquaredMinus1 = fourYSquaredMinus1;
        biggestIndex = 2;
    }
    if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
    {
        fourBiggestSquaredMinus1 = fourZSquaredMinus1;
        biggestIndex = 3;
    }

    // Perform Square root and division
    float biggestVal = sqrtf(fourBiggestSquaredMinus1 + 1.0f) * 0.5f;
    float mult = 0.25f / biggestVal;

    Quaternion q;

    // Apply table to compute quaternion values
    switch (biggestIndex)
    {
    case 0:
        q.W = biggestVal;
        q.X = (m23 - m32) * mult;
        q.Y = (m31 - m13) * mult;
        q.Z = (m12 - m21) * mult;
        break;
    case 1:
        q.X = biggestVal;
        q.W = (m23 - m32) * mult;
        q.Y = (m12 - m21) * mult;
        q.Z = (m31 - m13) * mult;
        break;
    case 2:        
        q.Y = biggestVal;
        q.W = (m31 - m13) * mult;
        q.X = (m12 - m21) * mult;
        q.Z = (m23 - m32) * mult;
        break;
    case 3:
        q.Z = biggestVal;
        q.W = (m12 - m21) * mult;
        q.X = (m31 - m13) * mult;
        q.Y = (m23 - m32) * mult;
        break;
    }

    return q;
}

Quaternion Quaternion::FromEuler(float x, float y, float z)
{
    // TODO: precompute the sin and cos to reduce function calls
    float hh = x/2.0f, hp = y/2.0f, hb = z/2.0f;
    return Quaternion(
        cosf(hh)*cosf(hp)*cosf(hb) + sinf(hh)*sinf(hp)*sinf(hb),
        cosf(hh)*sinf(hp)*cosf(hb) + sinf(hh)*cosf(hp)*sinf(hb),
        sinf(hh)*cosf(hp)*cosf(hb) - cosf(hh)*sinf(hp)*sinf(hb),
        cosf(hh)*cosf(hp)*sinf(hb) - sinf(hh)*sinf(hp)*cosf(hb)
    );
}