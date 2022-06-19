#include "VectorMath.h"
#include <math.h>
#include <string>
#include <stdlib.h>




fvec3 Zero() { return { 0.0f, 0.0f, 0.0f }; }
fvec3 Neg(const fvec3& v) { return { -v.x, -v.y, -v.z }; }
fvec3 Cross(const fvec3& v1, const fvec3& v2) { return { v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x }; }
fvec3 Sub(const fvec3& v1, const fvec3& v2) { return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z }; }
fvec3 Add(const fvec3& v1, const fvec3& v2) { return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z }; }
fvec3 Norm(const fvec3& v) { const float normVal = 1.0f / Len(v); return { v.x * normVal, v.y * normVal, v.z * normVal }; }
float Len(const fvec3& v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
float Dot(const fvec3& v1, const fvec3& v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

fvec4 fvec4::Zero() { return { 0.0f, 0.0f, 0.0f, 0.0f }; }
fvec4 fvec4::Neg(const fvec4& v) { return { -v.x, -v.y, -v.z, 0.0f }; }
fvec4 fvec4::Cross(const fvec4& v1, const fvec4& v2) { return { v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x, 0.0f }; }
fvec4 fvec4::Sub(const fvec4& v1, const fvec4& v2) { return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, 0.0f }; }
fvec4 fvec4::Add(const fvec4& v1, const fvec4& v2) { return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, 0.0f }; }
fvec4 fvec4::Norm(const fvec4& v) { const float normVal = 1.0f / Len(v); return { v.x * normVal, v.y * normVal, v.z * normVal, 0.0f }; }
float fvec4::Len(const fvec4& v) { return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z); }
float fvec4::Dot(const fvec4& v1, const fvec4& v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }



void ScalarSinCos(float* pSin, float* pCos, float value)
{
	static constexpr float PI = 3.14159f;
	static constexpr float PIDIV2 = PI / 2.0f;
	static constexpr float _2PI = 2.0f * PI;
	static constexpr float _1DIV2PI = 1.0f / _2PI;
	float quotient = _1DIV2PI * value;
	if (value > 0.0f)
	{
		quotient = static_cast<float>(static_cast<int>(quotient + 0.5f));
	}
	else
	{
		quotient = static_cast<float>(static_cast<int>(quotient - 0.5f));
	}
	float y = value - _2PI * quotient;
	float sign;
	if (y > PIDIV2)
	{
		y = PI - y;
		sign = -1.0f;
	}
	else if (y < -PIDIV2)
	{
		y = -PI - y;
		sign = -1.0f;
	}
	else
	{
		sign = +1.0f;
	}
	float y2 = y * y;
	// 11-degree minimax approximation
	*pSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;
	// 10-degree minimax approximation
	float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
	*pCos = sign * p;
}

Matrix4x4 Matrix4x4::Identity()
{
	Matrix4x4 ident;
	memset(ident.m, 0, sizeof(Matrix4x4));
	ident.m[0] = 1.0f;
	ident.m[5] = 1.0f;
	ident.m[10] = 1.0f;
	ident.m[15] = 1.0f;
	return ident;
}
Matrix4x4 Matrix4x4::Translation(float x, float y, float z)
{
	Matrix4x4 trans = Identity();
	trans.m[12] = x;
	trans.m[13] = y;
	trans.m[14] = z;
	return trans;
}
Matrix4x4 Matrix4x4::Rotation(float dirX, float dirY, float dirZ, float angle)
{
	Matrix4x4 rotMatrix;
	const float sinAngle = sin(angle);
	const float cosAngle = cos(angle);
	const float cosAngle1Minus = 1.0f - cosAngle;

	const float xDirCacheVal = dirX * cosAngle1Minus;
	rotMatrix.m[0] = dirX * xDirCacheVal + cosAngle;
	rotMatrix.m[4] = dirY * xDirCacheVal + dirZ * sinAngle;
	rotMatrix.m[8] = dirZ * xDirCacheVal - dirY * sinAngle;
	rotMatrix.m[12] = 0.0f;

	const float yDirCacheVal = dirY * cosAngle1Minus;
	rotMatrix.m[1] = dirX * yDirCacheVal - dirZ * sinAngle;
	rotMatrix.m[5] = dirY * yDirCacheVal + cosAngle;
	rotMatrix.m[9] = dirZ * yDirCacheVal + dirX * sinAngle;
	rotMatrix.m[13] = 0.0f;

	const float zDirCacheVal = dirZ * cosAngle1Minus;
	rotMatrix.m[2] = dirX * zDirCacheVal + dirY * sinAngle;
	rotMatrix.m[6] = dirY * zDirCacheVal - dirX * sinAngle;
	rotMatrix.m[10] = dirZ * zDirCacheVal + cosAngle;

	rotMatrix.m[3] = 0.0f;
	rotMatrix.m[7] = 0.0f;
	rotMatrix.m[11] = 0.0f;
	rotMatrix.m[15] = 1.0f;
	return rotMatrix;
}

