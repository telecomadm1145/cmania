export module GameplayScreen;
import ScreenController;
import OsuBeatmap;
import OsuMods;
import <fstream>;
import <filesystem>;
import AudioService;
import "AudioManager.h";
import OsuStatic;
import <map>;
import String;
import <functional>;
import <vector>;
import WinDebug;
import "stb_image.h";
import "stb_image_resize.h";
import Stopwatch;
import KeyBinds;
import Hpet;

using fspath = std::filesystem::path;

export class GameplayScreen : public Screen
{
	unsigned char* background = 0;
	int bg_w = 0;
	int bg_h = 0;
	unsigned char* resized_bg = 0;
	int rbg_w = 0;
	int rbg_h = 0;
	OsuMods mods;
	fspath path;
	double length = 0;
	fspath p_path;
	bool loading = true;
	bool pause = false;
	int keys = 0;
	double scrollspeed = 500;
	Stopwatch sw{};
	std::map<OsuStatic::HitResult, double> hitranges;
	double hiterr_anim_clk = 0;
	OsuStatic::HitResult hiterr_ui = OsuStatic::HitResult::None;
	int maxcombo = 0;
	int combo = 0;
	double score = 0;
	std::vector<double> HitErrors;
	bool wt_mode = true;
	double hiterr_avg = 0;
	double accuracy = 0;
	double acc_sum = 0;
	double acc_count = 0;
	std::map<OsuStatic::HitResult, int> HitCounter{
		{ OsuStatic::HitResult::Perfect, 0 },
		{ OsuStatic::HitResult::Great,0 },
		{ OsuStatic::HitResult::Good,0 },
		{ OsuStatic::HitResult::Ok,0 },
		{ OsuStatic::HitResult::Meh,0 },
		{ OsuStatic::HitResult::Miss, 0 },
	};
	IAudioManager::IAudioStream* bgm = 0;
	fspath sample_folder = ".\\samples\\triangles";
	std::vector<ConsoleKey> KeyBinds;
	std::vector<double> KeyHighlight = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
public:
	GameplayScreen(const std::string& bmp_path, OsuMods mod)
	{
		path = fspath(bmp_path);
		p_path = fspath(bmp_path).parent_path();
		mods = mod;
	}
private:
	struct ManiaObject
	{
		int Column;
		double StartTime;
		double EndTime;
		IAudioManager::ISample* ssample;
		IAudioManager::IAudioStream* ssample_stream;
		IAudioManager::ISample* ssamplew;
		IAudioManager::IAudioStream* ssamplew_stream;
		std::vector<IAudioManager::ISample*> samples;
		bool Multi;
		bool HasHit;
		bool HasSlide;
	};
	std::vector<ManiaObject> objects;
	std::string GetHitResultName(OsuStatic::HitResult res)
	{
		switch (res)
		{
		case OsuStatic::HitResult::Miss:
			return "Miss";
		case OsuStatic::HitResult::Meh:
			return "Meh";
		case OsuStatic::HitResult::Ok:
			return "Ok";
		case OsuStatic::HitResult::Good:
			return "Good";
		case OsuStatic::HitResult::Great:
			return "Great";
		case OsuStatic::HitResult::Perfect:
			return "Perf";
		default:
			return "Perf";
		}
	}
	int GetBaseScore(OsuStatic::HitResult res)
	{
		switch (res)
		{
		case OsuStatic::HitResult::Miss:
			return 0;
		case OsuStatic::HitResult::Meh:
			return 50;
		case OsuStatic::HitResult::Ok:
			return 100;
		case OsuStatic::HitResult::Good:
			return 200;
		case OsuStatic::HitResult::Great:
			return 300;
		case OsuStatic::HitResult::Perfect:
			return 320;
		default:
			return 20;
		}
	}
	void ApplyHit(ManiaObject& mo, OsuStatic::HitResult res, double err)
	{
		mo.HasHit = true;
		hiterr_anim_clk = HighPerformanceTimer::GetMilliseconds();
		hiterr_ui = res;
		HitCounter[res]++;
		acc_count++;
		acc_sum += GetBaseScore(res) / GetBaseScore(OsuStatic::HitResult::Great) * 100;
		accuracy = acc_sum / acc_count;
		if (res != OsuStatic::HitResult::Miss)
		{
			HitErrors.push_back(err);
			auto hiterr_sum = 0.0;
			for (auto he : HitErrors)
			{
				hiterr_sum += he;
			}
			hiterr_avg = hiterr_sum / HitErrors.size();
			combo++;
		}
		else
		{
			combo = 0;
		}
		maxcombo = std::max(maxcombo, combo);
		score += GetBaseScore(res) * std::max(1.0, std::pow(combo, 0.1));
	}
	inline double offset()
	{
		return 3000;
	}
	inline double aoffset()
	{
		return 3000;
	}
	virtual void Key(KeyEventArgs kea)
	{
		if (loading)
		{
			return;
		}
		if (kea.Pressed)
		{
			if (kea.Key == ConsoleKey::Escape)
			{
				parent->Back();
				return;
			}
		}
		if (!HasFlag(mods, OsuMods::Auto)) // ×è¶Ï Auto mod ÏÂµÄÊäÈë
		{
			for (size_t i = 0; i < KeyBinds.size(); i++)
			{
				if (kea.Key == KeyBinds[i])
				{
					auto e_ms = KeyHighlight[i] = sw.Elapsed() - offset();
					for (auto& obj : objects)
					{
						if (obj.Column == i)
						{
							//if (obj.EndTime != 0 && !obj.HasSlide)
							//{
							//	if (wt_mode)
							//	{
							//		if (obj.EndTime >= e_ms)
							//		{
							//			if (!kea.Pressed)
							//			{
							//				obj.HasSlide = true;
							//				ApplyHit(obj, OsuStatic::HitResult::Perfect, 0);
							//			}
							//		}
							//		return;
							//	}
							//}
							if (kea.Pressed)
								if (!obj.HasHit)
								{
									auto err = e_ms - obj.StartTime;
									if (err >= hitranges[OsuStatic::HitResult::Meh])
									{
										ApplyHit(obj, OsuStatic::HitResult::Miss, err);
										return;
									}
									for (auto res : std::vector(HitCounter.rbegin(), HitCounter.rend()))
									{
										if (std::abs(err) <= hitranges[res.first])
										{
											for (auto sample : obj.samples)
											{
												if (sample != 0)
													sample->generateStream()->play();
											}
											if (obj.EndTime != 0) {
												if (obj.ssample)
												{
													obj.ssample_stream = obj.ssample->generateStream();
												}
												if (obj.ssamplew)
												{
													obj.ssamplew_stream = obj.ssamplew->generateStream();
												}
											}
											ApplyHit(obj, res.first, err);
											return;
										}
									}
									return;
								}
						}
					}
				}
			}
		}
	}
	virtual void Tick(double)
	{
		if (bgm != 0 && !bgm->isPlaying())
		{
			if (sw.Elapsed() - aoffset() - 1000 > length)
			{
				parent->Back();
			}
		}
		if (sw.Elapsed() >= aoffset() && sw.Elapsed() <= aoffset() + 1000)
		{
			if (!bgm->isPlaying())
			{
				bgm->play();
				bgm->setVolume(0.5);
			}
		}
		if (!loading)
		{
			auto e_ms = sw.Elapsed() - offset();
			for (auto& obj : objects)
			{
				if (!obj.HasHit)
					if (e_ms - obj.StartTime > hitranges[OsuStatic::HitResult::Meh])
					{
						ApplyHit(obj, OsuStatic::HitResult::Miss, 0);
					}
				if (e_ms < obj.EndTime)
				{
					if (obj.ssample_stream != 0)
					{
						if (!obj.ssample_stream->isPlaying())
							obj.ssample_stream->play();
					}
					if (obj.ssamplew_stream != 0)
					{
						if (!obj.ssamplew_stream->isPlaying())
							obj.ssamplew_stream->play();
					}
				}
				else
				{
					if (obj.ssample_stream != 0)
					{
						obj.ssample_stream->stop();
						delete obj.ssample_stream;
						obj.ssample_stream = 0;
					}
					if (obj.ssamplew_stream != 0)
					{
						obj.ssamplew_stream->stop();
						delete obj.ssamplew_stream;
						obj.ssamplew_stream = 0;
					}
				}
			}
		}
	}
	virtual void Render(GameBuffer& buf)
	{
		if (loading)
		{
			buf.DrawString("Loading...", 0, 0, {}, {});
			return;
		}
		if (background != 0)
		{
			if (resized_bg == 0 || rbg_h != buf.Height || rbg_w != buf.Width)
			{
				if (resized_bg != 0)
					delete resized_bg;
				resized_bg = new unsigned char[buf.Width * buf.Height * 3];
				stbir_resize_uint8(background, bg_w, bg_h, 0, resized_bg, buf.Width, buf.Height, 0, 3);
				rbg_h = buf.Height;
				rbg_w = buf.Width;
			}
			int x = 0;
			int y = 0;
			for (size_t i = 0; i < buf.Width * buf.Height * 3; i += 3)
			{
				buf.SetPixel(x, y, { {},{255,(unsigned char)(resized_bg[i + 0] / 2),(unsigned char)(resized_bg[i + 1] / 2),(unsigned char)(resized_bg[i + 2] / 2)},' ' });
				x++;
				if (x >= buf.Width)
				{
					x = 0;
					y++;
				}
			}
		}
		auto e_ms = sw.Elapsed() - offset();
		double key_width = 10;
		double centre = (double)buf.Width / 2;
		double centre_start = centre - (keys * key_width) / 2;
		double judge_height = 4;
		auto j = 0;
		for (double i = centre_start; i < keys * key_width + centre_start; i += key_width)
		{
			buf.FillRect(i + 1, buf.Height - judge_height + 1, i + key_width, buf.Height, { {},{120,255,255,255},' ' });
			for (auto obj : objects)
			{
				auto off = obj.StartTime - e_ms;
				auto off2 = obj.EndTime - e_ms;
				if (obj.EndTime == 0)
				{
					if (off > scrollspeed || off < -scrollspeed / 5)
						continue;
				}
				else
				{
					if (off > scrollspeed || off2 < 0)
						continue;
				}
				if (obj.Column == j)
				{
					auto ratio = 1 - (obj.StartTime - e_ms) / scrollspeed;
					auto starty = ratio * (buf.Height - judge_height);
					//Break();
					auto base = GameBuffer::Color{ 0,0,160,230 };
					if (obj.Multi)
						base = { 0,204,187,102 };
					if (obj.EndTime != 0 && !obj.HasSlide)
					{
						auto ratio2 = 1 - (obj.EndTime - e_ms) / scrollspeed;
						auto endy = ratio2 * (buf.Height - judge_height);
						auto a = std::min(starty, buf.Height - judge_height);
						base.Alpha = 180;
						buf.FillRect(i + 1, a, i + key_width, endy, { {},base,' ' });
						base.Alpha = 255;
						buf.FillRect(i + 1, a, i + key_width, a, { {},base,' ' });
						continue;
					}
					base.Alpha = 255;
					if (!obj.HasHit)
						buf.FillRect(i + 1, starty, i + key_width, starty, { {},base,' ' });
				}
			}
			buf.DrawLineH(i, 0, buf.Height, { {190,255,255,255},{},'|' });
			buf.DrawLineH(i + key_width, 0, buf.Height, { {190,255,255,255},{},'|' });
			j++;
		}
		buf.DrawLineV(0, buf.Width, 0, { {},{60,255,255,255},' ' });
		buf.DrawLineV(0, (e_ms / length) * buf.Width, 0, { {},{60,90,255,100},' ' });
		auto length_text = std::to_string(int(length / 1000 / 60)) + ":" + std::to_string(std::abs(int(length / 1000) % 60));
		buf.DrawString(length_text, buf.Width - length_text.size() - 1, 0, {}, {});
		auto current_text = std::to_string(int(e_ms / 1000 / 60)) + ":" + std::to_string(std::abs(int(e_ms / 1000) % 60));
		buf.DrawString(current_text, 0, 0, {}, {});
		int o = 0;
		for (auto res : HitCounter)
		{
			auto tstr = GetHitResultName(res.first) + ":" + std::to_string(res.second);
			buf.DrawString(tstr, buf.Width - tstr.size() - 1, (buf.Height - HitCounter.size()) / 2 + o, {}, {});
			o++;
		}
		auto accstr = std::to_string(accuracy);
		accstr.erase(accstr.find('.') + 3);
		accstr += "%";
		buf.DrawString(accstr, buf.Width - accstr.size() - 1, 2, {}, {});
		auto scorestr = std::to_string(int(score));
		scorestr.insert(0, 16 - scorestr.size(), '0');
		buf.DrawString(scorestr, buf.Width - scorestr.size() - 1, 1, {}, {});
		auto combo_text = std::to_string(combo);
		combo_text += "x";
		buf.DrawString(combo_text, 0, buf.Height - 1, {}, {});
		auto he_avg_text = std::to_string(hiterr_avg);
		he_avg_text.erase(he_avg_text.find('.') + 2);
		he_avg_text += "ms";
		buf.DrawString(he_avg_text, 0, buf.Height - 2, {}, {});
	}
	static int CalcColumn(double xpos, int keys)
	{
		double begin = 512 / keys / 2;
		double mid = begin;
		for (int i = 0; i < keys; i++)
		{
			if (std::abs(mid - xpos) < begin)
			{
				return i;
			}
			mid += begin * 2;
		}
		return 0;
	}
	// Define the audio sample metadata struct
	struct AudioSampleMetadata
	{
		OsuStatic::SampleBank sampleBank;
		OsuStatic::HitSoundType hitSoundType;
		fspath filename;
		int sampleset;
	};
	// Function to search the audio sample files and build an index
	static std::vector<AudioSampleMetadata> BuildSampleIndex(fspath folder)
	{
		std::vector<AudioSampleMetadata> sampleIndex;

		for (const auto& file : std::filesystem::directory_iterator(folder))
		{
			if (file.is_directory())
				continue;
			auto filename = file.path().filename().string();
			if (!EndsWith(filename, ".wav") && !EndsWith(filename, ".ogg") && !EndsWith(filename, ".mp3"))
				continue;
			// Parsing the file name
			size_t startPos = filename.find_first_of('-');
			if (startPos == std::string::npos)
				continue;
			std::string sampleBankStr = filename.substr(0, startPos);
			OsuStatic::SampleBank sampleBank = OsuStatic::SampleBank::None;
			if (sampleBankStr == "normal") sampleBank = OsuStatic::SampleBank::Normal;
			else if (sampleBankStr == "soft") sampleBank = OsuStatic::SampleBank::Soft;
			else if (sampleBankStr == "drum") sampleBank = OsuStatic::SampleBank::Drum;

			auto endPos = filename.find_first_of('.', startPos);
			std::string hitSoundTypeStr = filename.substr(startPos + 1, endPos - startPos);
			OsuStatic::HitSoundType hitSoundType = OsuStatic::HitSoundType::None;
			if (StartsWith(hitSoundTypeStr, "hitclap")) hitSoundType = OsuStatic::HitSoundType::Clap;
			else if (StartsWith(hitSoundTypeStr, "hitfinish")) hitSoundType = OsuStatic::HitSoundType::Finish;
			else if (StartsWith(hitSoundTypeStr, "hitnormal")) hitSoundType = OsuStatic::HitSoundType::Normal;
			else if (StartsWith(hitSoundTypeStr, "hitwhistle")) hitSoundType = OsuStatic::HitSoundType::Whistle;
			else if (StartsWith(hitSoundTypeStr, "sliderslide")) hitSoundType = OsuStatic::HitSoundType::Slide;
			else if (StartsWith(hitSoundTypeStr, "sliderwhistle")) hitSoundType = OsuStatic::HitSoundType::SlideWhistle;
			else if (StartsWith(hitSoundTypeStr, "slidertick")) hitSoundType = OsuStatic::HitSoundType::SlideTick;
			std::string sample_str;
			auto first = std::find_if(filename.begin() + startPos, filename.end(), [](char chr) {return isdigit(chr); });
			int sampleset = 0;
			if (first != filename.end())
			{
				sample_str.resize(filename.size());
				std::copy(first, filename.end(), sample_str.begin());
				sampleset = std::stoi(sample_str);
			}
			// Build the index using the parsed data
			AudioSampleMetadata metadata{ sampleBank, hitSoundType,file.path(), sampleset };
			sampleIndex.push_back(metadata);
		}

		return sampleIndex;
	}
	static std::vector<AudioSampleMetadata> CombineSamples(std::vector<AudioSampleMetadata>& base, std::vector<AudioSampleMetadata>& up)
	{
		std::vector<AudioSampleMetadata> result;
		result.reserve(base.size() + up.size());
		result.insert(result.end(), base.begin(), base.end());
		result.insert(result.end(), up.begin(), up.end());
		result.erase(std::unique(result.begin(), result.end(), [](auto& a, auto& b) {return a.sampleBank == b.sampleBank && a.hitSoundType == b.hitSoundType; }), result.end());
		return result;
	}
	static std::vector<fspath> GetSample(std::vector<AudioSampleMetadata> metadata,
		OsuStatic::SampleBank bank, OsuStatic::HitSoundType hs, int sampleset)
	{
		std::vector<fspath> paths;
		for (auto& sample : metadata)
		{
			if (sample.sampleBank == bank)
				if (HasFlag(hs, sample.hitSoundType))
				{
					if (sample.sampleset == sampleset)
					{
						paths.push_back(sample.filename);
					}
				}
		}
		return paths;
	}
	static OsuBeatmap::TimingPoint GetTimingPoint(OsuBeatmap& bmp, double time)
	{
		for (auto tp : bmp.TimingPoints)
		{
			if (time > tp.Time)
			{
				return tp;
			}
		}
		return bmp.TimingPoints[0];
	}
	std::vector<std::function<void()>> destructions;
	static void Load(GameplayScreen* pthis)
	{
		auto sampleindex = BuildSampleIndex(pthis->p_path);
		auto defaultsampleindex = BuildSampleIndex(pthis->sample_folder);
		auto combined = CombineSamples(sampleindex, defaultsampleindex);
		auto ifs = std::ifstream(pthis->path.string());
		OsuBeatmap osub = OsuBeatmap::Parse(ifs);
		for (auto& obj : osub.HitObjects)
		{
			if (!obj.CustomSampleFilename.empty())
			{
				AudioSampleMetadata asm2{};
				asm2.filename = pthis->p_path.string().append("\\").append(obj.CustomSampleFilename);
				combined.push_back(asm2);
			}
		}
		std::map<std::string, IAudioManager::ISample*> samplecache;
		{
			LoadEventArgs lea{};
			IAudioManager::ISample* res;
			lea.sample = true;
			lea.callback = new std::function<void(const LoadCompleteEventArgs&)>([&res](const LoadCompleteEventArgs& lcea) {
				res = lcea.sample;
				});
			for (auto& fs : defaultsampleindex)
			{
				auto str = fs.filename.string();
				if (samplecache[str] == 0)
				{
					lea.requested_path = str.c_str();
					pthis->game->Raise("load", lea);
					samplecache[str] = res;
				}
			}
			for (auto& fs : combined)
			{
				auto str = fs.filename.string();
				if (samplecache[str] == 0)
				{
					lea.requested_path = str.c_str();
					pthis->game->Raise("load", lea);
					samplecache[str] = res;
				}
			}
		}
		pthis->destructions.push_back([samplecache]() {
			for (auto& cache : samplecache)
			{
				delete cache.second;
			}
			});
		if (HasFlag(pthis->mods, OsuMods::Hardrock))
		{
			osub.OverallDifficulty *= 2;
			osub.OverallDifficulty = std::min(osub.OverallDifficulty, 10.0);
		}
		auto playbackrate = 1.0;
		pthis->hitranges = OsuStatic::GetHitRanges(osub.OverallDifficulty);
		for (auto hr : pthis->hitranges)
		{
			hr.second /= playbackrate;
		}
		pthis->keys = int(osub.CircleSize);

		if (HasFlag(pthis->mods, OsuMods::Nightcore))
		{
			playbackrate = 1.5;
		}
		if (HasFlag(pthis->mods, OsuMods::HalfTime))
		{
			playbackrate = 0.75;
		}
		LoadEventArgs lea{};
		auto str = pthis->p_path.string().append("\\").append(osub.AudioFilename);
		lea.requested_path = str.c_str();
		lea.callback = new std::function<void(const LoadCompleteEventArgs&)>([pthis](const auto& lcea) {pthis->bgm = lcea.stream; });
		pthis->game->Raise("load", lea);
		pthis->bgm->setPlaybackRate(playbackrate);
		for (auto& obj : osub.HitObjects)
		{
			ManiaObject mo{ 0 };
			mo.StartTime = obj.StartTime / playbackrate;
			mo.Column = CalcColumn(obj.X, pthis->keys);
			auto tp = GetTimingPoint(osub, obj.StartTime);
			auto samplebank = tp.SampleBank;
			auto vol = tp.SampleVolume;
			auto sampleset = tp.SampleSet;
			auto hitsoundtype = (OsuStatic::HitSoundType)((int)obj.SoundType | (int)OsuStatic::HitSoundType::Normal);
			auto defaultsamples = GetSample(combined, samplebank, hitsoundtype, sampleset);
			for (auto fspath : defaultsamples)
			{
				auto smp = samplecache[fspath.string()];
				if (smp != 0)
					mo.samples.emplace_back(smp);
			}
			if (mo.samples.empty())
			{
				auto defsamples2 = GetSample(defaultsampleindex, samplebank, hitsoundtype, 0);
				for (auto fspath : defsamples2)
				{
					auto smp = samplecache[fspath.string()];
					if (smp != 0)
						mo.samples.emplace_back(smp);
				}
			}
			if (!obj.CustomSampleFilename.empty())
			{
				mo.samples.emplace_back(samplecache[pthis->p_path.string().append("\\").append(obj.CustomSampleFilename)]);
			}
			if (obj.EndTime != 0)
			{
				auto slidesamples = GetSample(combined, samplebank, OsuStatic::HitSoundType::Slide, sampleset);
				if (!slidesamples.empty())
				{
					mo.ssample = samplecache[slidesamples[0].string()];
				}
				if (HasFlag(obj.SoundType, OsuStatic::HitSoundType::Whistle))
				{
					auto wsamples = GetSample(combined, samplebank, OsuStatic::HitSoundType::SlideWhistle, sampleset);
					if (!wsamples.empty())
					{
						mo.ssamplew = samplecache[wsamples[0].string()];
					}
				}
			}
			mo.EndTime = obj.EndTime / playbackrate;
			for (auto& obj2 : osub.HitObjects)
			{
				if (&obj2 == &obj)
					continue;
				if (obj2.EndTime != 0)
				{
					if (std::abs(obj2.EndTime - obj.StartTime) < 0.01)
					{
						mo.Multi = true;
						break;
					}
				}
				if (std::abs(obj2.StartTime - obj.StartTime) < 0.01)
				{
					mo.Multi = true;
					break;
				}
			}
			pthis->objects.push_back(mo);
			pthis->length = std::max(std::max(mo.StartTime, mo.EndTime), pthis->length);
		}
		int x = 0;
		int y = 0;
		fspath pth = pthis->p_path;
		pth /= osub.Background;
		auto str2 = pth.string();
		pthis->background = (unsigned char*)stbi_load(str2.c_str(), &x, &y, 0, 3);
		pthis->bg_w = x;
		pthis->bg_h = y;
		pthis->loading = false;
		pthis->KeyBinds = GetKeyBinds(pthis->keys);
		pthis->sw.Start();
	}
	virtual void Activate(bool y)
	{
		if (y)
		{
			std::thread(Load, this).detach();
		}
		else
		{
			if (bgm != 0)
			{
				delete bgm;
				bgm = 0;
			}
			if (background != 0)
			{
				delete background;
				background = 0;
			}
			if (resized_bg != 0)
			{
				delete resized_bg;
				resized_bg = 0;
			}
			for (auto& des : destructions)
			{
				des();
			}
		}
	}
};