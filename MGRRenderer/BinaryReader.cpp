#include "BinaryReader.h"
#include "Logger.h"
#include <assert.h>
#include <ShlObj.h>

namespace mgrrenderer
{

BinaryReader::BinaryReader() : _buffer(nullptr), _length(0), _position(0)
{
}

BinaryReader::~BinaryReader()
{
	_buffer = nullptr;
}

void BinaryReader::init(unsigned char* buffer, size_t length)
{
	assert(buffer != nullptr);
	assert(length > 0);
	_buffer = buffer;
	_length = length;
}

size_t BinaryReader::read(void* outResult, size_t size, size_t count)
{
	assert(_buffer != nullptr);

	if (_position >= _length)
	{
		Logger::log("warning: bundle reader out of range");
		return 0;
	}

	size_t validLength = _length - _position;
	if (size * count > validLength)
	{
		Logger::log("warning: bundle reader out of range");
	}

	size_t validCount = validLength / size;
	size_t readCount = min(validCount, count);

	memcpy(outResult, _buffer + _position, size * readCount);
	_position += size * readCount;
	return readCount;
}

size_t BinaryReader::tell() const
{
	return _position;
}

void BinaryReader::seek(long int offset, int origin)
{
	assert(_buffer != nullptr);

	switch (origin)
	{
	case SEEK_CUR:
		_position += offset;
		break;
	case SEEK_SET:
		_position = offset;
		break;
	case SEEK_END:
		_position = _length + offset;
		break;
	default:
		assert(false);
		break;
	}
}

void BinaryReader::rewind()
{
	assert(_buffer != nullptr);
	_position = 0;
}

std::string BinaryReader::readString()
{
	// 4バイトの長さ、char配列の形式

	// 長さ読み取り
	unsigned int length;
	size_t readCount = read(&length, 4, 1); // sizeof(unsigned int)を使わない。型のサイズは変わりうるので。
	if (readCount != 1)
	{
		// 長さがとれず読み取り失敗
		return "";
	}

	if (length == 0)
	{
		return "";
	}

	// 文字列読み取り
	std::string ret;
	ret.resize(length);
	readCount = read(&ret[0], 1, length);
	if (readCount != length)
	{
		// 長さだけ正常に読み取れなかったら読み取り失敗とする
		return "";
	}

	return ret;
}

bool BinaryReader::readMatrix(float* m) // m[4][4]
{
	size_t readCount = read(m, 4, 16);
	return readCount == 16;
}

} // namespace mgrrenderer
