#include "BeatmapDownloadingService.h"

void BeatmapDownloadingService::StartDownloading(DownloadingQuery query) {
	simdjson::dom::parser parser{};
	simdjson::json_parse(std::string("{}"), parser);

}

std::vector<IDownloadingProvider::DownloadingItem> BeatmapDownloadingService::GetDownloadingItems() {
	return std::vector<DownloadingItem>();
}

void BeatmapDownloadingService::CancelDownload(int sid) {
}
