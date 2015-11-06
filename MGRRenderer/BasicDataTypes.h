#pragma once
#include <gles/include/glew.h>
// TODO:ssize_tを使うためだが、もっとましな方法はないかな。。
#include <ShlObj.h>
#include <math.h> // fabsやfmodを使うため
#include <stdio.h> // printfのため

#ifndef __SSIZE_T
#define __SSIZE_T
typedef SSIZE_T ssize_t;
#endif // __SSIZE_T

#include <assert.h>

namespace mgrrenderer
{

static const float PI_OVER2 = 1.57079632679489661923f; // pi / 2
static const float FLOAT_EPSILON = 0.000001f;
static float convertDegreeToRadian(float angle)
{
	return angle * 0.0174532925f; // 2*pi / 360
}

struct Vec2
{
	float x;
	float y;

	Vec2() : x(0), y(0) {}
	Vec2(float x, float y) : x(x), y(y) {}

	const Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
	const Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
	Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; }
	Vec2& operator-=(const Vec2& v) { x -= v.x; y -= v.y; }
	const Vec2 operator*(float a) const { return Vec2(a * x, a * y); }
	const Vec2 operator/(float a) const { assert(a != 0.0);  return Vec2(x / a, y / a); } //TODO:CCMathBaseのMATH_TOLELRANCEみたいの考慮してない
	Vec2& operator*=(float a) { x *= a; y *= a; }
	Vec2& operator/=(float a) { assert(a != 0.0); x /= a; y /= a; }
	bool operator==(const Vec2& v) const { return (x == v.x && y == v.y); } //TODO:うーん。。。誤差考慮してない
	bool operator!=(const Vec2& v) const { return (x != v.x || y != v.y);}
	void normalize() {
		float n = x * x + y * y;
		if (n == 1.0f)
		{
			return;
		}

		n = sqrt(n);
		if (n == 0.0f) //TODO:CCMathBaseのMATH_TOLELRANCEみたいの考慮してない
		{
			return;
		}
		assert(n > 0.0f);

		n = 1.0f / n;
		x *= n;
		y *= n;
	}
	float dot(const Vec2& v) { return (x * v.x + y * v.y); }
	static float dot(const Vec2& v1, const Vec2& v2) { return (v1.x * v2.x + v1.y * v2.y); }
};

struct Vec3
{
	float x;
	float y;
	float z;

	Vec3() : x(0), y(0), z(0) {}
	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

	const Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
	const Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
	Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; }
	Vec3& operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z;}
	const Vec3 operator*(float a) const { return Vec3(a * x, a * y, a * z); }
	const Vec3 operator/(float a) const { assert(a != 0.0);  return Vec3(x / a, y / a, z / a); } //TODO:CCMathBaseのMATH_TOLELRANCEみたいの考慮してない
	Vec3& operator*=(float a) { x *= a; y *= a; z *= a; }
	Vec3& operator/=(float a) { assert(a != 0.0); x /= a; y /= a; z /= a; }
	bool operator==(const Vec3& v) const { return (x == v.x && y == v.y && z == v.z); } //TODO:うーん。。。誤差考慮してない
	bool operator!=(const Vec3& v) const { return (x != v.x || y != v.y || z != v.z);}
	void normalize() {
		float n = x * x + y * y + z * z;
		if (n == 1.0f)
		{
			return;
		}

		n = sqrt(n);
		if (n == 0.0f) //TODO:CCMathBaseのMATH_TOLELRANCEみたいの考慮してない
		{
			return;
		}
		assert(n > 0.0f);

		n = 1.0f / n;
		x *= n;
		y *= n;
		z *= n;
	}
	float dot(const Vec3& v) { return (x * v.x + y * v.y + z * v.z); }
	static float dot(const Vec3& v1, const Vec3& v2) { return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z); }
	const Vec3 cross(const Vec3& v) { return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
	static Vec3 cross(const Vec3& v1, const Vec3& v2)
	{
		return Vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
	}
};

struct Mat4
{
	float m[16];

	Mat4() {
		//　インデックスは列方向に増えていく
		m[0] = 0.0f; m[4] = 0.0f; m[8] = 0.0f; m[12] = 0.0f;
		m[1] = 0.0f; m[5] = 0.0f; m[9] = 0.0f; m[13] = 0.0f;
		m[2] = 0.0f; m[6] = 0.0f; m[10] = 0.0f; m[14] = 0.0f;
		m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f; m[15] = 0.0f;
	}

	Mat4(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33
		)
	{
		//　インデックスは列方向に増えていく
		m[0] = m00; m[4] = m01; m[8] = m02; m[12] = m03;
		m[1] = m10; m[5] = m11; m[9] = m12; m[13] = m13;
		m[2] = m20; m[6] = m21; m[10] = m22; m[14] = m23;
		m[3] = m30; m[7] = m31; m[11] = m32; m[15] = m33;
	}

