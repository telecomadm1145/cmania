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
#include <unordered_set>
#include "OsuMods.h"
#include "File.h"
#include "SettingsScreen.h"
#include "BackgroundComponent.h"
#include "GameplayScreen.h"
#include "SongSelectScreen.h"
#include "Animator.h"
#include "OpenFileDialog.h"
#include "RulesetManager.h"

class SongSelectScreen : public Screen {
	BackgroundComponent bg{ 0.65 };
	std::string bgloc;
	int h_cache = 0;
	int w_cache = 0;
	std::mutex res_lock;
	std::vector<wchar_t> search_buf;
	std::string audioloc;
	std::unique_ptr<IAudioManager::IAudioStream> preview;
	bool mod_flyout;
	bool ruleset_flyout;
	std::unordered_set<int> selected_ruleset;
	OsuMods mods;
#ifdef __clang__
	class DurationRangeLimiter_0 {
	public:
		static inline auto Get(auto x) {
			return std::clamp(10 * x, 800.0, 3500.0);
		}
	};
	using TransOut = Transition<
		EaseOut<CubicEasingFunction>,
		DurationRangeLimiter_0>;
#else
	using TransOut = Transition<
		EaseOut<CubicEasingFunction>,
		DurationRangeLimiter<800.0, 3500.0, LinearEasingDurationCalculator<1>>>;
#endif
	Color difficultyToRGBColor(float difficulty) {
		static constexpr Color ranges[9] = {
			{ 0, 77, 200, 46 },	 // 灰
			{ 0, 77, 200, 46 },	 // 绿
			{ 0, 0, 105, 204 },	 // 蓝
			{ 0, 230, 153, 46 }, // 橙
			{ 0, 230, 55, 46 },	 // Red
			{ 0, 202, 46, 230 }, // Purple
			{ 0, 108, 0, 127 },	 // Dark Purple
			{ 0, 0, 0, 0 }		 // 黑
		};
		auto d = std::clamp(difficulty, 0.0f, 7.9f);
		float intg = 0.0;
		float frac = std::modf(d, &intg);
		int intg_1 = (int)intg;
		auto clr1 = ranges[intg_1];
		auto clr2 = ranges[intg_1 + 1];
		frac /= 4;
		auto clrf = (clr1 ^ (1 - frac)) + (clr2 ^ frac);
		clrf.Alpha = 255;
		return clrf;
	}
	TransOut OffsetTrans{};
	TransOut OffsetTrans2{};
	TransOut Itemstrans[182]{};
	double difficulty_val = 0;
	DifficultyInfo di;
	int di_loading = 0;
	int di_size = 0;
	int selected_tab = 0;

