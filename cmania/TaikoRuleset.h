#pragma once
#include <vector>
#include "Animator.h"
#include "TaikoObject.h"
#include "Ruleset.h"
#include "OsuBeatmap.h"
#include "TaikoScoreProcessor.h"
#include <filesystem>
#include <set>
#include "File.h"
#include "BassAudioManager.h"
#include "ConsoleInput.h"
#include "KeyBinds.h"
#include "OsuSample.h"

class TaikoRuleset : public Ruleset<TaikoObject> {
	std::vector<Animator<PowerEasingFunction<1.5>>> KeyHighlight;
	Animator<PowerEasingFunction<4.0>> LastHitResultAnimator{ 255, 0, 400 };
	HitResult LastHitResult = HitResult::None;
	AudioStream bgm;
	double scrollspeed = 0;
	double endtime = 0;
	std::string skin_path;
	OsuBeatmap orig_bmp;
	std::filesystem::path parent_path;
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

public:
	TaikoRuleset() {
		RulesetScoreProcessor = new TaikoScoreProcessor();
		KeyHighlight.resize(18, { 180, 0, 150 });
	}
	~TaikoRuleset() {
		delete RulesetScoreProcessor;
	}
	// 通过 Ruleset 继承
	virtual void LoadSettings(BinaryStorage& settings) override {
		scrollspeed = settings["ScrollSpeed"].Get<double>();
		if (scrollspeed <= 0) {
			scrollspeed = 500;
		}
		offset = settings["Offset"].Get<double>();
		no_hs = settings["NoBmpHs"].Get<bool>();

		jump_helper = settings["JumpHelper"].Get<bool>();
		tail_hs = settings["TailHs"].Get<bool>();
		const char* skinpath = settings["SkinPath"].GetString();
		if (skinpath == 0 || !std::filesystem::exists(skinpath)) {
			skinpath = "Samples/Triangles";
		}
		skin_path = skinpath;
		auto& binds = settings["KeyBinds"];
		if (binds.Size < key_binds.size() * sizeof(ConsoleKey)) {
			binds.SetArray(key_binds.data(), key_binds.size());
			settings.Write();
		}
		auto dat = binds.GetArray<ConsoleKey>();
		std::copy(dat, dat + key_binds.size(), key_binds.begin());
	}
	virtual void Load(std::filesystem::path beatmap_path) override {
		auto am = GetBassAudioManager(); // 获取Bass引擎

		// 获取osu谱面的父目录（也就是谱面根目录）
		std::filesystem::path parent = beatmap_path.parent_path();
		parent_path = parent;

		// 打开并解析osu谱面文件
		{
			// 打开osu谱面文件
			auto ifs = std::ifstream(beatmap_path);
			if (!ifs.good())
				throw std::invalid_argument("Cannot open file.");

			// 解析osu谱面
			orig_bmp = OsuBeatmap::Parse(ifs);

			RulesetRecord.BeatmapTitle = orig_bmp.Title;
			RulesetRecord.BeatmapVersion = orig_bmp.Version;
			RulesetRecord.BeatmapHash = 0;
		} // RAII 销毁 ifstream 资源

		keys = static_cast<int>(orig_bmp.CircleSize); // 获取键数

		// 存储需要加载的采样
		std::set<std::string> Samples;

		Samples > AddRangeSet(orig_bmp.HitObjects > Select([](const auto& ho) -> std::string { return ho.CustomSampleFilename; }) > Where([](const auto& str) -> bool { return !str.empty(); }) > Select([&](const auto& str) -> auto { return (parent / str).string(); })) > AddRangeSet(
																																																																				  orig_bmp.StoryboardSamples > Select([&](const auto& item) -> auto { return (parent / item.path).string(); }));

		auto SampleIndex = BuildSampleIndex(parent, 1);		   // 构建谱面采样索引(sampleset==1默认)
		auto SkinSampleIndex = BuildSampleIndex(skin_path, 0); // 构建皮肤采样索引(sampleset==0)

		auto selector = [](const AudioSampleMetadata& md) -> auto {
			return md.filename.string();
		}; // linq 查询

		Samples > AddRangeSet(Select(SampleIndex, selector))  // 添加谱面采样路径
			> AddRangeSet(Select(SkinSampleIndex, selector)); // 添加皮肤采样路径

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
		Beatmap > AddRange(Select(
					  orig_bmp.HitObjects, [&](const OsuBeatmap::HitObject& obj) -> auto {
						  TaikoObject to{};
						  first_obj = std::min(first_obj, obj.StartTime);
						  end_obj = std::max(end_obj, obj.StartTime);
						  to.StartTime = obj.StartTime;
						  auto tp = GetTimingPointTiming(orig_bmp, obj.StartTime);
						  auto bpm = tp.BPM();
						  auto blen = 60.0 / bpm;
						  auto vec = GetTimingPointNonTiming(orig_bmp, obj.StartTime).SpeedMultiplier();
						  auto odd_s = std::abs(orig_bmp.SliderTickRate - 3) < 0.01;
						  bool isKat = HasFlag(obj.SoundType, HitSoundType::Whistle) || HasFlag(obj.SoundType, HitSoundType::Clap);
						  bool isLarge = HasFlag(obj.SoundType, HitSoundType::Finish);
						  auto scoringdist = BASE_SCORING_DISTANCE * orig_bmp.SliderMultiplier * vec;
						  auto velocity = scoringdist / tp.BeatLength;
						  if (HasFlag(obj.Type, HitObjectType::Slider)) {
							  // 我们需要计算 Slider 需要击打几下
							  /*
							  摘要自wiki:

							  对于BPM 在 125 及以下的谱面，会给出 1/8 而非 1/4 黄条。

							  请注意，音乐中不常使用 1/8 节奏。不建议在 1/8 节奏下放置滑条。

							  也要注意，如果滑条点倍率为 3，则会给出 1/6 黄条。
							  */
							  auto snap = odd_s ? 6 : (bpm <= 125.0 ? 8 : 4);
							  blen /= snap;
							  auto duration = obj.Length / velocity;
							  auto hits = (int)(duration / blen);
							  to.TotalHits = to.RemainsHits = hits;
							  to.TickTime = blen;
							  to.ObjectType = ModifyFlag(to.ObjectType,isKat ? TaikoObject::Kat : TaikoObject::Don);
							  if (isLarge)
								  to.ObjectType = ModifyFlag(to.ObjectType, TaikoObject::Large);
							  to.EndTime = to.StartTime + duration;
						  }
						  else if (HasFlag(obj.Type, HitObjectType::Spinner)) {
							  auto snap = odd_s ? 6 : (bpm <= 125.0 ? 8 : 4);
							  blen /= snap;
							  auto duration = obj.Length / velocity;
							  auto hits = (int)(duration / blen);
							  to.TotalHits = to.RemainsHits = hits;
							  to.TickTime = blen;
							  to.ObjectType = TaikoObject::Spinner;
							  to.EndTime = to.StartTime + duration;
						  }
						  else if (HasFlag(obj.Type, HitObjectType::Circle)) {
							  to.ObjectType = ModifyFlag(to.ObjectType, isKat ? TaikoObject::Kat : TaikoObject::Don);
							  if (isLarge)
								  to.ObjectType = ModifyFlag(to.ObjectType, TaikoObject::Large);
						  }
						  return to;
					  }));

		// 加载bgm
		{
			auto dat = ReadAllBytes((parent / orig_bmp.AudioFilename).string());
			bgm = AudioStream(am->load(dat.data(), dat.size()));
		}

		if (RulesetInputHandler == 0)
			throw std::invalid_argument("RulesetInputHandler mustn't be nullptr.");

		auto binds = Select(
			GetKeyBinds(keys), [](const auto& val) -> auto { return (int)val; })
						 .ToList<int>();
		RulesetInputHandler->SetBinds(binds);

		RulesetRecord.Mods = Mods;
		RulesetRecord.RatingGraph.resize(((end_obj - first_obj) + 11000) / 100);

		RulesetScoreProcessor->RulesetRecord = &RulesetRecord;
		RulesetScoreProcessor->SetDifficulty(orig_bmp.OverallDifficulty);
		RulesetScoreProcessor->SetMods(Mods);

		RulesetScoreProcessor->ApplyBeatmap(1 * GetPlaybackRate(Mods));

		miss_offset = GetHitRanges(orig_bmp.OverallDifficulty)[HitResult::Meh];

		Clock.SetRate(GetPlaybackRate(Mods));
		Clock.Offset(std::min(first_obj - 5000, -3000.0)); // 让玩家有时间准备击打
		Clock.Start();									   // 开始Hpet计时器

		RulesetInputHandler->SetClockSource(Clock);

		GameStarted = true;
	}
	virtual void Update() override {
		if (GameEnded)
			return;
		auto time = Clock.Elapsed();

		if (bgm != 0 && (time > bgm->getDuration() * 1000 + 3000 || time > end_obj + 3000)) {
			GameEnded = true;
			Clock.Stop();
			return;
		}

		if (time > first_obj) {
			RulesetRecord.RatingGraph[(time - first_obj) / 100] = RulesetScoreProcessor->Rating;
		}

		if (time < resume_time || !Clock.Running())
			return;

		if (bgm != 0 && time > -30 && time < bgm->getDuration() * 1000 - 3000) {
			if (!bgm->isPlaying()) {
				if (!bgm->isPaused()) // 这里用了一些小窍门让音频和Clock保持同步
				{
					Clock.Stop();
					bgm->setPlaybackRate(Clock.ClockRate());
					bgm->play();
					while (bgm->getCurrent() < 0.003) {
					}
					Clock.Reset();
					Clock.Offset(bgm->getCurrent() * 1000 + offset);
					Clock.Start();
				}
				else {
					bgm->pause(false);
				}
			}
			else {
				auto err = time - bgm->getCurrent() * 1000;
				// auto str = std::to_string(err);
				// DbgOutput(str.c_str());
				// DbgOutput("\n");
				if (std::abs(err) > 150) // bgm get too far away from hpet timer
				{
					bgm->setCurrent(time / 1000); // seek earlier.

					// We doesn't really care about what will happen on a low end machine right?
				}
			}
		}
		while (true) {
			auto evt = RulesetInputHandler->PollEvent();
			if (!evt.has_value())
				break;
			auto& e = *evt;
			RulesetRecord.Events.push_back(e);
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
	}

	// 通过 Ruleset 继承
	virtual void Pause() override {
		for (auto& light : KeyHighlight) {
			light.Reset();
		}
		bgm->pause(true);
		Clock.Stop();
	}
	virtual void Resume() override {
		resume_time = Clock.Elapsed();
		Clock.Offset(-3000);
		Clock.Start();
	}
	virtual void Skip() override {
		if (first_obj - 3000 > 1000 && Clock.Elapsed() < first_obj - 3000) {
			Clock.Reset();
			Clock.Offset(first_obj - 3000);
		}
	}

	// 通过 Ruleset 继承
	virtual double GetCurrentTime() override {
		return Clock.Elapsed() - first_obj;
	}
	virtual double GetDuration() override {
		return end_obj - first_obj;
	}

	// 通过 Ruleset 继承
	virtual std::string GetBgPath() override {
		return (parent_path / orig_bmp.Background).string();
	}

	// 通过 Ruleset 继承
	virtual Record GetAutoplayRecord() override {
		Record record{};
		record.PlayerName = "Autoplay";
		record.Events.clear();
		return record;
	}
};