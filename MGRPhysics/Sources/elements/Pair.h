#pragma once

namespace mgrphysics
{
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
};
} // namespace mgrphysics