	static Mat4 createLookAt(const Vec3& eyePosition, const Vec3& targetPosition, const Vec3& up)
	{
		Vec3 upVec = up;
		upVec.normalize();

		Vec3 zAxis = eyePosition - targetPosition;
		zAxis.normalize();

		Vec3 xAxis = Vec3::cross(up, zAxis);
		xAxis.normalize();

		Vec3 yAxis = Vec3::cross(zAxis, xAxis);
		yAxis.normalize();

		return Mat4(
			xAxis.x,	xAxis.y,	xAxis.z,	-Vec3::dot(xAxis, eyePosition),
			yAxis.x,	yAxis.y,	yAxis.z,	-Vec3::dot(yAxis, eyePosition),
			zAxis.x,	zAxis.y,	zAxis.z,	-Vec3::dot(zAxis, eyePosition),
			0.0f,		0.0f,		0.0f,		1.0f
			);
	}

	static Mat4 createPerspective(float fieldOfView, float aspectRatio, float zNearPlane, float zFarPlane)
	{
		assert(zFarPlane != zNearPlane);

		float distanceFactor = 1.0f / (zFarPlane - zNearPlane);
		float fieldOfViewRadian = convertDegreeToRadian(fieldOfView) * 0.5f;

		if (fabs(fmod(fieldOfViewRadian, PI_OVER2)) < FLOAT_EPSILON)
		{
			printf("Invalid field of view value (%f) causes attempted calculation tan(%f), which is undefined.", fieldOfView, fieldOfViewRadian);
			assert(false);

			Mat4 zeroMat;
			return zeroMat;
		}

		float divisor = tan(fieldOfViewRadian);
		assert(divisor != 0);
		float factor = 1.0f / divisor;

		return Mat4(
			factor / aspectRatio,	0.0f,	0.0f,										0.0f,
			0.0f,					factor,	0.0f,										0.0f,
			0.0f,					0.0f,	-(zNearPlane + zFarPlane) * distanceFactor,	-2.0f * zNearPlane * zFarPlane * distanceFactor,
			0.0f,					.0f,	-1.0f,										0.0f
			);
	}

	static Mat4 createOrthographicAtCenter(float width, float height, float zNearPlane, float zFarPlane)
	{
		assert(width > 0.0f);
		assert(height > 0.0f);
		assert(zNearPlane != zFarPlane);
		float halfWidth = width / 2.0f;
		float halfHeight = height / 2.0f;
		return createOrthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, zNearPlane, zFarPlane);
	}

	static Mat4 createOrthographic(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
	{
		assert(left != right);
		assert(bottom != top);
		assert(zNearPlane != zFarPlane);
		return Mat4(
			2 / (right - left),	0.0f,				0.0f,							(left + right) / (left - right),
			0.0f,				2 / (top - bottom),	0.0f,							(bottom + top) / (bottom - top),
			0.0f,				0.0f,				2 / (zFarPlane - zNearPlane),	(zNearPlane + zFarPlane) / (zNearPlane - zFarPlane),
			0.0f,				0.0f,				0.0f,							1.0f	
			);
	}
};

struct Size
{
	float width;
	float height;

	Size() : width(0), height(0) {}
	Size(float width, float height) : width(width), height(height) {}
};

struct Rect
{
	Vec2 origin;
	Size size;

	Rect() : origin(Vec2()), size(Size()) {}
	Rect(float x, float y, float w, float h) : origin(Vec2(x, y)), size(Size(w, h)) {}
};

struct TextureCoordinates
{
	float u;
	float v;

	TextureCoordinates() : u(0), v(0) {}
	TextureCoordinates(float u, float v) : u(u), v(v) {}
};

struct Position2DTextureCoordinates
{
	Vec2 pos;
	TextureCoordinates texCoords;

	Position2DTextureCoordinates() :
	pos(Vec2()),
	texCoords(TextureCoordinates())
	{}

	Position2DTextureCoordinates(const Vec2& position, const TextureCoordinates& textureCoordinates) :
	pos(position),
	texCoords(textureCoordinates)
	{}
};

struct Quadrangle2D
{
	Position2DTextureCoordinates topLeft;
	Position2DTextureCoordinates bottomLeft;
	Position2DTextureCoordinates topRight;
	Position2DTextureCoordinates bottomRight;
};

struct OpenGLProgramData
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint shaderProgram;
	GLint attributeVertexPosition;
	GLint attributeTextureCoordinates;
	GLint uniformTexture;
	GLint uniformViewMatrix;
	GLint uniformProjectionMatrix;
};

} // namespace mgrrenderer
