#pragma once

namespace mgrphysics
{
class Allocator {
public:
	// メモリ確保時に呼ばれるメソッド
	virtual void* allocate(size_t bytes) = 0;
	// メモリ解放時に呼ばれるメソッド
	virtual void* deallocate(void* p) = 0;
};
} // namespace mgrphysics
