module;
#pragma warning(disable:4244)
export module SongSelectScreen;
import "BassAudioManager.h";
import ScreenController;
import GameBuffer;
import BeatmapManagementService;
import Settings;
import <vector>;
import Unicode;
import String;
import AudioService;
import <chrono>;
import <mutex>;
import <tuple>;
import OsuMods;
import SettingsScreen;
import GameplayScreen;

export class SongSelectScreen : public Screen
{
	int h_cache = 0; int w_cache = 0;
	std::mutex LOCKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK;
	std::vector<wchar_t> search_buf;
	IAudioManager::IAudioStream* preview;
	bool mod_flyout;
	OsuMods mods;
	GameBuffer::Color difficultyToRGBColor(float difficulty) {
		std::vector<std::tuple<double, double, GameBuffer::Color>> ranges = {
			{0, 1, {128, 128, 128}},        // Gray
			{1, 2, {0, 0, 255}},             // Blue
			{2, 3, {0, 255, 255}},           // Yellow
			{3, 4, {255, 165, 0}},           // Orange
			{4, 5, {255, 0, 0}},             // Red
			{5, 10, {128, 0, 128}},          // Purple
			{10, std::numeric_limits<double>::infinity(), {128, 0, 128}}   // Dark Purple
		};

		// Iterate through the ranges and find the corresponding RGB color
		for (const auto& r : ranges) {
			if (std::get<0>(r) <= difficulty && difficulty < std::get<1>(r)) {
				auto clr = std::get<2>(r);
				clr.Alpha = 255;
				return clr;
			}
		}

		// If the difficulty does not fall within any range, return black color
		return { 255,0,0,0 };
	}
	virtual void Render(GameBuffer& buf)
	{
		std::lock_guard lock(LOCKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK);
		h_cache = buf.Height;
		w_cache = buf.Width;
		if (!ready)
		{
			if (require_songs_path)
			{
				buf.DrawString("请拖入Songs文件夹或者手动键入Songs文件夹路径.", 0, 0, {}, {});
				buf.DrawString(std::wstring{ input_buf.begin(),input_buf.end() }, 0, 1, {}, {});
				return;
			}
			buf.DrawString("Loading...", 0, 0, {}, {});
			return;
		}
		auto& caches = matched_caches;
		if (search_buf.empty())
		{
			caches = this->caches;
		}
		int c1 = 45;
		int c2 = std::min((int)(buf.Width / 2), 10) + c1;
		int gap = 1;
		int songheight = 3;
		int originpointx = c1 + buf.Width;
		int originpointy = (int)(buf.Height / 2);
		double startangle = calculateAngle(buf.Width, 0, originpointx, originpointy); //起始角度
		double endangle = calculateAngle(buf.Width, buf.Height, originpointx, originpointy);
		double distance = sqrt(pow(c1, 2) + pow(buf.Height / 2, 2));

		int index = (int)(offset / (songheight + gap) + buf.Height / 2);
		int max = (int)(buf.Height / (songheight + gap) + 10);
		int min = index - h_cache / 2 - 5 - (selected_entry != 0 ? selected_entry->difficulties.size() : 0);
		if (caches.empty())
		{
			buf.DrawString("空空如也", buf.Width - 30, buf.Height / 2, {}, {});
		}
		else {
			for (int i = min; i < index + max; i++)
			{
				int j = abs((int)(i % caches.size()));
				auto& cache = caches[j];
				auto basicoff = (i * (songheight + gap) - offset);
				if (i > selected && selected_entry != 0)
				{
					basicoff += selected_entry->difficulties.size() * (songheight + gap);
				}
				double b = distance * sin(basicoff / buf.Height * (startangle - endangle) - startangle);
				int b2 = buf.Width + abs(b) - c2;
				buf.FillRect(b2, basicoff, b2 + c2, basicoff + songheight, { {},{120,128,128,128} ,' ' }); //左上右下
				buf.DrawString(cache.artist + " - " + cache.title, b2 + 1, basicoff + 1, {}, {});
				if (i == selected)
				{
					int k = 1;
					int diffxpos = 3;
					for (auto& diff : cache.difficulties)
					{
						auto diffoff = basicoff + (k * (songheight + gap));
						buf.FillRect(b2 + diffxpos, diffoff, b2 + c2 + diffxpos, diffoff + songheight, { {},{160,128,128,128} ,' ' });
						buf.DrawString(diff.name, b2 + 1 + diffxpos, diffoff + 1, {}, {});
						k++;
					}
				}
			}
		}
		buf.FillPolygon({ {0,2},{50,2},{40,12},{0,12} }, { {},{130,128,128,128},' ' });
		if (selected_entry != 0)
		{
			buf.DrawString(selected_entry->titleunicode, 1, 3, {}, {});
			buf.DrawString(selected_entry->artistunicode, 1, 5, {}, {});
			if (selected_entry_2 != 0)
			{
				auto npstext = std::to_string(selected_entry_2->nps);
				npstext.resize(npstext.find('.') + 2);
				auto clr = difficultyToRGBColor(sqrt(selected_entry_2->nps));
				clr.Alpha = 255;
				buf.DrawString(npstext, 48 - npstext.size(), 3, GameBuffer::Color::Blend(clr, { 64,255,255,255 }), clr);
				auto minutes = int(selected_entry_2->length / (1000 * 60));
				auto seconds = int(selected_entry_2->length / 1000) % 60;
				auto info = std::to_string(minutes) + ":" + std::to_string(seconds) + "   " + std::to_string(int(selected_entry_2->keys)) + "K" + "   OD" + std::to_string(int(selected_entry_2->od));
				buf.DrawString(info, 1, 10, {}, {});

				// records
				buf.DrawString("Personal Best: 0", 1, 14, {}, {});
			}
		}
		else
		{
			buf.DrawString("No song selected.", 1, 3, {}, {});
		}
		buf.DrawString("Esc - 返回 上下左右/鼠标 - 选歌 F2 Mods F3 选项 Enter 进入", 0, buf.Height - 1, {}, {});
		buf.FillRect(buf.Width - 30, 2, buf.Width - 3, 2, { {},{130,128,128,128},' ' });
		buf.DrawString("搜索:", buf.Width - 30, 2, { 255,100,255,150 }, {});
		buf.DrawString(std::wstring{ search_buf.begin(),search_buf.end() }, buf.Width - 24, 2, { 255,100,255,150 }, {});
		if (mod_flyout)
		{
			buf.FillRect(0, 0, buf.Width, buf.Height, { {},{170,20,20,20},' ' });
			buf.DrawString("Mod 选 择", 5, 6, {}, {});
			buf.DrawString("Mod 提供了一种让别人相当目害的游戏体验，可以提高或者降(变相)低(提高)游戏难度", 5, 8, {}, {});
			buf.DrawString("难度降低: (E)asy (N)oFall (H)alfTime", 5, 12, {}, {});
			buf.DrawString("难度提高: (H)ardRock N(i)ghtcore ", 5, 16, {}, {});
			buf.DrawString("特殊:     (1-10)Keys (C)o-op (A)uto (M)irror N(o)Jump", 5, 20, {}, {});
			buf.DrawString("Esc - 返回", 0, buf.Height - 1, {}, {});
		}
	}
	double offset = 0;
	int selected = INT_MAX;
	SongsCacheEntry* selected_entry;
	DifficultyCacheEntry* selected_entry_2;
	double calculateAngle(double x1, double y1, double x2, double y2) {
		double deltaX = x2 - x1;
		double deltaY = y2 - y1;
		double angleRad = atan2(deltaY, deltaX);
		return angleRad;
	}
	std::vector<SongsCacheEntry> caches;
	std::vector<SongsCacheEntry> matched_caches;
	bool ready;
	bool require_songs_path;
	std::vector<wchar_t> input_buf;
	virtual void Activate(bool y)
	{
		if (y)
		{
			if (caches.empty())
			{
				RebuildCache();
			}
		}
		else
		{
			if (preview != 0)
			{
				delete preview;
				preview = 0;
			}
		}
	}
	virtual void Wheel(WheelEventArgs wea)
	{
		if (!mod_flyout)
			offset += wea.Delta;
	}
	virtual void Move(MoveEventArgs mea)
	{
		if (!mod_flyout)
			if (last_y >= 0)
			{
				offset = last_offset - mea.Y + last_y;
			}
	}
	void PlayPreview()
	{
		if (preview != 0)
		{
			preview->stop();
			delete preview;
			preview = 0;
		}
		DifficultyCacheEntry* entry = selected_entry_2;
		if (entry == 0)
		{
			if (selected_entry == 0)
				return;
			entry = &selected_entry->difficulties[0];
		}
		LoadEventArgs lea{};
		lea.requested_file = entry->audio.c_str();
		lea.callback = new std::function<void(const LoadCompleteEventArgs&)>([this, entry](const LoadCompleteEventArgs& lcea) { preview = lcea.stream; preview->play(); preview->setCurrent(std::max(entry->preview / 1000, 0.0)); });
		game->Raise("load", lea);
	}
	int last_x; int last_y = -999;
	double last_offset;
	void MakeSelected(int i, auto& cache)
	{
		selected = i;
		if (i != INT_MAX)
		{
			selected_entry = &cache;
		}
		selected_entry_2 = 0;
		PlayPreview();
	}
	void MakeSelected(int i, auto& cache, auto& diff)
	{
		selected = i;
		if (i != INT_MAX)
		{
			selected_entry = &cache;
			selected_entry_2 = &diff;
		}
		PlayPreview();
	}
	virtual void MouseKey(MouseKeyEventArgs mkea)
	{
		if (mod_flyout)
		{
			return;
		}
		if (mkea.Pressed)
		{
			last_x = mkea.X;
			last_y = mkea.Y;
			last_offset = offset;
		}
		else
		{
			if (mkea.X == last_x && mkea.Y == last_y && !caches.empty())
			{
				auto& caches = matched_caches;
				if (search_buf.empty())
				{
					caches = this->caches;
				}
				if (caches.empty())
					return;
				int c1 = 45;
				int c2 = std::min((int)(w_cache / 2), 10) + c1;
				int gap = 1;
				int songheight = 3;
				int originpointx = c1 + w_cache;
				int originpointy = (int)(h_cache / 2);
				double startangle = calculateAngle(w_cache, 0, originpointx, originpointy); //起始角度
				double endangle = calculateAngle(w_cache, h_cache, originpointx, originpointy);
				double distance = sqrt(pow(c1, 2) + pow(h_cache / 2, 2));

				int index = (int)(offset / (songheight + gap) + h_cache / 2);
				int max = (int)(h_cache / (songheight + gap) + 10);
				int min = index - h_cache / 2 - 5 - (selected_entry != 0 ? selected_entry->difficulties.size() : 0);
				for (int i = min; i < index + max; i++)
				{
					int j = std::abs((int)(i % caches.size()));
					auto& cache = caches[j];
					auto basicoff = (i * (songheight + gap) - offset);
					if (i > selected && selected_entry != 0)
					{
						basicoff += selected_entry->difficulties.size() * (songheight + gap);
					}
					double b = distance * sin(basicoff / h_cache * (startangle - endangle) - startangle);
					int b2 = w_cache + abs(b) - c2;
					if (mkea.X >= b2 && mkea.Y >= basicoff && mkea.Y <= basicoff + songheight)
					{
						MakeSelected(i, caches[j]);
						break;
					}
					if (i == selected)
					{
						int k = 1;
						int diffxpos = 3;
						for (auto& diff : cache.difficulties)
						{
							auto diffoff = basicoff + (k * (songheight + gap));
							if (mkea.X >= b2 + diffxpos && mkea.Y >= diffoff && mkea.Y <= diffoff + songheight)
							{
								MakeSelected(i, cache, diff);
							}
							k++;
						}
					}
				}
			}
			last_y = -999;
		}
	}
	virtual void ProcessEvent(const char* evt, const void* evtargs)
	{
		if (strcmp(evt, "songs_cache_ready") == 0)
		{
			auto& screa = *(SongsCahceReadyEventArgs*)evtargs;
			caches = screa.Songs;
			ready = true;
		}
		if (strcmp(evt, "require") == 0)
		{
			auto str = (const char*)evtargs;
			if (strcmp(str, "songs_path") == 0)
			{
				input_buf.resize(0);
				require_songs_path = true;
			}
		}
	}
	void RebuildCache()
	{
		ready = false;
		game->Raise("get_songs_cache");
	}
	void UpdateSearch()
	{
		selected = INT_MAX;
		selected_entry = 0;
		matched_caches.clear();
		std::string str = Utf162Utf8(std::wstring{ search_buf.begin(),search_buf.end() });
		std::copy_if(caches.begin(), caches.end(), std::back_inserter(matched_caches), [this, str](const SongsCacheEntry& sce) {
			return search_meta(str, sce.artist, sce.title, sce.artistunicode, sce.titleunicode, sce.tags, sce.source);
			});
	}
	virtual void Key(KeyEventArgs kea)
	{
		std::lock_guard lock(LOCKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK);
		if (kea.Pressed)
		{
			if (mod_flyout)
			{
				if (kea.Key == ConsoleKey::Escape)
				{
					mod_flyout = !mod_flyout;
				}
				return;
			}
			if (kea.Key == ConsoleKey::Escape)
			{
				parent->Back();
			}
			if (!ready && require_songs_path)
			{
				if (kea.Key == ConsoleKey::Backspace)
				{
					if (input_buf.size() > 0)
					{
						input_buf.resize(input_buf.size() - 1);
					}
					return;
				}
				if (kea.Key == ConsoleKey::Enter)
				{
					SetEventArgs sea{ "songs_path",Utf162Utf8(std::wstring{input_buf.begin(),input_buf.end()}) };
					game->Raise("set", sea);
					RebuildCache();
					require_songs_path = false;
				}
				if (kea.UnicodeChar >= 31)
				{
					input_buf.push_back(kea.UnicodeChar);
				}
				return;
			}
			{
				if (kea.Key == ConsoleKey::Backspace)
				{
					if (search_buf.size() >= 2 && IsMultiUtf16(search_buf[search_buf.size() - 2]) && IsMultiUtf16(search_buf[search_buf.size() - 1]))
					{
						search_buf.resize(search_buf.size() - 2);
						UpdateSearch();
					}
					else if (search_buf.size() >= 1)
					{
						search_buf.resize(search_buf.size() - 1);
						UpdateSearch();
					}
					return;
				}
				if (kea.UnicodeChar > 31)
				{
					search_buf.push_back(kea.UnicodeChar);
					if (!IsMultiUtf16(kea.UnicodeChar))
					{
						UpdateSearch();
					}
				}
			}
			if (kea.Key == ConsoleKey::F2)
			{
				mod_flyout = !mod_flyout;
			}
			if (kea.Key == ConsoleKey::F3)
			{
				parent->Navigate<SettingsScreen>();
			}
			if (kea.Key == ConsoleKey::Enter)
			{
				if (selected_entry_2 != 0 && selected != INT_MAX)
				{
					parent->Navigate(new GameplayScreen(selected_entry_2->path, mods));
				}
			}
		}
	}
};