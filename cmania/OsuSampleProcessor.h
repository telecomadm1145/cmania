#pragma once
#include "OsuStatic.h"
#include <filesystem>
#include "EnumFlag.h"
#include <vector>
#include "AudioManager.h"
class OsuSampleProcessor {
private:
	struct AudioSampleMetadata {
		SampleBank sampleBank;
		HitSoundType hitSoundType;
		std::filesystem::path filename;
		int sampleset;
	};
	std::vector<AudioSampleMetadata> fallback;
	std::vector<AudioSampleMetadata> def;
	std::vector<AudioSampleMetadata> extra;
	std::map<std::filesystem::path, AudioSample> cache;

	static std::vector<AudioSampleMetadata> Load(std::filesystem::path folder, int defsampleset) {
		std::vector<AudioSampleMetadata> sampleIndex;
		if (!std::filesystem::exists(folder))
			return sampleIndex;
		for (const auto& file : std::filesystem::directory_iterator(folder)) {
			if (file.is_directory())
				continue;
			auto filename = file.path().filename().string();
			if (!filename.ends_with(".wav") && !filename.ends_with(".ogg") && !filename.ends_with(".mp3"))
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
			if (hitSoundTypeStr.starts_with("hitclap"))
				hitSoundType = HitSoundType::Clap;
			else if (hitSoundTypeStr.starts_with("hitfinish"))
				hitSoundType = HitSoundType::Finish;
			else if (hitSoundTypeStr.starts_with("hitnormal"))
				hitSoundType = HitSoundType::Normal;
			else if (hitSoundTypeStr.starts_with("hitwhistle"))
				hitSoundType = HitSoundType::Whistle;
			else if (hitSoundTypeStr.starts_with("sliderslide"))
				hitSoundType = HitSoundType::Slide;
			else if (hitSoundTypeStr.starts_with("sliderwhistle"))
				hitSoundType = HitSoundType::SlideWhistle;
			else if (hitSoundTypeStr.starts_with("slidertick"))
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

public:
	void LoadSkin(std::filesystem::path folder) {
		fallback = Load(folder, 0);
	}
	void LoadBeatmap(std::filesystem::path folder) {
		def = Load(folder, 1);
	}
	void LoadSampleEx(std::filesystem::path path) {
		extra.emplace_back(SampleBank::None,HitSoundType::None,path,1);
	}

	void BuildCache() {
		DoCache(fallback);
		DoCache(def);
		DoCache(extra);
	}

private:
	void DoCache(std::vector<AudioSampleMetadata> meta) {
		auto bam = GetBassAudioManager();
		for (auto& item : meta) {
			try {
				auto dat = ReadAllBytes(item.filename.string());
				cache[item.filename] = AudioSample(bam->loadSample(dat.data(), dat.size()));
			}
			catch (...) {
			}
		}
	}
	static std::vector<std::filesystem::path> GetSample(std::vector<AudioSampleMetadata> metadata, SampleBank bank, HitSoundType hs, int sampleset) {
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

public:
	AudioSample GetSampleCustom(std::filesystem::path path) {
		return cache[path];
	}
	std::vector<AudioSample> GetSampleLayered(SampleBank bank, HitSoundType hs, int sampleset) {
		auto samples = GetSample(def, bank, hs, sampleset);
		if (samples.empty()) {
			if (hs == HitSoundType::None)
				hs = HitSoundType::Normal;
			samples = GetSample(fallback, bank, hs, 0);
		}
		std::vector<AudioSample> csamples;
		for (auto& str:samples) {
			auto s = GetSampleCustom(str);
			if (s == 0)
				continue;
			csamples.push_back(s);
		}
		return csamples;
	}
	AudioSample GetSampleLayeredOne(SampleBank bank, HitSoundType hs, int sampleset) {
		auto samples = GetSampleLayered(bank, hs, sampleset);
		if (samples.empty())
			return {};
		return *samples.begin();
	}
};