Matrix4x4 Matrix4x4::Translate(const Matrix4x4& mat, const fvec4& translation)
{
	Matrix4x4 result = mat;
	result.m[12] += translation.x;
	result.m[13] += translation.y;
	result.m[14] += translation.z;
	result.m[15] = 1.0f;
	return result;
}
Matrix4x4 Matrix4x4::Translate(const Matrix4x4& mat, const fvec3& translation)
{
	Matrix4x4 result = mat;
	result.m[12] += translation.x;
	result.m[13] += translation.y;
	result.m[14] += translation.z;
	result.m[15] = 1.0f;
	return result;
}

Matrix4x4 Matrix4x4::Rotate(const Matrix4x4& mat, const fvec3& rotation, float angle)
{
	return Mul(mat, Rotation(rotation.x, rotation.y, rotation.z, angle));
}
Matrix4x4 Matrix4x4::RotateX(const Matrix4x4& mat, float angle)
{
	return Mul(mat, Rotation(1.0f, 0.0f, 0.0f, angle));
}
Matrix4x4 Matrix4x4::RotateY(const Matrix4x4& mat, float angle)
{
	return Mul(mat, Rotation(0.0f, 1.0f, 0.0f, angle));
}
Matrix4x4 Matrix4x4::RotateZ(const Matrix4x4& mat, float angle)
{
	return Mul(mat, Rotation(0.0f, 0.0f, 1.0f, angle));
}

