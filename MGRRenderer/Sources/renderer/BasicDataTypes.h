#pragma once

#include "Config.h"

#if defined(MGRRENDERER_USE_OPENGL)
#include <gles/include/glew.h>
#endif
#include <math.h> // fabsやfmodを使うため
#include <string>
#include <vector>
#include "utility/Logger.h"

// TODO:ssize_tを使うためだが、もっとましな方法はないかな。。
#include <ShlObj.h>
#ifndef __SSIZE_T
#define __SSIZE_T
typedef SSIZE_T ssize_t;
#endif // __SSIZE_T

namespace mgrrenderer
{

static const float PI_OVER2 = 1.57079632679489661923f; // pi / 2
static const float FLOAT_TOLERANCE = 2e-37f;
static const float FLOAT_EPSILON = 0.000001f;
static float convertDegreeToRadian(float angle)
{
	return angle * 0.0174532925f; // 2*pi / 360
}

struct Vec2
{
	union
	{
		float x;
		float u;
		float s;
	};
	union
	{
		float y;
		float v;
		float t;
	};

	Vec2() : x(0), y(0) {}
	Vec2(float x, float y) : x(x), y(y) {}

	Vec2 operator+(const Vec2& vec) const { return Vec2(x + vec.x, y + vec.y); }
	Vec2 operator-(const Vec2& vec) const { return Vec2(x - vec.x, y - vec.y); }
	Vec2& operator+=(const Vec2& vec) { x += vec.x; y += vec.y; return *this; }
	Vec2& operator-=(const Vec2& vec) { x -= vec.x; y -= vec.y; return *this; }
	Vec2 operator*(float a) const { return Vec2(a * x, a * y); }
	Vec2 operator/(float a) const { Logger::logAssert(a != 0.0, "0で除算している。");  return Vec2(x / a, y / a); } //TODO:CCMathBaseのMATH_TOLELRANCEみたいの考慮してない
	Vec2& operator*=(float a) { x *= a; y *= a; return *this; }
	Vec2& operator/=(float a) { Logger::logAssert(a != 0.0, "0で除算している。"); x /= a; y /= a; return *this; }
	bool operator==(const Vec2& vec) const { return (x == vec.x && y == vec.y); } //TODO:うーん。。。誤差考慮してない
	bool operator!=(const Vec2& vec) const { return (x != vec.x || y != vec.y);}
	void normalize()
	{
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
		Logger::logAssert(n > 0.0f, "0で除算している。");

		n = 1.0f / n;
		x *= n;
		y *= n;
	}
	float dot(const Vec2& vec) { return (x * vec.x + y * vec.y); }
	static float dot(const Vec2& v1, const Vec2& v2) { return (v1.x * v2.x + v1.y * v2.y); }
};

struct Vec3
{
	union
	{
		float x;
		float r;
	};
	union
	{
		float y;
		float g;
	};
	union
	{
		float z;
		float b;
	};

	Vec3() : x(0), y(0), z(0) {}
	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

	Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
	Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
	Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
	Vec3& operator-=(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	Vec3 operator*(float a) const { return Vec3(a * x, a * y, a * z); }
	Vec3 operator/(float a) const { Logger::logAssert(a != 0.0, "0で除算している。");  return Vec3(x / a, y / a, z / a); } //TODO:CCMathBaseのMATH_TOLELRANCEみたいの考慮してない
	Vec3& operator*=(float a) { x *= a; y *= a; z *= a; return *this; }
	Vec3& operator/=(float a) { Logger::logAssert(a != 0.0, "0で除算している。"); x /= a; y /= a; z /= a; return *this; }
	bool operator==(const Vec3& v) const { return (x == v.x && y == v.y && z == v.z); } //TODO:うーん。。。誤差考慮してない
	bool operator!=(const Vec3& v) const { return (x != v.x || y != v.y || z != v.z);}
	float length() const {
		return sqrt(x * x + y * y + z * z);
	}

	void normalize() {
		float n = x * x + y * y + z * z;
		if (n == 1.0f)
		{
			return;
		}

		n = sqrt(n);
		if (n < FLOAT_TOLERANCE)
		{
			Logger::logAssert(n > FLOAT_TOLERANCE, "0に近い値で除算している。");
			return;
		}

		n = 1.0f / n;
		x *= n;
		y *= n;
		z *= n;
	}
	float dot(const Vec3& v) const { return (x * v.x + y * v.y + z * v.z); }
	static float dot(const Vec3& v1, const Vec3& v2) { return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z); }
	Vec3 cross(const Vec3& v) const { return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }
	static Vec3 cross(const Vec3& v1, const Vec3& v2)
	{
		return Vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
	}
};

struct Color3B
{
	static const Color3B WHITE;
	static const Color3B RED;
	static const Color3B GREEN;
	static const Color3B BLUE;
	static const Color3B YELLOW;
	static const Color3B MAGENTA;
	static const Color3B ORANGE;
	static const Color3B GRAY;
	static const Color3B BLACK;

