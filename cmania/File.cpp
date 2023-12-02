#include <vector>
#include <string>
#include <Windows.h>
#include "File.h"

std::vector<char> ReadAllBytes(const std::string& filePath) {
	std::vector<char> buffer;

	HANDLE fileHandle = CreateFileA(
		filePath.c_str(),	   // File path
		GENERIC_READ,		   // Read access
		FILE_SHARE_READ,	   // Share mode
		NULL,				   // Security attributes
		OPEN_EXISTING,		   // Open existing file
		FILE_ATTRIBUTE_NORMAL, // File attributes
		NULL				   // Template file handle
	);

	if (fileHandle == INVALID_HANDLE_VALUE) {
		return buffer;
	}

	DWORD fileSize = GetFileSize(fileHandle, NULL);
	if (fileSize == INVALID_FILE_SIZE) {
		CloseHandle(fileHandle);
		return buffer;
	}

	buffer.resize(fileSize);

	DWORD bytesRead;
	if (!ReadFile(fileHandle, buffer.data(), fileSize, &bytesRead, NULL)) {
		CloseHandle(fileHandle);
		return buffer;
	}

	CloseHandle(fileHandle);

	return buffer;
}

std::vector<char> GetAvaliableDrives() {
	std::vector<char> res{};
	DWORD drives = GetLogicalDrives();
	for (char i = 'A'; i <= 'Z'; i++) {
		if (drives & 1) {
			res.push_back(i);
		}
		drives >>= 1;
	}
	return res;
}
