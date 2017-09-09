#include "BroadPhase.h"
#include "../elements/State.h"
#include "../elements/Collidable.h"
#include "../elements/Pair.h"
// TODO:インクルードパスにMGRRenderer/sourcesを追加している。あとで外す
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
	Logger::logAssert(states != nullptr, "states引数がnullptr。");
	Logger::logAssert(collidables != nullptr, "collidables引数がnullptr。");
	Logger::logAssert(oldPairs != nullptr, "oldPairs引数がnullptr。");
	Logger::logAssert(newPairs != nullptr, "newPairs引数がnullptr。");

	numNewPairs = 0;

	// AABBの交差ペアを見つける
	// とりあえずまずは総当たりでやっている
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
			Vec3& halfA = orientationA.createAbsolute() * (collidableA.half + Vec3(AABB_EXPAND, AABB_EXPAND, AABB_EXPAND)); // AABBサイズは若干拡張する

			Mat3& orientationB = Mat3::createRotation(stateB.orientation);
			Vec3& centerB = stateB.position + orientationB * collidableB.center;
			Vec3& halfB = orientationB.createAbsolute() * (collidableB.half + Vec3(AABB_EXPAND, AABB_EXPAND, AABB_EXPAND)); // AABBサイズは若干拡張する

			if (intersectAABB(centerA, halfA, centerB, halfB))
			{
				Pair& newPair = newPairs[numNewPairs++];

				newPair.rigidBodyA = i < j ? i : j;
				newPair.rigidBodyB = i > j ? j : i;
				newPair.contact = nullptr;
			}
		}
	}


	///  作業途中！！！　///
}
} // namespace mgrphysics
