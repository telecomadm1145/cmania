#pragma once
#include <vector>
#include "OsuStatic.h"
#include <filesystem>
#include "String.h"
#include "EnumFlag.h"
#include "OsuBeatmap.h"
struct AudioSampleMetadata {
	SampleBank sampleBank;
	HitSoundType hitSoundType;
	std::filesystem::path filename;
	int sampleset;
};
static std::vector<AudioSampleMetadata> BuildSampleIndex(std::filesystem::path folder, int defsampleset) {
	std::vector<AudioSampleMetadata> sampleIndex;
	if (!std::filesystem::exists(folder))
		return sampleIndex;
	for (const auto& file : std::filesystem::directory_iterator(folder)) {
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
		SampleBank sampleBank = SampleBank::None;
		if (sampleBankStr == "normal")
			sampleBank = SampleBank::Normal;
		else if (sampleBankStr == "soft")
			sampleBank = SampleBank::Soft;
		else if (sampleBankStr == "drum")
			sampleBank = SampleBank::Drum;

		auto endPos = filename.find_first_of('.', startPos);
		std::string hitSoundTypeStr = filename.substr(startPos + 1, endPos - startPos);
		HitSoundType hitSoundType = HitSoundType::None;
		if (StartsWith(hitSoundTypeStr, "hitclap"))
			hitSoundType = HitSoundType::Clap;
		else if (StartsWith(hitSoundTypeStr, "hitfinish"))
			hitSoundType = HitSoundType::Finish;
		else if (StartsWith(hitSoundTypeStr, "hitnormal"))
			hitSoundType = HitSoundType::Normal;
		else if (StartsWith(hitSoundTypeStr, "hitwhistle"))
			hitSoundType = HitSoundType::Whistle;
		else if (StartsWith(hitSoundTypeStr, "sliderslide"))
			hitSoundType = HitSoundType::Slide;
		else if (StartsWith(hitSoundTypeStr, "sliderwhistle"))
			hitSoundType = HitSoundType::SlideWhistle;
		else if (StartsWith(hitSoundTypeStr, "slidertick"))
			hitSoundType = HitSoundType::SlideTick;
		std::string sample_str;
		auto first = std::find_if(filename.begin() + startPos, filename.end(), [](char chr) { return isdigit(chr); });
		int sampleset = defsampleset;
		if (first != filename.end()) {
			sample_str.resize(filename.size());
			std::copy(first, filename.end(), sample_str.begin());
			sampleset = std::stoi(sample_str);
		}
		// Build the index using the parsed data
		AudioSampleMetadata metadata{ sampleBank, hitSoundType, file.path(), sampleset };
		sampleIndex.push_back(metadata);
	}

	return sampleIndex;
}
static std::vector<std::filesystem::path> GetSample(std::vector<AudioSampleMetadata> metadata,
	SampleBank bank, HitSoundType hs, int sampleset) {
	std::vector<std::filesystem::path> paths;
	for (auto& sample : metadata) {
		if (sample.sampleBank == bank)
			if (HasFlag(hs, sample.hitSoundType)) {
				if (sample.sampleset == sampleset) {
					paths.push_back(sample.filename);
				}
			}
	}
	return paths;
}
static std::vector<std::filesystem::path> GetSampleLayered(std::vector<AudioSampleMetadata> def, std::vector<AudioSampleMetadata> fallback,
	SampleBank bank, HitSoundType hs, int sampleset) {
	auto samples = GetSample(def, bank, hs, sampleset);
	if (samples.empty()) {
		if (hs == HitSoundType::None)
			hs = HitSoundType::Normal;
		samples = GetSample(fallback, bank, hs, 0);
	}
	return samples;
}
static OsuBeatmap::TimingPoint& GetTimingPoint(OsuBeatmap& bmp, double time) {
	for (auto& tp : bmp.TimingPoints) {
		if (time > tp.Time) {
			return tp;
		}
	}
	return bmp.TimingPoints[0];
}
static OsuBeatmap::TimingPoint& GetTimingPointTiming(OsuBeatmap& bmp, double time) {
	for (auto& tp : bmp.TimingPoints) {
		if (time > tp.Time && tp.TimingChange) {
			return tp;
		}
	}
	return bmp.TimingPoints[0];
}
static OsuBeatmap::TimingPoint& GetTimingPointNonTiming(OsuBeatmap& bmp, double time) {
	for (auto& tp : bmp.TimingPoints) {
		if (time > tp.Time && !tp.TimingChange) {
			return tp;
		}
	}
	return bmp.TimingPoints[0];
}