	unsigned char r;
	unsigned char g;
	unsigned char b;

	Color3B(unsigned char rVal, unsigned char gVal, unsigned char bVal) : r(rVal), g(gVal), b(bVal)
	{
	}
};

struct Color3F
{
	Vec3 color;

	Color3F()
	{
	}

	Color3F(const Color3B& color3B) : color(color3B.r / 255.0f, color3B.g / 255.0f, color3B.b / 255.0f)
	{
	}

	Color3F(float r, float g, float b) : color(r, g, b)
	{
	}

	Color3F operator*(float a)
	{
		color *= a;
		return *this;
	}
};

struct Vec4
{
	union
	{
		float x;
		float r;
	};
	union
	{
		float y;
		float g;
	};
	union
	{
		float z;
		float b;
	};
	union
	{
		float w;
		float a;
	};

	Vec4() : x(0), y(0), z(0), w(0) {}
	Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	Vec4(const Vec3& vec3) : x(vec3.x), y(vec3.y), z(vec3.z), w(1.0f) {}

	Vec4 operator+(const Vec4& v) const { return Vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
	Vec4 operator-(const Vec4& v) const { return Vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
	Vec4& operator+=(const Vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
	Vec4& operator-=(const Vec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
	Vec4 operator*(float val) const { return Vec4(val * x, val * y, val * z, val * w); }
	Vec4 operator/(float val) const { Logger::logAssert(val != 0.0, "0で除算している。");  return Vec4(x / val, y / val, z / val, w / val); } //TODO:CCMathBaseのMATH_TOLELRANCEみたいの考慮してない
	Vec4& operator*=(float val) { x *= val; y *= val; z *= val; w *= val; return *this; }
	Vec4& operator/=(float val) { Logger::logAssert(val != 0.0, "0で除算している。"); x /= val; y /= val; z /= val; w /= val; return *this; }
	bool operator==(const Vec4& v) const { return (x == v.x && y == v.y && z == v.z && w == v.w); } //TODO:うーん。。。誤差考慮してない
	bool operator!=(const Vec4& v) const { return (x != v.x || y != v.y || z != v.z || w != v.w);}
	Vec4& normalize() {
		float n = x * x + y * y + z * z + w * w;
		if (n == 1.0f)
		{
			return *this;
		}

		n = sqrt(n);
		if (n < FLOAT_TOLERANCE)
		{
			Logger::logAssert(n > FLOAT_TOLERANCE, "0に近い値で除算している。");
			// 0べクトルを返す
			*this = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
			return *this;
		}

		n = 1.0f / n;
		x *= n;
		y *= n;
		z *= n;
		w *= n;
		return *this;
	}
};

struct Color4B
{
	static const Color4B WHITE;
	static const Color4B RED;
	static const Color4B GREEN;
	static const Color4B BLUE;
	static const Color4B YELLOW;
	static const Color4B MAGENTA;
	static const Color4B ORANGE;
	static const Color4B GRAY;
	static const Color4B BLACK;

	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;

	Color4B(unsigned char rVal, unsigned char gVal, unsigned char bVal, unsigned char aVal) : r(rVal), g(gVal), b(bVal), a(aVal)
	{
	}
};

struct Color4F
{
	Vec4 color;

	Color4F()
	{
	}

	Color4F(const Color4B& color4B) : color(color4B.r / 255.0f, color4B.g / 255.0f, color4B.b / 255.0f, color4B.a / 255.0f)
	{
	}

	Color4F(float r, float g, float b, float a) : color(r, g, b, a)
	{
	}

	Color4F(const Color3B& color3B) : color(color3B.r / 255.0f, color3B.g / 255.0f, color3B.b / 255.0f, 1.0f)
	{
	}

	Color4F operator*(float a)
	{
		color *= a;
		return *this;
	}
};

struct Quaternion {
	float x;
	float y;
	float z;
	float w;

	Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
	Quaternion(float xVal, float yVal, float zVal, float wVal) : x(xVal), y(yVal), z(zVal), w(wVal) {}
	Quaternion(const Vec3& angleVec);
	bool operator==(const Quaternion& q) const { return (x == q.x && y == q.y && z == q.z && w == q.w); } //TODO:うーん。。。誤差考慮してない
	bool operator!=(const Quaternion& q) const { return (x != q.x || y != q.y || z != q.z || w != q.w);}

	static Quaternion slerp(const Quaternion& q1, const Quaternion& q2, float t)
	{
		Logger::logAssert(0.0f <= t && t <= 1.0f, "slerpのパラメターが0から1の範囲外。");
		if (t == 0.0f)
		{
			return q1;
		}
		else
		{
			return q2;
		}
		// TODO:slerpの計算が間違っている
		//else if (t == 1.0f)
		//{
		//	return q2;
		//}
		//else if (t == 1.0f)
		//{
		//	return q2;
		//}

		//if (q1 == q2)
		//{
		//	return q1;
		//}

		////TODO: ここから先はまだ理論がよくわかってない
		//float halfY, alpha, beta;
		//float u, f1, f2a, f2b;
		//float ratio1, ratio2;
		//float halfSecHalfTheta, versHalfTheta;
		//float sqNotU, sqU;

		//float cosTheta = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

		//alpha = cosTheta >= 0 ? 1.0f : -1.0f;
		//halfY = 1 + alpha * cosTheta;

		//f2b = t - 0.5f;
		//u = f2b >= 0 ? f2b : -f2b;
		//f2a = u - f2b;
		//f2b += u;
		//u += u;
		//f1 = 1.0f - u;

		//halfSecHalfTheta = 1.09f - (0.476537f - 0.903321f * halfY) * halfY;
		//halfSecHalfTheta *= 1.5f - halfY * halfSecHalfTheta * halfSecHalfTheta;
		//versHalfTheta = 1.0f - halfY * halfSecHalfTheta;

		//sqNotU = f1 * f1;
		//ratio2 = 0.0000440917108f * versHalfTheta;
		//ratio1 = -0.00158730159f + (sqNotU - 16.0f) * ratio2;
		//ratio1 = 0.0333333333f + ratio1 * (sqNotU - 9.0f) * versHalfTheta;
		//ratio1 = -0.0333333333f + ratio1 * (sqNotU - 4.0f) * versHalfTheta;
		//ratio1 = 1.0f + ratio1 * (sqNotU - 1.0f) * versHalfTheta;

		//sqU = u * u;
		//ratio2 = -0.00158730159f + (sqU - 16.0f) * ratio2;
		//ratio2 = 0.0333333333f + ratio2 * (sqU - 9.0f) * versHalfTheta;
		//ratio2 = -0.0333333333f + ratio2 * (sqU - 4.0f) * versHalfTheta;
		//ratio2 = 1.0f + ratio2 * (sqU - 1.0f) * versHalfTheta;

		//f1 *= ratio1 * halfSecHalfTheta;
		//f2a *= ratio2;
		//f2b *= ratio2;
		//alpha *= f1 + f2a;
		//beta = f1 + f2b;

		//float x = alpha * q1.x + beta * q2.x;
		//float y = alpha * q1.y + beta * q2.y;
		//float z = alpha * q1.z + beta * q2.z;
		//float w = alpha * q1.w + beta * q2.w;

		//f1 = 1.5f - 0.5f * (x * x + y * y + z * z + w * w);
		//return Quaternion(x * f1, y * f1, z * f1, w * f1);
	}
};

struct Mat4
{
	float m[4][4];

	static const Mat4 IDENTITY;
	static const Mat4 ZERO;
	static const Mat4 CHIRARITY_CONVERTER; // 左手系と右手系を切り替えるための行列
#if defined(MGRRENDERER_USE_DIRECT3D)
	static const Mat4 TEXTURE_COORDINATE_CONVERTER; // 通常のy-upの座標系とテクスチャ座標系のy-downを切り替えるための行列
#endif

	Mat4() {
		setZero();
	}

	~Mat4() {
		setZero();
	}

	Mat4(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33
		)
	{
		//　普通にイメージする行列とは転置関係になる
		//　cocos2d-xの一次配列との対応をコメントに記載する
		m[0][0] = m00;/*[0]*/ m[1][0] = m01;/*[4]*/ m[2][0] = m02;/*[8]*/ m[3][0] = m03;/*[12]*/
		m[0][1] = m10;/*[1]*/ m[1][1] = m11;/*[5]*/ m[2][1] = m12;/*[9]*/ m[3][1] = m13;/*[13]*/
		m[0][2] = m20;/*[2]*/ m[1][2] = m21;/*[6]*/ m[2][2] = m22;/*[10]*/ m[3][2] = m23;/*[14]*/
		m[0][3] = m30;/*[3]*/ m[1][3] = m31;/*[7]*/ m[2][3] = m32;/*[11]*/ m[3][3] = m33;/*[15]*/
	}

	Mat4(float** mat)
	{
		memcpy(m[0], mat[0], 4 * 4);
	}

	Mat4& operator*=(float a)
	{
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				m[i][j] *= a;
			}
		}
		return *this;
	}

	Mat4& operator/=(float a)
	{
		Logger::logAssert(a != 0.0, "0で除算している。");
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				m[i][j] /= a;
			}
		}
		return *this;
	} //TODO:CCMathBaseのMATH_TOLELRANCEみたいの考慮してない

