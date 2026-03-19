#ifndef MATRIX_4X4_H
#define MATRIX_4X4_H

struct Vector3;
struct Vector4;

struct alignas(16) Matrix4x4
{
	alignas(16) float M[4][4];

    static constexpr int NumRows = 4;
    static constexpr int NumCols = 4;

	static Matrix4x4 Identity;

	Matrix4x4() = default;
	float *operator[](int index);

	Matrix4x4 operator+(const Matrix4x4& m) const;
	Matrix4x4 operator*(float val) const;
	Matrix4x4 operator*(const Matrix4x4& m) const;
	Vector4 operator*(const Vector4& vec) const;

	Vector3 GetTranslation();
	//Quaternion GetRotation();
	//Vector3 GetScale();

	static Vector3 TransformPoint(const Matrix4x4& mat, const Vector3& vec);
	static Vector3 TransformVector(const Matrix4x4& mat, const Vector3& vec);
	static Matrix4x4 Frustum(float l, float r, float b, float t, float n, float f);
	static Matrix4x4 Perspective(float fov, float aspect, float znear, float zfar);
	static Matrix4x4 Orthographic(float l, float r, float b, float t, float n, float f);
	static Matrix4x4 LookAt(const Vector3& position, const Vector3& target, const Vector3& up);
	static Matrix4x4 Translate(float x, float y, float z);
	static Matrix4x4 Translate(const Vector3& pos);
	static Matrix4x4 Scale(const Vector3& scale);
	static Matrix4x4 Scale(float x, float y, float z);
	static Matrix4x4 RotateX(float angle);
	static Matrix4x4 RotateY(float angle);
	static Matrix4x4 RotateZ(float angle);
	static Matrix4x4 TransformFromBasis(const Vector3& o, const Vector3& r, const Vector3& u, const Vector3& f);
	static Matrix4x4 TransformFromEuler(float x, float y, float z);
	static Matrix4x4 TransformFromEuler(const Vector3& rotation);
	static Matrix4x4 Transposed(const Matrix4x4& m);
	//static float Determinant(const Matrix4x4& m);
	//static Matrix4x4 Adjugate(const Matrix4x4& m);
	//static Matrix4x4 Inverse(const Matrix4x4& m);
};

#endif