#include "BeatmapDownloadingScreen.h"
#include "ScreenController.h"
#include "BeatmapDownloadingService.h"
#include "GameBuffer.h"
#include "Unicode.h"
#include "Animator.h"
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

// Decode JSON \uXXXX escape sequences to UTF-8
static std::string DecodeJsonUnicode(const std::string& input) {
	std::string result;
	result.reserve(input.size());

	for (size_t i = 0; i < input.size(); i++) {
		if (i + 5 < input.size() && input[i] == '\\' && input[i + 1] == 'u') {
			// Parse \uXXXX
			std::string hex = input.substr(i + 2, 4);
			try {
				unsigned int codepoint = std::stoul(hex, nullptr, 16);
				// Convert to UTF-8
				if (codepoint < 0x80) {
					result += (char)codepoint;
				}
				else if (codepoint < 0x800) {
					result += (char)(0xC0 | (codepoint >> 6));
					result += (char)(0x80 | (codepoint & 0x3F));
				}
				else {
					result += (char)(0xE0 | (codepoint >> 12));
					result += (char)(0x80 | ((codepoint >> 6) & 0x3F));
					result += (char)(0x80 | (codepoint & 0x3F));
				}
				i += 5; // Skip \uXXXX
			}
			catch (...) {
				result += input[i];
			}
		}
		else {
			result += input[i];
		}
	}
	return result;
}

class BeatmapDownloadingScreen : public Screen {
	BeatmapDownloadingService service;

	// Search state
	std::vector<wchar_t> searchBuf;
	std::string searchText;
	IDownloadingProvider::OnlineBeatmapListing searchResults;
	std::atomic<bool> isSearching{ false };
	std::mutex resultMutex;

	// Selection state
	int selectedIndex = 0;
	double scrollOffset = 0;
	int viewMode = 0; // 0 = search results, 1 = downloads

	// Search thread
	std::thread searchThread;
	std::atomic<bool> searchPending{ false };
	double lastSearchTime = 0;

	// Mouse drag state
	int lastMouseY = -999;
	double lastScrollOffset = 0;
	int itemHeight = 3; // Height per beatmap item
	int h_cache = 0;
	int w_cache = 0;

#ifdef __clang__
	class ConstantEasingDurationCalculator_200 {
	public:
		static inline auto Get(auto x) { return 200.0; }
	};
	using TransOut = Transition<EaseOut<CubicEasingFunction>, ConstantEasingDurationCalculator_200>;
#else
	using TransOut = Transition<EaseOut<CubicEasingFunction>, ConstantEasingDurationCalculator<200.0>>;
#endif
	TransOut scrollTrans{};

	void DoSearch() {
		isSearching = true;
		IDownloadingProvider::OnlineSearchQuery query;
		query.Keyword = searchText;
		query.Offset = 0;
		query.Limit = 50;

		auto results = service.Search(query);

		{
			std::lock_guard<std::mutex> lock(resultMutex);
			searchResults = results;
		}
		isSearching = false;
	}

	std::string GetStatusString(IDownloadingProvider::RankStatus status) {
		switch (status) {
		case IDownloadingProvider::RankStatus::Ranked:
			return "[Ranked]";
		case IDownloadingProvider::RankStatus::Approved:
			return "[Approved]";
		case IDownloadingProvider::RankStatus::Qualified:
			return "[Qualified]";
		case IDownloadingProvider::RankStatus::Loved:
			return "[Loved]";
		case IDownloadingProvider::RankStatus::Pending:
			return "[Pending]";
		case IDownloadingProvider::RankStatus::WIP:
			return "[WIP]";
		case IDownloadingProvider::RankStatus::Graveyard:
			return "[Graveyard]";
		default:
			return "[?]";
		}
	}

