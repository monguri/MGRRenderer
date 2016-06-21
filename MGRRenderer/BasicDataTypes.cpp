#include "BasicDataTypes.h"

namespace mgrrenderer
{

const Color3B Color3B::WHITE = Color3B(255, 255, 255);
const Color3B Color3B::RED = Color3B(255, 0, 0);
const Color3B Color3B::GREEN = Color3B(0, 255, 0);
const Color3B Color3B::BLUE = Color3B(0, 0, 255);
const Color3B Color3B::YELLOW = Color3B(255, 255, 0);
const Color3B Color3B::MAGENTA = Color3B(255, 0, 255);
const Color3B Color3B::ORANGE = Color3B(255, 127, 255);
const Color3B Color3B::GRAY = Color3B(166, 166, 166);
const Color3B Color3B::BLACK = Color3B(0, 0, 0);

const Color4B Color4B::WHITE = Color4B(255, 255, 255, 255);
const Color4B Color4B::RED = Color4B(255, 0, 0, 255);
const Color4B Color4B::GREEN = Color4B(0, 255, 0, 255);
const Color4B Color4B::BLUE = Color4B(0, 0, 255, 255);
const Color4B Color4B::YELLOW = Color4B(255, 255, 0, 255);
const Color4B Color4B::MAGENTA = Color4B(255, 0, 255, 255);
const Color4B Color4B::ORANGE = Color4B(255, 127, 255, 255);
const Color4B Color4B::GRAY = Color4B(166, 166, 166, 255);
const Color4B Color4B::BLACK = Color4B(0, 0, 0, 255);

Quaternion::Quaternion(const Vec3& angleVec)
{
	float halfRadianX = convertDegreeToRadian(angleVec.x / 2.0f);
	float halfRadianY = convertDegreeToRadian(angleVec.y / 2.0f);
	float halfRadianZ = -convertDegreeToRadian(angleVec.z / 2.0f); //TODO:なぜマイナス？

	float cosHalfRadianX = cosf(halfRadianX);
	float sinHalfRadianX = sinf(halfRadianX);
	float cosHalfRadianY = cosf(halfRadianY);
	float sinHalfRadianY = sinf(halfRadianY);
	float cosHalfRadianZ = cosf(halfRadianZ);
	float sinHalfRadianZ = sinf(halfRadianZ);

	x = sinHalfRadianX * cosHalfRadianY * cosHalfRadianZ - cosHalfRadianX * sinHalfRadianY * sinHalfRadianZ;
	y = cosHalfRadianX * sinHalfRadianY * cosHalfRadianZ + sinHalfRadianX * cosHalfRadianY * sinHalfRadianZ;
	z = cosHalfRadianX * cosHalfRadianY * sinHalfRadianZ - sinHalfRadianX * sinHalfRadianY * cosHalfRadianZ;
	w = cosHalfRadianX * cosHalfRadianY * cosHalfRadianZ + sinHalfRadianX * sinHalfRadianY * sinHalfRadianZ;
}

const Mat4 Mat4::IDENTITY = Mat4(
							1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f
							);

const Mat4 Mat4::ZERO = Mat4(
							0.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 0.0f
							);

const Mat4 Mat4::CHIRARITY_CONVERTER = Mat4(
							1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, -1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f
							);

#if defined(MGRRENDERER_USE_DIRECT3D)
const Mat4 Mat4::TEXTURE_COORDINATE_CONVERTER = Mat4(
							1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, -1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f
							);
#endif

} // namespace mgrrenderer
