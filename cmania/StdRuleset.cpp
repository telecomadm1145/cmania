#include <filesystem>
#include "Ruleset.h"
#include "StdObject.h"
#include "StdScoreProcessor.h"
#include "BassAudioManager.h"
#include "Animator.h"
#include "OsuBeatmap.h"
#include "OsuSample.h"
#include <set>
#include <stdexcept>
#include "File.h"
#include "ConsoleInput.h"
#include "StdRuleset.h"
#include "Crc.h"
#include "KeyBinds.h"

class StdGameplay : public GameplayBase {
#ifdef __clang__
	std::vector<Animator<CubicEasingFunction>> KeyHighlight;
	Animator<CubicEasingFunction> LastHitResultAnimator{ 255, 0, 400 };
#else
	std::vector<Animator<PowerEasingFunction<1.5>>> KeyHighlight;
	Animator<PowerEasingFunction<4.0>> LastHitResultAnimator{ 255, 0, 400 };
#endif // __clang__
	HitResult LastHitResult = HitResult::None;
	AudioStream bgm;
	double endtime = 0;
	std::string skin_path;
	OsuBeatmap orig_bmp;
	std::filesystem::path parent_path;
	double offset = 0;
	double first_obj = 1e300;
	double end_obj = -1e300;
	double resume_time = -1e300;
	bool no_hs = false;
	double last_rec = 0;
	double miss_offset = 200;
	double cs = 0;
	double ar = 0;
	StdScoreProcessor ScoreProcessor;

private:
	virtual ScoreProcessorBase* GetScoreProcessor() {
		return &ScoreProcessor;
	}
	virtual void Load(::Ruleset* rul, ::Beatmap* bmp) override {
		auto am = GetBassAudioManager(); // 获取Bass引擎

		if (bmp->RulesetId() != "osustd") {
			throw std::runtime_error("Provide a osu!std beatmap to this gameplay.");
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
			GetKeyBinds(2), [](const auto& val) -> auto { return (int)val; })
						 .ToList<int>();

		cs = Beatmap->GetDifficultyValue("CS");
		ar = Beatmap->GetDifficultyValue("AR");

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
	Record GetAutoplayRecord() override {
		return {};
	}
	struct Rect {
		int x1, y1, x2, y2;
	};

	Rect calcViewport(int width, int height, int viewportWidth, int viewportHeight) {
		float aspectRatio = (float)width / height;
		float viewportAspectRatio = (float)viewportWidth / viewportHeight;

		Rect rect{};

		if (aspectRatio > viewportAspectRatio) {
			// 缩小宽度
			rect.x1 = 0;
			rect.x2 = viewportWidth;
			rect.y2 = height * (viewportWidth / (float)width);
			rect.y1 = (viewportHeight - rect.y2) / 2;
		}
		else if (aspectRatio < viewportAspectRatio) {
			// 缩小高度
			rect.y1 = 0;
			rect.y2 = viewportHeight;
			rect.x2 = width * (viewportHeight / (float)height);
			rect.x1 = (viewportWidth - rect.x2) / 2;
		}
		else {
			// 不需要缩放
			rect.x1 = 0;
			rect.y1 = 0;
			rect.x2 = viewportWidth;
			rect.y2 = viewportHeight;
		}

		return rect;
	}
	struct Trail {
		PointD loc{};
		double time = -1e300;
	};
	constexpr static auto trail_count = 64;
	Trail cursortrail[trail_count];
	double lastrec = -1e300;
	void Render(GameBuffer& buf) override { // 512 , 384 osu pixel
		const auto rt = 2;
		auto vp = calcViewport(512 * rt, 384, buf.Width - 4, buf.Height - 4);
		vp.x1 += 2;
		vp.y1 += 2;
		buf.FillRect(vp.x1, vp.y1, vp.x1 + vp.x2, vp.y1 + vp.y2, { {}, { 60, 0, 0, 0 }, ' ' });
		auto preempt = DifficultyPreempt(ar);
		auto fadein = DifficultyFadeIn(preempt);
		preempt *= 0.8;
		fadein *= 0.8;
		auto scale = DifficultyScale(cs) * vp.y2 / 384.0 * 128;
		auto t = Clock.Elapsed();
		for (auto& ho : Beatmap->super<StdObject>()) {
			if (ho.EndTime == 0) {
				if (t > ho.StartTime + 200)
					continue;
			}
			else {
				if (t > ho.EndTime + 200)
					continue;
			}

			if (ho.EndTime != 0 && ho.Path == 0) {
				// 转盘
				continue;
			}
			Color comboclr{ 255, 120, 160, 240 };
			if (ho.Path != 0) {
				comboclr = { 255, 255, 0, 0 };
			}
			// Slider
			if (ho.Path != 0 && t > ho.StartTime - preempt) {
				// 渲染滑条体...
				// auto points = ho.Path->calcedPath;
				// for (int i = 0; i < points.size() - 1; i++) {
				//	PointD currentPoint = points[i];
				//	PointD nextPoint = points[(i + 1) % points.size()];

				//	double dx = nextPoint.X - currentPoint.X;
				//	double dy = nextPoint.Y - currentPoint.Y;
				//	double distance = std::sqrt(dx * dx + dy * dy);

				//	float stepX = static_cast<float>(dx) / distance;
				//	float stepY = static_cast<float>(dy) / distance;

				//	for (int j = 0; j < distance; j++) {
				//		double x = currentPoint.X + static_cast<int>(stepX * j);
				//		double y = currentPoint.Y + static_cast<int>(stepY * j);

				//		buf.FillCircle(vp.x1 + x / 512 * vp.x2, vp.y1 + y / 384 * vp.y2,scale,rt+1, { {}, { 10, 255, 255, 255 }, ' ' });
				//	}
				//}
				if (t < ho.EndTime && t > ho.StartTime) {
					auto duration = ho.Path->actualLength / ho.Velocity;
					auto progress = std::clamp(fmod(t - ho.StartTime, duration * 2) / duration, 0.0, 2.0);
					if (progress > 1)
						progress = 2 - progress;
					auto sb = ho.Path->PositionAt(progress);
					buf.FillCircle(vp.x1 + sb.X / 512.0 * vp.x2, vp.y1 + sb.Y / 384.0 * vp.y2, scale, rt, { {}, (comboclr * 0.8), ' ' });
				}
			}
			// HeadCircle
			if (t > ho.StartTime - preempt) {
				auto alpha = (t - (ho.StartTime - preempt)) / (preempt - fadein);
				if (t > ho.StartTime - fadein) {
					alpha = 1.0;
				}
				if (ho.Path != 0) {
					if (t > ho.StartTime)
						continue;
				}
				buf.FillCircle(vp.x1 + ho.Location.X / 512.0 * vp.x2, vp.y1 + ho.Location.Y / 384.0 * vp.y2, scale, rt, { {}, (comboclr * (alpha * 0.8)), ' ' });
				// buf.DrawCircle(vp.x1 + ho.Location.X / 512.0 * vp.x2, vp.y1 + ho.Location.Y / 384.0 * vp.y2, scale, 1.5, rt + 1, { {}, (Color{ 255, 255, 255, 255 } * alpha), ' ' });
				// 缩圈
				auto progress = std::max(1 + (1 - (t - (ho.StartTime - preempt)) / preempt) * 3, 1.0);
				buf.DrawCircle(vp.x1 + ho.Location.X / 512.0 * vp.x2, vp.y1 + ho.Location.Y / 384.0 * vp.y2, scale * progress, 0.5, rt, { {}, (Color{ 255, 255, 255, 255 } * alpha), ' ' });
			}
		}
		auto mpos = GameInputHandler->GetMousePosition();
		auto last = cursortrail[0];
		// for (size_t i = 1; i < trail_count; i++) {
		//	auto anim = std::clamp(1 - (t - cursortrail[i].time) / 500.0, 0.0, 1.0);
		//	if (anim <= 0.01)
		//		continue;
		//	if ((cursortrail[i].loc - last.loc).SquaredLength() < 4)
		//		continue;
		//	auto clr = Color{ 160, 255, 255, 255 } * anim;
		//	buf.DrawLine(last.loc.X, cursortrail[i].loc.X, last.loc.Y, cursortrail[i].loc.Y, { {}, clr, ' ' });
		//	last = cursortrail[i];
		// }
		if (t > lastrec + 20) {
			const auto t2 = trail_count - 1;
			Trail buff[t2];
			std::memcpy(buff, cursortrail, t2 * sizeof(Trail));
			std::memcpy(cursortrail + 1, buff, t2 * sizeof(Trail));
			cursortrail[0] = { { (double)std::get<0>(mpos), (double)std::get<1>(mpos) }, t };
			lastrec = t;
		}
		if (scale <= 8) {
			buf.SetPixel(std::get<0>(mpos), std::get<1>(mpos), { {}, { 255, 255, 255, 255 }, ' ' });
		}
		else {
			buf.FillCircle(std::get<0>(mpos), std::get<1>(mpos), scale / 4.0, rt, { {}, { 255, 255, 255, 255 }, ' ' });
		}
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
	void Update() override {
		if (!GameStarted)
			return;
		if (GameEnded)
			return;
		auto time = Clock.Elapsed();

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
					Clock.Offset(bgm->getCurrent() * 1000);
					Clock.Start();
				}
				else {
					bgm->pause(false);
				}
			}
			else {
				auto err = time - bgm->getCurrent() * 1000;
				if (std::abs(err) > 150) {
					bgm->setCurrent(time / 1000);
				}
			}
		}
	}
};
class StdBeatmap : public Beatmap {
public:
	OsuBeatmap orig_bmp;
	std::vector<StdObject> storage;
	path bmp_root;
	Hash bmp_hash;
	double first_obj = 1e300;
	double last_obj;
	size_t maxcombo;
	virtual std::string RulesetId() const noexcept {
		return "osustd";
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
		return (StdBeatmap*)this;
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
		if (key == "AR") {
			return orig_bmp.ApproachRate;
		}
		if (key == "CS") {
			return orig_bmp.CircleSize;
		}
		if (key == "HP") {
			return orig_bmp.HPDrainRate;
		}
		if (key == "SliderMultiplier") {
			return orig_bmp.SliderMultiplier;
		}
		if (key == "SliderTickRate") {
			return orig_bmp.SliderTickRate;
		}
		return 0;
	}
	virtual std::unordered_set<std::string> GetDifficultyValues() const noexcept {
		return { "OD", "AR", "SliderMultiplier", "SliderTickRate", "HP", "CS" };
	}
};

