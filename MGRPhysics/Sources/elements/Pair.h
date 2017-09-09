#pragma once

namespace mgrphysics
{
struct Contact;

enum class PairType : int
{
	NEW,
	KEEP,
};

struct Pair
{
	PairType type;
	union
	{
		// ���j�[�N�ȃL�[
		unsigned long long key;
		struct
		{
			// ����A�̃C���f�b�N�X
			unsigned int rigidBodyA;
			// ����B�̃C���f�b�N�X
			unsigned int rigidBodyB;
		};
	};

	// �Փˏ��
	Contact* contact;
};
} // namespace mgrphysics
