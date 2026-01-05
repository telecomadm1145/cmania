#include "HttpClient.h"
#include <sstream>
#include <iomanip>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

HttpResponse HttpClient::Get(const std::string& url) {
	return Get(url, {});
}

HttpResponse HttpClient::Get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers) {
	HttpResponse response;

	// Parse URL
	std::wstring wurl(url.begin(), url.end());

	URL_COMPONENTS urlComp = {};
	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.dwSchemeLength = -1;
	urlComp.dwHostNameLength = -1;
	urlComp.dwUrlPathLength = -1;
	urlComp.dwExtraInfoLength = -1;

	if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.length(), 0, &urlComp)) {
		response.Error = "Failed to parse URL";
		return response;
	}

	std::wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
	std::wstring urlPath(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
	if (urlComp.dwExtraInfoLength > 0) {
		urlPath += std::wstring(urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
	}

	HINTERNET hSession = WinHttpOpen(L"cmania/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		response.Error = "Failed to open WinHTTP session";
		return response;
	}

	HINTERNET hConnect = WinHttpConnect(hSession, hostName.c_str(), urlComp.nPort, 0);
	if (!hConnect) {
		WinHttpCloseHandle(hSession);
		response.Error = "Failed to connect";
		return response;
	}

	DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath.c_str(),
		NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if (!hRequest) {
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		response.Error = "Failed to open request";
		return response;
	}

	// Add custom headers
	for (const auto& header : headers) {
		std::wstring headerLine(header.first.begin(), header.first.end());
		headerLine += L": ";
		headerLine += std::wstring(header.second.begin(), header.second.end());
		WinHttpAddRequestHeaders(hRequest, headerLine.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
	}

	if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		response.Error = "Failed to send request";
		return response;
	}

	if (!WinHttpReceiveResponse(hRequest, NULL)) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		response.Error = "Failed to receive response";
		return response;
	}

	// Get status code
	DWORD statusCode = 0;
	DWORD statusCodeSize = sizeof(statusCode);
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
	response.StatusCode = statusCode;

	// Read body
	std::string body;
	DWORD bytesAvailable = 0;
	do {
		bytesAvailable = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
			break;
		if (bytesAvailable == 0)
			break;

		std::vector<char> buffer(bytesAvailable);
		DWORD bytesRead = 0;
		if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
			body.append(buffer.data(), bytesRead);
		}
	} while (bytesAvailable > 0);

	response.Body = body;

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return response;
}

HttpResponse HttpClient::Post(const std::string& url, const std::string& body, const std::string& contentType) {
	HttpResponse response;

	// Parse URL
	std::wstring wurl(url.begin(), url.end());

	URL_COMPONENTS urlComp = {};
	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.dwSchemeLength = -1;
	urlComp.dwHostNameLength = -1;
	urlComp.dwUrlPathLength = -1;
	urlComp.dwExtraInfoLength = -1;

	if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.length(), 0, &urlComp)) {
		response.Error = "Failed to parse URL";
		return response;
	}

	std::wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
	std::wstring urlPath(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
	if (urlComp.dwExtraInfoLength > 0) {
		urlPath += std::wstring(urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
	}

	HINTERNET hSession = WinHttpOpen(L"cmania/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		response.Error = "Failed to open WinHTTP session";
		return response;
	}

	HINTERNET hConnect = WinHttpConnect(hSession, hostName.c_str(), urlComp.nPort, 0);
	if (!hConnect) {
		WinHttpCloseHandle(hSession);
		response.Error = "Failed to connect";
		return response;
	}

	DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlPath.c_str(),
		NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if (!hRequest) {
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		response.Error = "Failed to open request";
		return response;
	}

	// Add Content-Type header
	std::wstring wContentType(contentType.begin(), contentType.end());
	std::wstring header = L"Content-Type: " + wContentType;
	WinHttpAddRequestHeaders(hRequest, header.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

	if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			(LPVOID)body.c_str(), (DWORD)body.length(), (DWORD)body.length(), 0)) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		response.Error = "Failed to send request";
		return response;
	}

	if (!WinHttpReceiveResponse(hRequest, NULL)) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		response.Error = "Failed to receive response";
		return response;
	}

	// Get status code
	DWORD statusCode = 0;
	DWORD statusCodeSize = sizeof(statusCode);
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);
	response.StatusCode = statusCode;

	// Read body
	std::string respBody;
	DWORD bytesAvailable = 0;
	do {
		bytesAvailable = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
			break;
		if (bytesAvailable == 0)
			break;

		std::vector<char> buffer(bytesAvailable);
		DWORD bytesRead = 0;
		if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
			respBody.append(buffer.data(), bytesRead);
		}
	} while (bytesAvailable > 0);

	response.Body = respBody;

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return response;
}

