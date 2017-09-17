#include "BroadPhase.h"
#include "Sort.h"
#include "../elements/State.h"
#include "../elements/Collidable.h"
#include "../elements/Pair.h"
#include "../elements/Contact.h"
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

	// ���̃u���b�N��Key�̏��������ɔ�r���邽�߂Ƀ\�[�g���Ă���
	{
		Pair* sortBuff = (Pair*)malloc(sizeof(Pair) * numNewPairs);
		sort<Pair>(newPairs, sortBuff, numNewPairs);
		free(sortBuff);
	}

	// �V�������o�����y�A�Ɖߋ��̃y�A���r���āA�ߋ��̃y�A���ێ�����Ă�����̂ƁA�V�K�Ƀy�A�ɂȂ������̂𕪂���
	// Contact��malloc������̂��Œ���ɂ��邽�߁A�ߋ��̃y�A���ێ�����Ă�����͍̂ė��p����
	{
		Pair* outNewPairs = (Pair*)malloc(sizeof(Pair) * numNewPairs);
		Pair* outKeepPairs = (Pair*)malloc(sizeof(Pair) * numOldPairs);
		Logger::logAssert(outNewPairs != nullptr, "malloc�Ɏ��s�B");
		Logger::logAssert(outKeepPairs != nullptr, "malloc�Ɏ��s�B");
		unsigned int nNew = 0;
		unsigned int nKeep = 0;
		unsigned int oldId = 0, newId = 0;

		while (oldId < numOldPairs && newId < numNewPairs)
		{
			if (newPairs[newId].key > oldPairs[oldId].key)
			{
				// �폜
				free(oldPairs[oldId].contact);
				oldPairs[oldId].contact = nullptr;
				oldId++;
			}
			else if (newPairs[newId].key == oldPairs[oldId].key)
			{
				// �L�[�v
				Logger::logAssert(nKeep <= numOldPairs, "�L�[�v����OldPair�ɂ��������𒴂����B");
				outKeepPairs[nKeep] = oldPairs[oldId];
				nKeep++;
				oldId++;
				newId++;
			}
			else
			{
				// �V�K�y�A�Ƃ��ēo�^
				Logger::logAssert(nNew <= numNewPairs, "�V�K�쐬����NewPair�ɂ��������𒴂����B");
				outNewPairs[nNew] = newPairs[newId];
				nNew++;
				newId++;
			}
		}

		if (newId < numNewPairs)
		{
			// �]�����V�K�y�A�͐V�K�y�A�Ƃ��đS�o�^
			for (; newId < numNewPairs; ++newId, ++nNew)
			{
				Logger::logAssert(nNew <= numNewPairs, "�V�K�쐬����NewPair�ɂ��������𒴂����B");
				outNewPairs[nNew] = newPairs[newId];
			}
		}
		else if (oldId < numOldPairs)
		{
			// �]�����ߋ��̃y�A�͑S�폜
			for (; oldId < numOldPairs; ++oldId)
			{
				free(oldPairs[oldId].contact);
				oldPairs[oldId].contact = nullptr;
			}
		}

		// Pair�̃R���^�N�g���̏�����
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

	// �L�[�v�������̂��O���Ɍł܂��Ă���̂ōēx�\�[�g����
	{
		Pair* sortBuff = (Pair*)malloc(sizeof(Pair) * numNewPairs);
		sort<Pair>(newPairs, sortBuff, numNewPairs);
		free(sortBuff);
	}
}
} // namespace mgrphysics
