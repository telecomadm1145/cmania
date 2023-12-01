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
#include "TaikoDifficultyCalculator.h"

class TaikoRuleset : public Ruleset<TaikoObject> {
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
	Transition<CubicEasingFunction, ConstantEasingDurationCalculator<50>> SpinnerHitTrans{};

	AudioSample DonSample;
	AudioSample KatSample;

public:
	TaikoRuleset() {
		RulesetScoreProcessor = new TaikoScoreProcessor();
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

		// 存储需要加载的采样
		std::set<std::string> Samples;

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

		DonSample = SampleCaches[GetSampleLayered(SampleIndex, SkinSampleIndex, SampleBank::Drum, HitSoundType::Normal, 0)[0].string()];
		KatSample = SampleCaches[GetSampleLayered(SampleIndex, SkinSampleIndex, SampleBank::Drum, HitSoundType::Clap, 0)[0].string()];
		// 加载物件
		for (auto& obj : orig_bmp.HitObjects) {
			TaikoObject to{};
			first_obj = std::min(first_obj, obj.StartTime);
			end_obj = std::max(end_obj, obj.StartTime);
			to.StartTime = obj.StartTime;
			auto tp = GetTimingPointTiming(orig_bmp, obj.StartTime);
			auto bpm = tp.BPM();
			auto blen = 1000 / (bpm / 60.0);
			auto vec = GetTimingPointNonTiming(orig_bmp, obj.StartTime).SpeedMultiplier();
			auto odd_s = std::abs(orig_bmp.SliderTickRate - 3) < 0.01;
			bool isKat = HasFlag(obj.SoundType, HitSoundType::Whistle) || HasFlag(obj.SoundType, HitSoundType::Clap);
			bool isLarge = HasFlag(obj.SoundType, HitSoundType::Finish);
			auto scoringdist = BASE_SCORING_DISTANCE * orig_bmp.SliderMultiplier * vec;
			auto velocity = scoringdist / tp.BeatLength;
			to.Velocity = vec;
			{
				auto sample_selector = [&](const std::filesystem::path& sample) -> auto {
					return SampleCaches[sample.string()];
				};
				to.samples > AddRange(GetSampleLayered(SampleIndex, SkinSampleIndex, SampleBank::Drum, obj.SoundType, tp.SampleSet) > Select(sample_selector));
			}
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
				for (auto i = obj.StartTime + blen; i < obj.StartTime + duration; i += blen) {
					TaikoObject to{};
					to.StartTime = i;
					to.ObjectType = TaikoObject::SliderTick;
					to.Velocity = vec;
					Beatmap.push_back(to);
				}
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
				RulesetScoreProcessor->BeatmapMaxCombo++;
				to.ObjectType = ModifyFlag(to.ObjectType, isKat ? TaikoObject::Kat : TaikoObject::Don);
				if (isLarge) {
					RulesetScoreProcessor->BeatmapMaxCombo++;
					to.ObjectType = ModifyFlag(to.ObjectType, TaikoObject::Large);
				}
			}
			if (to.EndTime != 0) {
				end_obj = std::max(end_obj, to.EndTime);
			}
			Beatmap.push_back(to);
		}

		// 加载bgm
		{
			auto dat = ReadAllBytes((parent / orig_bmp.AudioFilename).string());
			bgm = AudioStream(am->load(dat.data(), dat.size()));
		}

		if (RulesetInputHandler == 0)
			throw std::invalid_argument("RulesetInputHandler mustn't be nullptr.");

		auto binds = Select(
			GetKeyBinds(4), [](const auto& val) -> auto { return (int)val; })
						 .ToList<int>();
		RulesetInputHandler->SetBinds(binds);

		RulesetRecord.Mods = Mods;
		RulesetRecord.RatingGraph.resize(((end_obj - first_obj) + 11000) / 100);

		RulesetScoreProcessor->RulesetRecord = &RulesetRecord;
		RulesetScoreProcessor->SetDifficulty(orig_bmp.OverallDifficulty);
		RulesetScoreProcessor->SetMods(Mods);

		RulesetScoreProcessor->ApplyBeatmap(CalculateDiff(Beatmap, Mods) * GetPlaybackRate(Mods));

		miss_offset = GetHitRanges(orig_bmp.OverallDifficulty)[HitResult::Meh];

		Clock.SetRate(GetPlaybackRate(Mods));
		Clock.Offset(std::min(first_obj - 5000, -3000.0)); // 让玩家有时间准备击打
		Clock.Start();									   // 开始Hpet计时器

		RulesetInputHandler->SetClockSource(Clock);