Matrix4x4 Matrix4x4::Scale(const Matrix4x4& mat, float scale)
{
	Matrix4x4 result = mat;
	result.m[0] *= scale;
	result.m[5] *= scale;
	result.m[10] *= scale;
	result.m[15] = 1.0f;
	return result;
}
Matrix4x4 Matrix4x4::Scale(const Matrix4x4& mat, const fvec3& scale)
{
	Matrix4x4 result = mat;
	result.m[0] *= scale.x;
	result.m[5] *= scale.y;
	result.m[10] *= scale.z;
	result.m[15] = 1.0f;
	return result;
}
Matrix4x4 Matrix4x4::Scale(const Matrix4x4& mat, const fvec4& scale)
{
	Matrix4x4 result = mat;
	result.m[0] *= scale.x;
	result.m[5] *= scale.y;
	result.m[10] *= scale.z;
	result.m[15] = 1.0f;
	return result;
}
fvec4 Matrix4x4::Mul(const Matrix4x4& m, const fvec4& v)
{
	fvec4 vResult;
	vResult.x = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w;
	vResult.y = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w;
	vResult.z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w;
	vResult.w = m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w;
	return vResult;
}
Matrix4x4 Matrix4x4::Mul(const Matrix4x4& M1, const Matrix4x4& M2)
{
	Matrix4x4 mResult;
	float x = M1.m[0];
	float y = M1.m[1];
	float z = M1.m[2];
	float w = M1.m[3];
	mResult.m[0] = (M2.m[0] * x) + (M2.m[4] * y) + (M2.m[8] * z) + (M2.m[12] * w);
	mResult.m[1] = (M2.m[1] * x) + (M2.m[5] * y) + (M2.m[9] * z) + (M2.m[13] * w);
	mResult.m[2] = (M2.m[2] * x) + (M2.m[6] * y) + (M2.m[10] * z) + (M2.m[14] * w);
	mResult.m[3] = (M2.m[3] * x) + (M2.m[7] * y) + (M2.m[11] * z) + (M2.m[15] * w);
	x = M1.m[4];
	y = M1.m[5];
	z = M1.m[6];
	w = M1.m[7];
	mResult.m[4] = (M2.m[0] * x) + (M2.m[4] * y) + (M2.m[8] * z) + (M2.m[12] * w);
	mResult.m[5] = (M2.m[1] * x) + (M2.m[5] * y) + (M2.m[9] * z) + (M2.m[13] * w);
	mResult.m[6] = (M2.m[2] * x) + (M2.m[6] * y) + (M2.m[10] * z) + (M2.m[14] * w);
	mResult.m[7] = (M2.m[3] * x) + (M2.m[7] * y) + (M2.m[11] * z) + (M2.m[15] * w);
	x = M1.m[8];
	y = M1.m[9];
	z = M1.m[10];
	w = M1.m[11];
	mResult.m[8] = (M2.m[0] * x) + (M2.m[4] * y) + (M2.m[8] * z) + (M2.m[12] * w);
	mResult.m[9] = (M2.m[1] * x) + (M2.m[5] * y) + (M2.m[9] * z) + (M2.m[13] * w);
	mResult.m[10] = (M2.m[2] * x) + (M2.m[6] * y) + (M2.m[10] * z) + (M2.m[14] * w);
	mResult.m[11] = (M2.m[3] * x) + (M2.m[7] * y) + (M2.m[11] * z) + (M2.m[15] * w);
	x = M1.m[12];
	y = M1.m[13];
	z = M1.m[14];
	w = M1.m[15];
	mResult.m[12] = (M2.m[0] * x) + (M2.m[4] * y) + (M2.m[8] * z) + (M2.m[12] * w);
	mResult.m[13] = (M2.m[1] * x) + (M2.m[5] * y) + (M2.m[9] * z) + (M2.m[13] * w);
	mResult.m[14] = (M2.m[2] * x) + (M2.m[6] * y) + (M2.m[10] * z) + (M2.m[14] * w);
	mResult.m[15] = (M2.m[3] * x) + (M2.m[7] * y) + (M2.m[11] * z) + (M2.m[15] * w);
	return mResult;
}

float Matrix4x4::Determinant(const Matrix4x4& mat)
{
	const float* m = mat.m;
	return m[3] * m[6] * m[9] * m[12] - m[2] * m[7] * m[9] * m[12] - m[3] * m[5] * m[10] * m[12] + m[1] * m[7] * m[10] * m[12] + m[2] * m[5] * m[11] * m[12] - m[1] * m[6] * m[11] * m[12] - m[3] * m[6] * m[8] * m[13] + m[2] * m[7] * m[8] * m[13] + m[3] * m[4] * m[10] * m[13]
		- m[0] * m[7] * m[10] * m[13] - m[2] * m[4] * m[11] * m[13] + m[0] * m[6] * m[11] * m[13] + m[3] * m[5] * m[8] * m[14] - m[1] * m[7] * m[8] * m[14] - m[3] * m[4] * m[9] * m[14] + m[0] * m[7] * m[9] * m[14] + m[1] * m[4] * m[11] * m[14] - m[0] * m[5] * m[11] * m[14]
		- m[2] * m[5] * m[8] * m[15] + m[1] * m[6] * m[8] * m[15] + m[2] * m[4] * m[9] * m[15] - m[0] * m[6] * m[9] * m[15] - m[1] * m[4] * m[10] * m[15] + m[0] * m[5] * m[10] * m[15];
}
// CALCULATES THE INVERSE BY FOLLOWING FORMULA: adjugate(mat) * det(mat)^-1 = mat^-1
Matrix4x4 Matrix4x4::Inverse(const Matrix4x4& mat)
{
	Matrix4x4 invMatrix;
	float* inv = invMatrix.m;
	float det;
	const float* m = mat.m;

	inv[0] = m[5] * m[10] * m[15] -
		m[5] * m[11] * m[14] -
		m[9] * m[6] * m[15] +
		m[9] * m[7] * m[14] +
		m[13] * m[6] * m[11] -
		m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	if (det == 0)
	{
		return invMatrix;
	}
	det = 1.0 / det;

	for (int i = 0; i < 16; i++)
	{
		inv[i] = inv[i] * det;

	}
	return invMatrix;
}

