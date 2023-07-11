export module OsuBeatmap;
import OsuStatic;
import String;
import <string>;
import <istream>;
import <vector>;

export class OsuBeatmap
{
public:
	struct TimingPoint {
		double Time;
		double BeatLength = 100;
		int TimeSignature = 4;
		OsuStatic::SampleBank SampleBank;
		int SampleSet;
		double SampleVolume;
		bool TimingChange;
		OsuStatic::EffectFlags Effects;

		double BPM() const {
			return 60000 / BeatLength;
		}

		double SpeedMultiplier() const {
			return BeatLength < 0 ? 100.0 / -BeatLength : 1;
		}
	};
	struct HitObject {
		double X;
		double Y;
		double StartTime;
		OsuStatic::HitObjectType Type;
		OsuStatic::HitSoundType SoundType;
		std::string PathRecord;
		int RepeatCount;
		double Length;
		double EndTime;
		double CustomSampleVolume;
		std::string CustomSampleFilename;
		std::string CustomSampleBanks;
		OsuStatic::HitObjectType GetHitobjectType()
		{
			return (OsuStatic::HitObjectType)((int)Type & ((int)OsuStatic::HitObjectType::Circle | (int)OsuStatic::HitObjectType::Slider | (int)OsuStatic::HitObjectType::Spinner | (int)OsuStatic::HitObjectType::Hold));
		}
		void ResolveCustomSampleBanks()
		{
			auto args = split(CustomSampleBanks, ':');
			if (args.size() > 4)
			{
				CustomSampleVolume = std::stod(args[3]);
				CustomSampleFilename = args[4];
			}
		}
	};
	std::vector<std::pair<double, double>> BreakPeriods;
	std::vector<TimingPoint> TimingPoints;
	std::vector<HitObject> HitObjects;
	std::string Title;
	std::string Artist;
	std::string TitleUnicode;
	std::string ArtistUnicode;
	std::string AudioFilename;
	std::string Version;
	std::string Creator;
	std::string Background;
	std::string Video;
	int VideoOffset;
	std::string Source;
	std::string Tags;
	int PreviewTime = 0;
	std::string SampleSet;
	double StackLeniency = 0.7;
	int Countdown = 0;
	OsuStatic::GameMode Mode = OsuStatic::GameMode::Std;
	double HPDrainRate = 0;
	double CircleSize = 0;
	double OverallDifficulty = 0;
	double ApproachRate = 0;
	double SliderMultiplier = 0;
	double SliderTickRate = 0;
	struct StoryboardSoundSample
	{
		double StartTime;
		std::string path;
	};
	std::vector<StoryboardSoundSample> StoryboardSamples;
	std::vector<std::tuple<std::string, std::string, std::string>> Others;

