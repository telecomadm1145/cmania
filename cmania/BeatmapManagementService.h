#pragma once
#include "Game.h"
#include "SettingStorage.h"
#include <string>
#include <vector>
#include <filesystem>
struct DifficultyCacheEntry {
	std::string background;
	std::string audio;
	std::string name;
	double preview;
	double nps;
	double diff;
	double length;
	double keys;
	double od;
	std::string path;
	std::vector<std::string> records;
	void Write(std::ostream& os) const {
		Binary::Write(os, background);
		Binary::Write(os, audio);
		Binary::Write(os, name);
		Binary::Write(os, preview);
		Binary::Write(os, nps);
		Binary::Write(os, diff);
		Binary::Write(os, length);
		Binary::Write(os, keys);
		Binary::Write(os, od);
		Binary::Write(os, records);
		Binary::Write(os, path);
	}
	void Read(std::istream& is) {
		Binary::Read(is, background);
		Binary::Read(is, audio);
		Binary::Read(is, name);
		Binary::Read(is, preview);
		Binary::Read(is, nps);
		Binary::Read(is, diff);
		Binary::Read(is, length);
		Binary::Read(is, keys);
		Binary::Read(is, od);
		Binary::Read(is, records);
		Binary::Read(is, path);
	}
};
struct SongsCacheEntry {
	std::chrono::system_clock::duration lastchanged;
	std::string path;
	std::string artist;
	std::string artistunicode;
	std::string title;
	std::string titleunicode;
	std::string tags;
	std::string source;
	std::vector<DifficultyCacheEntry> difficulties;
	void Write(std::ostream& os) const {
		Binary::Write(os, lastchanged);
		Binary::Write(os, path);
		Binary::Write(os, artist);
		Binary::Write(os, artistunicode);
		Binary::Write(os, title);
		Binary::Write(os, titleunicode);
		Binary::Write(os, tags);
		Binary::Write(os, source);
		Binary::Write(os, difficulties);
	}
	void Read(std::istream& is) {
		Binary::Read(is, lastchanged);
		Binary::Read(is, path);
		Binary::Read(is, artist);
		Binary::Read(is, artistunicode);
		Binary::Read(is, title);
		Binary::Read(is, titleunicode);
		Binary::Read(is, tags);
		Binary::Read(is, source);
		Binary::Read(is, difficulties);
	}
};
struct SongsCache {
	std::chrono::system_clock::duration lastchanged;
	std::vector<SongsCacheEntry> caches;
	void Write(std::ostream& os) const {
		Binary::Write(os, lastchanged);
		Binary::Write(os, caches);
	}
	void Read(std::istream& is) {
		Binary::Read(is, lastchanged);
		Binary::Read(is, caches);
	}
};
struct SongsCahceReadyEventArgs {
	SongsCache* Songs;
};
GameComponent* MakeBeatmapManagementService();