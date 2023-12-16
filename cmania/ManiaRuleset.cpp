#pragma once
#include <vector>
#include "Animator.h"
#include "ManiaObject.h"
#include "Ruleset.h"
#include "OsuBeatmap.h"
#include "ManiaScoreProcessor.h"
#include <filesystem>
#include <set>
#include "File.h"
#include "BassAudioManager.h"
#include "ConsoleInput.h"
#include "KeyBinds.h"
#include "DifficultyCalculator.h"
#include "OsuSample.h"
#include "Debug.h"
#include "ManiaRuleset.h"

class ManiaRuleset : public Ruleset<ManiaObject> {
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
	ManiaRuleset() {
		RulesetScoreProcessor = new ManiaScoreProcessor();
		KeyHighlight.resize(18, { 180, 0, 150 });
	}
	~ManiaRuleset() {
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
		((ManiaScoreProcessor*)RulesetScoreProcessor)->SetWtMode(wt_mode = settings["WtMode"].Get<bool>());
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
						  ManiaObject mo{};

						  // 计算物件的列
						  mo.Column = CalcColumn(obj.X, keys);

						  // 复制起始和终止时间
						  mo.StartTime = obj.StartTime;
						  mo.EndTime = obj.EndTime;

						  endtime = std::max(endtime, std::max(obj.EndTime, obj.StartTime));

						  first_obj = std::min(first_obj, obj.StartTime);
						  end_obj = std::max(end_obj, obj.StartTime);
						  if (obj.EndTime != 0)
							  end_obj = std::max(end_obj, obj.EndTime);

						  // 计算是否为多押
						  if (jump_helper)
							  orig_bmp.HitObjects > ForEach([&](const auto& obj2) {
								  if (&obj2 != &obj && (std::abs(obj.StartTime - obj2.StartTime) < 0.5 || (obj2.EndTime != 0 && std::abs(obj.StartTime - obj2.EndTime) < 0.5)))
									  mo.Multi = true;
							  });

						  // 获取最近的 TimingPoint
						  auto& tp = GetTimingPoint(orig_bmp, obj.StartTime);

						  // 将 fspath 转换为 AudioSample
						  auto sample_selector = [&](const std::filesystem::path& sample) -> auto {
							  return SampleCaches[sample.string()];
						  };

						  // 给物件加载 Samples
						  mo.samples > AddRange(GetSampleLayered(SampleIndex, SkinSampleIndex, tp.SampleBank, obj.SoundType, tp.SampleSet) > Select(sample_selector));

						  if (!obj.CustomSampleFilename.empty()) {
							  mo.samples.push_back(SampleCaches[(parent / obj.CustomSampleFilename).string()]);
						  }

						  RulesetScoreProcessor->BeatmapMaxCombo++;

						  // 判断是否是 Hold
						  if (obj.EndTime != 0) {
							  // 加载滑动音效
							  First(GetSampleLayered(SampleIndex, SkinSampleIndex, tp.SampleBank, HitSoundType::Slide, tp.SampleSet) > Select(sample_selector), mo.ssample);

							  // 加载 Whistle 滑动音效
							  if (HasFlag(obj.SoundType, HitSoundType::Whistle))
								  First(GetSampleLayered(SampleIndex, SkinSampleIndex, tp.SampleBank, HitSoundType::SlideWhistle, tp.SampleSet) > Select(sample_selector), mo.ssamplew);

							  if (!wt_mode)
								  RulesetScoreProcessor->BeatmapMaxCombo++;
						  }

						  return mo;
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

		auto diff = CalculateDiff(Beatmap, Mods, keys);
		RulesetScoreProcessor->ApplyBeatmap(diff * GetPlaybackRate(Mods));

		miss_offset = GetHitRanges(orig_bmp.OverallDifficulty)[HitResult::Meh];

		Clock.SetRate(GetPlaybackRate(Mods));
		Clock.Offset(std::min(first_obj - 5000, -3000.0)); // 让玩家有时间准备击打
		Clock.Start();									   // 开始Hpet计时器

		RulesetInputHandler->SetClockSource(Clock);

		GameStarted = true;
	}
	void ProcessAction(int action, bool pressed, double clock) {
		if (!pressed) {
			auto first_hold = Beatmap > Where([&](ManiaObject& obj) -> bool {
				return obj.Column == action && obj.EndTime != 0 && !(obj.HasHold || obj.HoldBroken) && obj.HasHit;
			}) > FirstOrDefault();
			if (first_hold != 0) {
				auto result = RulesetScoreProcessor->ApplyHit(*first_hold, clock - first_hold->EndTime);
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
			auto first_hit = Beatmap > Where([&](ManiaObject& obj) -> bool { return obj.Column == action && !obj.HasHit; }) > FirstOrDefault();
			if (first_hit != 0) {
				auto result = RulesetScoreProcessor->ApplyHit(*first_hit, clock - first_hit->StartTime);
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
				if (RulesetInputHandler->GetKeyStatus(i)) {
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
			RulesetRecord.RatingGraph[(time - first_obj) / 100] = RulesetScoreProcessor->Rating;
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

		Beatmap > ForEach([&](ManiaObject& obj) {
			if (obj.EndTime != 0 && !(obj.HasHold || obj.HoldBroken) && !wt_mode) {
				if (time > obj.StartTime + miss_offset) {
					if (RulesetInputHandler->GetKeyStatus(obj.Column)) {
						obj.LastHoldOff = time;
					}
					if (time > obj.EndTime + miss_offset || (obj.LastHoldOff != -1 && time > obj.LastHoldOff + miss_offset)) {
						RulesetScoreProcessor->ApplyHit(obj, 1.0 / 0 * 0);
						LastHitResult = HitResult::Miss;
						LastHitResultAnimator.Start(Clock.Elapsed());
						return;
					}
				}
			}
			if (!obj.HasHit) {
				if (time > obj.StartTime + miss_offset) {
					RulesetScoreProcessor->ApplyHit(obj, 1.0 / 0 * 0);
					LastHitResult = HitResult::Miss;
					LastHitResultAnimator.Start(Clock.Elapsed());
				}
			}

			// Handles sliding sample.
			if (obj.HasHit && obj.EndTime != 0) {
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
						if (RulesetInputHandler->GetKeyStatus(obj.Column)) // Pressed
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
			auto evt = RulesetInputHandler->PollEvent();
			if (!evt.has_value())
				break;
			auto& e = *evt;
			RulesetRecord.Events.push_back(e);
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
		double key_height = int(std::max(buffer.Height / 50.0, 0.0));
		key_width = int(std::min(std::max(key_width, (double)buffer.Width * 0.3 / keys), (double)buffer.Width / keys * 2 - 3));
		double centre = (double)buffer.Width / 2;
		double centre_start = centre - (keys * key_width) / 2;
		double judge_height = std::max((key_height + 1) * 2, 4.0);
		auto j = 0;
		for (double i = centre_start; i < keys * key_width + centre_start; i += key_width) {
			int visible = 0;
			auto scrollspeed = this->scrollspeed * Clock.ClockRate();
			for (auto& obj : Beatmap) {
				auto off = obj.StartTime - e_ms;
				auto off2 = obj.EndTime - e_ms;
				if (obj.EndTime == 0) {
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
					if (obj.EndTime != 0 && !obj.HasHold) {
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
						buffer.FillRect(i + 1, a - key_height, i + key_width, endy + key_height, { {}, base, ' ' });
						base.Alpha = (unsigned char)(255 * flashlight_num);
						if (a >= buffer.Height - judge_height) {
							base.Alpha = 255;
						}
						if (obj.HoldBroken) {
							base.Alpha = base.Alpha * 0.2;
						}
						buffer.FillRect(i + 1, a - key_height, i + key_width, a + key_height, { {}, base, ' ' });
						continue;
					}
					base.Alpha = (unsigned char)(255 * flashlight_num);
					if (!obj.HasHit)
						buffer.FillRect(i + 1, starty - key_height, i + key_width, starty + key_height, { {}, base, ' ' });
					visible++;
				}
			}

			// 绘制 Mania 台阶
			auto clr = Color{ 255, 204, 187, 102 };
			buffer.DrawLineH(i, 0, buffer.Height, { clr, {}, '|' });
			buffer.DrawLineH(i + key_width, 0, buffer.Height, { clr, {}, '|' });
			buffer.FillRect(i + 1, buffer.Height - judge_height + 1, i + key_width, buffer.Height, { {}, { 120, 255, 255, 255 }, ' ' });

			KeyHighlight[j].Update(e_ms, [&](double light) {
				buffer.FillRect(i + 1, buffer.Height - judge_height + 1, i + key_width, buffer.Height, { {}, { (unsigned char)light, 255, 255, 255 }, ' ' });
				auto ratio = light / 240;
				auto lightning_height = std::max(15.0, buffer.Height * 0.3);
				if (ratio > 0 && ratio < 1) {
					for (int p = 0; p < ratio * lightning_height; p++) {
						buffer.DrawLineV(i + 1, i + key_width, buffer.Height - judge_height - p, { {}, { (unsigned char)(light * pow((ratio * lightning_height - p) / (ratio * lightning_height), 2)), 255, 255, 255 }, ' ' });
					}
				}
			});

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

	// 通过 Ruleset 继承
	virtual void Pause() override {
		for (auto& light : KeyHighlight) {
			light.Reset();
		}
		bgm->pause(true);
		Clock.Stop();
		for (auto& ho : Beatmap) {
			if (ho.ssample_stream != 0)
				ho.ssample_stream->stop();
			if (ho.ssamplew_stream != 0)
				ho.ssamplew_stream->stop();
		}
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

		for (auto& obj : Beatmap) {
			InputEvent evt{};
			evt.Action = obj.Column;
			evt.Pressed = true;
			evt.Clock = obj.StartTime;
			record.Events.push_back(evt);

			if (obj.EndTime != 0) {
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

RulesetBase* MakeManiaRuleset() {
	return new ManiaRuleset();
}