	Mat4 operator*(float a) const
	{
		Mat4 ret;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				ret.m[i][j] = a * m[i][j];
			}
		}
		return ret;
	}

	Mat4 operator/(float a) const
	{
		Logger::logAssert(a != 0.0, "0で除算している。");
		Mat4 ret;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				ret.m[i][j] = m[i][j] / a;
			}
		}
		return ret;
	} //TODO:CCMathBaseのMATH_TOLELRANCEみたいの考慮してない

	Vec4 operator*(const Vec4& v) const
	{
		Vec4 ret;
		// あれ？これ、GPUでやられる計算方法と逆じゃない？普通の乗算だ。cocosはこうなってたぞ。
		ret.x = m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w;
		ret.y = m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w;
		ret.z = m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w;
		// wは0のままでよい
		return ret;
	}

	Vec3 operator*(const Vec3& v) const
	{
		// Vec4のoperator*を利用する
		Vec4 vec4(v.x, v.y, v.z, 1.0f);
		vec4 = *this * vec4;
		return Vec3(vec4.x, vec4.y, vec4.z);
	}

	Mat4 operator*(const Mat4& mat) const
	{
		Mat4 ret;

		ret.m[0][0] = m[0][0] * mat.m[0][0] + m[1][0] * mat.m[0][1] + m[2][0] * mat.m[0][2] + m[3][0] * mat.m[0][3];
		ret.m[0][1] = m[0][1] * mat.m[0][0] + m[1][1] * mat.m[0][1] + m[2][1] * mat.m[0][2] + m[3][1] * mat.m[0][3];
		ret.m[0][2] = m[0][2] * mat.m[0][0] + m[1][2] * mat.m[0][1] + m[2][2] * mat.m[0][2] + m[3][2] * mat.m[0][3];
		ret.m[0][3] = m[0][3] * mat.m[0][0] + m[1][3] * mat.m[0][1] + m[2][3] * mat.m[0][2] + m[3][3] * mat.m[0][3];

		ret.m[1][0] = m[0][0] * mat.m[1][0] + m[1][0] * mat.m[1][1] + m[2][0] * mat.m[1][2] + m[3][0] * mat.m[1][3];
		ret.m[1][1] = m[0][1] * mat.m[1][0] + m[1][1] * mat.m[1][1] + m[2][1] * mat.m[1][2] + m[3][1] * mat.m[1][3];
		ret.m[1][2] = m[0][2] * mat.m[1][0] + m[1][2] * mat.m[1][1] + m[2][2] * mat.m[1][2] + m[3][2] * mat.m[1][3];
		ret.m[1][3] = m[0][3] * mat.m[1][0] + m[1][3] * mat.m[1][1] + m[2][3] * mat.m[1][2] + m[3][3] * mat.m[1][3];

		ret.m[2][0] = m[0][0] * mat.m[2][0] + m[1][0] * mat.m[2][1] + m[2][0] * mat.m[2][2] + m[3][0] * mat.m[2][3];
		ret.m[2][1] = m[0][1] * mat.m[2][0] + m[1][1] * mat.m[2][1] + m[2][1] * mat.m[2][2] + m[3][1] * mat.m[2][3];
		ret.m[2][2] = m[0][2] * mat.m[2][0] + m[1][2] * mat.m[2][1] + m[2][2] * mat.m[2][2] + m[3][2] * mat.m[2][3];
		ret.m[2][3] = m[0][3] * mat.m[2][0] + m[1][3] * mat.m[2][1] + m[2][3] * mat.m[2][2] + m[3][3] * mat.m[2][3];

		ret.m[3][0] = m[0][0] * mat.m[3][0] + m[1][0] * mat.m[3][1] + m[2][0] * mat.m[3][2] + m[3][0] * mat.m[3][3];
		ret.m[3][1] = m[0][1] * mat.m[3][0] + m[1][1] * mat.m[3][1] + m[2][1] * mat.m[3][2] + m[3][1] * mat.m[3][3];
		ret.m[3][2] = m[0][2] * mat.m[3][0] + m[1][2] * mat.m[3][1] + m[2][2] * mat.m[3][2] + m[3][2] * mat.m[3][3];
		ret.m[3][3] = m[0][3] * mat.m[3][0] + m[1][3] * mat.m[3][1] + m[2][3] * mat.m[3][2] + m[3][3] * mat.m[3][3];

		return ret;
	}

	Mat4& operator*=(const Mat4& mat)
	{
		*this = *this * mat;
		return *this;
	}

	bool operator==(const Mat4& mat) const {
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				if (m[i][j] != mat.m[i][j])
				{
					return false;
				}
			}
		}

		return true;
	} //TODO:うーん。。。誤差考慮してない

	bool operator!=(const Mat4& mat) const {
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				if (m[i][j] != mat.m[i][j])
				{
					return true;
				}
			}
		}

		return false;
	} //TODO:うーん。。。誤差考慮してない

	Mat4& setZero()
	{
		memset(m, 0, sizeof(Mat4));
		return *this;
	}

	Mat4& setIdentity()
	{
		memcpy(&m, &IDENTITY, sizeof(Mat4));
		return *this;
	}

	static Mat4 createLookAtFrom(const Vec3& eyePosition, const Vec3& targetPosition, const Vec3& up)
	{
		Vec3 upVec = up;
		upVec.normalize();


		Vec3 zAxis = eyePosition - targetPosition;
		zAxis.normalize();

		Vec3 xAxis = Vec3::cross(upVec, zAxis);
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

	static Mat4 createLookAtWithDirection(const Vec3& eyePosition, const Vec3& direction, const Vec3& up)
	{
		Vec3 upVec = up;
		upVec.normalize();

		Vec3 zAxis = direction * -1;
		zAxis.normalize();

		Vec3 xAxis = Vec3::cross(upVec, zAxis);
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
		Logger::logAssert(zFarPlane != zNearPlane, "ニアプレーンとファープレーンの値が同じ。");

		float distanceFactor = 1.0f / (zFarPlane - zNearPlane);
		float fieldOfViewRadian = convertDegreeToRadian(fieldOfView) * 0.5f;

		if (fabs(fmod(fieldOfViewRadian, PI_OVER2)) < FLOAT_EPSILON)
		{
			Logger::logAssert(false, "Invalid field of view value (%f) causes attempted calculation tan(%f), which is undefined.", fieldOfView, fieldOfViewRadian);

			Mat4 zeroMat;
			return zeroMat;
		}

		float divisor = tan(fieldOfViewRadian);
		Logger::logAssert(divisor != 0, "0で除算しようとしている。");
		float factor = 1.0f / divisor;

#if defined(MGRRENDERER_USE_DIRECT3D)
		// [-zNearPlane, -zFarPlane]を[0, -1]に変換
		// これに左からCHIRARITY_CONVERTERをかけた行列だと、[0, 1]に変換する
		return Mat4(
			factor / aspectRatio,	0.0f,	0.0f,							0.0f,
			0.0f,					factor,	0.0f,							0.0f,
			0.0f,					0.0f,	zFarPlane * distanceFactor,		zNearPlane * zFarPlane * distanceFactor,	
			0.0f,					0.0f,	-1.0f,							0.0f
		);
#elif defined(MGRRENDERER_USE_OPENGL)
		// [-zNearPlane, -zFarPlane]を[1, -1]に変換
		return Mat4(
			factor / aspectRatio,	0.0f,	0.0f,										0.0f,
			0.0f,					factor,	0.0f,										0.0f,
			0.0f,					0.0f,	-(zNearPlane + zFarPlane) * distanceFactor,	-2.0f * zNearPlane * zFarPlane * distanceFactor,
			0.0f,					0.0f,	-1.0f,										0.0f
			);
#endif
	}

	static Mat4 createOrthographicAtCenter(float width, float height, float zNearPlane, float zFarPlane)
	{
		Logger::logAssert(width > 0.0f, "widthが0以下。");
		Logger::logAssert(height > 0.0f, "heightが0以下。");
		Logger::logAssert(zNearPlane != zFarPlane, "ファープレインとニアプレインが同値。");
		float halfWidth = width / 2.0f;
		float halfHeight = height / 2.0f;
		return createOrthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, zNearPlane, zFarPlane);
	}

	static Mat4 createOrthographic(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
	{
		Logger::logAssert(left != right, "leftとrightが同値");
		Logger::logAssert(bottom != top, "bottomとtopが同値");
		Logger::logAssert(zNearPlane != zFarPlane, "ファープレインとニアプレインが同値。");
#if defined(MGRRENDERER_USE_DIRECT3D)
		// OpenGLはz座標を[-1, 1]に変換するがDirectXは[0, 1]なのでそこを修正
		return Mat4(
			2 / (right - left),	0.0f,				0.0f,							(left + right) / (left - right),
			0.0f,				2 / (top - bottom),	0.0f,							(bottom + top) / (bottom - top),
			0.0f,				0.0f,				1 / (zFarPlane - zNearPlane),	zNearPlane / (zNearPlane - zFarPlane),
			0.0f,				0.0f,				0.0f,							1.0f	
		);
#elif defined(MGRRENDERER_USE_OPENGL)
		return Mat4(
			2 / (right - left),	0.0f,				0.0f,							(left + right) / (left - right),
			0.0f,				2 / (top - bottom),	0.0f,							(bottom + top) / (bottom - top),
			0.0f,				0.0f,				2 / (zFarPlane - zNearPlane),	(zNearPlane + zFarPlane) / (zNearPlane - zFarPlane),
			0.0f,				0.0f,				0.0f,							1.0f	
		);
#endif
	}

	static Mat4 createTranslation(const Vec3& translation)
	{
		Mat4 ret;
		ret.setIdentity();

		ret.m[3][0] = translation.x;
		ret.m[3][1] = translation.y;
		ret.m[3][2] = translation.z;

		return ret;
	}

	static Mat4 createRotation(const Quaternion& rotation)
	{
		// TODO:理論をちゃんと知るべし
		float x2 = rotation.x + rotation.x;
		float y2 = rotation.y + rotation.y;
		float z2 = rotation.z + rotation.z;

		float xx2 = rotation.x * x2;
		float yy2 = rotation.y * y2;
		float zz2 = rotation.z * z2;

		float xy2 = rotation.x * y2;
		float xz2 = rotation.x * z2;
		float yz2 = rotation.y * z2;

		float wx2 = rotation.w * x2;
		float wy2 = rotation.w * y2;
		float wz2 = rotation.w * z2;

		return Mat4(
			1.0f - yy2 - zz2,	xy2 - wz2,			xz2 + wy2,			0.0f,
			xy2 + wz2,			1.0f - xx2 - zz2,	yz2 - wx2,			0.0f,
			xz2 - wy2,			yz2 + wx2,			1.0f - xx2 -yy2,	0.0f,
			0.0f,				0.0f,				0.0f,				1.0f
			);
	}

	static Mat4 createScale(const Vec3& scale)
	{
		Mat4 ret;
		ret.setIdentity();

		ret.m[0][0] = scale.x;
		ret.m[1][1] = scale.y;
		ret.m[2][2] = scale.z;
		return ret;
	}

	static Mat4 createTransform(const Vec3& translation, const Quaternion& rotation, const Vec3& scale)
	{
		Mat4 ret = createTranslation(translation);
		ret *= createRotation(rotation);
		ret *= createScale(scale);
		return ret;
	}

	Mat4& transpose()
	{
		Mat4 mat(
			m[0][0], m[0][1], m[0][2], m[0][3],
			m[1][0], m[1][1], m[1][2], m[1][3],
			m[2][0], m[2][1], m[2][2], m[2][3],
			m[3][0], m[3][1], m[3][2], m[3][3]
		);

		memcpy(this, &mat, sizeof(Mat4));
		return *this;
	}

	Mat4& inverse()
	{
		float a0 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
		float a1 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
		float a2 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
		float a3 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
		float a4 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
		float a5 = m[0][2] * m[1][3] - m[0][3] * m[1][2];

		float b0 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
		float b1 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
		float b2 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
		float b3 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
		float b4 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
		float b5 = m[2][2] * m[3][3] - m[2][3] * m[3][2];

		float det = a0 * b5 - a1 * b4 + a2 * b3 - a4 * b1 + a5 * b0;

		// Close to zero. cannot inverse.
		if (fabs(det) <= FLOAT_TOLERANCE)
		{
			Logger::logAssert(false, "Mat4::inverse(), determinant is 0.");
			this->setZero(); // ゼロ行列を返す
			return *this;
		}

		Mat4 inverse = Mat4(
			m[1][1] * b5 - m[1][2] * b4 + m[1][3] * b3, -m[1][0] * b5 + m[1][2] * b2 - m[1][3] * b1, m[1][0] * b4 - m[1][1] * b2 + m[1][3] * b0, -m[1][0] * b3 + m[1][1] * b1 - m[1][2] * b0,
			-m[0][1] * b5 + m[0][2] * b4 - m[0][3] * b3, m[0][0] * b5 - m[0][2] * b2 + m[0][3] * b1, -m[0][0] * b4 + m[0][1] * b2 - m[0][3] * b0, m[0][0] * b3 - m[0][1] * b1 + m[0][2] * b0,
			m[3][1] * a5 - m[3][2] * a4 + m[3][3] * a3, -m[3][0] * a5 + m[3][2] * a2 - m[3][3] * a1, m[3][0] * a4 - m[3][1] * a2 + m[3][3] * a0, -m[3][0] * a3 + m[3][1] * a1 - m[3][2] * a0,
			-m[2][1] * a5 + m[2][2] * a4 - m[2][3] * a3, m[2][0] * a5 - m[2][2] * a2 + m[2][3] * a1, -m[2][0] * a4 + m[2][1] * a2 - m[2][3] * a0, m[2][0] * a3 - m[2][1] * a1 + m[2][2] * a0
		);

		inverse /= det;
		memcpy(this, &inverse, sizeof(Mat4));

		return *this;
	}

	// 行列の絶対値を考慮してないため、乗算するとべクトルの長さが変わる。呼び出し側でベクトルを正規化すること
	static Mat4 createNormalMatrix(const Mat4& modelMatrix)
	{
		Mat4 normalMatrix = modelMatrix;
		normalMatrix.m[3][0] = 0.0f;
		normalMatrix.m[3][1] = 0.0f;
		normalMatrix.m[3][2] = 0.0f;
		normalMatrix.m[3][3] = 1.0f;
		normalMatrix.inverse();
		normalMatrix.transpose();
		return normalMatrix;
	}
};