class StdRuleset : public Ruleset {
	BinaryStorage* settings;
	void Init(BinaryStorage& settings) {
		this->settings = &settings;
	}
	std::string Id() {
		return "osustd";
	}
	std::string DisplayName() {
		return "Std";
	}
	Beatmap* LoadBeatmap(path beatmap_path, bool load_samples) {
		auto beatmap = new StdBeatmap();

		std::ifstream ifs(beatmap_path);
		if (!ifs.good())
			throw std::runtime_error("Failed to open beatmap file.");

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
		beatmap->storage > AddRange(Select(
							   beatmap->orig_bmp.HitObjects, [&](const OsuBeatmap::HitObject& obj) -> auto {
								   StdObject so{};
								   so.StartTime = obj.StartTime;
								   so.Location = { obj.X, obj.Y };
								   auto& tp = GetTimingPointTiming(beatmap->orig_bmp, obj.StartTime);
								   auto& tp2 = GetTimingPointNonTiming(beatmap->orig_bmp, obj.StartTime);

								   auto scoringdist = BASE_SCORING_DISTANCE * beatmap->orig_bmp.SliderMultiplier * tp2.SpeedMultiplier();

								   auto velocity = scoringdist / tp.BeatLength;
								   auto tickdist = scoringdist / beatmap->orig_bmp.SliderTickRate;
								   if (!obj.PathRecord.empty()) {
									   SliderPath sp = SliderPath::Parse(obj.PathRecord, { obj.X, obj.Y }, obj.Length);
									   so.Path = (new SliderPath(sp));
									   so.EndTime = obj.StartTime + obj.RepeatCount * sp.actualLength / velocity;
									   so.Velocity = velocity;
									   bool rev = false;
									   double time = obj.StartTime;
									   for (size_t i = 0; i < obj.RepeatCount; i++) {
										   if (tickdist > 20)
											   for (double j = tickdist; j < sp.actualLength; j += tickdist) {
												   Event evt{};
												   evt.EventType = Event::Tick;
												   auto prog = j / sp.actualLength;
												   evt.Location = sp.PositionAt(prog);
												   evt.StartTime = time + j / velocity;
												   so.Events.push_back(evt);
											   }
										   Event evt2{};
										   evt2.EventType = Event::Repeat;
										   time += sp.actualLength / velocity;
										   evt2.StartTime = time;
										   evt2.Location = rev ? PointD{ obj.X, obj.Y } : PointD(sp.PositionAt(1));
										   so.Events.push_back(evt2);
									   }
									   so.RepeatCount = obj.RepeatCount;
								   }

								   beatmap->first_obj = std::min(beatmap->first_obj, obj.StartTime);
								   beatmap->last_obj = std::max(beatmap->last_obj, obj.StartTime);
								   if (so.EndTime != 0)
									   beatmap->last_obj = std::max(beatmap->last_obj, so.EndTime);
								   return so;
							   }));
		return beatmap;
	}
	GameplayBase* GenerateGameplay() {
		// TODO
		return new StdGameplay();
	}
	double CalculateDifficulty(Beatmap* bmp, OsuMods mods) {
		auto GetTimeDiff = [&](double t) {
			t = std::abs(t);
			if (t > 0.5)
				return 6 / t;
			return 6 / 0.5;
		};
		return 0;
		// TODO
		if (bmp->size() < 10)
			return 0;
		double drain_time = 0;
		std::vector<double> diffs;

		return (std::accumulate(diffs.begin(), diffs.end(), 0.0) / diffs.size()) * 100;
	}
	DifficultyInfo PopulateDifficultyInfo(Beatmap* bmp) {
		Assert(bmp->RulesetId() == "osustd");
		StdBeatmap& mb = *(StdBeatmap*)bmp;
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
		for (auto dv : bmp->GetDifficultyValues()) {
			di.push_back(DifficultyInfoItem::MakeValueBar(dv, bmp->GetDifficultyValue(dv), 0, 20));
		}
		di.push_back(DifficultyInfoItem::MakeValueBar("Star(NM)", CalculateDifficulty(bmp, OsuMods::None), 0, 15));
		di.push_back(DifficultyInfoItem::MakeHeader("Hitwindow"));
		static constexpr auto mania_hitres = { HitResult::Great, HitResult::Ok, HitResult::Meh, HitResult::Miss };
		auto hitranges = GetHitRanges(bmp->GetDifficultyValue("OD"));
		auto max = 200;
		for (auto res : mania_hitres) {
			di.push_back(DifficultyInfoItem::MakeValueBar(GetHitResultName(res), hitranges[res], 0, max + 20));
		}
		return di;
	}
};
Ruleset* MakeStdRuleset() {
	return new StdRuleset();
}