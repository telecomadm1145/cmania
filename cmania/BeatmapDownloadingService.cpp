#include "BeatmapDownloadingService.h"
#include "HttpClient.h"
#include "OszExtractor.h"
#include <sstream>
#include <algorithm>

// Simple JSON parsing helpers (no external dependency)
// Sayobot API returns simple JSON that we can parse manually

static std::string GetJsonString(const std::string& json, const std::string& key) {
	std::string searchKey = "\"" + key + "\"";
	size_t pos = json.find(searchKey);
	if (pos == std::string::npos)
		return "";

	pos = json.find(':', pos);
	if (pos == std::string::npos)
		return "";

	// Skip whitespace
	pos++;
	while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
		pos++;

	if (pos >= json.size())
		return "";

	if (json[pos] == '"') {
		// String value
		pos++;
		size_t end = pos;
		while (end < json.size() && json[end] != '"') {
			if (json[end] == '\\' && end + 1 < json.size())
				end += 2;
			else
				end++;
		}
		return json.substr(pos, end - pos);
	}
	else if (json[pos] == 'n' && json.substr(pos, 4) == "null") {
		return "";
	}
	else {
		// Number or other
		size_t end = pos;
		while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != ']')
			end++;
		std::string result = json.substr(pos, end - pos);
		// Trim whitespace
		while (!result.empty() && (result.back() == ' ' || result.back() == '\t' || result.back() == '\r' || result.back() == '\n'))
			result.pop_back();
		return result;
	}
}

static long long GetJsonInt(const std::string& json, const std::string& key) {
	std::string val = GetJsonString(json, key);
	if (val.empty())
		return 0;
	try {
		return std::stoll(val);
	}
	catch (...) {
		return 0;
	}
}

static std::vector<std::string> SplitJsonArray(const std::string& json, const std::string& key) {
	std::vector<std::string> result;

	std::string searchKey = "\"" + key + "\"";
	size_t pos = json.find(searchKey);
	if (pos == std::string::npos)
		return result;

	pos = json.find('[', pos);
	if (pos == std::string::npos)
		return result;

	pos++; // Skip '['
	int depth = 1;
	size_t itemStart = pos;

	while (pos < json.size() && depth > 0) {
		char c = json[pos];
		if (c == '{') {
			if (depth == 1)
				itemStart = pos;
			depth++;
		}
		else if (c == '}') {
			depth--;
			if (depth == 1) {
				result.push_back(json.substr(itemStart, pos - itemStart + 1));
			}
		}
		else if (c == '[') {
			depth++;
		}
		else if (c == ']') {
			depth--;
		}
		pos++;
	}

	return result;
}

IDownloadingProvider::OnlineBeatmapListing BeatmapDownloadingService::Search(OnlineSearchQuery query) {
	OnlineBeatmapListing result;

	// Sayobot API: POST https://api.sayobot.cn/?post=
	// Body: {"cmd":"beatmaplist","limit":25,"offset":0,"type":"hot"/"search","keyword":"..."}
	std::string url = "https://api.sayobot.cn/?post=";

	std::string body = "{";
	body += "\"cmd\":\"beatmaplist\",";
	body += "\"limit\":" + std::to_string(query.Limit) + ",";
	body += "\"offset\":" + std::to_string(query.Offset) + ",";

	if (query.Keyword.empty()) {
		body += "\"type\":\"hot\"";
	}
	else {
		body += "\"type\":\"search\",";
		body += "\"keyword\":\"" + query.Keyword + "\"";
	}
	body += "}";

	HttpResponse response = HttpClient::Post(url, body);
	if (!response.Success()) {
		return result;
	}

	// Parse response
	// Format: {"status":0,"endid":xxx,"data":[{...}]}
	result.TotalCount = (int)GetJsonInt(response.Body, "endid");
	result.Offset = query.Offset;

	std::vector<std::string> beatmaps = SplitJsonArray(response.Body, "data");
	for (const auto& bmpJson : beatmaps) {
		OnlineBeatmap bmp;
		bmp.Id = GetJsonInt(bmpJson, "sid");
		bmp.title = GetJsonString(bmpJson, "title");
		bmp.titleU = GetJsonString(bmpJson, "titleU");
		bmp.artist = GetJsonString(bmpJson, "artist");
		bmp.artistU = GetJsonString(bmpJson, "artistU");
		bmp.creator = GetJsonString(bmpJson, "creator");

		int approved = (int)GetJsonInt(bmpJson, "approved");
		switch (approved) {
		case -2:
			bmp.status = RankStatus::Graveyard;
			break;
		case -1:
			bmp.status = RankStatus::WIP;
			break;
		case 0:
			bmp.status = RankStatus::Pending;
			break;
		case 1:
			bmp.status = RankStatus::Ranked;
			break;
		case 2:
			bmp.status = RankStatus::Approved;
			break;
		case 3:
			bmp.status = RankStatus::Qualified;
			break;
		case 4:
			bmp.status = RankStatus::Loved;
			break;
		default:
			bmp.status = RankStatus::Pending;
			break;
		}

		bmp.modes = (int)GetJsonInt(bmpJson, "modes");

		result.Beatmaps.push_back(bmp);
	}

	return result;
}