	std::string GetModesString(int modes) {
		std::string result;
		if (modes & 1)
			result += "Std ";
		if (modes & 2)
			result += "Taiko ";
		if (modes & 4)
			result += "Catch ";
		if (modes & 8)
			result += "Mania ";
		if (result.empty())
			result = "?";
		return result;
	}

public:
	virtual void Render(GameBuffer& buf) override {
		int w = buf.Width;
		int h = buf.Height;
		w_cache = w;
		h_cache = h;

		auto clk = HpetClock();
		scrollTrans.SetValue(clk, scrollOffset);
		double realScroll = scrollTrans.GetCurrentValue(clk);

		// Background
		buf.FillRect(0, 0, w, h, { {}, { 255, 25, 25, 35 }, ' ' });

		// Header area
		buf.FillRect(0, 0, w, 4, { {}, { 255, 35, 35, 50 }, ' ' });

		// Title
		buf.DrawString("Beatmap Download (Sayobot)", 2, 0, { 255, 255, 200, 100 }, {});

		// Search box
		std::string searchLabel = "Search: ";
		std::string searchDisplay = searchText;
		if (viewMode == 0) {
			searchDisplay += "_";
		}
		buf.DrawString(searchLabel + searchDisplay, 2, 1, { 255, 255, 255, 255 }, {});

		// Tab indicator
		std::string tabStr = viewMode == 0 ? "[Results]  Downloads " : " Results  [Downloads]";
		buf.DrawString(tabStr, w - 25, 0, { 255, 150, 150, 150 }, {});

		// Status
		if (isSearching) {
			buf.DrawString("Searching...", 2, 2, { 255, 255, 255, 100 }, {});
		}
		else {
			std::lock_guard<std::mutex> lock(resultMutex);
			buf.DrawString("Found: " + std::to_string(searchResults.Beatmaps.size()) + " beatmaps", 2, 2, { 255, 180, 180, 180 }, {});
		}

		int listY = 4;
		int listHeight = h - listY - 2;

		if (viewMode == 0) {
			// Search results
			std::lock_guard<std::mutex> lock(resultMutex);

			int startIdx = (int)(realScroll / itemHeight);
			int endIdx = startIdx + (listHeight / itemHeight) + 2;

			for (int idx = startIdx; idx < endIdx && idx < (int)searchResults.Beatmaps.size(); idx++) {
				if (idx < 0)
					continue;
				const auto& bmp = searchResults.Beatmaps[idx];

				int itemY = listY + idx * itemHeight - (int)realScroll;
				if (itemY < listY - itemHeight || itemY > h)
					continue;

				bool selected = (idx == selectedIndex);
				Color bg = selected ? Color{ 255, 70, 70, 120 } : Color{ 255, 40, 40, 55 };
				Color fg = selected ? Color{ 255, 255, 255, 255 } : Color{ 255, 200, 200, 200 };

				// Item background
				buf.FillRect(1, itemY, w - 1, itemY + itemHeight, { {}, bg, ' ' });

				// Title line
				std::string title = DecodeJsonUnicode(bmp.titleU.empty() ? bmp.title : bmp.titleU);
				std::string artist = DecodeJsonUnicode(bmp.artistU.empty() ? bmp.artist : bmp.artistU);
				std::string titleLine = artist + " - " + title;
				if (titleLine.size() > (size_t)(w - 4)) {
					titleLine = titleLine.substr(0, w - 7) + "...";
				}
				buf.DrawString(titleLine, 3, itemY, fg, {});

				// Info line
				std::string infoLine = GetStatusString(bmp.status) + "  " + GetModesString(bmp.modes);
				if (!bmp.creator.empty()) {
					infoLine += " by " + bmp.creator;
				}
				buf.DrawString(infoLine, 3, itemY + 1, { 255, 140, 140, 140 }, {});
			}

			if (searchResults.Beatmaps.empty() && !isSearching && !searchText.empty()) {
				buf.DrawString("No results found. Try a different search.", 4, listY + 2, { 255, 150, 150, 150 }, {});
			}
			if (searchResults.Beatmaps.empty() && searchText.empty()) {
				buf.DrawString("Type to search for beatmaps...", 4, listY + 2, { 255, 150, 150, 150 }, {});
			}
		}
		else {
			// Downloads list
			auto downloads = service.GetDownloadingItems();

			for (int i = 0; i < (int)downloads.size() && i * itemHeight < listHeight; i++) {
				const auto& dl = downloads[i];
				int itemY = listY + i * itemHeight;

				bool selected = (i == selectedIndex);
				Color bg = selected ? Color{ 255, 70, 70, 120 } : Color{ 255, 40, 40, 55 };
				Color fg = { 255, 255, 255, 255 };

				buf.FillRect(1, itemY, w - 1, itemY + itemHeight, { {}, bg, ' ' });

				std::string name = DecodeJsonUnicode(dl.name);
				buf.DrawString(name, 3, itemY, fg, {});

				if (dl.completed) {
					buf.DrawString("[Completed]", 3, itemY + 1, { 255, 100, 255, 100 }, {});
				}
				else if (dl.failed) {
					buf.DrawString("[Failed: " + dl.error + "]", 3, itemY + 1, { 255, 255, 100, 100 }, {});
				}
				else {
					// Progress bar
					int percent = (int)(dl.progress * 100);
					int barWidth = 30;
					int barX = 3;
					int filledWidth = (int)(barWidth * dl.progress);

					buf.FillRect(barX, itemY + 1, barX + filledWidth, itemY + 2, { {}, { 255, 100, 200, 100 }, ' ' });
					buf.FillRect(barX + filledWidth, itemY + 1, barX + barWidth, itemY + 2, { {}, { 255, 60, 60, 60 }, ' ' });
					buf.DrawString(std::to_string(percent) + "%", barX + barWidth + 2, itemY + 1, { 255, 180, 180, 180 }, {});
				}
			}

			if (downloads.empty()) {
				buf.DrawString("No active downloads", 4, listY + 2, { 255, 150, 150, 150 }, {});
			}
		}

		// Footer
		buf.FillRect(0, h - 2, w, h, { {}, { 255, 35, 35, 50 }, ' ' });
		buf.DrawString("[Esc] Back  [Tab] Switch  [Enter] Download  [Up/Down] Navigate", 2, h - 1, { 255, 150, 150, 150 }, {});
	}

