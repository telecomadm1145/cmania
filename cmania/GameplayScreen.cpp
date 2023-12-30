#include "ScreenController.h"
#include "ConsoleInputHandler.h"
#include "RecordInputHandler.h"
#include "BeatmapManagementService.h"
#include "BackgroundComponent.h"
#include "ResultScreen.h"
#include "Animator.h"
#include "Gameplay.h"

class GameplayScreen : public Screen {
	std::unique_ptr<GameplayBase> gameplay;
	std::string beatmap_path;
	std::unique_ptr<ConsolePlayerInputHandler> def_input_handler;
	std::unique_ptr<RecordInputHandler> rec_input_handler;
	bool pause = false;
	bool game_ended = false;
	bool rec_saved = false;
	std::string RecordPath;
	Record rec;
	OsuMods mods;
	int mode = 0;
	BackgroundComponent bg;

	using TransOut = Transition<EaseOut<CubicEasingFunction>,ConstantEasingDurationCalculator<500.0>>;

public:
	GameplayScreen(const std::string& bmp_path, OsuMods mod, int mode) : mode(mode) {
		LoadForGameplay(mod, bmp_path, mode);
	}
	void LoadForGameplay(OsuMods mod, const std::string& bmp_path, int mode) {
		
		if (!HasFlag(mod, OsuMods::Auto)) {
			def_input_handler = std::unique_ptr<ConsolePlayerInputHandler>(new ConsolePlayerInputHandler());
			gameplay->GameInputHandler = def_input_handler.get();
		}
		else {
			rec_input_handler = std::unique_ptr<RecordInputHandler>(new RecordInputHandler());
			gameplay->GameInputHandler = rec_input_handler.get();
		}
		mods = gameplay->Mods = mod;
		beatmap_path = bmp_path;
	}
	GameplayScreen(Record rec, const std::string& bmp_path, int mode) : mode(mode) {
		LoadForReplay(rec, bmp_path, mode);
	}
	void LoadForReplay(Record& rec, const std::string& bmp_path, int mode) {
		rec_input_handler = std::unique_ptr<RecordInputHandler>(new RecordInputHandler(rec));

		gameplay->GameInputHandler = rec_input_handler.get();
		mods = gameplay->Mods = rec.Mods;
		beatmap_path = bmp_path;
		this->rec = rec;
	}

private:
	TransOut AccTrans{};
	TransOut ScoreTrans{  };
	TransOut ErrorTrans{};
	TransOut VarianceTrans{ };
	TransOut RatingTrans{  };
	TransOut ComboTrans{  };

public:
	virtual void Render(GameBuffer& buf) {
		bg.Render(buf);
		if (pause) {
			buf.DrawString("暂停中，按任意键继续，再按一次Escape返回", 0, 0, {}, {});
			return;
		}
		auto gameplay = &*this->gameplay;
		if (gameplay != 0) {
			if (!gameplay->GameStarted) {
				buf.DrawString("Loading...", 0, 0, {}, {});
				return;
			}
			gameplay->Render(buf);

			// We need to render the result of score processor
			auto scp = gameplay->GetScoreProcessor();
			auto clk = gameplay->Clock.Elapsed();

			std::string centre1 = ""; // this is the major counter.
			buf.DrawLineV(0, buf.Width, 0, { {}, { 60, 255, 255, 255 }, ' ' });
			buf.DrawLineV(0, (gameplay->GetCurrentTime() / gameplay->GetDuration()) * buf.Width, 0, { {}, { 60, 90, 255, 100 }, ' ' });
			auto length_text = std::to_string(int(gameplay->GetDuration() / 1000 / 60)) + ":" + std::to_string(std::abs(int(gameplay->GetDuration() / 1000) % 60));
			buf.DrawString(length_text, buf.Width - length_text.size() - 1, 0, {}, {});

			auto clk_txt = std::to_string(gameplay->Clock.ClockRate());
			clk_txt.resize(4);
			clk_txt += "x";
			buf.DrawString(clk_txt, buf.Width - clk_txt.size() - 1, 1, {}, {});

			auto mods_txt = GetModsAbbr(mods);
			buf.DrawString(mods_txt, buf.Width - mods_txt.size() - 1, 2, {}, {});

			auto current_text = std::to_string(int(gameplay->GetCurrentTime() / 1000 / 60)) + ":" + std::to_string(std::abs(int(gameplay->GetCurrentTime() / 1000) % 60));
			buf.DrawString(current_text, 0, 0, {}, {});
			centre1.append(std::to_string(scp->Combo));
			centre1.push_back('x');
			centre1.append("\t\t\t\t\t");
			ScoreTrans.SetValue(clk, scp->Score * 1000000);
			auto scr = std::to_string((int)(ScoreTrans.GetCurrentValue(clk)));
			centre1.append(scr);
			centre1.append("\t\t\t\t\t");
			AccTrans.SetValue(clk, scp->Accuracy * 100);
			auto acc = std::to_string(AccTrans.GetCurrentValue(clk));
			acc.resize(6);
			centre1.append(acc);
			centre1.push_back('%');
			buf.DrawString(centre1, (buf.Width - centre1.size()) / 2, 1, { 255, 255, 255, 255 }, {});

			centre1 = "";
			centre1.append(std::to_string(scp->MaxCombo));
			centre1.push_back('x');
			centre1.push_back('(');
			centre1.append(std::to_string(scp->BeatmapMaxCombo));
			centre1.push_back('x');
			centre1.push_back(')');
			centre1.append("\t\t\t\t");
			RatingTrans.SetValue(clk, scp->Rating);
			auto rt = std::to_string(RatingTrans.GetCurrentValue(clk));
			rt.resize(6);
			centre1.append(rt);
			buf.DrawString(centre1, (buf.Width - centre1.size()) / 2, 2, { 255, 255, 255, 255 }, {});
			if (rec_input_handler != 0) {
				buf.DrawString("Replaying record of \"" + rec.PlayerName + "\"", 0, 1, { 255, 255, 255, 255 }, {});
			}
			{
				int i = buf.Height / 2 + (gameplay->GetScoreProcessor()->ResultCounter.size()) * 5 / 4 + 1;
				for (auto& res : gameplay->GetScoreProcessor()->ResultCounter) {
					auto name = GetHitResultName(res.first);
					auto clr = GetHitResultColor(res.first);
					auto val = std::to_string(res.second);
					clr.Alpha = 20;
					buf.FillRect(0, i - 2, 7, i + 1, { {}, clr, ' ' });
					buf.FillRect(buf.Width - 7, i - 2, buf.Width, i + 1, { {}, clr, ' ' });
					clr.Alpha = 255;
					buf.DrawString(val, (7 - val.size()) / 2, i - 1, clr, {});
					buf.DrawString(val, buf.Width - 7 - (val.size() - 7) / 2, i - 1, clr, {});
					i -= 3;
				}
			}

			std::string corner = "";
			ErrorTrans.SetValue(clk, scp->Mean);
			auto err = std::to_string(ErrorTrans.GetCurrentValue(clk));
			err.resize(5);
			corner.append(err);
			corner.append("ms");
			corner.append("(UR");
			corner.append(std::to_string((int)scp->Error));
			corner.append(")");
			buf.DrawString(corner, 0, buf.Height - 1, { 255, 255, 255, 255 }, {});
			if (gameplay->GameEnded) {
				buf.DrawString("按Escape键Continue", buf.Width / 2 - 8, buf.Height - 5, { 255, 255, 255, 255 }, {});
			}
		}
	};
	virtual void Tick(double) {
		auto gameplay = &*this->gameplay;
		if (gameplay != 0) {
			gameplay->Update();
			if (!game_ended) {
				if (gameplay->GameEnded) {
					if (rec_input_handler == 0 && !rec_saved) {
						gameplay->GetScoreProcessor()->SaveRecord();
						std::filesystem::create_directory("Records");
						RecordPath = "Records/CmaniaRecord_" + std::to_string(HpetClock()) + ".bin";
						std::fstream ofs(RecordPath, std::ios::out | std::ios::binary);
						if (!ofs.good())
							__debugbreak();
						gameplay->GameRecord.PlayerName = std::string((char*)game->Settings["Name"].Data, (char*)game->Settings["Name"].Data + game->Settings["Name"].Size);
						auto rec = gameplay->GameRecord;
						Binary::Write(ofs, rec);
						ofs.close();
						auto& caches = game->GetFeature<IBeatmapManagement>().GetSongsCache();
						auto match = std::find_if(caches.begin(), caches.end(), [&](SongsCacheEntry& c) { return c.path == std::filesystem::path(beatmap_path).parent_path(); });
						if (match != caches.end()) {
							auto& diffcache = match->difficulties;
							auto match2 = std::find_if(diffcache.begin(), diffcache.end(), [&](DifficultyCacheEntry& c) { return c.path == std::filesystem::path(beatmap_path); });
							if (match2 != diffcache.end()) {
								match2->records.push_back(RecordPath);
							}
						}
						game->GetFeature<IBeatmapManagement>().Save();
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
				gameplay->Clock.Start();
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
				gameplay->Clock.Stop();
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
	virtual void Move(MoveEventArgs mea){
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
		gameplay->LoadSettings(game->Settings);
		if (HasFlag(mods, OsuMods::Auto)) {
			if (rec_input_handler)
				rec_input_handler->LoadRecord(rec = gameplay->GetAutoplayRecord());
		}
	};
	virtual void MouseKey(MouseKeyEventArgs mkea){
		if (def_input_handler == nullptr)
			return;
		def_input_handler->OnMouseKey(mkea);
	};
};

Screen* MakeGameplayScreen(const std::string& bmp_path, OsuMods mod, int mode) {
	return new GameplayScreen(bmp_path, mod, mode);
}

Screen* MakeGameplayScreen(Record rec, const std::string& bmp_path, int mode) {
	return new GameplayScreen(rec, bmp_path, mode);
}
