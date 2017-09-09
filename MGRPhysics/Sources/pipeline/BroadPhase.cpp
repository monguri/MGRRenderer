#include "BroadPhase.h"
#include "../elements/State.h"
#include "../elements/Collidable.h"
#include "../elements/Pair.h"
// TODO:�C���N���[�h�p�X��MGRRenderer/sources��ǉ����Ă���B���ƂŊO��
#include "utility/Logger.h"


namespace mgrphysics
{
static const float AABB_EXPAND = 0.01f;

static bool intersectAABB(const Vec3& centerA, const Vec3& halfA, const Vec3& centerB, const Vec3& halfB)
{
	if (fabsf(centerA.x - centerB.x) > halfA.x + halfB.x)
	{
		return false;
	}

	if (fabsf(centerA.y - centerB.y) > halfA.y + halfB.y)
	{
		return false;
	}

	if (fabsf(centerA.z - centerB.z) > halfA.z + halfB.z)
	{
		return false;
	}

	return true;
}

void broadPhase(
	const State* states,
	const Collidable* collidables,
	unsigned int numRigidBodies,
	Pair* oldPairs,
	unsigned int numOldPairs,
	Pair* newPairs,
	unsigned int numNewPairs,
	const unsigned int maxPairs,
	void* userData
)
{
	Logger::logAssert(states != nullptr, "states������nullptr�B");
	Logger::logAssert(collidables != nullptr, "collidables������nullptr�B");
	Logger::logAssert(oldPairs != nullptr, "oldPairs������nullptr�B");
	Logger::logAssert(newPairs != nullptr, "newPairs������nullptr�B");

	numNewPairs = 0;

	// AABB�̌����y�A��������
	// �Ƃ肠�����܂��͑�������ł���Ă���
	for (unsigned int i = 0; i < numRigidBodies; ++i)
	{
		for (unsigned int j = i + 1; j < numRigidBodies; ++j)
		{
			const State& stateA = states[i];
			const Collidable& collidableA = collidables[i];
			const State& stateB = states[j];
			const Collidable& collidableB = collidables[j];

			Mat3& orientationA = Mat3::createRotation(stateA.orientation);
			Vec3& centerA = stateA.position + orientationA * collidableA.center;
			Vec3& halfA = orientationA.createAbsolute() * (collidableA.half + Vec3(AABB_EXPAND, AABB_EXPAND, AABB_EXPAND)); // AABB�T�C�Y�͎኱�g������

			Mat3& orientationB = Mat3::createRotation(stateB.orientation);
			Vec3& centerB = stateB.position + orientationB * collidableB.center;
			Vec3& halfB = orientationB.createAbsolute() * (collidableB.half + Vec3(AABB_EXPAND, AABB_EXPAND, AABB_EXPAND)); // AABB�T�C�Y�͎኱�g������

			if (intersectAABB(centerA, halfA, centerB, halfB))
			{
				Pair& newPair = newPairs[numNewPairs++];

				newPair.rigidBodyA = i < j ? i : j;
				newPair.rigidBodyB = i > j ? j : i;
				newPair.contact = nullptr;
			}
		}
	}


	///  ��Ɠr���I�I�I�@///
}
} // namespace mgrphysics