	virtual void Key(KeyEventArgs kea) override {
		if (!kea.Pressed)
			return;

		if (kea.Key == ConsoleKey::Escape) {
			parent->Back();
			return;
		}

		if (kea.Key == ConsoleKey::Tab) {
			viewMode = (viewMode + 1) % 2;
			selectedIndex = 0;
			scrollOffset = 0;
			return;
		}

		if (viewMode == 0) {
			std::lock_guard<std::mutex> lock(resultMutex);

			if (kea.Key == ConsoleKey::DownArrow) {
				if (selectedIndex < (int)searchResults.Beatmaps.size() - 1) {
					selectedIndex++;
					// Auto scroll
					double itemBottom = (selectedIndex + 1) * itemHeight;
					double viewBottom = scrollOffset + (h_cache - 6);
					if (itemBottom > viewBottom) {
						scrollOffset = itemBottom - (h_cache - 6);
					}
				}
				return;
			}

			if (kea.Key == ConsoleKey::UpArrow) {
				if (selectedIndex > 0) {
					selectedIndex--;
					double itemTop = selectedIndex * itemHeight;
					if (itemTop < scrollOffset) {
						scrollOffset = itemTop;
					}
				}
				return;
			}

			if (kea.Key == ConsoleKey::PageDown) {
				scrollOffset += h_cache - 6;
				int maxScroll = (int)searchResults.Beatmaps.size() * itemHeight - (h_cache - 6);
				if (scrollOffset > maxScroll)
					scrollOffset = maxScroll;
				if (scrollOffset < 0)
					scrollOffset = 0;
				return;
			}

			if (kea.Key == ConsoleKey::PageUp) {
				scrollOffset -= h_cache - 6;
				if (scrollOffset < 0)
					scrollOffset = 0;
				return;
			}

			if (kea.Key == ConsoleKey::Enter) {
				if (selectedIndex >= 0 && selectedIndex < (int)searchResults.Beatmaps.size()) {
					const auto& bmp = searchResults.Beatmaps[selectedIndex];
					IDownloadingProvider::DownloadingQuery query;
					query.sid = (int)bmp.Id;
					query.name = DecodeJsonUnicode((bmp.artistU.empty() ? bmp.artist : bmp.artistU) + " - " + (bmp.titleU.empty() ? bmp.title : bmp.titleU));
					query.song_path = game->Settings["SongsPath"].GetString();
					service.StartDownloading(query);
					viewMode = 1;
				}
				return;
			}

			if (kea.Key == ConsoleKey::Backspace) {
				if (!searchBuf.empty()) {
					searchBuf.pop_back();
					searchText = Utf162Utf8(std::wstring(searchBuf.begin(), searchBuf.end()));
					searchPending = true;
				}
				return;
			}

			// Text input (outside lock to avoid issues)
		}

		if (viewMode == 0 && kea.UnicodeChar >= 32) {
			searchBuf.push_back((wchar_t)kea.UnicodeChar);
			searchText = Utf162Utf8(std::wstring(searchBuf.begin(), searchBuf.end()));
			searchPending = true;
			return;
		}

		if (viewMode == 1) {
			auto downloads = service.GetDownloadingItems();
			if (kea.Key == ConsoleKey::DownArrow && selectedIndex < (int)downloads.size() - 1) {
				selectedIndex++;
			}
			if (kea.Key == ConsoleKey::UpArrow && selectedIndex > 0) {
				selectedIndex--;
			}
			if ((kea.Key == ConsoleKey::Delete || kea.Key == ConsoleKey::X) && selectedIndex < (int)downloads.size()) {
				service.CancelDownload(downloads[selectedIndex].sid);
			}
		}
	}

