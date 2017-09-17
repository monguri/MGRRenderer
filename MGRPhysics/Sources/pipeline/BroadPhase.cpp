#include "BroadPhase.h"
#include "Sort.h"
#include "../elements/State.h"
#include "../elements/Collidable.h"
#include "../elements/Pair.h"
#include "../elements/Contact.h"
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

	// 次のブロックでKeyの小さい順に比較するためにソートしておく
	{
		Pair* sortBuff = (Pair*)malloc(sizeof(Pair) * numNewPairs);
		sort<Pair>(newPairs, sortBuff, numNewPairs);
		free(sortBuff);
	}

	// 新しく検出したペアと過去のペアを比較して、過去のペアが維持されているものと、新規にペアになったものを分ける
	// Contactをmallocするものを最低限にするため、過去のペアが維持されているものは再利用する
	{
		Pair* outNewPairs = (Pair*)malloc(sizeof(Pair) * numNewPairs);
		Pair* outKeepPairs = (Pair*)malloc(sizeof(Pair) * numOldPairs);
		Logger::logAssert(outNewPairs != nullptr, "mallocに失敗。");
		Logger::logAssert(outKeepPairs != nullptr, "mallocに失敗。");
		unsigned int nNew = 0;
		unsigned int nKeep = 0;
		unsigned int oldId = 0, newId = 0;

		while (oldId < numOldPairs && newId < numNewPairs)
		{
			if (newPairs[newId].key > oldPairs[oldId].key)
			{
				// 削除
				free(oldPairs[oldId].contact);
				oldPairs[oldId].contact = nullptr;
				oldId++;
			}
			else if (newPairs[newId].key == oldPairs[oldId].key)
			{
				// キープ
				Logger::logAssert(nKeep <= numOldPairs, "キープ数がOldPairにあった数を超えた。");
				outKeepPairs[nKeep] = oldPairs[oldId];
				nKeep++;
				oldId++;
				newId++;
			}
			else
			{
				// 新規ペアとして登録
				Logger::logAssert(nNew <= numNewPairs, "新規作成数がNewPairにあった数を超えた。");
				outNewPairs[nNew] = newPairs[newId];
				nNew++;
				newId++;
			}
		}

		if (newId < numNewPairs)
		{
			// 余った新規ペアは新規ペアとして全登録
			for (; newId < numNewPairs; ++newId, ++nNew)
			{
				Logger::logAssert(nNew <= numNewPairs, "新規作成数がNewPairにあった数を超えた。");
				outNewPairs[nNew] = newPairs[newId];
			}
		}
		else if (oldId < numOldPairs)
		{
			// 余った過去のペアは全削除
			for (; oldId < numOldPairs; ++oldId)
			{
				free(oldPairs[oldId].contact);
				oldPairs[oldId].contact = nullptr;
			}
		}

		// Pairのコンタクト情報の初期化
		for (unsigned int i = 0; i < nNew; ++i)
		{
			outNewPairs[i].contact = (Contact*)malloc(sizeof(Contact));
			outNewPairs[i].contact->reset();
		}

		for (unsigned int i = 0; i < nKeep; ++i)
		{
			outKeepPairs[i].contact->refresh(
				states[outKeepPairs[i].rigidBodyA].position,
				states[outKeepPairs[i].rigidBodyA].orientation,
				states[outKeepPairs[i].rigidBodyB].position,
				states[outKeepPairs[i].rigidBodyB].orientation
			);
		}

		numNewPairs = 0;
		for (unsigned int i = 0; i < nKeep; ++i)
		{
			outKeepPairs[i].type = PairType::KEEP;
			newPairs[numNewPairs++] = outKeepPairs[i];
		}

		for (unsigned int i = 0; i < nNew; ++i)
		{
			outNewPairs[i].type = PairType::NEW;
			newPairs[numNewPairs++] = outNewPairs[i];
		}

		free(outKeepPairs);
		free(outNewPairs);
	}

	// キープしたものが前半に固まっているので再度ソートする
	{
		Pair* sortBuff = (Pair*)malloc(sizeof(Pair) * numNewPairs);
		sort<Pair>(newPairs, sortBuff, numNewPairs);
		free(sortBuff);
	}
}
} // namespace mgrphysics
