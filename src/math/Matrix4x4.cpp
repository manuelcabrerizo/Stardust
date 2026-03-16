#include "Matrix4x4.h"
#include "Vector3.h"
#include "Vector4.h"
#include <cassert>
#include <cmath>

Matrix4x4 Matrix4x4::Identity = {{
	{1.0f, 0.0f, 0.0f, 0.0f},
	{0.0f, 1.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f, 0.0f},
	{0.0f, 0.0f, 0.0f, 1.0f}
}};

float *Matrix4x4::operator[](int index)
{
	assert(index >= 0 && index < NumRows);
	return M[index];
}

Matrix4x4 Matrix4x4::operator+(const Matrix4x4& m) const
{
	Matrix4x4 matrix;
	for(int row = 0; row < NumRows; row++)
	{
		for(int col = 0; col < NumCols; col++)
		{
			matrix.M[row][col] = M[row][col] + m.M[row][col];
		}
	}
	return matrix;
}

Matrix4x4 Matrix4x4::operator*(float val) const
{
	Matrix4x4 matrix;
	for(int row = 0; row < NumRows; row++)
	{
		for(int col = 0; col < NumCols; col++)
		{
			matrix.M[row][col] = M[row][col] * val;
		}
	}
	return matrix;
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& m) const
{
    Matrix4x4 matrix;
    for(int row = 0; row < NumRows; row++) {
        for(int col = 0; col < NumCols; col++) {
            matrix.M[row][col] =
                M[row][0] * m.M[0][col] +
                M[row][1] * m.M[1][col] +
                M[row][2] * m.M[2][col] +
                M[row][3] * m.M[3][col];
        }
    }
    return matrix;
}

Vector4 Matrix4x4::operator*(const Vector4& vec) const
{
    Vector4 vector;
    for (int col = 0; col < NumCols; col++) {
        vector.V[col] =
            M[0][col] * vec.X +
            M[1][col] * vec.Y +
            M[2][col] * vec.Z +
            M[3][col] * vec.W;
    }
    return vector;
}


Vector3 Matrix4x4::GetTranslation()
{
	return Vector3(M[3][0], M[3][1], M[3][2]);
}

//Quaternion Matrix4x4::GetRotation();
//Vector3 Matrix4x4::GetScale();

Vector3 Matrix4x4::TransformPoint(const Matrix4x4& mat, const Vector3& vec)
{
	return Vector3(mat * Vector4(vec, 1.0f));
}

Vector3 Matrix4x4::TransformVector(const Matrix4x4& mat, const Vector3& vec)
{
	return Vector3(mat * Vector4(vec, 0.0f));
}

Matrix4x4 Matrix4x4::Frustum(float l, float r, float b, float t, float n, float f)
{
    assert(!(l == r || t == b || n == f));
    Matrix4x4 mat = {{
    	{(2*n) /(r-l),           0, -(r+l)/(r-l),            0},
        {           0, (2*n)/(t-b), -(t+b)/(t-b),            0},
        {           0,           0,      f/(f-n), -(f*n)/(f-n)},
        {           0,           0,            1,            0}
    }};
    return Matrix4x4::Transposed(mat);
}

Matrix4x4 Matrix4x4::Perspective(float fov, float aspect, float znear, float zfar)
{
    float ymax = znear * tanf(fov*0.5f);
    float xmax = ymax * aspect;
    return Frustum(-xmax, xmax, -ymax, ymax, znear, zfar);
}

Matrix4x4 Matrix4x4::Ortho(float l, float r, float b, float t, float n, float f)
{
    assert(!(l == r || t == b || n == f));
    Matrix4x4 mat = {{
    	{2.0f / (r - l),              0,              0, -((r + l) / (r - l))},
        {             0, 2.0f / (t - b),              0,   -(t + b) / (t - b)},
        {             0,              0, 1.0f / (f - n),       -(n / (f - n))},
        {             0,              0,              0,                    1}
    }};
    return Matrix4x4::Transposed(mat);
}