	virtual void Render(GameBuffer& buf) {
		std::lock_guard lock(res_lock);
		h_cache = buf.Height;
		w_cache = buf.Width;
		if (!ready) {
			buf.DrawString("Loading...", 0, 0, {}, {});
			return;
		}
		auto& caches = matched_caches;

		bg.Render(buf);
		int gap = 1;
		int songheight = 3;
		auto clk = HpetClock();
		OffsetTrans.SetValue(clk, offset);
		auto realoff = OffsetTrans.GetCurrentValue(clk);
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
				Itemstrans[(i % 70) + 70].SetValue(clk, 50 - std::abs(basicoff - (buf.Height / 2)));
				int c2 = Itemstrans[(i % 70) + 70].GetCurrentValue(clk) / (buf.Height / 2) * 8;
				int b2 = buf.Width * 0.7 - c2;
				if (i == selected) {
					buf.FillRect(b2, basicoff, buf.Width, basicoff + songheight, { {}, { 60, 255, 255, 255 }, ' ' }); // 高亮
					int k = 1;
					int diffxpos = 3;
					for (auto& diff : cache.difficulties) {
						auto diffoff = basicoff + (k * (songheight + gap));
						buf.FillRect(b2 + diffxpos, diffoff, buf.Width, diffoff + songheight, { {}, { 100, 32, 32, 32 }, ' ' });
						if (&diff == selected_entry_2) {
							buf.FillRect(b2 + diffxpos, diffoff, buf.Width, diffoff + songheight, { {}, { 40, 255, 255, 255 }, ' ' });
						}
						auto rul = GetRulesetName(diff.mode);
						buf.DrawString(rul, b2 + 1 + diffxpos, std::round(diffoff) + 1, {}, { 255, 120, 120, 120 });
						buf.DrawString(diff.name, b2 + 1 + diffxpos + rul.size() + 1, std::round(diffoff) + 1, {}, {});
						k++;
					}
				}
				buf.FillRect(b2, basicoff, buf.Width, basicoff + songheight, { {}, { 150, 32, 32, 32 }, ' ' });
				buf.DrawString(cache.artist + " - " + cache.title, b2 + 1, std::round(basicoff) + 1, {}, {});
			}
		}
		buf.FillRect(0, 2, 50, 12, { {}, { 150, 32, 32, 32 }, ' ' });
		auto entry1 = selected_entry;
		if (entry1 != 0) {
			auto title = entry1->titleunicode;
			if (title.empty())
				title = entry1->title;
			if (title.size() > 34) {
				title.resize(34);
				title += "...";
			}
			buf.DrawString(title, 1, 3, {}, {});
			auto artist = entry1->artistunicode;
			if (artist.empty())
				artist = entry1->artist;
			if (artist.size() > 30) {
				artist.resize(30);
				artist += "...";
			}
			buf.DrawString(artist, 1, 5, {}, {});
			auto entry2 = selected_entry_2;
			if (entry2 != 0) {
				auto minutes = int(entry2->length / (1000 * 60));
				auto seconds = int(entry2->length / 1000) % 60;
				auto info = GetRulesetName(entry2->mode) + "  " + std::to_string(minutes) + ":" + std::to_string(seconds);
				buf.DrawString(info, 1, 10, {}, {});
				// records
				buf.FillRect(0, 14, 8, 15, { {}, { 150, 32, 32, 32 }, ' ' });
				buf.DrawString("Details", 0, 14, {}, {});
				buf.FillRect(0, 15, 50, buf.Height - 3, { {}, { 150, 32, 32, 32 }, ' ' });
				if (di_loading) {
					buf.DrawString("Loading...", 0, 15, {}, {});
				}
				else {
					if (difficulty_val > 0) {
#ifdef _WIN32
						auto diff = std::format("{:.1f}", difficulty_val);
#else
						auto diff = std::to_string(difficulty_val);
#endif
						auto clr = difficultyToRGBColor(difficulty_val);
						auto clr2 = Color::Blend(clr, Color{ 200, 255, 255, 255 });
						buf.DrawString(diff, 50 - diff.size() - 1, 3, clr2, clr);
					}
					buf.SetBounds(0, 15, 50, buf.Height - 4);
					OffsetTrans2.SetValue(clk, offset2);
					int z = (int)OffsetTrans2.GetCurrentValue(clk);
					int max_di_size = buf.Height - 19;
					int i = 15 - z;
					for (auto& d : di) {
						if (d.Type == DifficultyInfoItem::ValueBar) {
							buf.DrawString(d.Text, 0, i, {}, {});
#ifdef _WIN32
							std::string v = std::format("{:.2f}", d.Value);
#else
							std::string v = std::to_string(d.Value);
#endif
							buf.DrawString(v, 50 - v.size() - 1, i, {}, {});
							buf.DrawLineV(12, 50 - 7 - 2, i, { {}, { 150, 32, 32, 32 }, ' ' });
							double prog = std::clamp(d.Value / d.MaxValue, 0.0, 1.0);
							buf.DrawLineV(12, prog * (50 - 7 - 2 - 12) + 12, i, { {}, { 150, 255, 0, 0 }, ' ' });
							i++;
						}
						else if (d.Type == DifficultyInfoItem::Header) {
							i++;
							buf.DrawString(d.Text, 0, i, {}, {});
							i++;
						}
						else if (d.Type == DifficultyInfoItem::Header2) {
							buf.DrawString(d.Text, 0, i, {}, {});
							i++;
						}
						else if (d.Type == DifficultyInfoItem::PlainText) {
							buf.DrawString(d.Text, 0, i, {}, {});
							auto v = d.Text2;
							buf.DrawString(v, 50 - v.size() - 1, i, {}, {});
							i++;
						}
						else if (d.Type == DifficultyInfoItem::PlainValue) {
							buf.DrawString(d.Text, 0, i, {}, {});
#ifdef _WIN32
							auto v = std::format("{:.2f}", d.Value);
#else
							auto v = std::to_string(d.Value);
#endif
							buf.DrawString(v, 50 - v.size() - 1, i, {}, {});
							i++;
						}
					}
					di_size = i - 15 + z;

					// Draw scrollbar
					if (di_size > max_di_size) {
						int scrollbar_height = 1;
						int scrollbar_pos = z * (max_di_size - scrollbar_height) / (di_size - max_di_size);
						buf.FillRect(50 - 1, 15, 50, buf.Height - 3, { {}, { 150, 32, 32, 32 }, ' ' });
						buf.FillRect(50 - 1, 15 + scrollbar_pos, 50, 15 + scrollbar_pos + scrollbar_height, { { 255, 255, 255, 255 }, {}, '|' });
					}
					buf.ResetBounds();
				}
				/* buf.DrawString("个人最佳: " + (records.size() > 0 ? std::to_string((int)(records[0].Score * 10000000)) : "0"), 1, 14, {}, {});
				int i = 16;
				for (auto& rec : records) {
					buf.DrawString(std::to_string((int)(rec.Score * 10000000)) + "@" + std::to_string(rec.Accuracy * 100) + "%", 1, i, {}, {});
					i += 2;
				}*/
			}
		}
		else {
			buf.DrawString("No song selected.", 1, 3, {}, {});
		}
		buf.FillRect(0, buf.Height - 2, 50, buf.Height, { {}, { 130, 20, 20, 20 }, ' ' });
		buf.DrawString(" Esc      F1       F2      F3      F4      Enter", 0, buf.Height - 2, { 255, 0, 105, 204 }, {});
		buf.DrawString(" 返回  Rulesets  Random   Mods   Options   进入", 0, buf.Height - 1, {}, {});
		buf.FillRect(buf.Width - 30, 2, buf.Width - 3, 2, { {}, { 130, 128, 128, 128 }, ' ' });
		buf.DrawString("搜索:", buf.Width - 30, 2, { 255, 100, 255, 150 }, {});
		buf.DrawString(std::wstring{ search_buf.begin(), search_buf.end() }, buf.Width - 24, 2, { 255, 100, 255, 150 }, {});
		if (mod_flyout) {
			buf.FillRect(0, 0, buf.Width, buf.Height, { { 255, 255, 255, 255 }, { 150, 20, 20, 20 }, ' ' });
			buf.FillRect(0, 0, buf.Width, 3, { { 255, 51, 156, 255 }, { 240, 20, 20, 20 }, ' ' });
			buf.DrawString("Mod 选择", 5, 1, {}, {});
			// buf.DrawString("", 5, 8, {}, {});
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
			buf.DrawString(line1, 5, 8, {}, {});
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
			buf.DrawString(line1, 5, 12, {}, {});
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
			buf.DrawString(line1, 5, 16, {}, {});
			auto modscale = GetModScale(mods);
			auto num = std::to_string(modscale);
			num.resize(4);
			auto modscale_tip = "分数倍率:" + num + "x";
			buf.DrawString(modscale_tip, 5, 22, {}, {});

			buf.DrawString("已选择: " + GetModsAbbr(mods), 5, 20, {}, {});

			buf.FillRect(0, buf.Height - 2, 5, buf.Height, { {}, { 130, 20, 20, 20 }, ' ' });
			buf.DrawString(" Esc", 0, buf.Height - 2, { 255, 0, 105, 204 }, {});
			buf.DrawString(" 返回", 0, buf.Height - 1, {}, {});
		}
		if (ruleset_flyout) {
			buf.FillRect(0, 0, buf.Width, buf.Height, { { 255, 255, 255, 255 }, { 150, 20, 20, 20 }, ' ' });
			buf.FillRect(0, 0, buf.Width, 3, { { 255, 51, 156, 255 }, { 240, 20, 20, 20 }, ' ' });
			buf.DrawString("Ruleset 选择", 5, 1, {}, {});
			// buf.DrawString("在此可以选择你想要看到的Rulesets", 5, 8, {}, {});
			std::string line1 = "Osu! Rulesets: ";
			line1.push_back('[');
			line1.push_back(IsRulesetToggled(0) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(S)td");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(IsRulesetToggled(1) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(T)aiko");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(IsRulesetToggled(2) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(C)atch");
			line1.push_back(' ');
			line1.push_back('[');
			line1.push_back(IsRulesetToggled(3) ? 'x' : ' ');
			line1.push_back(']');
			line1.append("(M)ania");
			buf.DrawString(line1, 5, 8, {}, {});
			buf.FillRect(0, buf.Height - 2, 5, buf.Height, { {}, { 130, 20, 20, 20 }, ' ' });
			buf.DrawString(" Esc", 0, buf.Height - 2, { 255, 0, 105, 204 }, {});
			buf.DrawString(" 返回", 0, buf.Height - 1, {}, {});
		}
	}
	double random_last_off = 1.0 / 0 * 0;
	int random_last_i = 0;
	double offset = 0;
	double offset2 = 0;
	double last_offset2 = 0;
	int ctl_drag = 0;
	int selected = INT_MAX;
	SongsCacheEntry* selected_entry;
	DifficultyCacheEntry* selected_entry_2;
	std::vector<Record> records;
	std::vector<SongsCacheEntry> matched_caches;
	bool ready;
	bool require_songs_path;
	~SongSelectScreen() {
		thd_alive = false;
		background_info_worker.join();
	}
	bool thd_alive = true;
	std::thread background_info_worker = std::thread([&]() {
		auto old_en = selected_entry_2;
		while (thd_alive) {
			if (selected_entry_2 != old_en) {
				if (selected_entry_2 != 0) {
					di_loading = 1;
					old_en = selected_entry_2;
					auto ruleset = &game->GetFeature<IRulesetManager>().GetRuleset(GetRulesetId(old_en->mode));
					auto bmp = ruleset->LoadBeatmap(old_en->path, false);
					difficulty_val = ruleset->CalculateDifficulty(bmp, mods) * GetModScale(mods) * GetPlaybackRate(mods);
					di = ruleset->PopulateDifficultyInfo(bmp);
					delete bmp;
					di_loading = 0;
				}
				else
					old_en = 0;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	});
	std::vector<wchar_t> input_buf;
	Ruleset* ruleset = 0;
	virtual void Activate(bool y) {
		if (y) {
			if (require_songs_path) {
				parent->Back();
				return;
			}
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
		if (!mod_flyout && !ruleset_flyout)
			offset += wea.Delta;
	}
	virtual void Move(MoveEventArgs mea) {
		if (!mod_flyout && !ruleset_flyout) {
			if (last_y >= 0) {
				if (ctl_drag) {
					offset = last_offset - (mea.Y - last_y) * 2;
				}
				else {
					offset2 = last_offset2 - (mea.Y - last_y) * 2;
				}
			}
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
			ruleset = &game->GetFeature<IRulesetManager>().GetRuleset(GetRulesetId(diff.mode));
			if (selected_entry == &cache && selected_entry_2 == &diff) {
				parent->Navigate(MakeGameplayScreen(ruleset, selected_entry_2->path, mods, selected_entry_2->mode));
				return;
			}
			selected_entry = &cache;
			selected_entry_2 = &diff;
			difficulty_val = 0;
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
		if (mod_flyout || ruleset_flyout) {
			return;
		}
		if (mkea.Pressed) {
			if (mkea.X >= w_cache / 2) {
				last_x = mkea.X;
				last_y = mkea.Y;
				last_offset = offset;
				ctl_drag = 1;
			}
			else {
				last_x = mkea.X;
				last_y = mkea.Y;
				last_offset2 = offset2;
				ctl_drag = 0;
			}
		}
		else {
			if (ctl_drag == 0) {
				offset2 = std::clamp(offset2, 0.0, std::max((double)(di_size - h_cache + 20), 0.0));
			}
			if (mkea.X == last_x && mkea.Y == last_y && !matched_caches.empty()) {
				if (ctl_drag == 0) {
				}
				else {
					auto& caches = matched_caches;
					if (caches.empty())
						return;
					int gap = 1;
					int songheight = 3;

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
						if (mkea.Y >= basicoff && mkea.Y <= basicoff + songheight) {
							MakeSelected(i, caches[j]);
							break;
						}
						if (i == selected) {
							int k = 1;
							int diffxpos = 3;
							for (auto& diff : cache.difficulties) {
								auto diffoff = basicoff + (k * (songheight + gap));
								if (mkea.Y >= diffoff && mkea.Y <= diffoff + songheight) {
									MakeSelected(i, cache, diff);
								}
								k++;
							}
						}
					}
				}
			}
			last_y = -999;
		}
	}
	void RebuildCache() {
		ready = false;
		game->GetFeature<IBeatmapManagement>().Refesh([this](bool yes) {
			ready = yes;
			if (yes) {
				UpdateSearch();
			}
			else {
				require_songs_path = true;
				parent->Navigate(PickFile(
					"请选择Songs文件夹...", [this](std::filesystem::path pth) {
						require_songs_path = false;
						std::filesystem::remove("Songs.bin");
						game->Settings["SongsPath"].SetArray(pth.string().c_str(), pth.string().size());
						game->Settings.Write();
					},
					{}, true, game->Settings["SongsPath"].GetString()));
			}
		});
	}
	void UpdateSearch() {
		selected = INT_MAX;
		selected_entry = 0;
		matched_caches.clear();
		std::string str = Utf162Utf8(std::wstring{ search_buf.begin(), search_buf.end() });
		auto& caches = game->GetFeature<IBeatmapManagement>().GetSongsCache();
		for (auto sce : caches) {
#ifdef __clang__
			bool match = true;
#else
			bool match = search_meta(str, sce.artist, sce.title, sce.artistunicode, sce.titleunicode, sce.tags, sce.source);
#endif
			if (str.empty())
				match = true;
			if (!match)
				continue;
			std::vector<DifficultyCacheEntry> new_diffs;
			std::sort(sce.difficulties.begin(), sce.difficulties.end(), [](DifficultyCacheEntry& a, DifficultyCacheEntry& b) {
				return a.mode < b.mode;
			});
			for (auto& diff : sce.difficulties) {
				if (selected_ruleset.empty() || (selected_ruleset.find(diff.mode) != selected_ruleset.end())) {
					new_diffs.push_back(diff);
				}
			}
			sce.difficulties = new_diffs;
			if (!sce.difficulties.empty())
				matched_caches.push_back(sce);
		}
	}
	virtual void Key(KeyEventArgs kea) {
		if (!ready) {
			return;
		}
		if (kea.Pressed) {
			if (kea.Key == ConsoleKey::F10) {
				preview->stop();
				return;
			}
			if (kea.Key == ConsoleKey::F9) {
				preview->play();
				return;
			}
			{
				std::lock_guard lock(res_lock);
				if (ruleset_flyout) {
					if (kea.Key == ConsoleKey::Escape) {
						ruleset_flyout = !ruleset_flyout;
					}
					if (kea.Key == ConsoleKey::S) {
						ToggleRuleset(0);
					}
					if (kea.Key == ConsoleKey::T) {
						ToggleRuleset(1);
					}
					if (kea.Key == ConsoleKey::C) {
						ToggleRuleset(2);
					}
					if (kea.Key == ConsoleKey::M) {
						ToggleRuleset(3);
					}
					return;
				}
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
						parent->Navigate(MakeGameplayScreen(ruleset, selected_entry_2->path, mods, selected_entry_2->mode));
					}
				}
				if (kea.Key == ConsoleKey::Insert) {
					if (selected_entry_2 != 0 && selected != INT_MAX) {
						if (records.size() > 0)
							parent->Navigate(MakeGameplayScreen(ruleset, records[0], selected_entry_2->path, selected_entry_2->mode));
					}
				}
			}
			if (kea.Key == ConsoleKey::PageDown) {
				offset += h_cache;
			}
			if (kea.Key == ConsoleKey::PageUp) {
				offset -= h_cache;
			}
			if (kea.Key == ConsoleKey::F1) {
				ruleset_flyout = !ruleset_flyout;
			}
			if (kea.Key == ConsoleKey::F2) {
				auto& caches = matched_caches;
				if (HasFlag(kea.KeyState, ControlKeyState::Shift)) {
					if (!std::isnan(random_last_off)) {
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
					if (caches.empty())
						return;
					int c1 = 45;
					int c2 = std::min((int)(w_cache / 2), 10) + c1;
					int gap = 1;
					int songheight = 3;

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
						int c2 = 40;
						int b2 = w_cache - c2;
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
	void ToggleRuleset(int id) {
		bool exists = IsRulesetToggled(id);
		if (!exists)
			selected_ruleset.emplace(id);
		else
			selected_ruleset.erase(id);
		UpdateSearch();
	}
	bool IsRulesetToggled(int id) {
		return selected_ruleset.find(id) != selected_ruleset.end();
	}
};

Screen* MakeSongSelectScreen() {
	return new SongSelectScreen();
}
