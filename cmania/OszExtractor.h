#pragma once
#include <filesystem>
#include <functional>
#include <string>

class OszExtractor {
public:
	// Extract osz file to destination directory
	// Returns true on success
	static bool Extract(const std::filesystem::path& oszPath, const std::filesystem::path& destDir,
		std::function<void(const std::string& filename)> progressCallback = nullptr);
};
