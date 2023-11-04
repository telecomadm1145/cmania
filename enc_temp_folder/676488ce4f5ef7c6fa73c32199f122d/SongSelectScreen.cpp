#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include "BassAudioManager.h"
#include "ScreenController.h"
#include "GameBuffer.h"
#include "BeatmapManagementService.h"
#include <vector>
#include "Unicode.h"
#include "String.h"
#include <chrono>
#include <mutex>
#include <tuple>
#include "OsuMods.h"
#include "File.h"
#include "SettingsScreen.h"
#include "BackgroundComponent.h"
#include "GameplayScreen.h"
#include "SongSelectScreen.h"
#include "Animator.h"

class SongSelectScreen : public Screen {
	BackgroundComponent bg;
	std::string bgloc;
	int h_cache = 0;
	int w_cache = 0;
	std::mutex res_lock;
	std::vector<wchar_t> search_buf;
	std::string audioloc;
	std::unique_ptr<IAudioManager::IAudioStream> preview;
	bool mod_flyout;
	OsuMods mods;
	using TransOut = Transition<EaseOut<PowerEasingFunction<12.0>>>;
	Color difficultyToRGBColor(float difficulty) {
		std::vector<std::tuple<double, double, Color>> ranges = {
			{ 0, 1, { 0, 204, 204, 204 } },								   // 灰
			{ 1, 2, { 0, 77, 230, 46 } },								   // 绿
			{ 2, 3, { 0, 61, 135, 204 } },								   // 蓝
			{ 3, 4, { 0, 230, 153, 46 } },								   // 橙
			{ 4, 5, { 0, 230, 55, 46 } },								   // Red
			{ 5, 6, { 0, 202, 46, 230 } },								   // Purple
			{ 6, 7, { 0, 108, 0, 127 } },								   // Dark Purple
			{ 8, std::numeric_limits<double>::infinity(), { 0, 0, 0, 0 } } // 黑
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
		return { 255, 0, 0, 0 };
	}
	TransOut OffsetTrans{ 0, 1500 };
	virtual void Render(GameBuffer& buf) {
		std::lock_guard lock(res_lock);
		h_cache = buf.Height;
		w_cache = buf.Width;
		if (!ready) {
			if (require_songs_path) {
				buf.DrawString("请拖入Songs文件夹或者手动键入Songs文件夹路径.", 0, 0, {}, {});
				buf.DrawString(std::wstring{ input_buf.begin(), input_buf.end() }, 0, 1, {}, {});
				return;
			}
			buf.DrawString("Loading...", 0, 0, {}, {});
			return;
		}
		auto& caches = matched_caches;
		if (search_buf.empty()) {
			caches = game->GetFeature<IBeatmapManagement>().GetSongsCache();
		}

		bg.Render(buf);
		int c1 = 45;
		int c2 = std::min((int)(buf.Width / 2), 10) + c1;
		int gap = 1;
		int songheight = 3;
		int originpointx = c1 + buf.Width;
		int originpointy = (int)(buf.Height / 2);
		double startangle = calculateAngle(buf.Width, 0, originpointx, originpointy); // 起始角度
		double endangle = calculateAngle(buf.Width, buf.Height, originpointx, originpointy);
		double distance = sqrt(pow(c1, 2) + pow(buf.Height / 2, 2));
		OffsetTrans.SetValue(HpetClock(), offset);
		auto realoff = OffsetTrans.GetCurrentValue(HpetClock());
		int index = (int)(realoff / (songheight + gap) + buf.Height / 2);
		int max = (int)(buf.Height / (songheight + gap) + 10);
		int min = index - h_cache / 2 - 5 - (selected_entry != 0 ? selected_entry->difficulties.size() : 0);

		if (caches.empty()) {
			buf.DrawString("空空如也", buf.Width - 30, buf.Height / 2, {}, {});
		}
		else {
			for (int i = min; i < index + max; i++) {
				int j = abs((int)(i % caches.size()));
				auto& cache = caches[j];
				auto basicoff = (i * (songheight + gap) - realoff);
				if (i > selected && selected_entry != 0) {
					basicoff += selected_entry->difficulties.size() * (songheight + gap);
				}
				double b = distance * sin(basicoff / buf.Height * (startangle - endangle) - startangle);
				int b2 = buf.Width + abs(b) - c2;
				if (i == selected) {
					if (b2 > w_cache - 40) {
						b2 = w_cache - 40;
					}
					buf.FillRect(b2, basicoff, b2 + c2, basicoff + songheight, { {}, { 100, 255, 255, 255 }, ' ' }); // 高亮
					int k = 1;
					int diffxpos = 3;
					for (auto& diff : cache.difficulties) {
						auto diffoff = basicoff + (k * (songheight + gap));
						if (&diff == selected_entry_2) {
							buf.FillRect(b2 + diffxpos, diffoff, b2 + c2 + diffxpos, diffoff + songheight, { {}, { 120, 255, 255, 255 }, ' ' });
						}
						buf.FillRect(b2 + diffxpos, diffoff, b2 + c2 + diffxpos, diffoff + songheight, { {}, { 200, 32, 32, 32 }, ' ' });
						buf.DrawString(diff.name, b2 + 1 + diffxpos, diffoff + 1, {}, {});
						k++;
					}
				}
				buf.FillRect(b2, basicoff, b2 + c2, basicoff + songheight, { {}, { 150, 32, 32, 32 }, ' ' });
				buf.DrawString(cache.artist + " - " + cache.title, b2 + 1, basicoff + 1, {}, {});
			}
		}
		buf.FillPolygon({ { 0, 2 }, { 50, 2 }, { 40, 12 }, { 0, 12 } }, { {}, { 150, 32, 32, 32 }, ' ' });
		auto entry1 = selected_entry;
		if (entry1 != 0) {
			buf.DrawString(entry1->titleunicode, 1, 3, {}, {});
			buf.DrawString(entry1->artistunicode, 1, 5, {}, {});
			auto entry2 = selected_entry_2;
			if (entry2 != 0) {
				auto minutes = int(entry2->length / (1000 * 60));
				auto seconds = int(entry2->length / 1000) % 60;
				auto npstext = std::to_string(entry2->nps);
				npstext.resize(npstext.find('.') + 2);
				std::string m = "Unknown";
				if (entry2->mode == 0)
					m = "Std";
				else if (entry2->mode == 3)
					m = "Mania";
				auto info = m + "  " + std::to_string(minutes) + ":" + std::to_string(seconds) + "   " + std::to_string(int(entry2->keys)) + "K" + "   OD" + std::to_string(int(entry2->od)) + "  " + npstext + "NPS";
				buf.DrawString(info, 1, 10, {}, {});

				// records
				buf.DrawString("个人最佳: " + (records.size() > 0 ? std::to_string((int)(records[0].Score * 10000000)) : "0"), 1, 14, {}, {});
				int i = 16;
				for (auto& rec : records) {
					buf.DrawString(std::to_string((int)(rec.Score * 10000000)) + "@" + std::to_string(rec.Accuracy * 100) + "%", 1, i, {}, {});
					i += 2;
				}
			}
		}
		else {
			buf.DrawString("No song selected.", 1, 3, {}, {});
		}
		buf.DrawString("Esc - 返回 上下左右/鼠标 - 选歌 F2 随机 F3 Mods F4 选项 Enter/点击 开始", 0, buf.Height - 1, {}, {});
		buf.FillRect(buf.Width - 30, 2, buf.Width - 3, 2, { {}, { 130, 128, 128, 128 }, ' ' });
		buf.DrawString("搜索:", buf.Width - 30, 2, { 255, 100, 255, 150 }, {});
		buf.DrawString(std::wstring{ search_buf.begin(), search_buf.end() }, buf.Width - 24, 2, { 255, 100, 255, 150 }, {});
		if (mod_flyout) {
			buf.FillRect(0, 0, buf.Width, buf.Height, { {}, { 170, 20, 20, 20 }, ' ' });
			buf.DrawString("Mod 选 择", 5, 6, {}, {});
			buf.DrawString("Mod 提供了一种让别人相当目害的游戏体验，可以提高或者降(变相)低(提高)游戏难度", 5, 8, {}, {});
			std::string line1 = "难度降低: ";
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::Easy) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(E)asy");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::NoFall) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(N)oFall");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::HalfTime) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(H)alfTime");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::Relax) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("Re(l)ax");
			buf.DrawString(line1, 5, 12, {}, {});
			line1 = "难度提高: ";
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::Hardrock) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("Ha(r)drock");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::Nightcore) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("N(i)ghtcore");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::Hidden) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("Hi(d)den");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::FadeOut) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(F)adeOut");
			buf.DrawString(line1, 5, 16, {}, {});
			line1 = "特殊:     ";
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::Auto) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(A)uto");
			line1.push_back(' ');
			line1.push_back('[');
			auto keyoverride = ((unsigned long long)mods >> 16) & 0xf;
			if (HasFlag(mods, OsuMods::Coop)) {
				keyoverride *= 2;
			}
			line1.append(keyoverride == 0 ? " " : std::to_string(keyoverride));
			line1.push_back(']');
			line1.append("Key");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::Coop) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(C)oop");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::Mirror) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(M)irror");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(HasFlag(mods, OsuMods::Random) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("Rand(o)m");
			buf.DrawString(line1, 5, 20, {}, {});
			auto modscale = GetModScale(mods);
			auto num = std::to_string(modscale);
			num.resize(4);
			auto modscale_tip = "分数倍率:" + num + "x";
			buf.DrawString(modscale_tip, 5, 24, {}, {});
			buf.DrawString("需要额外注意的是: Keys mod 需要 Random mod ,用小键盘 1-9 选择.   Relax 忽略除 miss 外判定", 5, 28, {}, {});
			buf.DrawString("Esc - 返回", 0, buf.Height - 1, {}, {});
		}
	}
	double random_last_off = 1.0 / 0 * 0;
	int random_last_i = 0;
	double offset = 0;
	int selected = INT_MAX;
	SongsCacheEntry* selected_entry;
	DifficultyCacheEntry* selected_entry_2;
	std::vector<Record> records;
	double calculateAngle(double x1, double y1, double x2, double y2) {
		double deltaX = x2 - x1;
		double deltaY = y2 - y1;
		double angleRad = atan2(deltaY, deltaX);
		return angleRad;
	}
	std::vector<SongsCacheEntry> matched_caches;
	bool ready;
	bool require_songs_path;
	std::vector<wchar_t> input_buf;
	virtual void Activate(bool y) {
		if (y) {
			mods = game->Settings["Mods"].Get<OsuMods>();
			if (!ready) {
				RebuildCache();
			}
			if (selected_entry_2 != 0) {
				RefreshRecords();
			}
			if (preview != 0) {
				preview->play();
			}
		}
		else {
			if (preview != 0) {
				preview->stop();
			}
		}
	}
	virtual void Wheel(WheelEventArgs wea) {
		if (!mod_flyout)
			offset += wea.Delta;
	}
	virtual void Move(MoveEventArgs mea) {
		if (!mod_flyout)
			if (last_y >= 0) {
				offset = last_offset - (mea.Y - last_y) * 2;
			}
	}
	void PlayPreview() {
		auto entry = selected_entry_2;
		if (entry == 0) {
			if (selected_entry == 0)
				return;
			entry = &selected_entry->difficulties[0];
		}
		if (audioloc != entry->audio) {
			auto bam = GetBassAudioManager();
			auto dat = ReadAllBytes(entry->audio);
			preview = std::unique_ptr<IAudioManager::IAudioStream>(bam->load(dat.data(), dat.size()));
			preview->play();
			if (entry->preview > 0 && entry->preview < preview->getDuration() * 1000)
				preview->setCurrent(entry->preview / 1000);
			audioloc = entry->audio;
		}
		else {
			if (preview != 0 && !preview->isPlaying())
				preview->play();
		}
	}
	void LoadBg() {
		bool disableBg = game->Settings["NoBg"].Get<bool>();
		if (!disableBg) {
			if (selected == INT_MAX)
				return;
			auto entry = selected_entry_2;
			if (entry == 0) {
				if (selected_entry == 0)
					return;
				entry = &selected_entry->difficulties[0];
			}
			if (bgloc != entry->background) {
				std::lock_guard lock(res_lock);
				bg.LoadBackground(entry->background);
				bgloc = entry->background;
			}
		}
		else {
			bg.LoadBackground("");
		}
	}
	int last_x;
	int last_y = -999;
	double last_offset;
	void MakeSelected(int i, auto& cache) {
		random_last_off = 1.0 / 0.0 * 0.0;
		selected = i;
		if (i != INT_MAX) {
			selected_entry = &cache;
		}
		selected_entry_2 = 0;
		LoadBg();
		PlayPreview();
	}
	void FocusCurrent() {
	}
	void MakeSelected(int i, auto& cache, auto& diff) {
		selected = i;
		if (i != INT_MAX) {
			if (selected_entry == &cache && selected_entry_2 == &diff) {
				parent->Navigate(MakeGameplayScreen(selected_entry_2->path, mods, selected_entry_2->mode));
				return;
			}
			selected_entry = &cache;
			selected_entry_2 = &diff;
			RefreshRecords();
		}
		LoadBg();
		PlayPreview();
	}
	void RefreshRecords() {
		records.clear();
		for (auto& rec : selected_entry_2->records) {
			Record rec2{};
			std::ifstream ifs(rec, std::ios::binary | std::ios::in);
			if (!ifs.good())
				continue;
			Binary::Read(ifs, rec2);
			records.push_back(rec2);
		}
		std::sort(records.begin(), records.end(), [](Record& a, Record& b) { return a.Score > b.Score; });
	}
	virtual void MouseKey(MouseKeyEventArgs mkea) {
		if (mod_flyout) {
			return;
		}
		if (mkea.Pressed) {
			last_x = mkea.X;
			last_y = mkea.Y;
			last_offset = offset;
		}
		else {
			if (mkea.X == last_x && mkea.Y == last_y && !matched_caches.empty()) {
				auto& caches = matched_caches;
				if (search_buf.empty()) {
					caches = game->GetFeature<IBeatmapManagement>().GetSongsCache();
				}
				if (caches.empty())
					return;
				int c1 = 45;
				int c2 = std::min((int)(w_cache / 2), 10) + c1;
				int gap = 1;
				int songheight = 3;
				int originpointx = c1 + w_cache;
				int originpointy = (int)(h_cache / 2);
				double startangle = calculateAngle(w_cache, 0, originpointx, originpointy); // 起始角度
				double endangle = calculateAngle(w_cache, h_cache, originpointx, originpointy);
				double distance = sqrt(pow(c1, 2) + pow(h_cache / 2, 2));

				int index = (int)(offset / (songheight + gap) + h_cache / 2);
				int max = (int)(h_cache / (songheight + gap) + 10);
				int min = index - h_cache / 2 - 5 - (selected_entry != 0 ? selected_entry->difficulties.size() : 0);
				for (int i = min; i < index + max; i++) {
					int j = std::abs((int)(i % caches.size()));
					auto& cache = caches[j];
					auto basicoff = (i * (songheight + gap) - offset);
					if (i > selected && selected_entry != 0) {
						basicoff += selected_entry->difficulties.size() * (songheight + gap);
					}
					double b = distance * sin(basicoff / h_cache * (startangle - endangle) - startangle);
					int b2 = w_cache + abs(b) - c2;
					if (mkea.X >= b2 && mkea.Y >= basicoff && mkea.Y <= basicoff + songheight) {
						MakeSelected(i, caches[j]);
						break;
					}
					if (i == selected) {
						if (b2 > w_cache - 40) {
							b2 = w_cache - 40;
						}
						int k = 1;
						int diffxpos = 3;
						for (auto& diff : cache.difficulties) {
							auto diffoff = basicoff + (k * (songheight + gap));
							if (mkea.X >= b2 + diffxpos && mkea.Y >= diffoff && mkea.Y <= diffoff + songheight) {
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
	void RebuildCache() {
		ready = false;
		game->GetFeature<IBeatmapManagement>().Refesh([&](bool yes) {
			ready = yes;
			require_songs_path = !yes;
		});
	}
	void UpdateSearch() {
		selected = INT_MAX;
		selected_entry = 0;
		matched_caches.clear();
		std::string str = Utf162Utf8(std::wstring{ search_buf.begin(), search_buf.end() });
		auto& caches = game->GetFeature<IBeatmapManagement>().GetSongsCache();
		std::copy_if(caches.begin(), caches.end(), std::back_inserter(matched_caches), [str](const SongsCacheEntry& sce) {
			return search_meta(str, sce.artist, sce.title, sce.artistunicode, sce.titleunicode, sce.tags, sce.source);
		});
	}
	virtual void Key(KeyEventArgs kea) {
		if (kea.Pressed) {
			{
				std::lock_guard lock(res_lock);
				if (mod_flyout) {
					if (kea.Key == ConsoleKey::Escape) {
						mod_flyout = !mod_flyout;
					}

					// Key binds
					if (kea.Key == ConsoleKey::E)
						mods = ToggleFlag(mods, OsuMods::Easy);
					if (kea.Key == ConsoleKey::N)
						mods = ToggleFlag(mods, OsuMods::NoFall);
					if (kea.Key == ConsoleKey::H)
						mods = ToggleFlag(mods, OsuMods::HalfTime);
					if (kea.Key == ConsoleKey::R)
						mods = ToggleFlag(mods, OsuMods::Hardrock);
					if (kea.Key == ConsoleKey::I)
						mods = ToggleFlag(mods, OsuMods::Nightcore);
					if (kea.Key == ConsoleKey::D)
						mods = ToggleFlag(mods, OsuMods::Hidden);
					if (kea.Key == ConsoleKey::F)
						mods = ToggleFlag(mods, OsuMods::FadeOut);
					if (kea.Key == ConsoleKey::A)
						mods = ToggleFlag(mods, OsuMods::Auto);
					if (kea.Key == ConsoleKey::O)
						mods = ToggleFlag(mods, OsuMods::Random);
					if (kea.Key == ConsoleKey::L)
						mods = ToggleFlag(mods, OsuMods::Relax);
					if (kea.Key == ConsoleKey::C)
						mods = ToggleFlag(mods, OsuMods::Coop);
					if (kea.Key == ConsoleKey::M)
						mods = ToggleFlag(mods, OsuMods::Mirror);

					if (kea.Key >= ConsoleKey::NumPad1 && kea.Key <= ConsoleKey::NumPad9) {
						mods = OsuMods((((unsigned long long)mods) & 0xfffffffffff0ffff) | (((int)kea.Key - 97ULL + 1) << 16));
						if (!HasFlag(mods, OsuMods::Random)) {
							mods = ToggleFlag(mods, OsuMods::Random);
						}
					}
					if (kea.Key == ConsoleKey::NumPad0) {
						mods = OsuMods(((unsigned long long)mods) & 0xfffffffffff0ffff);
					}
					game->Settings["Mods"].Set(mods);
					game->Settings.Write();
					return;
				}
				if (kea.Key == ConsoleKey::Escape) {
					parent->Back();
				}
				if (!ready && require_songs_path) {
					if (kea.Key == ConsoleKey::Backspace) {
						if (input_buf.size() > 0) {
							input_buf.resize(input_buf.size() - 1);
						}
						return;
					}
					if (kea.Key == ConsoleKey::Enter) {
						auto str = Utf162Utf8(std::wstring{ input_buf.begin(), input_buf.end() });
						game->Settings["SongsPath"].SetArray(str.c_str(), str.size() + 1);
						game->Settings.Write();
						RebuildCache();
						require_songs_path = false;
					}
					if (kea.UnicodeChar >= 31) {
						input_buf.push_back(kea.UnicodeChar);
					}
					return;
				}
				{
					if (kea.Key == ConsoleKey::Backspace) {
						if (search_buf.size() >= 2 && IsMultiUtf16(search_buf[search_buf.size() - 2]) && IsMultiUtf16(search_buf[search_buf.size() - 1])) {
							search_buf.resize(search_buf.size() - 2);
							UpdateSearch();
						}
						else if (search_buf.size() >= 1) {
							search_buf.resize(search_buf.size() - 1);
							UpdateSearch();
						}
						return;
					}
					if (kea.UnicodeChar > 31) {
						search_buf.push_back(kea.UnicodeChar);
						if (!IsMultiUtf16(kea.UnicodeChar)) {
							UpdateSearch();
						}
					}
				}
				if (kea.Key == ConsoleKey::F3) {
					mod_flyout = !mod_flyout;
				}
				if (kea.Key == ConsoleKey::F4) {
					parent->Navigate(MakeSettingsScreen());
				}
				if (kea.Key == ConsoleKey::Enter) {
					if (selected_entry_2 != 0 && selected != INT_MAX) {
						if (kea.CtrlDown()) {
							mods = ModifyFlag(mods, OsuMods::Auto);
						}
						parent->Navigate(MakeGameplayScreen(selected_entry_2->path, mods,selected_entry_2->mode));
					}
				}
				if (kea.Key == ConsoleKey::Insert) {
					if (selected_entry_2 != 0 && selected != INT_MAX) {
						if (records.size() > 0)
							parent->Navigate(MakeGameplayScreen(records[0], selected_entry_2->path, selected_entry_2->mode));
					}
				}
			}
			if (!ready)
				return;
			if (kea.Key == ConsoleKey::PageDown) {
				offset += h_cache;
			}
			if (kea.Key == ConsoleKey::PageUp) {
				offset -= h_cache;
			}
			auto& caches = game->GetFeature<IBeatmapManagement>().GetSongsCache();
			if (kea.Key == ConsoleKey::F2) {
				if (HasFlag(kea.KeyState, ControlKeyState::Shift)) {
					if (!isnan(random_last_off)) {
						offset = random_last_off;
						MakeSelected(random_last_i, caches[random_last_i % caches.size()]);
					}
					return;
				}
				auto soff = offset;
				auto sslt = selected;
				offset += rand() % 100 - 50;
				auto val = size_t(offset / 4 + 2) % caches.size();
				MakeSelected(offset / 4 + 2, caches[val]);
				random_last_off = soff;
				random_last_i = sslt;
			}
			if (kea.Key == ConsoleKey::DownArrow || kea.Key == ConsoleKey::UpArrow ||
				kea.Key == ConsoleKey::RightArrow || kea.Key == ConsoleKey::LeftArrow) {
				bool up_or_down = kea.Key == ConsoleKey::DownArrow || kea.Key == ConsoleKey::LeftArrow;
				bool forced = kea.Key == ConsoleKey::RightArrow || kea.Key == ConsoleKey::LeftArrow;
				if (selected_entry != 0 && !kea.CtrlDown()) {
					auto& caches = matched_caches;
					if (search_buf.empty()) {
						caches = game->GetFeature<IBeatmapManagement>().GetSongsCache();
					}
					if (caches.empty())
						return;
					int c1 = 45;
					int c2 = std::min((int)(w_cache / 2), 10) + c1;
					int gap = 1;
					int songheight = 3;
					int originpointx = c1 + w_cache;
					int originpointy = (int)(h_cache / 2);
					double startangle = calculateAngle(w_cache, 0, originpointx, originpointy); // 起始角度
					double endangle = calculateAngle(w_cache, h_cache, originpointx, originpointy);
					double distance = sqrt(pow(c1, 2) + pow(h_cache / 2, 2));

					int index = (int)(offset / (songheight + gap) + h_cache / 2);
					int max = (int)(h_cache / (songheight + gap) + 10);
					int min = index - h_cache / 2 - 5 - (selected_entry != 0 ? selected_entry->difficulties.size() : 0);
					for (int i = min; i < index + max; i++) {
						int j = std::abs((int)(i % caches.size()));
						auto& cache = caches[j];
						auto basicoff = (i * (songheight + gap) - offset);
						if (i > selected && selected_entry != 0) {
							basicoff += selected_entry->difficulties.size() * (songheight + gap);
						}
						double b = distance * sin(basicoff / h_cache * (startangle - endangle) - startangle);
						int b2 = w_cache + abs(b) - c2;
						if (forced && i == selected) {
							auto sfi = j + (up_or_down ? 1 : -1);
							if (sfi >= caches.size())
								sfi = 0;
							if (sfi < 0)
								sfi = caches.size() - 1;
							MakeSelected(i + (up_or_down ? 1 : -1), caches[sfi]);
							offset += basicoff - 8;
							break;
						}
						if (i == selected) {
							int k = 1;
							int diffxpos = 3;
							for (auto& diff : cache.difficulties) {
								auto diffoff = basicoff + (k * (songheight + gap));
								if (selected_entry_2 == &diff || selected_entry_2 == 0) {
									auto sfi2 = k - 1 + (up_or_down ? 1 : -1);
									if (selected_entry_2 == 0) {
										sfi2 = 0;
									}
									if (sfi2 >= cache.difficulties.size() || sfi2 < 0) {
										auto sfi = j + (up_or_down ? 1 : -1);
										if (sfi >= caches.size())
											sfi = 0;
										if (sfi < 0)
											sfi = caches.size() - 1;
										MakeSelected(i + (up_or_down ? 1 : -1), caches[sfi]);
										offset += basicoff - 8;
										return;
									}
									else {
										MakeSelected(i, cache, cache.difficulties[sfi2]);
										offset += diffoff - 12;
										return;
									}
								}
								k++;
							}
						}
					}
				}
				else {
					offset += up_or_down ? -4 : 4;
				}
			}
		}
	}
};

Screen* MakeSongSelectScreen() {
	return new SongSelectScreen();
}
