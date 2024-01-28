#pragma once
#include <string>
#include <vector>

class IDownloadingProvider {
public:
	struct OnlineBeatmap {
		int bid = 0;
	};
	struct OnlineBeatmapListing {
		std::vector<OnlineBeatmap> Beatmaps;
		int Offset = 0;
	};
	struct OnlineSearchQuery {
		std::string Keyword;
	};
	OnlineBeatmapListing Search(OnlineSearchQuery query);
	void Download(int sid);
};
class BeatmapDownloadingService {
};