Matrix4x4 Matrix4x4::LookAtLH(const fvec4& eyePosition, const fvec4& focusPosition, const fvec4& upDirection)
{
	fvec4 EyeDirection = fvec4::Sub(focusPosition, eyePosition);
	fvec4 R2 = fvec4::Norm(EyeDirection);
	fvec4 R0 = fvec4::Cross(upDirection, R2);
	R0 = fvec4::Norm(R0);
	fvec4 R1 = fvec4::Cross(R2, R0);
	fvec4 NegEyePosition = fvec4::Neg(eyePosition);
	R0.w = fvec4::Dot(R0, NegEyePosition);
	R1.w = fvec4::Dot(R1, NegEyePosition);
	R2.w = fvec4::Dot(R2, NegEyePosition);
	Matrix4x4 M;
	M.m[0] = R0.x; M.m[4] = R0.y; M.m[8] = R0.z; M.m[12] = R0.w;
	M.m[1] = R1.x; M.m[5] = R1.y; M.m[9] = R1.z; M.m[13] = R1.w;
	M.m[2] = R2.x; M.m[6] = R2.y; M.m[10] = R2.z; M.m[14] = R2.w;
	M.m[3] = 0.0f; M.m[7] = 0.0f; M.m[11] = 0.0f; M.m[15] = 1.0f;
	return M;
}
Matrix4x4 Matrix4x4::PerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ)
{
	float    SinFov;
	float    CosFov;
	ScalarSinCos(&SinFov, &CosFov, 0.5f * fovAngleY);

	float Height = CosFov / SinFov;
	float Width = Height / aspectRatio;
	float fRange = farZ / (farZ - nearZ);

	Matrix4x4 M;
	M.m[0] = Width;
	M.m[1] = 0.0f;
	M.m[2] = 0.0f;
	M.m[3] = 0.0f;

	M.m[4] = 0.0f;
	M.m[5] = Height;
	M.m[6] = 0.0f;
	M.m[7] = 0.0f;

	M.m[8] = 0.0f;
	M.m[9] = 0.0f;
	M.m[10] = fRange;
	M.m[11] = 1.0f;

	M.m[12] = 0.0f;
	M.m[13] = 0.0f;
	M.m[14] = -fRange * nearZ;
	M.m[15] = 0.0f;
	return M;
}
Matrix4x4 Matrix4x4::Ortho(float left, float right, float bottom, float top, float nearDepth, float farDepth)
{
	float r_l = (right - left);
	float t_b = (top - bottom);
	float f_n = (nearDepth - farDepth);
	Matrix4x4 M;
	M.m[0] = 2.0f / r_l;
	M.m[1] = 0.0f;
	M.m[2] = 0.0f;
	M.m[3] = 0.0f;
	M.m[4] = 0.0f;
	M.m[5] = 2.0f / t_b;
	M.m[6] = 0.0f;
	M.m[7] = 0.0f;
	M.m[8] = 0.0f;
	M.m[9] = 0.0f;
	M.m[10] = -1.0f / f_n;
	M.m[11] = 0.0f;
	M.m[12] = -(left + right) / r_l;
	M.m[13] = -(top + bottom) / t_b;
	M.m[14] = nearDepth / f_n;
	M.m[15] = 1.0f;
	return M;
}
Matrix4x4 Matrix4x4::OrthoLH(float left, float right, float top, float bottom, float nearDepth, float farDepth)
{
	float width = right - left;
	float height = bottom - top;
	float depth = farDepth - nearDepth;
	Matrix4x4 M;
	M.m[0] = 2.0f / width;
	M.m[1] = 0.0f;
	M.m[2] = 0.0f;
	M.m[3] = 0.0f;
	M.m[4] = 0.0f;
	M.m[5] = 2.0f / height;
	M.m[6] = 0.0f;
	M.m[7] = 0.0f;
	M.m[8] = 0.0f;
	M.m[9] = 0.0f;
	M.m[10] = 1.0f / depth;
	M.m[11] = 0.0f;
	M.m[12] = 0.0f;
	M.m[13] = 0.0f;
	M.m[14] = -nearDepth / depth;
	M.m[15] = 1.0f;
	return M;
}




