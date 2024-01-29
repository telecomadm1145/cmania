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
#include "TaikoRuleset.h"
#include "Crc.h"

class TaikoGameplay : public GameplayBase {
	Animator<PowerEasingFunction<1.5>> KeyHighlight[4]{
		{ 180, 0, 150 }, { 180, 0, 150 }, { 180, 0, 150 }, { 180, 0, 150 }
	};
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
	bool jump_helper = false;
	bool no_hs = false;
	bool wt_mode = false;
	bool tail_hs = false;
	double last_rec = 0;
	double miss_offset = 200;
	int SpinnerHits = 0;
	int SpinnerKatOrDon = 0;
	TaikoScoreProcessor ScoreProcessor;
	Transition<CubicEasingFunction, ConstantEasingDurationCalculator<50>> SpinnerHitTrans{};

public:
	virtual void RenderDebug(GameBuffer& buf) {
		buf.FillRect(0, 0, 50, 50, { {}, {100,20,20,20},' '});
	}
	virtual void Load(::Ruleset* rul, ::Beatmap* bmp) override {
		auto am = GetBassAudioManager(); // 获取Bass引擎

		if (bmp->RulesetId() != "osutaiko") {
			throw std::exception("Provide a osu!taiko beatmap to this gameplay.");
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

		auto binds = Select(
			GetKeyBinds(4), [](const auto& val) -> auto { return (int)val; })
						 .ToList<int>();

		GameInputHandler->SetBinds(binds);
		first_obj = bmp->FirstObject();
		end_obj = first_obj + bmp->Length();
		GameRecord.Mods = Mods;
		GameRecord.RatingGraph.resize(((end_obj - first_obj) + 11000) / 100);

		auto od = Beatmap->GetDifficultyValue("OD");

		ScoreProcessor.RulesetRecord = &GameRecord;
		ScoreProcessor.SetDifficulty(od);
		ScoreProcessor.SetMods(Mods);

		ScoreProcessor.BeatmapMaxCombo = bmp->MaxCombo();
		GameRecord.BeatmapHash = bmp->BeatmapHashcode();
		GameRecord.BeatmapTitle = bmp->Title();
		GameRecord.BeatmapVersion = bmp->Version();

		auto diff = Ruleset->CalculateDifficulty(bmp, Mods);
		ScoreProcessor.ApplyBeatmap(diff * GetPlaybackRate(Mods));

		miss_offset = GetHitRanges(od)[HitResult::Meh];

		Clock.SetRate(GetPlaybackRate(Mods));
		Clock.Offset(std::min(first_obj - 5000, -3000.0)); // 让玩家有时间准备击打
		Clock.Start();									   // 开始Hpet计时器

		GameInputHandler->SetClockSource(Clock);

		GameStarted = true;
	}
	void ProcessAction(int action, bool pressed, double clock) {
		auto hit_kat = (action == 1) || (action == 2) || (action == 17);
		if (pressed) {
			auto rx = HasFlag(Mods, OsuMods::Relax);
			auto first_hit = Beatmap->super<TaikoObject>() > Where([&](TaikoObject& obj) -> bool {
				if (!obj.HasHit && obj.RemainsHits > 0) {
					return true;
				}
				return !obj.HasHit && (obj.StartTime - clock < miss_offset) && !HasFlag(obj.ObjectType, TaikoObject::Barline) && !HasFlag(obj.ObjectType, TaikoObject::Slider);
			}) > FirstOrDefault();
			if (first_hit != 0) {
				auto kat = HasFlag(first_hit->ObjectType, TaikoObject::Kat);
				auto large = HasFlag(first_hit->ObjectType, TaikoObject::Large);
				if (first_hit->TotalHits > 0) {
					bool flag = false;
					if (SpinnerKatOrDon == 0) {
						flag = true;
					}
					else if ((SpinnerKatOrDon - 1) == kat) {
						flag = true;
					}
					if (flag)
						if (clock >= first_hit->StartTime - 3 && first_hit->RemainsHits > 0) {
							ScoreProcessor.ApplyHit(*first_hit, 0);
							first_hit->RemainsHits--;
						}
					SpinnerKatOrDon = kat + 1;
					goto goff;
				}
				if (HasFlag(first_hit->ObjectType, TaikoObject::SliderTick)) {
					if (clock > first_hit->StartTime - 3)
						ScoreProcessor.ApplyHit(*first_hit, 0);
					goto goff;
				}
				// miss if a wrong action has been pressed
				if (kat != hit_kat && !rx) {
					ScoreProcessor.ApplyHit(*first_hit, 1.0 / 0.0 * 0.0);
					LastHitResult = HitResult::Miss;
					LastHitResultAnimator.Start(Clock.Elapsed());
					goto goff;
				}
				auto result = ScoreProcessor.ApplyHit(*first_hit, clock - first_hit->StartTime);
				if (result != HitResult::None) {
					LastHitResult = result;
					LastHitResultAnimator.Start(Clock.Elapsed());
					hit_kat = kat;
					first_hit->PlaySample();
					return;
				}
			}
		goff:
			auto first_sound = Beatmap->super<TaikoObject>() > Where([&](TaikoObject& obj) -> bool {
				return !obj.HasHit && !HasFlag(obj.ObjectType, TaikoObject::Barline) && !HasFlag(obj.ObjectType, TaikoObject::SliderTick) && !HasFlag(obj.ObjectType, TaikoObject::Slider) && !(HasFlag(obj.ObjectType, TaikoObject::Kat) ^ hit_kat);
			}) > FirstOrDefault();
			if (first_sound != 0)
				first_sound->PlaySample();
		}
	}
	virtual void Update() override {
		if (GameEnded)
			return;
		auto time = Clock.Elapsed();

		if (!wt_mode) {
			for (int i = 0; i < 4; i++) {
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
			GameRecord.RatingGraph[(time - first_obj) / 100] = ScoreProcessor.Rating;
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
		Beatmap->super<TaikoObject>() > ForEach([&](TaikoObject& obj) {
			// if (obj.EndTime != 0 && !(obj.HasHold || obj.HoldBroken) && !wt_mode) {
			//	if (time > obj.StartTime + miss_offset) {
			//		if (RulesetInputHandler->GetKeyStatus(obj.Column)) {
			//			obj.LastHoldOff = time;
			//		}
			//		if (time > obj.EndTime + miss_offset || (obj.LastHoldOff != -1 && time > obj.LastHoldOff + miss_offset)) {
			//			RulesetScoreProcessor->ApplyHit(obj, 1.0 / 0 * 0);
			//			LastHitResult = HitResult::Miss;
			//			LastHitResultAnimator.Start(Clock.Elapsed());
			//			return;
			//		}
			//	}
			// }
			auto tick = HasFlag(obj.ObjectType, TaikoObject::SliderTick);
			if (tick) {
				if (time > obj.StartTime + miss_offset && !obj.HasHit) {
					obj.HasHit = true;
				}
				return;
			}
			auto spin = HasFlag(obj.ObjectType, TaikoObject::Spinner);
			if (spin) {
				if (!obj.HasHit)
					if (obj.RemainsHits <= 0 || time > obj.EndTime) {
						obj.HasHit = true;
						SpinnerHits = 0;
						SpinnerKatOrDon = 0;
					}
					else if (time > obj.StartTime && time < obj.EndTime) {
						SpinnerHits = obj.RemainsHits;
					}
				return;
			}
			auto large = HasFlag(obj.ObjectType, TaikoObject::Large);
			if (large) {
				if (obj.RemainsHits == -1 && time > obj.StartTime + miss_offset) {
					obj.HasHit = true;
				}
			}
			if (HasFlag(obj.ObjectType, TaikoObject::Slider)) {
				if (time > obj.EndTime) {
					obj.HasHit = true;
				}
				return;
			}
			if (!obj.HasHit && !HasFlag(obj.ObjectType, TaikoObject::Slider) && !HasFlag(obj.ObjectType, TaikoObject::Barline)) {
				if (time > obj.StartTime + miss_offset) {
					ScoreProcessor.ApplyHit(obj, 1.0 / 0 * 0);
					LastHitResult = HitResult::Miss;
					LastHitResultAnimator.Start(Clock.Elapsed());
				}
			}
		});

		while (true) {
			auto evt = GameInputHandler->PollEvent();
			if (!evt.has_value())
				break;
			auto& e = *evt;
			GameRecord.Events.push_back(e);

			if (wt_mode || e.Pressed) {
				ProcessAction(e.Action, e.Pressed, e.Clock);
			}
			if (wt_mode && e.Pressed && e.Action < 4) {
				KeyHighlight[e.Action].Start(time);
			}
		}
	}
	double CalcFlashlight(OsuMods mods, double ratio) {
		if (ratio > 1 && ratio < 0)
			return 1;
		auto hd = HasFlag(mods, OsuMods::Hidden);
		auto fo = HasFlag(mods, OsuMods::FadeOut);
		if (hd && fo) {
			if (ratio < 0.7) {
				return pow(ratio / 0.7, 3);
			}
			if (ratio > 0.3) {
				return pow((1 - ratio) / 0.7, 3);
			}
		}
		if (hd) {
			return pow(ratio, 4);
		}
		if (fo) {
			if (ratio > 0.55) {
				return pow((1 - ratio) / 0.45, 2);
			}
		}
		return 1;
	}
	virtual void Render(GameBuffer& buf) override {
		const auto rt = 1.75;
		auto e_ms = Clock.Elapsed();

		// now we need to render all objects.

		auto scale = buf.Height / 40.0;

		auto hitpos = PointI{ (int)(20 * scale) + 5, buf.Height * 3 / 8 };
		buf.FillRect(0, buf.Height / 4, buf.Width, buf.Height * 2 / 4, { {}, { 255, 40, 40, 40 }, ' ' });
		auto lightsz = hitpos.X / 5;
		for (size_t i = 0; i < 4; i++) {
			buf.FillRect((i)*lightsz + 2, buf.Height / 4, (i + 1) * lightsz + 2, buf.Height * 2 / 4, { {}, { 120, 80, 80, 80 }, ' ' });
			KeyHighlight[i].Update(e_ms, [&](double v) {
				Color clr{ 220, 20, 212, 255 };
				if ((i == 1) || (i == 2)) {
					clr = { 220, 255, 30, 30 };
				}
				clr.Alpha = v;
				buf.FillRect((i)*lightsz + 2, buf.Height / 4, (i + 1) * lightsz + 2, buf.Height * 2 / 4, { {}, clr, ' ' });
			});
		}
		for (auto& obj : Beatmap->super<TaikoObject>()) {
			if (obj.HasHit)
				continue;
			auto off = obj.StartTime - e_ms;
			Color fill = { 220, 20, 212, 255 };
			Color outter = { 255, 255, 255, 255 };
			double sz = scale * 6;
			if (HasFlag(obj.ObjectType, TaikoObject::Kat)) {
				fill = { 220, 255, 30, 30 };
			}
			if (HasFlag(obj.ObjectType, TaikoObject::SliderTick)) {
				fill = { 220, 255, 237, 77 };
				sz = scale * 3;
			}
			if (HasFlag(obj.ObjectType, TaikoObject::Slider)) {
				fill = { 220, 255, 237, 77 };
			}
			if (HasFlag(obj.ObjectType, TaikoObject::Large)) {
				sz = scale * 8;
			}
			auto v = off / obj.Velocity / 1000;
			auto objx = hitpos.X + v * (double)buf.Width;
			if (objx > buf.Width || objx < 0)
				continue;
			if (HasFlag(obj.ObjectType, TaikoObject::Spinner)) {
				sz = scale * 8;
				fill = { 220, 255, 255, 255 };
				if (objx < hitpos.X)
					continue;
			}
			outter.Alpha = fill.Alpha = (unsigned char)(CalcFlashlight(Mods, 1 - v) * 255) * std::clamp((1 - v) * 3, 0.0, 1.0);
			buf.FillCircle(objx, hitpos.Y, sz, rt, { {}, fill, ' ' });
			if (!HasFlag(obj.ObjectType, TaikoObject::SliderTick) && !HasFlag(obj.ObjectType, TaikoObject::Slider))
				buf.DrawCircle(objx, hitpos.Y, sz, 0.25, rt, { {}, outter, ' ' });
		}

		LastHitResultAnimator.Update(e_ms, [&](double val) {
			if (LastHitResult == HitResult::None)
				return;
			auto label = GetHitResultName(LastHitResult);
			auto color = GetHitResultColor(LastHitResult);
			color.Alpha = (unsigned char)val;
			buf.DrawString(label, hitpos.X - label.size() / 2, buf.Height / 4 - 2, color, {});
			color.Alpha = (unsigned char)(pow(val / 255, 3) * 20);
			buf.FillCircle(hitpos.X, hitpos.Y, scale * 8, rt, { {}, color, ' ' });
		});
		{
			SpinnerHitTrans.SetValue(e_ms, SpinnerHits > 0 ? 255 : 0);
			auto alpha = SpinnerHitTrans.GetCurrentValue(e_ms);
			if (alpha > 1) {
				auto label = std::to_string(SpinnerHits);
				buf.DrawString(label, hitpos.X - label.size() / 2, buf.Height / 2 + 2, { (unsigned char)alpha, 255, 255, 255 }, {});
				buf.FillCircle(hitpos.X, hitpos.Y, scale * 12, rt, { {}, { (unsigned char)(alpha * 0.6), 255, 255, 255 }, ' ' });
			}
		}
		buf.DrawLineH(hitpos.X, buf.Height / 4, buf.Height * 2 / 4, { { 255, 255, 255, 255 }, {}, '|' });
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
		return Beatmap->BgPath().string();
	}

	// 通过 Ruleset 继承
	virtual Record GetAutoplayRecord() override {
		Record record{};
		record.PlayerName = "Autoplay";
		record.Events.clear();
		bool alt = false;
		bool salt = false;
		for (auto obj : Beatmap->super<TaikoObject>()) {
			if (HasFlag(obj.ObjectType, TaikoObject::Spinner)) {
				InputEvent ie{};
				int count = obj.TotalHits;
				for (double t = obj.StartTime; t < obj.EndTime; t += obj.TickTime) {
					if (count <= 0)
						break;
					ie.Clock = t;
					ie.Pressed = true;
					ie.Action = alt ? (salt ? 1 : 2) : (salt ? 0 : 3);
					record.Events.push_back(ie);
					ie.Clock = t + 10;
					ie.Pressed = false;
					ie.Action = alt ? (salt ? 1 : 2) : (salt ? 0 : 3);
					record.Events.push_back(ie);
					alt = !alt;
					if (alt)
						salt = !salt;
					count--;
				}
				continue;
			}
			auto kat = HasFlag(obj.ObjectType, TaikoObject::Kat);
			auto large = HasFlag(obj.ObjectType, TaikoObject::Large);
			InputEvent ie{};
			ie.Clock = obj.StartTime;
			ie.Pressed = true;
			ie.Action = kat ? (alt ? 1 : 2) : (alt ? 0 : 3);
			record.Events.push_back(ie);
			if (large) {
				alt = !alt;
				InputEvent ie2{};
				ie2.Clock = obj.StartTime;
				ie2.Pressed = true;
				ie2.Action = kat ? (alt ? 1 : 2) : (alt ? 0 : 3);
				record.Events.push_back(ie2);
				ie2.Clock += 50;
				ie2.Pressed = false;
				record.Events.push_back(ie2);
				alt = !alt;
			}
			ie.Clock += 50;
			ie.Pressed = false;
			record.Events.push_back(ie);
			alt = !alt;
		}
		return record;
	}
	virtual ScoreProcessorBase* GetScoreProcessor() {
		return &ScoreProcessor;
	}
};
class TaikoBeatmap : public Beatmap {
public:
	OsuBeatmap orig_bmp;
	std::vector<TaikoObject> storage;
	path bmp_root;
	Hash bmp_hash;
	double first_obj = 1e300;
	double last_obj;
	size_t maxcombo;
	virtual std::string RulesetId() const noexcept {
		return "osutaiko";
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
		return (TaikoBeatmap*)this;
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
		return 0;
	}
	virtual std::unordered_set<std::string> GetDifficultyValues() const noexcept {
		return { "OD" };
	}
};


class TaikoRuleset : public Ruleset {
	BinaryStorage* settings;
	void Init(BinaryStorage& settings) {
		this->settings = &settings;
	}
	std::string Id() {
		return "osutaiko";
	}
	std::string DisplayName() {
		return "Taiko";
	}
	Beatmap* LoadBeatmap(path beatmap_path, bool load_samples) {
		auto beatmap = new TaikoBeatmap();

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
			if (load_samples) {
				try {
					auto dat = ReadAllBytes(path);
					SampleCaches[path] = AudioSample(am->loadSample(dat.data(), dat.size()));
				}
				catch (...) {
				}
			}
			else {
				SampleCaches[path];
			}
		});

		// 加载物件
		for (auto& obj : beatmap->orig_bmp.HitObjects) {
			TaikoObject to{};
			beatmap->first_obj = std::min(beatmap->first_obj, obj.StartTime);
			beatmap->last_obj = std::max(beatmap->last_obj, obj.StartTime);
			to.StartTime = obj.StartTime;
			auto tp = GetTimingPointTiming(beatmap->orig_bmp, obj.StartTime);
			auto soundtp = GetTimingPoint(beatmap->orig_bmp, obj.StartTime);
			auto bpm = tp.BPM();
			auto blen = 1000 / (bpm / 60.0);
			auto vec = GetTimingPointNonTiming(beatmap->orig_bmp, obj.StartTime).SpeedMultiplier();
			auto odd_s = std::abs(beatmap->orig_bmp.SliderTickRate - 3) < 0.01;
			bool isKat = HasFlag(obj.SoundType, HitSoundType::Whistle) || HasFlag(obj.SoundType, HitSoundType::Clap);
			bool isLarge = HasFlag(obj.SoundType, HitSoundType::Finish);
			auto scoringdist = BASE_SCORING_DISTANCE * beatmap->orig_bmp.SliderMultiplier * vec;
			auto velocity = scoringdist / tp.BeatLength;
			to.Velocity = vec;
			{
				auto sample_selector = [&](const std::filesystem::path& sample) -> auto {
					return SampleCaches[sample.string()];
				};
				to.samples > AddRange(GetSampleLayered(SampleIndex, SkinSampleIndex, SampleBank::Drum, obj.SoundType, soundtp.SampleSet) > Select(sample_selector));
			}
			if (HasFlag(obj.Type, HitObjectType::Slider)) {
				auto snap = odd_s ? 6 : (bpm <= 125.0 ? 8 : 4);
				blen /= snap;
				auto duration = obj.Length / velocity;
				for (auto i = obj.StartTime + blen; i < obj.StartTime + duration; i += blen) {
					TaikoObject to{};
					to.StartTime = i;
					to.ObjectType = TaikoObject::SliderTick;
					to.Velocity = vec;
					beatmap->storage.push_back(to);
				}
				to.ObjectType = TaikoObject::Slider;
				to.TickTime = blen;
				to.EndTime = to.StartTime + duration;
				to.Velocity = vec;
			}
			else if (HasFlag(obj.Type, HitObjectType::Spinner)) {
				auto snap = odd_s ? 6 : (bpm <= 125.0 ? 8 : 4);
				blen /= snap;
				auto duration = obj.EndTime - obj.StartTime;
				auto hits = (int)(duration / blen);
				to.TotalHits = to.RemainsHits = hits;
				to.ObjectType = TaikoObject::Spinner;
				to.EndTime = obj.EndTime;
				to.TickTime = blen;
			}
			else if (HasFlag(obj.Type, HitObjectType::Circle)) {
				beatmap->maxcombo++;
				to.ObjectType = ModifyFlag(to.ObjectType, isKat ? TaikoObject::Kat : TaikoObject::Don);
				if (isLarge) {
					beatmap->maxcombo++;
					to.ObjectType = ModifyFlag(to.ObjectType, TaikoObject::Large);
				}
			}
			if (to.EndTime != 0) {
				beatmap->last_obj = std::max(beatmap->last_obj, to.StartTime);
			}
			beatmap->storage.push_back(to);
		}
		return beatmap;
	}
	GameplayBase* GenerateGameplay() {
		return new TaikoGameplay();
	}
	double CalculateDifficulty(Beatmap* bmp, OsuMods mods) {
		auto GetTimeDiff = [&](double t) {
			t = std::abs(t);
			if (t > 0.5)
				return 6 / t;
			return 6 / 0.5;
		};
		if (bmp->size() < 10)
			return 0;
		double drain_time = 0;
		std::vector<double> diffs;
		for (const auto& ho : bmp->super<TaikoObject>()) {
			if (HasFlag(ho.ObjectType, TaikoObject::Barline) || HasFlag(ho.ObjectType, TaikoObject::Spinner) ||
				HasFlag(ho.ObjectType, TaikoObject::SliderTick) || HasFlag(ho.ObjectType, TaikoObject::Slider)) {
				continue;
			}
			auto large = HasFlag(ho.ObjectType, TaikoObject::Large);
			double lastjudge = ho.StartTime;
			double timeDiff = 0;
			double nearestTime = -1;
			for (const auto& ho2 : bmp->super<TaikoObject>()) {
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
			double multi = 1;
			if (large)
				multi *= 1.5;
			diffs.push_back(timeDiff * multi);
		}
		return (std::accumulate(diffs.begin(), diffs.end(), 0.0) / diffs.size()) * 100;
	}
	DifficultyInfo PopulateDifficultyInfo(Beatmap* bmp) {
		Assert(bmp->RulesetId() == "osutaiko");
		TaikoBeatmap& mb = *(TaikoBeatmap*)bmp;
		DifficultyInfo di;
		di.push_back(DifficultyInfoItem::MakeHeader("Metadata"));
		di.push_back(DifficultyInfoItem::MakeHeader("Title:"));
		di.push_back(DifficultyInfoItem::MakeHeader2(mb.orig_bmp.Title));
		di.push_back(DifficultyInfoItem::MakeHeader2(mb.orig_bmp.TitleUnicode));
		di.push_back(DifficultyInfoItem::MakeHeader("Artist:"));
		di.push_back(DifficultyInfoItem::MakeHeader2(mb.orig_bmp.Artist));
		di.push_back(DifficultyInfoItem::MakeHeader2(mb.orig_bmp.ArtistUnicode));
		di.push_back(DifficultyInfoItem::MakeHeader("Beatmap Version:"));
		di.push_back(DifficultyInfoItem::MakeHeader2(mb.orig_bmp.Version));
		di.push_back(DifficultyInfoItem::MakeHeader("Mapper:"));
		di.push_back(DifficultyInfoItem::MakeHeader2(mb.orig_bmp.Creator));
		di.push_back(DifficultyInfoItem::MakeHeader("Basic"));
		di.push_back(DifficultyInfoItem::MakeValue("Durtaion(s)", bmp->Length() / 1000));
		di.push_back(DifficultyInfoItem::MakeText("BeatmapHash", Hex(bmp->BeatmapHashcode())));
		di.push_back(DifficultyInfoItem::MakeText("MaxCombo", std::to_string(bmp->MaxCombo())));
		di.push_back(DifficultyInfoItem::MakeText("Objects", std::to_string(bmp->size())));
		di.push_back(DifficultyInfoItem::MakeHeader("Difficulty values"));
		di.push_back(DifficultyInfoItem::MakeValueBar("OD", bmp->GetDifficultyValue("OD"), 0, 20));
		di.push_back(DifficultyInfoItem::MakeValueBar("Star(NM)", CalculateDifficulty(bmp, OsuMods::None), 0, 15));
		di.push_back(DifficultyInfoItem::MakeHeader("Hitwindow"));
		static constexpr auto mania_hitres = { HitResult::Great, HitResult::Ok, HitResult::Miss };
		auto hitranges = GetHitRanges(bmp->GetDifficultyValue("OD"));
		auto max = 200;
		for (auto res : mania_hitres) {
			di.push_back(DifficultyInfoItem::MakeValueBar(GetHitResultName(res), hitranges[res], 0, max + 20));
		}
		return di;
	}
};
Ruleset* MakeTaikoRuleset() {
	return new TaikoRuleset();
}