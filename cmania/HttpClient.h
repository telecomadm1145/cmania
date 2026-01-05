#pragma once
#include <string>
#include <functional>
#include <filesystem>
#include <vector>

struct HttpResponse {
	int StatusCode = 0;
	std::string Body;
	std::string Error;
	bool Success() const { return StatusCode >= 200 && StatusCode < 300 && Error.empty(); }
};

class HttpClient {
public:
	// Synchronous GET request
	static HttpResponse Get(const std::string& url);

	// Synchronous GET request with custom headers
	static HttpResponse Get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers);

	// Synchronous POST request with body and content type
	static HttpResponse Post(const std::string& url, const std::string& body, const std::string& contentType = "application/json");

	// Download file to path with optional progress callback (0.0 - 1.0)
	// Returns true on success
	static bool DownloadFile(const std::string& url, const std::filesystem::path& destPath,
		std::function<void(double progress, size_t downloaded, size_t total)> progressCallback = nullptr);

	// URL encode a string
	static std::string UrlEncode(const std::string& str);
};