void Camera3D::Update()
{
	// update here
	SetMatrix();
}
void Camera3D::SetDefault()
{
	camPos = { 0.0f, 3.0, 2.0f };
	camTarget = { 0.0f, 0.0f, 1.0f };
	camUp = { 0, 1.0f, 0.0f };
	yaw = 3.14159f;
	SetMatrix();
}
void Camera3D::SetMatrix()
{
	camTarget = YawPitchRollToLookVec(yaw, pitch, 0.0f);
	fvec4 eyePosVec = { camPos.x, camPos.y, camPos.z, 0.0f };
	fvec4 lookAtVec = { camPos.x + camTarget.x, camPos.y + camTarget.y, camPos.z + camTarget.z, 0.0f };
	fvec4 upVec = { camUp.x, camUp.y, camUp.z, 0.0f };

	Matrix4x4 camViewMatrix = Matrix4x4::LookAtLH(eyePosVec, lookAtVec, upVec);
	Matrix4x4 projMatrix = Matrix4x4::PerspectiveFovLH((90.0f / 180.0f) * 3.14159f, 16.0f / 9.0f, 0.1f, 500.0f);
	mat = Matrix4x4::Mul(camViewMatrix, projMatrix);
}

fvec3 Camera3D::YawPitchRollToLookVec(float yaw, float pitch, float roll)
{
	return { cosf(pitch) * sinf(yaw),sinf(pitch),cosf(pitch) * cosf(yaw) };
}
Matrix4x4 Camera3D::GetLightMatrix(const fvec4& lightDir, fvec4* centerPoint)
{
	const Matrix4x4 inv = Matrix4x4::Inverse(mat);
	static constexpr float multVal = 1.0f / 8.0f;
	fvec4 center; fvec2 rangeX, rangeY, rangeZ;
	fvec4 pts[8];
	center = { 0.0f, 0.0f, 0.0f, 0.0f };
	rangeX = { 10000.0f, -10000.0f };
	rangeY = { 10000.0f, -10000.0f };
	rangeZ = { 10000.0f, -10000.0f };
	for (uint32_t x = 0; x < 2; x++)
	{
		for (uint32_t y = 0; y < 2; y++)
		{
			for (uint32_t z = 0; z < 2; z++)
			{
				auto& pt = pts[x + y * 2 + z * 4];
				pt = Matrix4x4::Mul(inv, fvec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 1.999f * z - 1.0f, 1.0f));
				const float w = 1.0f / pt.w;
				pt.x *= w; pt.y *= w; pt.z *= w; pt.w = 1.0f;
				center.x += pt.x; center.y += pt.y; center.z += pt.z;
			}
		}
	}
	center.x *= multVal; center.y *= multVal; center.z *= multVal; center.w = 0.0f;

	Matrix4x4 lightView = Matrix4x4::LookAtLH({ center.x + lightDir.x, center.y + lightDir.y, center.z + lightDir.z, 0.0f }, center, { 0.0f, 1.0f, 0.0f, 0.0f });
	for (uint32_t i = 0; i < 8; i++)
	{
		fvec4 pt = Matrix4x4::Mul(lightView, pts[i]);

		rangeX.x = std::min(rangeX.x, pt.x);
		rangeX.y = std::max(rangeX.y, pt.x);
		rangeY.x = std::min(rangeY.x, pt.y);
		rangeY.y = std::max(rangeY.y, pt.y);
		rangeZ.x = std::min(rangeZ.x, pt.z);
		rangeZ.y = std::max(rangeZ.y, pt.z);
	}

	constexpr float zMult = 10.0f;

	if (rangeZ.x < 0.0f)
	{
		rangeZ.x *= zMult;
	}
	else {
		rangeZ.x /= zMult;
	}
	if (rangeZ.y < 0.0f)
	{
		rangeZ.y /= zMult;
	}
	else {
		rangeZ.y *= zMult;
	}

	if (centerPoint)
	{
		centerPoint->x = center.x;
		centerPoint->y = center.y;
		centerPoint->z = center.z;
		centerPoint->w = center.w;
	}
	Matrix4x4 lightProj = Matrix4x4::OrthoLH(rangeX.x, rangeX.y, rangeY.x, rangeY.y, rangeZ.x, rangeZ.y);
	return Matrix4x4::Mul(lightView, lightProj);
}