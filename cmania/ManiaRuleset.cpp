#pragma once
#include "Defines.h"
#include "Animator.h"
#include "ManiaObject.h"
#include "OsuBeatmap.h"
#include "ManiaScoreProcessor.h"
#include "File.h"
#include "BassAudioManager.h"
#include "ConsoleInput.h"
#include "KeyBinds.h"
#include "OsuSample.h"
#include "Gameplay.h"
#include "Ruleset.h"
#include "Crc.h"

class ManiaGameplay : public GameplayBase {

public:
	std::vector<Animator<PowerEasingFunction<1.5>>> KeyHighlight;
	Animator<PowerEasingFunction<4.0>> LastHitResultAnimator{ 255, 0, 400 };
	HitResult LastHitResult = HitResult::None;
	AudioStream bgm;
	double scrollspeed = 0;
	std::string skin_path;
	double offset = 0;
	double first_obj = 1e300;
	double end_obj = -1e300;
	double resume_time = -1e300;
	int keys = 0;
	bool jump_helper = false;
	bool no_hs = false;
	bool wt_mode = false;
	bool tail_hs = false;
	double last_rec = 0;
	double miss_offset = 200;
	ManiaScoreProcessor RulesetScoreProcessor{};
	ManiaGameplay() {
		KeyHighlight.resize(18, { 180, 0, 150 });
	}
	virtual ScoreProcessorBase* GetScoreProcessor() override {
		return &RulesetScoreProcessor;
	}
	virtual void Load(::Ruleset* rul, ::Beatmap* bmp) override {
		auto am = GetBassAudioManager(); // 获取Bass引擎

		if (bmp->RulesetId() != "osumania") {
			throw std::exception("Provide a osu!mania beatmap to this gameplay.");
		}

		this->Beatmap = bmp;
		this->Ruleset = rul;

		// 加载bgm
		{
			auto dat = ReadAllBytes(bmp->BgmPath().string());
			bgm = AudioStream(am->load(dat.data(), dat.size()));
		}

		if (GameInputHandler == 0)
			throw std::invalid_argument("RulesetInputHandler mustn't be nullptr.");

		keys = bmp->GetDifficultyValue("Keys");

		auto binds = Select(
			GetKeyBinds(keys), [](const auto& val) -> auto { return (int)val; })
						 .ToList<int>();

		GameInputHandler->SetBinds(binds);
		first_obj = bmp->FirstObject();
		end_obj = first_obj + bmp->Length();
		GameRecord.Mods = Mods;
		GameRecord.RatingGraph.resize(((end_obj - first_obj) + 11000) / 100);

		auto od = Beatmap->GetDifficultyValue("OD");

		RulesetScoreProcessor.RulesetRecord = &GameRecord;
		RulesetScoreProcessor.SetDifficulty(od);
		RulesetScoreProcessor.SetMods(Mods);

		RulesetScoreProcessor.BeatmapMaxCombo = bmp->MaxCombo();
		GameRecord.BeatmapHash = bmp->BeatmapHashcode();
		GameRecord.BeatmapTitle = bmp->Title();
		GameRecord.BeatmapVersion = bmp->Version();

		auto diff = Ruleset->CalculateDifficulty(bmp, Mods);
		RulesetScoreProcessor.ApplyBeatmap(diff * GetPlaybackRate(Mods));

		miss_offset = GetHitRanges(od)[HitResult::Meh];

		Clock.SetRate(GetPlaybackRate(Mods));
		Clock.Offset(std::min(first_obj - 5000, -3000.0)); // 让玩家有时间准备击打
		Clock.Start();									   // 开始Hpet计时器

		GameInputHandler->SetClockSource(Clock);

		GameStarted = true;
	}
	void ProcessAction(int action, bool pressed, double clock) {
		if (!pressed) {
			auto first_hold = Beatmap->super<ManiaObject>() > Where([&](ManiaObject& obj) -> bool {
				return obj.Column == action && obj.IsHold() && !(obj.HasHold || obj.HoldBroken) && obj.HasHit;
			}) > FirstOrDefault();
			if (first_hold != 0) {
				auto result = RulesetScoreProcessor.ApplyHit(*first_hold, clock - first_hold->EndTime);
				if (result != HitResult::None) {
					if (tail_hs) {
						first_hold->PlaySample();
					}
					LastHitResult = result;
					LastHitResultAnimator.Start(Clock.Elapsed());
				}
			}
			return;
		}
		else {
			auto first_hit = Beatmap->super<ManiaObject>() > Where([&](ManiaObject& obj) -> bool { return obj.Column == action && !obj.HasHit; }) > FirstOrDefault();
			if (first_hit != 0) {
				auto result = RulesetScoreProcessor.ApplyHit(*first_hit, clock - first_hit->StartTime);
				first_hit->PlaySample();
				if (result != HitResult::None) {
					LastHitResult = result;
					LastHitResultAnimator.Start(Clock.Elapsed());
				}
			}
		}
	}
	virtual void Update() override {
		if (GameEnded)
			return;
		auto time = Clock.Elapsed();

		if (!wt_mode) {
			for (int i = 0; i < 18; i++) {
				if (GameInputHandler->GetKeyStatus(i)) {
					KeyHighlight[i].Start(time);
				}
			}
		}

		if (bgm != 0 && (time > bgm->getDuration() * 1000 + 3000 || time > end_obj + 3000)) {
			GameEnded = true;
			Clock.Stop();
			return;
		}

		if (time > first_obj) {
			GameRecord.RatingGraph[(time - first_obj) / 100] = RulesetScoreProcessor.Rating;
		}

		if (time < resume_time || !Clock.Running())
			return;

		if (bgm != 0 && time > -std::max(offset * Clock.ClockRate(), 0.0) - 30 && time < bgm->getDuration() * 1000 - 3000) {
			if (!bgm->isPlaying()) {
				if (!bgm->isPaused()) // 这里用了一些小窍门让音频和Clock保持同步
				{
					Clock.Stop();
					bgm->setPlaybackRate(Clock.ClockRate());
					bgm->play();
					while (bgm->getCurrent() < 0.003) {
					}
					Clock.Reset();
					Clock.Offset(bgm->getCurrent() * 1000 + offset * Clock.ClockRate());
					Clock.Start();
				}
				else {
					bgm->pause(false);
				}
			}
			else {
				auto err = time - bgm->getCurrent() * 1000 - offset * Clock.ClockRate();
				if (std::abs(err) > 150) {
					bgm->setCurrent(time / 1000); // 调整...

					Clock.Stop();
					while (bgm->getCurrent() < time / 1000 + 0.003) {
					}
					Clock.Reset();
					Clock.Offset(bgm->getCurrent() * 1000 + offset * Clock.ClockRate());
					Clock.Start();
				}
			}
		}

		Beatmap->super<ManiaObject>() > ForEach([&](ManiaObject& obj) {
			if (obj.IsHold() && !(obj.HasHold || obj.HoldBroken) && !wt_mode) {
				if (time > obj.StartTime + miss_offset) {
					if (GameInputHandler->GetKeyStatus(obj.Column)) {
						obj.LastHoldOff = time;
					}
					if (time > obj.EndTime + miss_offset || (obj.LastHoldOff != -1 && time > obj.LastHoldOff + miss_offset)) {
						RulesetScoreProcessor.ApplyHit(obj, 1.0 / 0 * 0);
						LastHitResult = HitResult::Miss;
						LastHitResultAnimator.Start(Clock.Elapsed());
						return;
					}
				}
			}
			if (!obj.HasHit) {
				if (time > obj.StartTime + miss_offset) {
					RulesetScoreProcessor.ApplyHit(obj, 1.0 / 0 * 0);
					LastHitResult = HitResult::Miss;
					LastHitResultAnimator.Start(Clock.Elapsed());
				}
			}

			// Handles sliding sample.
			if (obj.HasHit && obj.IsHold()) {
				if (wt_mode) {
					if (time > obj.StartTime && time < obj.EndTime) {
						if (obj.ssample != 0 && obj.ssample_stream == 0) {
							obj.ssample_stream = AudioStream(obj.ssample->generateStream());
							obj.ssample_stream->play();
						}
						if (obj.ssamplew != 0 && obj.ssamplew_stream == 0) {
							obj.ssamplew_stream = AudioStream(obj.ssamplew->generateStream());
							obj.ssamplew_stream->play();
						}
						if (obj.ssample_stream && !obj.ssample_stream->isPlaying()) {
							obj.ssample_stream->play();
						}
						if (obj.ssamplew_stream && !obj.ssamplew_stream->isPlaying()) {
							obj.ssamplew_stream->play();
						}
					}
					if (time > obj.EndTime) {
						if (obj.ssample_stream) {
							obj.ssample_stream->stop();
							obj.ssample_stream = 0;
						}
						if (obj.ssamplew_stream) {
							obj.ssamplew_stream->stop();
							obj.ssamplew_stream = 0;
						}
					}
				}
				else {
					if (!(obj.HasHold || obj.HoldBroken)) {
						if (GameInputHandler->GetKeyStatus(obj.Column)) // Pressed
						{
							if (obj.ssample != 0 && obj.ssample_stream == 0) {
								obj.ssample_stream = AudioStream(obj.ssample->generateStream());
								obj.ssample_stream->play();
							}
							if (obj.ssamplew != 0 && obj.ssamplew_stream == 0) {
								obj.ssamplew_stream = AudioStream(obj.ssamplew->generateStream());
								obj.ssamplew_stream->play();
							}
						}
						if (obj.ssample_stream && !obj.ssample_stream->isPlaying()) {
							obj.ssample_stream->play();
						}
						if (obj.ssamplew_stream && !obj.ssamplew_stream->isPlaying()) {
							obj.ssamplew_stream->play();
						}
					}
					else {
						if (obj.ssample_stream) {
							obj.ssample_stream->stop();
							obj.ssample_stream = 0;
						}
						if (obj.ssamplew_stream) {
							obj.ssamplew_stream->stop();
							obj.ssamplew_stream = 0;
						}
					}
				}
			}
		});

		while (true) {
			auto evt = GameInputHandler->PollEvent();
			if (!evt.has_value())
				break;
			auto& e = *evt;
			GameRecord.Events.push_back(e);
			if (!wt_mode || e.Pressed)
				ProcessAction(e.Action, e.Pressed, e.Clock);
			if (wt_mode && e.Pressed)
				KeyHighlight[e.Action].Start(time);
		}
	}
	double CalcFlashlight(OsuMods mods, double ratio) {
		if (ratio > 1 && ratio < 0)
			return 1;
		if (HasFlag(mods, OsuMods::Hidden)) {
			if (ratio < 0.4) {
				return pow(ratio / 0.4, 2);
			}
		}
		if (HasFlag(mods, OsuMods::FadeOut)) {
			if (ratio > 0.6) {
				return pow((1 - ratio) / 0.4, 2);
			}
		}
		return 1;
	}
	virtual void Render(GameBuffer& buffer) override {
		auto e_ms = Clock.Elapsed();
		double key_width = 10;
		double key_height = std::clamp(buffer.Height / 50.0, 0.5, 2.0);
		key_width = int(std::min(std::max(key_width, (double)buffer.Width * 0.3 / keys), (double)buffer.Width / keys * 2 - 3));
		double centre = (double)buffer.Width / 2;
		double centre_start = centre - (keys * key_width) / 2;
		double judge_height = int(std::max((key_height + 1) * 2, 4.0));
		auto j = 0;
		for (double i = centre_start; i < keys * key_width + centre_start; i += key_width) {
			int visible = 0;
			auto scrollspeed = this->scrollspeed * Clock.ClockRate();
			for (auto& obj : Beatmap->super<ManiaObject>()) {
				auto off = obj.StartTime - e_ms;
				auto off2 = obj.EndTime - e_ms ;
				if (!obj.IsHold()) {
					if (off > scrollspeed || off < -scrollspeed / 5)
						continue;
				}
				else {
					if (off > scrollspeed || off2 < 0)
						continue;
				}
				if (visible > 20)
					break;
				if (obj.Column == j) {
					auto ratio = 1 - (obj.StartTime - e_ms) / scrollspeed;
					auto starty = ratio * (buffer.Height - judge_height + 1);
					auto base = Color{ 0, 0, 160, 230 };
					auto flashlight_num = CalcFlashlight(Mods, ratio);
					if (obj.Multi)
						base = { 0, 204, 187, 102 };
					if (obj.IsHold() && !obj.HasHold) {
						auto ratio2 = 1 - (obj.EndTime - e_ms) / scrollspeed;
						auto endy = ratio2 * (buffer.Height - judge_height);
						auto a = obj.HasHit && !obj.HoldBroken ? std::min(starty, buffer.Height - judge_height) : starty;
						base.Alpha = 180;
						if (HasFlag(Mods, OsuMods::FadeOut) || HasFlag(Mods, OsuMods::Hidden)) {
							base.Alpha = 50;
						}
						if (obj.HoldBroken) {
							base.Alpha = base.Alpha * 0.2;
						}
						buffer.FillRect(int(i + 1), a, int(i + key_width), endy + key_height, { {}, base, ' ' });
						base.Alpha = (unsigned char)(255 * flashlight_num);
						if (a >= buffer.Height - judge_height) {
							base.Alpha = 255;
						}
						if (obj.HoldBroken) {
							base.Alpha = base.Alpha * 0.2;
						}
						buffer.FillRect(int(i + 1), a - key_height, int(i + key_width), a + key_height, { {}, base, ' ' });
						continue;
					}
					base.Alpha = (unsigned char)(255 * flashlight_num);
					if (!obj.HasHit)
						buffer.FillRect(int(i + 1), starty - key_height, int(i + key_width), starty + key_height, { {}, base, ' ' });
					visible++;
				}
			}

			// 绘制 Mania 台阶
			auto clr = Color{ 255, 204, 187, 102 };
			buffer.FillRect(int(i + 1), buffer.Height - judge_height + 1, int(i + key_width), buffer.Height, { {}, { 120, 255, 255, 255 }, ' ' });

			KeyHighlight[j].Update(e_ms, [&](double light) {
				buffer.FillRect(int(i + 1), buffer.Height - judge_height + 1, int(i + key_width), buffer.Height, { {}, { (unsigned char)light, 255, 255, 255 }, ' ' });
				auto ratio = light / 240;
				auto lightning_height = std::max(15.0, buffer.Height * 0.3);
				if (ratio > 0 && ratio < 1) {
					for (int p = 0; p < ratio * lightning_height; p++) {
						buffer.DrawLineV(int(i + 1), int(i + key_width), int(buffer.Height - judge_height - p), { {}, { (unsigned char)(light * pow((ratio * lightning_height - p) / (ratio * lightning_height), 2)), 255, 255, 255 }, ' ' });
					}
				}
			});
			buffer.DrawLineH(i, 0, buffer.Height, { clr, {}, '|' });
			buffer.DrawLineH(i + key_width, 0, buffer.Height, { clr, {}, '|' });
			j++;
		}

		LastHitResultAnimator.Update(e_ms, [&](double val) {
			if (LastHitResult == HitResult::None)
				return;
			auto label = GetHitResultName(LastHitResult);
			auto color = GetHitResultColor(LastHitResult);
			color.Alpha = (unsigned char)val;
			buffer.DrawString(label, (buffer.Width - label.size()) / 2, buffer.Height / 2, color, {});
		});
	}
	virtual void Skip() override {
		if (first_obj - 3000 > 1000 && Clock.Elapsed() < first_obj - 3000) {
			Clock.Reset();
			Clock.Offset(first_obj - 3000);
		}
	}
	virtual double GetCurrentTime() override {
		return Clock.Elapsed() - first_obj;
	}
	virtual double GetDuration() override {
		return end_obj - first_obj;
	}
	virtual std::string GetBgPath() override {
		return Beatmap->BgPath().string();
	}
	virtual Record GetAutoplayRecord() override {
		Record record{};
		record.PlayerName = "Autoplay";
		record.Events.clear();

		for (auto& obj : Beatmap->super<ManiaObject>()) {
			InputEvent evt{};
			evt.Action = obj.Column;
			evt.Pressed = true;
			evt.Clock = obj.StartTime;
			record.Events.push_back(evt);

			if (obj.IsHold()) {
				// Handle holds
				evt.Pressed = false;
				evt.Clock = obj.EndTime;
				record.Events.push_back(evt);
			}
			else {
				evt.Pressed = false;
				evt.Clock = obj.StartTime + 50;
				record.Events.push_back(evt);
			}
		}
		std::sort(record.Events.begin(), record.Events.end(), [](auto& a, auto& b) { return a.Clock < b.Clock; });

		return record;
	}
};
class ManiaBeatmap : public Beatmap {
public:
	OsuBeatmap orig_bmp;
	std::vector<ManiaObject> storage;
	path bmp_root;
	Hash bmp_hash;
	double first_obj;
	double last_obj;
	size_t maxcombo;
	virtual std::string RulesetId() const noexcept {
		return "osumania";
	}
	virtual std::string Title() const noexcept {
		return orig_bmp.Title;
	}
	virtual std::string Version() const noexcept {
		return orig_bmp.Version;
	}
	virtual Hash BeatmapHashcode() const noexcept {
		return bmp_hash;
	}
	virtual double Length() const noexcept {
		return last_obj - first_obj;
	}
	virtual size_t MaxCombo() const noexcept {
		return maxcombo;
	}
	virtual double FirstObject() const noexcept {
		return first_obj;
	}
	virtual size_t size() const noexcept {
		return storage.size();
	}
	virtual void* GetBeatmap() {
		return (ManiaBeatmap*)this;
	}
	virtual HitObject& at(size_t i) const {
		return (HitObject&)storage.at(i);
	}
	virtual path BgPath() const noexcept {
		return bmp_root / orig_bmp.Background;
	}
	virtual path BgmPath() const noexcept {
		return bmp_root / orig_bmp.AudioFilename;
	}
	virtual double GetDifficultyValue(std::string key) const noexcept {
		if (key == "OD") {
			return orig_bmp.OverallDifficulty;
		}
		if (key == "Keys") {
			return (int)orig_bmp.CircleSize;
		}
		return 0;
	}
	virtual std::unordered_set<std::string> GetDifficultyValues() const noexcept {
		return { "OD", "Keys" };
	}
};

