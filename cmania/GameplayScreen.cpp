#include "ManiaRuleset.h"
#include "ScreenController.h"
#include "ConsoleInputHandler.h"
#include "RecordInputHandler.h"
#include "BeatmapManagementService.h"

class GameplayScreen : public Screen {
	std::unique_ptr<RulesetBase> ruleset;
	std::string beatmap_path;
	std::unique_ptr<ConsolePlayerInputHandler> def_input_handler;
	std::unique_ptr<RecordInputHandler> rec_input_handler;
	bool pause = false;
	bool game_ended = false;
	bool rec_saved = false;
	std::string RecordPath;

public:
	GameplayScreen(const std::string& bmp_path, OsuMods mod) {
		def_input_handler = std::unique_ptr<ConsolePlayerInputHandler>(new ConsolePlayerInputHandler());
		ruleset = std::unique_ptr<RulesetBase>((RulesetBase*) new ManiaRuleset());
		ruleset->RulesetInputHandler = def_input_handler.get();
		ruleset->Mods = mod;
		beatmap_path = bmp_path;
	}
	GameplayScreen(Record rec, const std::string& bmp_path, OsuMods mod) {
		rec_input_handler = std::unique_ptr<RecordInputHandler>(new RecordInputHandler(rec));
		ruleset = std::unique_ptr<RulesetBase>((RulesetBase*) new ManiaRuleset());
		ruleset->RulesetInputHandler = rec_input_handler.get();
		ruleset->Mods = mod;
		beatmap_path = bmp_path;
	}
	virtual void Render(GameBuffer& buf) {
		if (pause) {
			buf.DrawString("暂停中，按任意键继续，再按一次Escape返回", 0, 1, {}, {});
		}
		auto ruleset = &*this->ruleset;
		if (ruleset != 0) {
			ruleset->Render(buf);

			// We need to render the result of score processor
			auto scp = ruleset->GetScoreProcessor();

			std::string centre1 = ""; // this is the major counter.
			buf.DrawLineV(0, buf.Width, 0, { {}, { 60, 255, 255, 255 }, ' ' });
			buf.DrawLineV(0, (ruleset->GetCurrentTime() / ruleset->GetDuration()) * buf.Width, 0, { {}, { 60, 90, 255, 100 }, ' ' });
			auto length_text = std::to_string(int(ruleset->GetDuration() / 1000 / 60)) + ":" + std::to_string(std::abs(int(ruleset->GetDuration() / 1000) % 60));
			buf.DrawString(length_text, buf.Width - length_text.size() - 1, 0, {}, {});
			auto current_text = std::to_string(int(ruleset->GetCurrentTime() / 1000 / 60)) + ":" + std::to_string(std::abs(int(ruleset->GetCurrentTime() / 1000) % 60));
			buf.DrawString(current_text, 0, 0, {}, {});
			centre1.append(std::to_string(scp->Combo));
			centre1.push_back('x');
			centre1.append("     ");
			centre1.append(std::to_string((int)(scp->Score * 1000000)));
			centre1.append("     ");
			centre1.append(std::to_string(scp->Accuracy * 100));
			centre1.push_back('%');
			buf.DrawString(centre1, (buf.Width - centre1.size()) / 2, 1, { 255, 255, 255, 255 }, {});

			centre1 = "";
			centre1.append(std::to_string(scp->MaxCombo));
			centre1.push_back('x');
			centre1.push_back('(');
			centre1.append(std::to_string(scp->BeatmapMaxCombo));
			centre1.push_back('x');
			centre1.push_back(')');
			centre1.append("     ");
			centre1.append(std::to_string(scp->Rating));
			buf.DrawString(centre1, (buf.Width - centre1.size()) / 2, 2, { 255, 255, 255, 255 }, {});

			std::string corner = "";
			corner.append(std::to_string(scp->Mean));
			corner.append("ms");
			corner.append("(UR");
			corner.append(std::to_string((int)scp->Error));
			corner.append(")");
			buf.DrawString(corner, 0, buf.Height - 1, { 255, 255, 255, 255 }, {});
			if (ruleset->GameEnded) {
				buf.DrawString("按Escape键Continue.", 0, 0, {}, {});
			}
		}
	};
	virtual void Tick(double) {
		auto ruleset = &*this->ruleset;
		if (ruleset != 0) {
			ruleset->Update();
			if (!game_ended) {
				if (ruleset->GameEnded) {
					if (rec_input_handler == 0) {
						ruleset->GetScoreProcessor()->SaveRecord();
						std::filesystem::create_directory("Records");
						RecordPath = "Records/CmaniaRecord_" + std::to_string(HpetClock()) + ".bin";
						std::fstream ofs(RecordPath, std::ios::out | std::ios::binary);
						if (!ofs.good())
							__debugbreak();
						auto rec = ruleset->RulesetRecord;
						Binary::Write(ofs, rec);
						ofs.close();
						std::fstream ifs(RecordPath, std::ios::in | std::ios::binary);
						Record rec2{};
						Binary::Read(ifs, rec2);
						game->Raise("get_songs_cache");
						game_ended = true;
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
				ruleset->Resume();
				pause = false;
			}
			return;
		}
		if (kea.Pressed && kea.RepeatCount <= 1) {
			if (kea.Key == ConsoleKey::Escape) {
				if (ruleset->GameEnded) {
					parent->Back();
					return;
				}
				pause = true;
				ruleset->Pause();
			}
			if (kea.Key == ConsoleKey::Spacebar) {
				ruleset->Skip();
			}
		}
		if (def_input_handler == nullptr)
			return;
		def_input_handler->OnKeyEvent(kea.Key, kea.Pressed);
	};
	virtual void Wheel(WheelEventArgs wea){

	};
	virtual void Move(MoveEventArgs mea){

	};
	virtual void Activate(bool yes) {
		if (!yes) {
			ruleset = 0;
		}
		else {
			if (ruleset != 0) {
				ruleset->LoadSettings(game->Settings);
				ruleset->Load(beatmap_path);
			}
		}
	};
	virtual void MouseKey(MouseKeyEventArgs mkea){

	};
	virtual void ProcessEvent(const char* evt, const void* evtargs) {
		if (game_ended && strcmp(evt, "songs_cache_ready") == 0) {
			auto& screa = *(SongsCahceReadyEventArgs*)evtargs;
			auto& caches = screa.Songs->caches;
			auto match = std::find_if(caches.begin(), caches.end(), [&](SongsCacheEntry& c) { return c.path == std::filesystem::path(beatmap_path).parent_path(); });
			if (match != caches.end()) {
				auto& diffcache = match->difficulties;
				auto match2 = std::find_if(diffcache.begin(), diffcache.end(), [&](DifficultyCacheEntry& c) { return c.path == std::filesystem::path(beatmap_path); });
				if (match2 != diffcache.end()) {
					match2->records.push_back(RecordPath);
				}
			}
			std::ofstream ofs("Songs.bin", std::ios::out | std::ios::binary);
			if (ofs.good()) {
				Binary::Write(ofs, *screa.Songs);
			}
		}
	};
};

Screen* MakeGameplayScreen(const std::string& bmp_path, OsuMods mod) {
	return new GameplayScreen(bmp_path, mod);
}

Screen* MakeGameplayScreen(Record rec, const std::string& bmp_path) {
	return new GameplayScreen(rec, bmp_path, rec.Mods);
}
