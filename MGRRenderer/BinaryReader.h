#pragma once

#include <string>

namespace mgrrenderer
{

class BinaryReader final
{
public:
	void init(unsigned char* buffer, size_t length);
	size_t read(void* outResult, size_t size, size_t count);
	size_t tell() const;
	void seek(long int offset, int origin); // ������cocos�̐^��������
	void rewind();
	std::string readString();
	bool readMatrix(float* m); // m[4][4]
	// readLong�Ƃ����������ǂݕ��͂��Ȃ��񂾂ȁB�B

	BinaryReader();
	~BinaryReader();

private:
	unsigned char* _buffer;
	size_t _length;
	size_t _position;
};

} // namespace mgrrenderer
