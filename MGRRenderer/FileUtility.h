#pragma once
#include <string>
//TODO:ssize_tÇÃÇΩÇﬂ
#include "BasicDataTypes.h"

namespace mgrrenderer
{

class FileUtility
{
// windowsàÀë∂Ç∆êÿÇËó£Ç∑ÇÃÇÕå„âÒÇµ
public:
	static FileUtility* getInstance();

	std::string getFullPathForFileName(const std::string& fileName) const;
	static bool isAbsolutePath(const std::string& path);
	bool isFileExistInternal(const std::string& path) const;
	unsigned char* getFileData(const std::string& fileName, ssize_t* size) const;
	static bool isValidFileNameAtWindows(const std::string& fullPath, const std::string& fileName);
	std::string getStringFromFile(const std::string& fileName) const;
	static std::string convertPathFormatToUnixStyle(const std::string& path);

private:
	static const int MAX_PATH_LENGTH = 512;
	static FileUtility* _instance;
	std::string _resourceRootPath;

	FileUtility();
};

} // namespace mgrrenderer