class ManiaRuleset : public Ruleset {
	BinaryStorage* settings;

public:
	virtual std::string Id() {
		return "osumania";
	}
	virtual std::string DisplayName() {
		return "Mania";
	}
	virtual Beatmap* LoadBeatmap(path beatmap_path) {
		auto beatmap = new ManiaBeatmap();

		std::ifstream ifs(beatmap_path);
		if (!ifs.good())
			throw std::exception("Failed to open beatmap file.");

		OsuBeatmap osub = OsuBeatmap::Parse(ifs);
		ifs.close();
		ifs.open(beatmap_path, std::ios::binary);
		ifs.seekg(0, std::ios::end);
		auto sz = ifs.tellg();
		auto buf = new char[sz];
		memset(buf, 0, sz);
		ifs.seekg(0);
		ifs.get(buf, sz);
		beatmap->bmp_hash = GetCrc(buf, sz);
		beatmap->orig_bmp = osub;
		auto parent = beatmap_path.parent_path();
		beatmap->bmp_root = parent;
		int keys = (int)osub.CircleSize;
		std::set<std::string> Samples;
		Samples >
			AddRangeSet(osub.HitObjects >
						Select([](const auto& ho) -> std::string { return ho.CustomSampleFilename; }) >
						Where([](const auto& str) -> bool { return !str.empty(); }) >
						Select([&](const auto& str) -> auto { return (parent / str).string(); })) >
			AddRangeSet(osub.StoryboardSamples > Select([&](const auto& item) -> auto { return (parent / item.path).string(); }));

		auto SampleIndex = BuildSampleIndex(parent, 1); // 构建谱面采样索引(sampleset==1默认)
		auto skin_path = (*settings)["SkinPath"].GetString();
		if (skin_path.empty()) {
			skin_path = "Samples\\Triangles";
		}
		(*settings)["SkinPath"].SetArray(skin_path.data(), skin_path.size());
		auto wt_mode = (*settings)["WtMode"].Get<bool>();
		auto SkinSampleIndex = BuildSampleIndex(skin_path, 0); // 构建皮肤采样索引(sampleset==0)

		auto selector = [](const AudioSampleMetadata& md) -> auto {
			return md.filename.string();
		}; // linq 查询

		Samples > AddRangeSet(Select(SampleIndex, selector))  // 添加谱面采样路径
			> AddRangeSet(Select(SkinSampleIndex, selector)); // 添加皮肤采样路径

		auto am = GetBassAudioManager();

		std::map<std::string, AudioSample> SampleCaches; // 采样缓存

		Samples > ForEach([&](const std::string& path) {
			try {
				auto dat = ReadAllBytes(path);
				SampleCaches[path] = AudioSample(am->loadSample(dat.data(), dat.size()));
			}
			catch (...) {
			}
		});

		// 加载物件
		beatmap->storage > AddRange(Select(
							   osub.HitObjects, [&](const OsuBeatmap::HitObject& obj) -> auto {
								   ManiaObject mo{};

								   // 计算物件的列
								   mo.Column = CalcColumn(obj.X, keys);

								   auto is_slider = HasFlag(obj.Type, HitObjectType::Slider);
								   auto is_hold = is_slider || HasFlag(obj.Type, HitObjectType::Hold) || HasFlag(obj.Type, HitObjectType::Spinner);

								   // 复制起始和终止时间
								   mo.StartTime = obj.StartTime;
								   
								   beatmap->first_obj = std::min(beatmap->first_obj, obj.StartTime);
								   beatmap->last_obj = std::max(beatmap->last_obj, obj.StartTime);
								   if (is_hold) {
									   mo.EndTime = obj.EndTime;
									   if (is_slider)
									   {
										   
									   }
									   beatmap->last_obj = std::max(beatmap->last_obj, mo.EndTime);
								   }

								   // 计算是否为多押
								   osub.HitObjects > ForEach([&](const auto& obj2) {
									   if (&obj2 != &obj && (std::abs(obj.StartTime - obj2.StartTime) < 0.5 || (obj2.EndTime != 0 && std::abs(obj.StartTime - obj2.EndTime) < 0.5)))
										   mo.Multi = true;
								   });

								   // 获取最近的 TimingPoint
								   auto& tp = GetTimingPoint(osub, obj.StartTime);

								   // 将 fspath 转换为 AudioSample
								   auto sample_selector = [&](const std::filesystem::path& sample) -> auto {
									   return SampleCaches[sample.string()];
								   };

								   // 给物件加载 Samples
								   mo.samples > AddRange(GetSampleLayered(SampleIndex, SkinSampleIndex, tp.SampleBank, obj.SoundType, tp.SampleSet) > Select(sample_selector));

								   if (!obj.CustomSampleFilename.empty()) {
									   mo.samples.push_back(SampleCaches[(parent / obj.CustomSampleFilename).string()]);
								   }

								   beatmap->maxcombo++;

								   // 判断是否是 Hold
								   if (is_hold) {
									   // 加载滑动音效
									   First(GetSampleLayered(SampleIndex, SkinSampleIndex, tp.SampleBank, HitSoundType::Slide, tp.SampleSet) > Select(sample_selector), mo.ssample);

									   // 加载 Whistle 滑动音效
									   if (HasFlag(obj.SoundType, HitSoundType::Whistle))
										   First(GetSampleLayered(SampleIndex, SkinSampleIndex, tp.SampleBank, HitSoundType::SlideWhistle, tp.SampleSet) > Select(sample_selector), mo.ssamplew);

									   if (!wt_mode)
										   beatmap->maxcombo++;
								   }

								   return mo;
							   }));

		return beatmap;
	}
	virtual GameplayBase* GenerateGameplay() {
		auto obj = new ManiaGameplay();
		auto val = (*settings)["ScrollSpeed"].Get<double>();
		if (val < 10.0) {
			val = 500;
		}
		obj->scrollspeed = val = std::clamp(val, 10.0, 100000.0);
		(*settings)["ScrollSpeed"].Set<double>(val);
		return obj;
	}
	virtual double CalculateDifficulty(Beatmap* bmp, OsuMods mods) {
		auto GetTimeDiff = [&](double t) {
			t = std::abs(t);
			if (t > 0.5)
				return 6 / t;
			return 6 / 0.5;
		};
		double drain_time = 0;
		std::vector<double> diffs;
		auto objs = bmp->super<ManiaObject>();
		for (int i = 0; i < objs.size() - 1; i++) {
			auto& ho = objs[i];

			double lastjudge = ho.StartTime;
			double timeDiff = 0;
			if (ho.IsHold()) {
				lastjudge = ho.EndTime;
			}
			double nearestSameColumnTime = -1;
			for (int j = i; j < objs.size() - 1; j++) {
				auto& ho2 = objs[j];
				if (ho2.Column == ho.Column && ho2.StartTime > (lastjudge + 1)) {
					nearestSameColumnTime = ho2.StartTime;
					break;
				}
			}
			if (nearestSameColumnTime != -1)
				timeDiff += GetTimeDiff(nearestSameColumnTime - lastjudge);
			auto multi = 1.0;
			double nearestTime = -1;
			for (int j = i; j < objs.size() - 1; j++) {
				auto& ho2 = objs[j];
				if (ho2.StartTime > (lastjudge + 1)) {
					nearestTime = ho2.StartTime;
					auto between = abs(ho2.StartTime - ho.StartTime);
					if (between < 500)
						drain_time += between;
					break;
				}
			}
			if (nearestTime != -1)
				timeDiff += GetTimeDiff(nearestTime - lastjudge);
			for (int j = 0; j < objs.size() - 1; j++) {
				auto& ho2 = objs[j];
				if (&ho2 == &ho)
					continue;

				if (std::abs(ho2.StartTime - ho.StartTime) < 0.1 || (ho2.IsHold() && std::abs(ho2.EndTime - ho.StartTime) < 0.1) || (ho2.EndTime != 0 && ho.EndTime != 0 && (std::abs(ho2.EndTime - ho.EndTime) < 0.1 || std::abs(ho2.StartTime - ho.EndTime) < 0.1))) {
					multi *= 1.06;
				}
				if (ho2.IsHold() && ho.StartTime >= ho2.StartTime && ho.StartTime <= ho2.EndTime) {
					multi *= 1.06;
				}
				if (std::abs(nearestTime - ho2.StartTime) < 0.1) {
					auto coldiff = pow(ho2.Column - ho.Column, 2);
					multi *= 1 + coldiff * 0.04;
				}
			}
			diffs.push_back(timeDiff * multi);
		}
		std::sort(diffs.begin(), diffs.end());
		double avgdiff = 0;
		size_t diffnum = diffs.size();
		double top20diff = 0;
		size_t top20num = diffs.size() / 5;
		for (size_t k = 0; k < diffnum; k++) {
			if (k >= diffnum - top20num) {
				top20diff += diffs[k];
			}
			avgdiff += std::max(0.003, diffs[k]);
		}
		avgdiff /= diffnum; // avgdiff 与耐力相关
		avgdiff *= 0.6;
		top20diff /= top20num;
		return (avgdiff + top20diff) * 20;
	}
	virtual DifficultyInfo PopulateDifficultyInfo(Beatmap* bmp) {
		Assert(bmp->RulesetId() == "osumania");
		ManiaBeatmap& mb = *(ManiaBeatmap*)bmp;
		DifficultyInfo di;
		di.push_back(DifficultyInfoItem::MakeValue("Durtaion(s)", bmp->Length() / 1000));
		di.push_back(DifficultyInfoItem::MakeText("BeatmapHash", Hex(bmp->BeatmapHashcode())));
		di.push_back(DifficultyInfoItem::MakeText("MaxCombo", std::to_string(bmp->MaxCombo())));
		di.push_back(DifficultyInfoItem::MakeText("Objects", std::to_string(bmp->size())));
		di.push_back(DifficultyInfoItem::MakeHeader("Difficulty values"));
		di.push_back(DifficultyInfoItem::MakeValueBar("Keys", bmp->GetDifficultyValue("Keys"), 0, 20));
		di.push_back(DifficultyInfoItem::MakeValueBar("OD", bmp->GetDifficultyValue("OD"), 0, 20));
		di.push_back(DifficultyInfoItem::MakeValueBar("Star(NM)", CalculateDifficulty(bmp, OsuMods::None), 0, 15));
		di.push_back(DifficultyInfoItem::MakeHeader("Hitwindow"));
		static constexpr auto mania_hitres = { HitResult::Perfect, HitResult::Great, HitResult::Good, HitResult::Ok, HitResult::Meh, HitResult::Miss };
		auto hitranges = GetHitRanges(bmp->GetDifficultyValue("OD"));
		auto max = 200;
		for (auto res : mania_hitres) {
			di.push_back(DifficultyInfoItem::MakeValueBar(GetHitResultName(res), hitranges[res], 0, max + 20));
		}
		return di;
	}
	virtual void Init(BinaryStorage& settings) override {
		this->settings = &settings;
	}
};

Ruleset* MakeManiaRuleset() {
	return new ManiaRuleset();
}