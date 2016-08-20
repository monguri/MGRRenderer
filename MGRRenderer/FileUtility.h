#pragma once
#include <string>
//TODO:ssize_t�̂���
#include "BasicDataTypes.h"

namespace mgrrenderer
{

class FileUtility
{
// windows�ˑ��Ɛ؂藣���̂͌��
public:
	static const int MAX_PATH_LENGTH = 512;

	static FileUtility* getInstance();

	// outPath�����ɂ͈��S�̂��߃T�C�YMAX_PATH_LENGTH��WCHAR�z���^���邱�Ƃ𐄏����邪�A���肬��̃T�C�Y�ł����삷��
	static void convertWCHARFilePath(const std::string& inPath, WCHAR outPath[], size_t size);
	std::string getFullPathForFileName(const std::string& fileName) const;
	static bool isAbsolutePath(const std::string& path);
	bool isFileExistInternal(const std::string& path) const;
	unsigned char* getFileData(const std::string& fileName, size_t* size, bool forString = false) const;
	static bool isValidFileNameAtWindows(const std::string& fullPath, const std::string& fileName);
	std::string getStringFromFile(const std::string& fileName) const;
	static std::string convertPathFormatToUnixStyle(const std::string& path);

private:
	static FileUtility* _instance;
	std::string _resourceRootPath;

	FileUtility();
};

} // namespace mgrrenderer