bool HttpClient::DownloadFile(const std::string& url, const std::filesystem::path& destPath,
	std::function<void(double progress, size_t downloaded, size_t total)> progressCallback) {

	// Parse URL
	std::wstring wurl(url.begin(), url.end());

	URL_COMPONENTS urlComp = {};
	urlComp.dwStructSize = sizeof(urlComp);
	urlComp.dwSchemeLength = -1;
	urlComp.dwHostNameLength = -1;
	urlComp.dwUrlPathLength = -1;
	urlComp.dwExtraInfoLength = -1;

	if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.length(), 0, &urlComp)) {
		return false;
	}

	std::wstring hostName(urlComp.lpszHostName, urlComp.dwHostNameLength);
	std::wstring urlPath(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
	if (urlComp.dwExtraInfoLength > 0) {
		urlPath += std::wstring(urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
	}

	HINTERNET hSession = WinHttpOpen(L"cmania/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession)
		return false;

	HINTERNET hConnect = WinHttpConnect(hSession, hostName.c_str(), urlComp.nPort, 0);
	if (!hConnect) {
		WinHttpCloseHandle(hSession);
		return false;
	}

	DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath.c_str(),
		NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if (!hRequest) {
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	if (!WinHttpReceiveResponse(hRequest, NULL)) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	// Get content length
	DWORD contentLength = 0;
	DWORD contentLengthSize = sizeof(contentLength);
	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
		WINHTTP_HEADER_NAME_BY_INDEX, &contentLength, &contentLengthSize, WINHTTP_NO_HEADER_INDEX);

	// Open destination file
	std::ofstream file(destPath, std::ios::binary);
	if (!file.is_open()) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		return false;
	}

	// Download with progress
	size_t totalDownloaded = 0;
	DWORD bytesAvailable = 0;
	do {
		bytesAvailable = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
			break;
		if (bytesAvailable == 0)
			break;

		std::vector<char> buffer(bytesAvailable);
		DWORD bytesRead = 0;
		if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
			file.write(buffer.data(), bytesRead);
			totalDownloaded += bytesRead;

			if (progressCallback && contentLength > 0) {
				double progress = (double)totalDownloaded / (double)contentLength;
				progressCallback(progress, totalDownloaded, contentLength);
			}
		}
	} while (bytesAvailable > 0);

	file.close();

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);

	return true;
}

#else
// Linux implementation using libcurl
#include <curl/curl.h>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
	size_t totalSize = size * nmemb;
	userp->append((char*)contents, totalSize);
	return totalSize;
}

HttpResponse HttpClient::Get(const std::string& url) {
	return Get(url, {});
}

HttpResponse HttpClient::Get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers) {
	HttpResponse response;

	CURL* curl = curl_easy_init();
	if (!curl) {
		response.Error = "Failed to initialize curl";
		return response;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.Body);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

	struct curl_slist* headerList = NULL;
	for (const auto& header : headers) {
		std::string headerLine = header.first + ": " + header.second;
		headerList = curl_slist_append(headerList, headerLine.c_str());
	}
	if (headerList) {
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
	}

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		response.Error = curl_easy_strerror(res);
	}
	else {
		long httpCode = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
		response.StatusCode = (int)httpCode;
	}

	if (headerList)
		curl_slist_free_all(headerList);
	curl_easy_cleanup(curl);

	return response;
}

HttpResponse HttpClient::Post(const std::string& url, const std::string& body, const std::string& contentType) {
	HttpResponse response;

	CURL* curl = curl_easy_init();
	if (!curl) {
		response.Error = "Failed to initialize curl";
		return response;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.Body);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.length());

	struct curl_slist* headerList = NULL;
	std::string headerLine = "Content-Type: " + contentType;
	headerList = curl_slist_append(headerList, headerLine.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		response.Error = curl_easy_strerror(res);
	}
	else {
		long httpCode = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
		response.StatusCode = (int)httpCode;
	}

	if (headerList)
		curl_slist_free_all(headerList);
	curl_easy_cleanup(curl);

	return response;
}

struct DownloadProgress {
	std::function<void(double, size_t, size_t)>* callback;
};

static size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, std::ofstream* file) {
	size_t totalSize = size * nmemb;
	file->write((char*)contents, totalSize);
	return totalSize;
}

static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t) {
	DownloadProgress* prog = (DownloadProgress*)clientp;
	if (prog->callback && dltotal > 0) {
		double progress = (double)dlnow / (double)dltotal;
		(*prog->callback)(progress, dlnow, dltotal);
	}
	return 0;
}

bool HttpClient::DownloadFile(const std::string& url, const std::filesystem::path& destPath,
	std::function<void(double progress, size_t downloaded, size_t total)> progressCallback) {

	CURL* curl = curl_easy_init();
	if (!curl)
		return false;

	std::ofstream file(destPath, std::ios::binary);
	if (!file.is_open()) {
		curl_easy_cleanup(curl);
		return false;
	}

	DownloadProgress prog;
	prog.callback = progressCallback ? &progressCallback : nullptr;

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);

	if (progressCallback) {
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	}

	CURLcode res = curl_easy_perform(curl);

	file.close();
	curl_easy_cleanup(curl);

	return res == CURLE_OK;
}

#endif

std::string HttpClient::UrlEncode(const std::string& str) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (char c : str) {
		if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped << c;
		}
		else {
			escaped << std::uppercase;
			escaped << '%' << std::setw(2) << int((unsigned char)c);
			escaped << std::nouppercase;
		}
	}

	return escaped.str();
}
