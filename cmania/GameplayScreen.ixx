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
import Stopwatch;

using fspath = std::filesystem::path;

export class GameplayScreen : public Screen
{
	OsuMods mods;
	fspath path;
	double length;
	fspath p_path;
	bool loading = true;
	bool pause = false;
	int keys;
	double scrollspeed = 500;
	Stopwatch sw;
	std::map<OsuStatic::HitResult, double> hitranges;
	IAudioManager::IAudioStream* bgm = 0;
	fspath sample_folder = ".\\samples\\triangles";
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
		if (!HasFlag(mods, OsuMods::Auto)) // ×è¶Ï Auto mod ÏÂµÄÊäÈë
		{

		}
		if (loading)
		{
			return;
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
					if (off > scrollspeed || off < -scrollspeed / 10)
						continue;
				}
				else
				{
					if (off > scrollspeed || off2 < -scrollspeed / 10)
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
		LoadEventArgs lea{};
		auto str = pthis->p_path.string().append("\\").append(osub.AudioFilename);
		lea.requested_path = str.c_str();
		lea.callback = new std::function<void(const LoadCompleteEventArgs&)>([pthis](const auto& lcea) {pthis->bgm = lcea.stream; });
		pthis->game->Raise("load", lea);
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
			for (auto& fs : combined)
			{
				auto str = fs.filename.string();
				lea.requested_path = str.c_str();
				pthis->game->Raise("load", lea);
				samplecache[fs.filename.string()] = res;
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
		pthis->hitranges = OsuStatic::GetHitRanges(osub.OverallDifficulty);
		pthis->keys = int(osub.CircleSize);
		if (HasFlag(pthis->mods, OsuMods::Nightcore))
		{
			for (auto hr : pthis->hitranges)
			{
				hr.second /= 1.5;
			}
		}
		for (auto& obj : osub.HitObjects)
		{
			ManiaObject mo{ 0 };
			mo.StartTime = obj.StartTime;
			mo.Column = CalcColumn(obj.X, pthis->keys);
			auto tp = GetTimingPoint(osub, obj.StartTime);
			auto samplebank = tp.SampleBank;
			auto vol = tp.SampleVolume;
			auto sampleset = tp.SampleSet;
			auto hitsoundtype = (OsuStatic::HitSoundType)((int)obj.SoundType | (int)OsuStatic::HitSoundType::Normal);
			auto defaultsamples = GetSample(combined, samplebank, hitsoundtype, sampleset);
			for (auto fspath : defaultsamples)
			{
				mo.samples.emplace_back(samplecache[fspath.string()]);
			}
			if (!obj.CustomSampleFilename.empty())
			{
				mo.samples.emplace_back(samplecache[pthis->p_path.string().append("\\").append(obj.CustomSampleFilename)]);
			}
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
			mo.EndTime = obj.EndTime;
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
		pthis->loading = false;
		pthis->sw.Start();
	}
	virtual void Activate(bool y)
	{
		if (y)
		{
			std::thread(Load, this).detach();
		}
	}
};