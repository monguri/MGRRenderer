#include "FileUtility.h"
#include <Shlobj.h>
#include <cstdlib>

namespace mgrrenderer
{

FileUtility* FileUtility::_instance = nullptr;

FileUtility* FileUtility::getInstance()
{
	if (_instance == nullptr)
	{
		_instance = new FileUtility();
	}

	return _instance;
}

FileUtility::FileUtility()
{
	WCHAR* utf16ExePath = nullptr;
	_get_wpgmptr(&utf16ExePath);

	// We need only directory part without exe
	WCHAR* utf16DirEnd = wcsrchr(utf16ExePath, L'\\');

	char utf8ExeDir[MAX_PATH_LENGTH] = { 0 };
	int num = WideCharToMultiByte(CP_UTF8, 0, utf16ExePath, utf16DirEnd - utf16ExePath + 1, utf8ExeDir, sizeof(utf8ExeDir), nullptr, nullptr);

	_resourceRootPath = convertPathFormatToUnixStyle(utf8ExeDir);
}

std::string FileUtility::convertPathFormatToUnixStyle(const std::string& path)
{
	std::string ret = path;
	ssize_t len = ret.length();
	for (int i = 0; i < (int)len; ++i)
	{
		if (ret[i] == '\\')
		{
			ret[i] = '/';
		}
	}
	return ret;
}

void FileUtility::convertWCHARFilePath(const std::string& inPath, WCHAR outPath[], size_t size)
{
	MultiByteToWideChar(CP_UTF8, 0, inPath.c_str(), -1, outPath, size);
}

std::string FileUtility::getFullPathForFileName(const std::string& fileName) const
{
	if (fileName.empty())
	{
		return "";
	}

	if (isAbsolutePath(fileName))
	{
		return fileName;
	}

	std::string ret = _resourceRootPath + fileName;

	// そのパスにファイルが存在するかどうか確認
	if (!isFileExistInternal(ret))
	{
		return "";
	}

	return ret;
}

bool FileUtility::isAbsolutePath(const std::string& path)
{
	if (path.length() > 2 &&
		(((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) && path[1] == ':')
		|| (path[0] == '/' && path[1] == '/')
		)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool FileUtility::isFileExistInternal(const std::string& path) const
{
	std::string strPath = path;

	if (!isAbsolutePath(strPath))
	{
		strPath.insert(0, _resourceRootPath);
	}

	WCHAR utf16buf[MAX_PATH_LENGTH] = {0};
	convertWCHARFilePath(path, utf16buf, MAX_PATH_LENGTH);

	DWORD attribute = GetFileAttributesW(utf16buf);
	if (attribute == INVALID_FILE_ATTRIBUTES || (attribute & FILE_ATTRIBUTE_DIRECTORY))
	{
		return false;
	}

	return true;
}

unsigned char* FileUtility::getFileData(const std::string& fileName, size_t* size, bool forString /* = false */) const
{
	unsigned char* ret = nullptr;
	*size = 0;

	std::string fullPath = getFullPathForFileName(fileName);

	// チェック。ダメでもログを吐くだけ。
	isValidFileNameAtWindows(fullPath, fileName);
	
	WCHAR wcharFullPath[MAX_PATH_LENGTH] = {0};
	convertWCHARFilePath(fullPath, wcharFullPath, MAX_PATH_LENGTH);

	// fopenに該当する処理
	HANDLE fileHandle = CreateFileW(wcharFullPath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, nullptr);
	if (fileHandle != INVALID_HANDLE_VALUE)
	{
		*size = GetFileSize(fileHandle, nullptr);

		if (forString)
		{
			// 0終端をつける。返すサイズは変えない
			ret = (unsigned char*)malloc(*size + 1);
			ret[*size] = '\0';
		}
		else
		{
			ret = (unsigned char*)malloc(*size);
		}

		DWORD sizeRead = 0;
		// freadに該当する処理
		BOOL successed = ReadFile(fileHandle, ret, *size, &sizeRead, nullptr);
		// fcloseに該当する処理
		CloseHandle(fileHandle);

		if (!successed)
		{
			free(ret);
			ret = nullptr;
		}
	}

	if (ret != nullptr)
	{
		std::string msg = "Get data from file(";
		std::string errCodeStr = "" + GetLastError();
		msg = msg + fileName + ") failed. error code is " + errCodeStr;
		Logger::log("%s", msg.c_str());
	}
	return ret;
}

bool FileUtility::isValidFileNameAtWindows(const std::string& fullPath, const std::string& fileName)
{
	// Windowsは大文字小文字区別しないでヒットしちゃうから、ここで大文字小文字区別しても該当するファイルがあるかチェックする
	// TODO:FileUtils-win32.cppのcheckFileNameから持ってきてるけど処理内容がよくわからん

	std::string& path = convertPathFormatToUnixStyle(fullPath);
	size_t pathLen = path.length();
	size_t nameLen = fileName.length();
	std::string& realName = std::string();

	while (path.length() >= pathLen - nameLen && path.length() > 2) // おそらく普通は一周しかしない。二周するケースが何なのか不明
	{
		WIN32_FIND_DATAA data;
		HANDLE h = FindFirstFileA(path.c_str(), &data); // FindFirstFileAだと、正確に大文字小文字も間違ってないファイル名がとれるのか？
		FindClose(h);

		if (h != INVALID_HANDLE_VALUE)
		{
			int fileLen = strlen(data.cFileName);
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				realName = "/" + realName;
			}
			realName = data.cFileName + realName;
			if (0 != strcmp(&path.c_str()[path.length() - fileLen], data.cFileName))
			{
				// パスから検索してきたファイルのファイル名と一致しないとき
				std::string msg = "File path error : \"";
				msg.append(fileName).append("\" the real name is: ").append(realName);

				// TODO:ちゃんとしたロガー作りたい
				Logger::log("%s", msg.c_str());
				return false;
			}
		}
		else
		{
			break;
		}

		do
		{
			path = path.substr(0, path.rfind("/"));
		} while (path.back() == '.');
	}

	return true;
}

std::string FileUtility::getStringFromFile(const std::string& fileName) const
{
	size_t size;
	const unsigned char* data = getFileData(fileName, &size, true);
	if (data == nullptr)
	{
		return "";
	}

	const std::string& ret = std::string((const char*)data, size);
	delete data;
	return ret;
}

} // namespace mgrrenderer