		GameStarted = true;
	}
	void ProcessAction(int action, bool pressed, double clock) {
		auto hit_kat = (action == 1) || (action == 2) || (action == 17);
		if (pressed) {
			auto rx = HasFlag(Mods, OsuMods::Relax);
			auto first_hit = Beatmap > Where([&](TaikoObject& obj) -> bool {
				if (!obj.HasHit && obj.RemainsHits > 0) {
					return true;
				}
				return !obj.HasHit && (obj.StartTime - clock < miss_offset) && !HasFlag(obj.ObjectType, TaikoObject::Barline);
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
							RulesetScoreProcessor->ApplyHit(*first_hit, 0);
							first_hit->RemainsHits--;
						}
					SpinnerKatOrDon = kat + 1;
					goto goff;
				}
				if (HasFlag(first_hit->ObjectType, TaikoObject::Slider))
					return;
				// miss if a wrong action has been pressed
				if (kat != hit_kat && !rx) {
					RulesetScoreProcessor->ApplyHit(*first_hit, 1.0 / 0.0 * 0.0);
					LastHitResult = HitResult::Miss;
					LastHitResultAnimator.Start(Clock.Elapsed());
					return;
				}
				auto result = RulesetScoreProcessor->ApplyHit(*first_hit, clock - first_hit->StartTime);
				if (result != HitResult::None) {
					LastHitResult = result;
					LastHitResultAnimator.Start(Clock.Elapsed());
					hit_kat = kat;
					first_hit->PlaySample();
					return;
				}
			}
		goff:
			auto first_sound = Beatmap > Where([&](TaikoObject& obj) -> bool {
				return !obj.HasHit && !HasFlag(obj.ObjectType, TaikoObject::Barline) && !HasFlag(obj.ObjectType, TaikoObject::SliderTick) && !HasFlag(obj.ObjectType, TaikoObject::Slider) && !(HasFlag(obj.ObjectType, TaikoObject::Kat) ^ hit_kat);
			}) > FirstOrDefault();
			if (first_sound != 0)
				first_sound->PlaySample();
			else {
				if (hit_kat) {
					KatSample->generateStream()->play();
				}
				else {
					DonSample->generateStream()->play();
				}
			}
		}
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

		if (!wt_mode) {
			for (int i = 0; i < 4; i++) {
				if (RulesetInputHandler->GetKeyStatus(i)) {
					KeyHighlight[i].Start(time);
				}
			}
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
		Beatmap > ForEach([&](TaikoObject& obj) {
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
					RulesetScoreProcessor->ApplyHit(obj, 1.0 / 0 * 0);
					LastHitResult = HitResult::Miss;
					LastHitResultAnimator.Start(Clock.Elapsed());
				}
			}
		});

		while (true) {
			auto evt = RulesetInputHandler->PollEvent();
			if (!evt.has_value())
				break;
			auto& e = *evt;
			RulesetRecord.Events.push_back(e);

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
		const auto rt = 1.7;
		auto e_ms = Clock.Elapsed();

		// now we need to render all objects.

		auto scale = buf.Height / 40.0;

		auto hitpos = PointI{ (int)(20 * scale) + 5, buf.Height * 3 / 8 };
		buf.FillRect(0, buf.Height / 4, buf.Width, buf.Height * 2 / 4, { {}, { 255, 40, 40, 40 }, ' ' });
		auto lightsz = hitpos.X / 5;
		for (size_t i = 0; i < 4; i++) {
			buf.FillRect((i)*lightsz + 1, buf.Height / 4, (i + 1) * lightsz + 1, buf.Height * 2 / 4, { {}, { 120, 80, 80, 80 }, ' ' });
			KeyHighlight[i].Update(e_ms, [&](double v) {
				Color clr{ 220, 20, 212, 255 };
				if ((i == 1) || (i == 2)) {
					clr = { 220, 255, 30, 30 };
				}
				clr.Alpha = v;
				buf.FillRect((i)*lightsz + 1, buf.Height / 4, (i + 1) * lightsz + 1, buf.Height * 2 / 4, { {}, clr, ' ' });
			});
		}
		for (auto& obj : Beatmap) {
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
			auto objx = (int)(hitpos.X + v * (double)buf.Width);
			if (objx > buf.Width || objx < 0)
				continue;
			if (HasFlag(obj.ObjectType, TaikoObject::Spinner))
			{
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
		return (parent_path / orig_bmp.Background).string();
	}

	// 通过 Ruleset 继承
	virtual Record GetAutoplayRecord() override {
		Record record{};
		record.PlayerName = "Autoplay";
		record.Events.clear();
		bool alt = false;
		bool salt = false;
		for (auto obj : Beatmap) {
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
			record.Events.push_back(ie);
			ie.Clock += 50;
			ie.Pressed = false;
			record.Events.push_back(ie);
			alt = !alt;
		}
		return record;
	}
};