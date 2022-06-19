#pragma once


struct fvec2 { float x, y; };
struct fvec3 { float x, y, z; };
struct ivec2 { int x, y; };
struct ivec3 { int x, y, yz; };




void ScalarSinCos(float* pSin, float* pCos, float value);

fvec3 Zero();
fvec3 Neg(const fvec3& v);
fvec3 Cross(const fvec3& v1, const fvec3& v2);
fvec3 Sub(const fvec3& v1, const fvec3& v2);
fvec3 Add(const fvec3& v1, const fvec3& v2);
fvec3 Norm(const fvec3& v);
float Len(const fvec3& v);
float Dot(const fvec3& v1, const fvec3& v2);
struct fvec4
{
	fvec4() { }
	constexpr fvec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) { }
	static fvec4 Zero();
	static fvec4 Neg(const fvec4& v);
	static fvec4 Cross(const fvec4& v1, const fvec4& v2);
	static fvec4 Sub(const fvec4& v1, const fvec4& v2);
	static fvec4 Add(const fvec4& v1, const fvec4& v2);
	static fvec4 Norm(const fvec4& v);
	static float Len(const fvec4& v);
	static float Dot(const fvec4& v1, const fvec4& v2);
	float x, y, z, w;
};
struct Matrix4x4
{
	Matrix4x4() { }
	static Matrix4x4 Identity();
	static Matrix4x4 Translation(float x, float y, float z);
	static Matrix4x4 Rotation(float dirX, float dirY, float dirZ, float angle);

	union
	{
		float m[16];
		fvec4 v[4];
	};
	static Matrix4x4 Translate(const Matrix4x4& mat, const fvec3& translation);
	static Matrix4x4 Translate(const Matrix4x4& mat, const fvec4& translation);

	static Matrix4x4 Rotate(const Matrix4x4& mat, const fvec3& rotation, float angle);
	static Matrix4x4 RotateX(const Matrix4x4& mat, float angle);
	static Matrix4x4 RotateY(const Matrix4x4& mat, float angle);
	static Matrix4x4 RotateZ(const Matrix4x4& mat, float angle);

	static Matrix4x4 Scale(const Matrix4x4& mat, float scale);
	static Matrix4x4 Scale(const Matrix4x4& mat, const fvec3& scale);
	static Matrix4x4 Scale(const Matrix4x4& mat, const fvec4& scale);

	static fvec4 Mul(const Matrix4x4& m, const fvec4& v);
	static Matrix4x4 Mul(const Matrix4x4& m1, const Matrix4x4& m2);

	static float Determinant(const Matrix4x4& mat);
	static Matrix4x4 Inverse(const Matrix4x4& mat);

	static Matrix4x4 LookAtLH(const fvec4& eyePosition, const fvec4& focusPosition, const fvec4& upDirection);
	static Matrix4x4 PerspectiveFovLH(float fovAngleY, float aspectRatio, float nearZ, float farZ);
	static Matrix4x4 Ortho(float left, float right, float bottom, float top, float nearDepth, float farDepth);
	static Matrix4x4 OrthoLH(float left, float right, float top, float bottom, float nearDepth, float farDepth);
};
struct Camera3D
{
	Camera3D() { };

	void Update();
	void SetDefault();
	void SetMatrix();

	static fvec3 YawPitchRollToLookVec(float yaw, float pitch, float roll);

	Matrix4x4 GetLightMatrix(const fvec4& lightDir, fvec4* centerPoint);

	Matrix4x4 mat;
	fvec3 camPos = { 0.0f, 0.0, -20.0f };
	fvec3 camTarget = { 0.0f, 0.0f, 1.0f };
	fvec3 camUp = { 0, -1.0f, 0.0f };
	float yaw = 0;
	float pitch = 0;

	bool mouseAttached = false;
};
