#pragma once
#include <string>
#include <vector>
#include "Defines.h"
#include <simdjson.h>
class IDownloadingProvider {
public:
	enum class RankStatus {
		Graveyard = -2,
		WIP = -1,
		Pending = 0,
		Ranked = 1,
		Approved = 2,
		Qualified = 3,
		Loved = 4,
	};
	struct OnlineBeatmap {
		ulonglong Id = 0;
		std::string title;
		std::string titleU;
		std::string artist;
		std::string artistU;
		RankStatus status;
	};
	struct OnlineBeatmapListing {
		std::vector<OnlineBeatmap> Beatmaps;
		int Offset = 0;
	};
	struct OnlineSearchQuery {
		std::string Keyword;
		int Offset = 0;
	};
	virtual OnlineBeatmapListing Search(OnlineSearchQuery query) = 0;
	struct DownloadingQuery {
		path song_path;
		int sid;
		std::string name;
	};
	struct DownloadingItem {
		int sid;
		std::string name;
		double progress;
	};
	virtual void StartDownloading(DownloadingQuery query) = 0;
	virtual std::vector<DownloadingItem> GetDownloadingItems() = 0;
	virtual void CancelDownload(int sid) = 0;
};
class BeatmapDownloadingService : public IDownloadingProvider {
public:
	void StartDownloading(DownloadingQuery query) override;
	std::vector<DownloadingItem> GetDownloadingItems() override;
	 void CancelDownload(int sid) override;
};
