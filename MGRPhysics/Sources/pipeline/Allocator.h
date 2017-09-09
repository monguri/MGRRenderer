#pragma once

namespace mgrphysics
{
class Allocator {
public:
	// �������m�ێ��ɌĂ΂�郁�\�b�h
	virtual void* allocate(size_t bytes) = 0;
	// ������������ɌĂ΂�郁�\�b�h
	virtual void* deallocate(void* p) = 0;
};
} // namespace mgrphysics