struct Size
{
	float width;
	float height;

	Size() : width(0.0f), height(0.0f) {}
	Size(float width, float height) : width(width), height(height) { Logger::logAssert(width >= 0.0f && height >= 0.0f, "Sizeの引数に負の値が入力された"); }
	Size(unsigned int width, unsigned int height) : width(static_cast<float>(width)), height(static_cast<float>(height)) {}
	Size(int width, int height) : width(static_cast<float>(width)), height(static_cast<float>(height)) { Logger::logAssert(width >= 0 && height >= 0, "Sizeの引数に負の値が入力された"); }

	const Size operator*(float a) const
	{
		Size ret;
		ret.width = width * a;
		ret.height = height * a;
		return ret;
	}
	const Size operator/(float a) const
	{
		Logger::logAssert(a != 0.0, "0で除算している。");
		Size ret;
		ret.width = width / a;
		ret.height = height / a;
		return ret;
	} //TODO:CCMathBaseのMATH_TOLELRANCEみたいの考慮してない
};

struct Rect
{
	Vec2 origin;
	Size size;

	Rect() : origin(Vec2()), size(Size()) {}
	Rect(float x, float y, float w, float h) : origin(Vec2(x, y)), size(Size(w, h)) {}
};

struct AnimationCurve
{
	std::vector<float> keytimeArray;
	std::vector<float> valueArray;

