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

} // namespace mgrrenderer