Matrix4x4 Matrix4x4::LookAt(const Vector3& position, const Vector3& target, const Vector3& up)
{
    Vector3 zaxis = (target - position).Normalized();
    Vector3 xaxis = up.Cross(zaxis).Normalized();
    Vector3 yaxis = zaxis.Cross(xaxis).Normalized();
    Matrix4x4 mat =  {{
        {xaxis.X, xaxis.Y, xaxis.Z, -xaxis.Dot(position)}, 
        {yaxis.X, yaxis.Y, yaxis.Z, -yaxis.Dot(position)}, 
        {zaxis.X, zaxis.Y, zaxis.Z, -zaxis.Dot(position)},
        {      0,       0,       0,                    1}
	}};
    return Matrix4x4::Transposed(mat);
}

Matrix4x4 Matrix4x4::Translate(float x, float y, float z)
{
	Matrix4x4 mat = {{
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{   x,    y,    z, 1.0f}
	}};
	return mat;
}

Matrix4x4 Matrix4x4::Translate(const Vector3& pos)
{
	Matrix4x4 mat = {{
		{ 1.0f,     0.0f,     0.0f, 0.0f},
		{ 0.0f,     1.0f,     0.0f, 0.0f},
		{ 0.0f,     0.0f,     1.0f, 0.0f},
		{pos.X,    pos.Y,    pos.Z, 1.0f}
	}};
	return mat;
}

Matrix4x4 Matrix4x4::Scale(float x, float y, float z)
{
	Matrix4x4 mat = {{
		{   x, 0.0f, 0.0f, 0.0f},
		{0.0f,    y, 0.0f, 0.0f},
		{0.0f, 0.0f,    z, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f}
	}};
	return mat;
}

Matrix4x4 Matrix4x4::Scale(const Vector3& scale)
{
	Matrix4x4 mat = {{
		{scale.X,    0.0f,    0.0f, 0.0f},
		{   0.0f, scale.Y,    0.0f, 0.0f},
		{   0.0f,    0.0f, scale.Z, 0.0f},
		{   0.0f,    0.0f,    0.0f, 1.0f}
	}};
	return mat;
}

Matrix4x4 Matrix4x4::RotateX(float angle)
{
	Matrix4x4 mat = {{
	    {1, 0, 0, 0},
	    {0, cosf(angle), sinf(angle), 0},
	    {0, -sinf(angle), cosf(angle), 0},
	    {0, 0, 0, 1}
	}};
	return mat;
}

Matrix4x4 Matrix4x4::RotateY(float angle)
{
    Matrix4x4 mat = {{
        {cosf(angle), 0, -sinf(angle), 0},
        {0, 1, 0, 0},
        {sinf(angle), 0, cosf(angle), 0},
        {0, 0, 0, 1}
    }};
    return mat;
}

Matrix4x4 Matrix4x4::RotateZ(float angle)
{
    Matrix4x4 mat = {{
        {cosf(angle), sinf(angle), 0, 0},
        {-sinf(angle), cosf(angle), 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    }};
    return mat;
}

Matrix4x4 Matrix4x4::TransformFromBasis(const Vector3& o, const Vector3& r, const Vector3& u, const Vector3& f)
{
    Matrix4x4 mat = {{
        {r.X, r.Y, r.Z, 0},
        {u.X, u.Y, u.Z, 0},
        {f.X, f.Y, f.Z, 0},
        {o.X, o.Y, o.Z, 1}
    }};
    return mat;
}

Matrix4x4 Matrix4x4::TransformFromEuler(float x, float y, float z)
{
	return RotateZ(z) * RotateX(x) * RotateY(y);
}

Matrix4x4 Matrix4x4::TransformFromEuler(const Vector3& rotation)
{
	return RotateZ(rotation.Z) * RotateX(rotation.X) * RotateY(rotation.Y);
}

Matrix4x4 Matrix4x4::Transposed(const Matrix4x4& m)
{
	Matrix4x4 mat = {{
		{m.M[0][0], m.M[1][0], m.M[2][0], m.M[3][0]},
		{m.M[0][1], m.M[1][1], m.M[2][1], m.M[3][1]},
		{m.M[0][2], m.M[1][2], m.M[2][2], m.M[3][2]},
		{m.M[0][3], m.M[1][3], m.M[2][3], m.M[3][3]}
	}};
	return mat;
}

/*
float Matrix4x4::Determinant(const Matrix4x4& m)
{

}

Matrix4x4 Matrix4x4::Adjugate(const Matrix4x4& m)
{

}

Matrix4x4 Matrix4x4::Inverse(const Matrix4x4& m)
{

}
*/