void BeatmapDownloadingService::DownloadWorker(DownloadingQuery query) {
	// Find or create download item
	{
		std::lock_guard<std::mutex> lock(downloadMutex);
		bool found = false;
		for (auto& item : downloads) {
			if (item.sid == query.sid) {
				found = true;
				break;
			}
		}
		if (!found) {
			DownloadingItem item;
			item.sid = query.sid;
			item.name = query.name;
			item.progress = 0.0;
			downloads.push_back(item);
		}
	}

	// Download URL: https://txy1.sayobot.cn/beatmaps/download/full/[sid]
	std::string url = "https://txy1.sayobot.cn/beatmaps/download/full/" + std::to_string(query.sid);

	path oszPath = query.song_path / (std::to_string(query.sid) + ".osz");

	bool success = HttpClient::DownloadFile(url, oszPath, [this, &query](double progress, size_t, size_t) {
		std::lock_guard<std::mutex> lock(downloadMutex);
		for (auto& item : downloads) {
			if (item.sid == query.sid) {
				item.progress = progress * 0.9; // 90% for download, 10% for extraction
				break;
			}
		}
	});

	if (!success || cancelRequested) {
		std::lock_guard<std::mutex> lock(downloadMutex);
		for (auto& item : downloads) {
			if (item.sid == query.sid) {
				item.failed = true;
				item.error = "Download failed";
				break;
			}
		}
		return;
	}


	// Sanitize path
	auto sanitize = [](std::string s) {
		for (char& c : s) {
			if (c == '<' || c == '>' || c == ':' || c == '"' || c == '/' || c == '\\' || c == '|' || c == '?' || c == '*') {
				c = '_';
			}
		}
		return s;
	};

	// Extract osz
	path destDir = query.song_path / (std::to_string(query.sid) + " " + sanitize(query.name));
	success = OszExtractor::Extract(oszPath, destDir);

	// Update status
	{
		std::lock_guard<std::mutex> lock(downloadMutex);
		for (auto& item : downloads) {
			if (item.sid == query.sid) {
				if (success) {
					item.progress = 1.0;
					item.completed = true;
				}
				else {
					item.failed = true;
					item.error = "Extraction failed";
				}
				break;
			}
		}
	}

	// Delete osz file after extraction
	if (success) {
		std::error_code ec;
		std::filesystem::remove(oszPath, ec);
	}
}

void BeatmapDownloadingService::StartDownloading(DownloadingQuery query) {
	downloadThreads.emplace_back(&BeatmapDownloadingService::DownloadWorker, this, query);
}

std::vector<IDownloadingProvider::DownloadingItem> BeatmapDownloadingService::GetDownloadingItems() {
	std::lock_guard<std::mutex> lock(downloadMutex);
	return downloads;
}

void BeatmapDownloadingService::CancelDownload(int sid) {
	cancelRequested = true;
	std::lock_guard<std::mutex> lock(downloadMutex);
	for (auto& item : downloads) {
		if (item.sid == sid && !item.completed) {
			item.failed = true;
			item.error = "Cancelled";
		}
	}
}

BeatmapDownloadingService::~BeatmapDownloadingService() {
	cancelRequested = true;
	for (auto& t : downloadThreads) {
		if (t.joinable())
			t.join();
	}
}
