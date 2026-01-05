#include "ScreenController.h"
#include "ConsoleInputHandler.h"
#include "RecordInputHandler.h"
#include "BeatmapManagementService.h"
#include "BackgroundComponent.h"
#include "ResultScreen.h"
#include "Animator.h"
#include "Gameplay.h"
#include "RulesetManager.h"
#include "LogOverlay.h"
#include "UI/UIUtils.h"

class GameplayScreen : public Screen {
	std::unique_ptr<GameplayBase> gameplay;
	std::unique_ptr<Beatmap> beatmap;
	std::string beatmap_path;
	std::unique_ptr<ConsolePlayerInputHandler> def_input_handler;
	std::unique_ptr<RecordInputHandler> rec_input_handler;
	bool pause = false;
	bool game_ended = false;
	bool rec_saved = false;
	std::string RecordPath;
	Record rec;
	Ruleset* ruleset;
	OsuMods mods;
	int mode = 0;
	BackgroundComponent bg{ 0.2 };
	bool is_replay = false;

	// Clang doesn't support double as nontype template argument so we need to do it yourself
#ifdef __clang__
	class ConstantEasingDurationCalculator_500 {
	public:
		static inline auto Get(auto x) {
			return 500.0;
		}
	};
	using TransOut = Transition<EaseOut<CubicEasingFunction>, ConstantEasingDurationCalculator_500>;
#else
	using TransOut = Transition<EaseOut<CubicEasingFunction>, ConstantEasingDurationCalculator<500.0>>;
#endif

public:
	GameplayScreen(Ruleset* rul, const std::string& bmp_path, OsuMods mod, int mode) : ruleset(rul), mode(mode), mods(mod), beatmap_path(bmp_path), is_replay(false) {
	}
	void LoadForGameplay(OsuMods mod, const std::string& bmp_path, int mode) {
		beatmap.reset(ruleset->LoadBeatmap(bmp_path));
		gameplay.reset(ruleset->GenerateGameplay());
		if (!HasFlag(mod, OsuMods::Auto)) {
			def_input_handler = std::unique_ptr<ConsolePlayerInputHandler>(new ConsolePlayerInputHandler());
			gameplay->GameInputHandler = def_input_handler.get();
		}
		else {
			rec_input_handler = std::unique_ptr<RecordInputHandler>(new RecordInputHandler());
			gameplay->GameInputHandler = rec_input_handler.get();
		}
		gameplay->Mods = mod;
		gameplay->Load(ruleset, beatmap.get());
	}
	GameplayScreen(Ruleset* rul, Record rec, const std::string& bmp_path, int mode) : ruleset(rul), mode(mode), mods(rec.Mods), beatmap_path(bmp_path), is_replay(true), rec(rec) {
	}
	void LoadForReplay(Record& rec, const std::string& bmp_path, int mode) {
		beatmap.reset(ruleset->LoadBeatmap(bmp_path));
		gameplay.reset(ruleset->GenerateGameplay());

		rec_input_handler = std::unique_ptr<RecordInputHandler>(new RecordInputHandler(rec));

		gameplay->GameInputHandler = rec_input_handler.get();
		gameplay->Mods = rec.Mods;
		gameplay->Load(ruleset, beatmap.get());
	}

private:
	TransOut AccTrans{};
	TransOut ScoreTrans{};
	TransOut ErrorTrans{};
	TransOut VarianceTrans{};
	TransOut RatingTrans{};
	TransOut ComboTrans{};

public:
	virtual void Render(GameBuffer& buf) {
		bg.Render(buf);
		if (pause) {
			buf.DrawString("暂停中，按任意键继续，再按一次Escape返回", 0, 0, {}, {});
			return;
		}
		auto gameplay = this->gameplay.get();
		if (gameplay != 0) {
			if (!gameplay->GameStarted) {
				buf.DrawString("Loading...", 0, 0, {}, {});
				return;
			}
			gameplay->Render(buf);

			auto scp = gameplay->GetScoreProcessor();
			auto clk = gameplay->Clock.Elapsed();

			UI::DrawProgressBar(buf, 0, 0, buf.Width, gameplay->GetCurrentTime() / gameplay->GetDuration(), UI::Color_Primary, UI::Color_Bg);

			// Time info
			auto length_text = std::to_string(int(gameplay->GetDuration() / 1000 / 60)) + ":" + std::to_string(std::abs(int(gameplay->GetDuration() / 1000) % 60));
			auto current_text = std::to_string(int(gameplay->GetCurrentTime() / 1000 / 60)) + ":" + std::to_string(std::abs(int(gameplay->GetCurrentTime() / 1000) % 60));
			buf.DrawString(current_text + " / " + length_text, 2, 1, UI::Color_Text, {});

			// Mods & Rate
			auto clk_txt = std::to_string(gameplay->Clock.ClockRate());
			clk_txt.resize(4);
			clk_txt += "x";
			auto mods_txt = GetModsAbbr(mods);
			buf.DrawString(mods_txt + " | " + clk_txt, 2, 2, UI::Color_TextDim, {});

			// Score Panel (Top Right)
			int scoreW = 30;
			int scoreX = buf.Width - scoreW - 2;
			int scoreY = 1;

			ScoreTrans.SetValue(clk, scp->Score * 1000000);
			auto scr = std::to_string((int)(ScoreTrans.GetCurrentValue(clk)));
			while (scr.length() < 7)
				scr = "0" + scr; // Pad score

			UI::DrawHeader(buf, scr, scoreY); // Reusing header style for score background? Maybe too big.
			// Let's just draw manually for custom look
			buf.FillRect(scoreX, scoreY, buf.Width, scoreY + 4, { {}, UI::Color_Bg, ' ' });

			buf.DrawString("SCORE", scoreX + 1, scoreY, UI::Color_Primary, {});
			buf.DrawString(scr, scoreX + 1, scoreY + 1, UI::Color_Text, {});

			AccTrans.SetValue(clk, scp->Accuracy * 100);
			auto acc = std::to_string(AccTrans.GetCurrentValue(clk));
			acc.resize(5);
			buf.DrawString(acc + "%", scoreX + 15, scoreY + 1, UI::Color_Accent, {});

			// Combo (Center)
			std::string comboStr = std::to_string(scp->Combo) + "x";
			int comboX = (buf.Width - comboStr.size()) / 2;
			int comboY = buf.Height - 10;
			buf.DrawString(comboStr, comboX, comboY, UI::Color_Primary, {});

			// Stats / Judgements (Left side vertical?) or Bottom Right
			{
				int i = buf.Height / 2 - (gameplay->GetScoreProcessor()->ResultCounter.size()) * 2 / 2;
				int statX = buf.Width - 12;
				for (auto& res : gameplay->GetScoreProcessor()->ResultCounter) {
					auto name = GetHitResultName(res.first);
					auto clr = GetHitResultColor(res.first);
					auto val = std::to_string(res.second);

					buf.FillRect(statX, i, buf.Width, i + 1, { {}, UI::Color_Bg, ' ' });
					buf.DrawString(name, statX + 1, i, clr, {});
					buf.DrawString(val, statX + 8, i, UI::Color_Text, {});
					i++;
				}
			}

			// Error Bar / Info
			std::string corner = "";
			ErrorTrans.SetValue(clk, scp->Mean);
			auto err = std::to_string(ErrorTrans.GetCurrentValue(clk));
			err.resize(5);
			corner.append(err);
			corner.append("ms");
			corner.append(" (UR ");
			corner.append(std::to_string((int)scp->Error));
			corner.append(")");
			buf.DrawString(corner, buf.Width / 2 - corner.size() / 2, buf.Height - 1, UI::Color_TextDim, {});

			// Replay Label
			if (rec_input_handler != 0) {
				buf.DrawString("Playing Replay: " + rec.PlayerName, 0, buf.Height - 2, UI::Color_Accent, {});
			}

			if (gameplay->GameEnded) {
				buf.DrawString("Press Escape to Continue", buf.Width / 2 - 12, buf.Height - 5, UI::Color_Text, {});
			}
		}
	};
	virtual void Tick(double) {
		auto gameplay = this->gameplay.get();
		if (gameplay != nullptr && gameplay->GameStarted) {
			gameplay->Update();
			if (!game_ended) {
				if (gameplay->GameEnded) {
					if (rec_input_handler == 0 && !rec_saved) {
						gameplay->GetScoreProcessor()->SaveRecord();
						std::filesystem::create_directory("Records");

						_debugbreak();
						// TODO: finish new record logic there.


						// RecordPath = "Records/CmaniaRecord_" + std::to_string(HpetClock()) + ".bin";
						// std::fstream ofs(RecordPath, std::ios::out | std::ios::binary);
						// if (!ofs.good())
						//	__debugbreak();
						// gameplay->GameRecord.PlayerName = std::string((char*)game->Settings["Name"].Data, (char*)game->Settings["Name"].Data + game->Settings["Name"].Size);
						// auto rec = gameplay->GameRecord;
						// Binary::Write(ofs, rec);
						// ofs.close();
						// auto& caches = game->GetFeature<IBeatmapManagement>().GetSongsCache();
						// auto match = std::find_if(caches.begin(), caches.end(), [&](SongsCacheEntry& c) { return c.path == std::filesystem::path(beatmap_path).parent_path(); });
						// if (match != caches.end()) {
						//	auto& diffcache = match->difficulties;
						//	auto match2 = std::find_if(diffcache.begin(), diffcache.end(), [&](DifficultyCacheEntry& c) { return c.path == std::filesystem::path(beatmap_path); });
						//	if (match2 != diffcache.end()) {
						//		match2->records.push_back(RecordPath);
						//	}
						// }
						// game->GetFeature<IBeatmapManagement>().Save();

						rec_saved = true;
					}
				}
			}
		}
	};
	virtual void Key(KeyEventArgs kea) {
		if (pause) {
			if (kea.Pressed && kea.RepeatCount <= 1) {
				if (kea.Key == ConsoleKey::Escape) {
					parent->Back();
					return;
				}
				if (kea.Key == ConsoleKey::Oem3 && def_input_handler != 0) {
					gameplay = 0;
					LoadForGameplay(mods, beatmap_path, mode); // 6
					LoadRuleset();
					pause = false;
					return;
				}
				gameplay->Resume();
				pause = false;
			}
			return;
		}
		if (kea.Pressed && kea.RepeatCount <= 1) {
			if (kea.Key == ConsoleKey::Escape) {
				if (gameplay->GameEnded) {
					if (def_input_handler != 0)
						parent->Navigate(MakeResultScreen(gameplay->GameRecord, gameplay->GetBgPath()));
					else
						parent->Back();
					return;
				}
				pause = true;
				gameplay->Pause();
			}
			if (kea.Key == ConsoleKey::Spacebar) {
				gameplay->Skip();
			}
		}
		if (def_input_handler == nullptr)
			return;
		def_input_handler->OnKeyEvent(kea.Key, kea.Pressed);
	};
	virtual void Wheel(WheelEventArgs wea){

	};
	virtual void Move(MoveEventArgs mea) {
		if (def_input_handler == nullptr)
			return;
		def_input_handler->OnMouseMove(mea);
	};
	virtual void Activate(bool yes) {
		if (!yes) {
			gameplay = 0;
		}
		else {
			if (gameplay != 0) {
				if (gameplay->GameEnded) {
					parent->Back();
					return;
				}
			}
			try {
				if (!is_replay) {
					LoadForGameplay(mods, beatmap_path, mode);
				}
				else {
					LoadForReplay(rec, beatmap_path, mode);
				}
			}
			catch (std::exception& ex) {
				game->GetFeature<ILogger>().LogError(ex.what());
				parent->Back();
				return;
			}
			catch (...) {
				game->GetFeature<ILogger>().LogError("Failed to load ruleset.");
				parent->Back();
				return;
			}
			if (gameplay != 0) {
				LoadRuleset();
				if (!game->Settings["NoBg"].Get<bool>())
					bg.LoadBackground(gameplay->GetBgPath());
			}
			else {
				parent->Back();
			}
		}
	}
	void LoadRuleset() {
		if (HasFlag(mods, OsuMods::Auto)) {
			if (rec_input_handler)
				rec_input_handler->LoadRecord(rec = gameplay->GetAutoplayRecord());
		}
	};
	virtual void MouseKey(MouseKeyEventArgs mkea) {
		if (def_input_handler == nullptr)
			return;
		def_input_handler->OnMouseKey(mkea);
	};
};

Screen* MakeGameplayScreen(Ruleset* rul, const std::string& bmp_path, OsuMods mod, int mode) {
	return new GameplayScreen(rul, bmp_path, mod, mode);
}

Screen* MakeGameplayScreen(Ruleset* rul, Record rec, const std::string& bmp_path, int mode) {
	return new GameplayScreen(rul, rec, bmp_path, mode);
}
