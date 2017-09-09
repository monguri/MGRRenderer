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
		// ユニークなキー
		unsigned long long key;
		struct
		{
			// 剛体Aのインデックス
			unsigned int rigidBodyA;
			// 剛体Bのインデックス
			unsigned int rigidBodyB;
		};
	};

	// 衝突情報
	Contact* contact;
};
} // namespace mgrphysics