	AnimationCurve(const std::vector<float>& keytimeArrayVal, const std::vector<float>& valueArrayVal) :
	keytimeArray(keytimeArrayVal), valueArray(valueArrayVal)
	{}
};

struct Position2DTextureCoordinates
{
	Vec2 position;
	Vec2 textureCoordinate;

	Position2DTextureCoordinates() :
	position(Vec2()),
	textureCoordinate(Vec2())
	{}

	Position2DTextureCoordinates(const Vec2& pos, const Vec2& texCoord) :
	position(pos),
	textureCoordinate(texCoord)
	{}
};

struct Quadrangle2D
{
	Position2DTextureCoordinates topLeft;
	Position2DTextureCoordinates bottomLeft;
	Position2DTextureCoordinates topRight;
	Position2DTextureCoordinates bottomRight;
};

struct Position3DTextureCoordinates
{
	Vec3 position;
	Vec2 textureCoordinate;

	Position3DTextureCoordinates() :
	position(Vec3()),
	textureCoordinate(Vec2())
	{}

	Position3DTextureCoordinates(const Vec3& pos, const Vec2& texCoord) :
	position(pos),
	textureCoordinate(texCoord)
	{}
};

struct Position3DNormalTextureCoordinates
{
	Vec3 position;
	Vec3 normal;
	Vec2 textureCoordinate;

	Position3DNormalTextureCoordinates() :
	position(Vec3()),
	normal(Vec3()),
	textureCoordinate(Vec2())
	{}

	Position3DNormalTextureCoordinates(const Vec3& pos, const Vec3& n, const Vec2& texCoord) :
	position(pos),
	normal(n),
	textureCoordinate(texCoord)
	{}
};

} // namespace mgrrenderer