	virtual void Wheel(WheelEventArgs wea) override {
		scrollOffset -= wea.Delta * 4;
		std::lock_guard<std::mutex> lock(resultMutex);
		int maxScroll = (int)searchResults.Beatmaps.size() * itemHeight - (h_cache - 6);
		if (scrollOffset > maxScroll)
			scrollOffset = maxScroll;
		if (scrollOffset < 0)
			scrollOffset = 0;
	}

	virtual void MouseKey(MouseKeyEventArgs mkea) override {
		if (mkea.Pressed) {
			lastMouseY = mkea.Y;
			lastScrollOffset = scrollOffset;
		}
		else {
			lastMouseY = -999;
		}
	}

	virtual void Move(MoveEventArgs mea) override {
		if (lastMouseY >= 0) {
			scrollOffset = lastScrollOffset - (mea.Y - lastMouseY) * 2;
			std::lock_guard<std::mutex> lock(resultMutex);
			int maxScroll = (int)searchResults.Beatmaps.size() * itemHeight - (h_cache - 6);
			if (scrollOffset > maxScroll)
				scrollOffset = maxScroll;
			if (scrollOffset < 0)
				scrollOffset = 0;
		}
	}

	virtual void Tick(double fromRun) override {
		if (searchPending && !isSearching) {
			if (fromRun - lastSearchTime > 0.5) {
				searchPending = false;
				lastSearchTime = fromRun;
				if (searchThread.joinable()) {
					searchThread.join();
				}
				searchThread = std::thread(&BeatmapDownloadingScreen::DoSearch, this);
			}
		}
	}

	~BeatmapDownloadingScreen() {
		if (searchThread.joinable()) {
			searchThread.join();
		}
	}
};

Screen* MakeBeatmapDownloadingScreen() {
	return new BeatmapDownloadingScreen();
}
