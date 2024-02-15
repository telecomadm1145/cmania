#include "OsuBeatmap.h"
#include "OsuStatic.h"
#include "String.h"
#include <iosfwd>
#include <utility>
#include <vector>

OsuBeatmap OsuBeatmap::Parse(std::istream& sr) {
	OsuBeatmap bm{};
	std::string line;
	std::string category;
	try {
		while (std::getline(sr, line)) {
			if (line[0] == '[' && line[line.length() - 1] == ']') {
				category = line.substr(1, line.length() - 2);
				continue;
			}
			if (line.empty() || line.substr(0, 2) == "//")
				continue;
			if (category == "Events") {
				if (line[0] == '2') {
					auto args = split(line, ',');
					bm.BreakPeriods.push_back(std::make_pair(std::stod(args[1]), std::stod(args[2])));
				}
				if (line[0] == '0') {
					auto args = split(line, ',');
					bm.Background = Trim(args[2], '\"');
				}
				if (StartsWith(line, "Video")) {
					auto args = split(line, ',');
					bm.Video = Trim(args[2], '\"');
					bm.VideoOffset = std::stoi(args[1]);
				}
				// ？
			}
			else if (category == "TimingPoints") {
				TimingPoint tp;
				auto args = split(line, ',');
				tp.Time = std::stod(args[0]);
				tp.BeatLength = std::stod(args[1]);
				if (args.size() > 2)
					tp.TimeSignature = std::stoi(args[2]);
				if (args.size() > 3)
					tp.SampleBank = (SampleBank)std::stoi(args[3]);
				if (args.size() > 4)
					tp.SampleSet = std::stoi(args[4]);
				if (args.size() > 5)
					tp.SampleVolume = std::stod(args[5]);
				if (args.size() > 6)
					tp.TimingChange = std::stoi(args[6]);
				if (args.size() > 7)
					tp.Effects = (EffectFlags)std::stoi(args[7]);
				bm.TimingPoints.push_back(tp);
			}
			else if (category == "HitObjects") {
				HitObject ho{ };
				auto args = split(line, ',');
				ho.X = std::stod(args[0]);
				ho.Y = std::stod(args[1]);
				ho.StartTime = std::stod(args[2]);
				ho.Type = (HitObjectType)std::stoi(args[3]);
				ho.SoundType = (HitSoundType)std::stoi(args[4]);
				switch (ho.GetHitobjectType()) {
				case HitObjectType::Circle: {
					if (args.size() > 5) {
						ho.CustomSampleBanks = args[5];
						ho.ResolveCustomSampleBanks();
					}
					bm.HitObjects.push_back(ho);
					break;
				}
				case HitObjectType::Slider: {
					ho.PathRecord = args[5];
					ho.RepeatCount = std::stoi(args[6]);
					ho.Length = std::stod(args[7]);
					if (args.size() > 10) {
						ho.CustomSampleBanks = args[10];
						ho.ResolveCustomSampleBanks();
					}
					bm.HitObjects.push_back(ho);
					break;
				}
				case HitObjectType::Hold: {
					size_t colonPos2 = args[5].find(':');
					if (colonPos2 != std::string::npos) {
						ho.CustomSampleBanks = args[5].substr(colonPos2 + 1);
						ho.EndTime = std::stod(args[5].substr(0, colonPos2));
						ho.ResolveCustomSampleBanks();
						bm.HitObjects.push_back(ho);
					}
					break;
				}
				case HitObjectType::Spinner: {
					ho.EndTime = std::stod(args[5]);
					if (args.size() > 6) {
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
			else {
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
						bm.Mode = (GameMode)ConvertFromString<int>(value);
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
	}
	catch (...) {
#if _DEBUG
		__debugbreak();
#endif
	}
	return bm;
}