	static OsuBeatmap Parse(std::istream& sr)
	{
		OsuBeatmap bm{};
		std::string line;
		std::string category;
		while (std::getline(sr, line))
		{
			if (line[0] == '[' && line[line.length() - 1] == ']')
			{
				category = line.substr(1, line.length() - 2);
				continue;
			}
			if (line.empty() || line.substr(0, 2) == "//")
				continue;
			if (category == "Events")
			{
				if (line[0] == '2')
				{
					auto args = split(line, ',');
					bm.BreakPeriods.push_back(std::make_pair(std::stod(args[1]), std::stod(args[2])));
				}
				if (line[0] == '0')
				{
					auto args = split(line, ',');
					bm.Background = Trim(args[2], '\"');
				}
				if (StartsWith(line, "Video"))
				{
					auto args = split(line, ',');
					bm.Video = Trim(args[2], '\"');
					bm.VideoOffset = std::stoi(args[1]);
				}
				//£¿
			}
			else if (category == "TimingPoints")
			{
				TimingPoint tp;
				auto args = split(line, ',');
				tp.Time = std::stod(args[0]);
				tp.BeatLength = std::stod(args[1]);
				if (args.size() > 2)
					tp.TimeSignature = std::stoi(args[2]);
				if (args.size() > 3)
					tp.SampleBank = (OsuStatic::SampleBank)std::stoi(args[3]);
				if (args.size() > 4)
					tp.SampleSet = std::stoi(args[4]);
				if (args.size() > 5)
					tp.SampleVolume = std::stod(args[5]);
				if (args.size() > 6)
					tp.TimingChange = std::stoi(args[6]);
				if (args.size() > 7)
					tp.Effects = (OsuStatic::EffectFlags)std::stoi(args[7]);
				bm.TimingPoints.push_back(tp);
			}
			else if (category == "HitObjects") {
				HitObject ho{0};
				auto args = split(line, ',');
				ho.X = std::stod(args[0]);
				ho.Y = std::stod(args[1]);
				ho.StartTime = std::stod(args[2]);
				ho.Type = (OsuStatic::HitObjectType)std::stoi(args[3]);
				ho.SoundType = (OsuStatic::HitSoundType)std::stoi(args[4]);
				switch (ho.GetHitobjectType())
				{
				case OsuStatic::HitObjectType::Circle: {
					if (args.size() > 5)
					{
						ho.CustomSampleBanks = args[5];
						ho.ResolveCustomSampleBanks();
					}
					bm.HitObjects.push_back(ho);
					break;
				}
				case OsuStatic::HitObjectType::Slider: {
					ho.PathRecord = args[5];
					ho.RepeatCount = std::stoi(args[6]);
					ho.Length = std::stod(args[7]);
					if (args.size() > 10)
					{
						ho.CustomSampleBanks = args[10];
						ho.ResolveCustomSampleBanks();
					}
					bm.HitObjects.push_back(ho);
					break;
				}
				case OsuStatic::HitObjectType::Hold: {
					size_t colonPos2 = args[5].find(':');
					if (colonPos2 != std::string::npos)
					{
						ho.CustomSampleBanks = args[5].substr(colonPos2);
						ho.EndTime = std::stod(args[5].substr(0, colonPos2));
						ho.ResolveCustomSampleBanks();
						bm.HitObjects.push_back(ho);
					}
					break;
				}
				case OsuStatic::HitObjectType::Spinner: {
					ho.EndTime = std::stod(args[5]);
					if (args.size() > 6)
					{
						ho.CustomSampleBanks = args[6];
						ho.ResolveCustomSampleBanks();
					}
					bm.HitObjects.push_back(ho);
					break;
				}
				default:
					break;
				}
			}
			else
			{
				size_t colonPos = line.find(':');
				if (colonPos != std::string::npos) {
					std::string key = Trim(line.substr(0, colonPos));
					std::string value = Trim(line.substr(colonPos + 1));
					// Find corresponding property in RawBeatmap and set its value
					if (key == "AudioFilename")
						bm.AudioFilename = value;
					else if (key == "PreviewTime")
						bm.PreviewTime = ConvertFromString<int>(value);
					else if (key == "SampleSet")
						bm.SampleSet = value;
					else if (key == "Tags")
						bm.Tags = value;
					else if (key == "Source")
						bm.Source = value;
					else if (key == "StackLeniency")
						bm.StackLeniency = ConvertFromString<double>(value);
					else if (key == "Countdown")
						bm.Countdown = ConvertFromString<int>(value);
					else if (key == "Mode")
						bm.Mode = (OsuStatic::GameMode)ConvertFromString<int>(value);
					else if (key == "TitleUnicode")
						bm.TitleUnicode = value;
					else if (key == "ArtistUnicode")
						bm.ArtistUnicode = value;
					else if (key == "Title")
						bm.Title = value;
					else if (key == "Artist")
						bm.Artist = value;
					else if (key == "Creator")
						bm.Creator = value;
					else if (key == "Version")
						bm.Version = value;
					else if (key == "HPDrainRate")
						bm.HPDrainRate = ConvertFromString<double>(value);
					else if (key == "CircleSize")
						bm.CircleSize = ConvertFromString<double>(value);
					else if (key == "OverallDifficulty")
						bm.OverallDifficulty = ConvertFromString<double>(value);
					else if (key == "ApproachRate")
						bm.ApproachRate = ConvertFromString<double>(value);
					else if (key == "SliderMultiplier")
						bm.SliderMultiplier = ConvertFromString<double>(value);
					else if (key == "SliderTickRate")
						bm.SliderTickRate = ConvertFromString<double>(value);
					else {
						// If property is not found, add it to Others vector
						bm.Others.emplace_back(category, key, value);
					}
				}
			}
		}
		return bm;
	}
};