#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include "Defines.h"

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
		std::string creator;
		RankStatus status = RankStatus::Pending;
		int modes = 0; // bitmask: 1=std, 2=taiko, 4=catch, 8=mania
	};
	struct OnlineBeatmapListing {
		std::vector<OnlineBeatmap> Beatmaps;
		int Offset = 0;
		int TotalCount = 0;
	};
	struct OnlineSearchQuery {
		std::string Keyword;
		int Offset = 0;
		int Limit = 20;
	};
	virtual OnlineBeatmapListing Search(OnlineSearchQuery query) = 0;
	struct DownloadingQuery {
		path song_path;
		int sid;
		std::string name;
	};
	struct DownloadingItem {
		int sid = 0;
		std::string name;
		double progress = 0.0;
		bool completed = false;
		bool failed = false;
		std::string error;
	};
	virtual void StartDownloading(DownloadingQuery query) = 0;
	virtual std::vector<DownloadingItem> GetDownloadingItems() = 0;
	virtual void CancelDownload(int sid) = 0;
	virtual ~IDownloadingProvider() = default;
};

class BeatmapDownloadingService : public IDownloadingProvider {
	std::vector<DownloadingItem> downloads;
	std::mutex downloadMutex;
	std::vector<std::thread> downloadThreads;
	std::atomic<bool> cancelRequested{ false };

	void DownloadWorker(DownloadingQuery query);

public:
	OnlineBeatmapListing Search(OnlineSearchQuery query) override;
	void StartDownloading(DownloadingQuery query) override;
	std::vector<DownloadingItem> GetDownloadingItems() override;
	void CancelDownload(int sid) override;
	~BeatmapDownloadingService();
};
