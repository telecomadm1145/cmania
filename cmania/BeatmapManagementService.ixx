export module BeatmapManagementService;
import Game;
import Settings;
import String;
import OsuBeatmap;
import <thread>;
import <string>;
import <vector>;
import <filesystem>;
import <iosfwd>;
import <fstream>;

export struct ResolveBeatmapEventArgs
{
	unsigned long long time;
	std::string path;
};
export struct DifficultyCacheEntry
{
	std::string background;
	std::string audio;
	std::string name;
	double preview;
	double nps;
	double length;
	double keys;
	double od;
	std::string path;
};
export struct SongsCacheEntry
{
	std::string artist;
	std::string artistunicode;
	std::string title;
	std::string titleunicode;
	std::string tags;
	std::string source;
	std::vector<DifficultyCacheEntry> difficulties;
};
export struct SongsCahceReadyEventArgs
{
	std::vector<SongsCacheEntry> Songs;
};

export class BeatmapManagementService : public GameComponent
{
	static void Resolve(BeatmapManagementService* pthis)
	{

	}
	static void GetSongsCache(BeatmapManagementService* pthis)
	{
		if (pthis->songs_path != 0 && std::filesystem::exists(pthis->songs_path))
		{
			SongsCahceReadyEventArgs screa{};
			auto songs_path = std::filesystem::path(pthis->songs_path);
			for (auto& song : std::filesystem::directory_iterator(songs_path))
			{
				if (song.is_directory())
				{
					SongsCacheEntry cache{};
					for (auto& diff : std::filesystem::directory_iterator(song))
					{
						if (!diff.is_directory())
						{
							if (EndsWith(diff.path().string(), ".osu"))
							{
								std::ifstream stm(diff.path());
								OsuBeatmap bmp = OsuBeatmap::Parse(stm);
								if (bmp.Mode != OsuStatic::GameMode::Mania)
									continue;
								if (bmp.CircleSize < 1)
									continue;
								if (bmp.HitObjects.size() == 0)
									continue;
								cache.artist = bmp.Artist;
								cache.title = bmp.Title;
								cache.titleunicode = bmp.TitleUnicode;
								cache.artistunicode = bmp.ArtistUnicode;
								cache.tags = bmp.Tags;
								cache.source = bmp.Source;
								DifficultyCacheEntry dce{};
								dce.audio = diff.path().parent_path().append(bmp.AudioFilename).string();
								dce.background = diff.path().parent_path().append(bmp.Background).string();
								double first = bmp.HitObjects[0].StartTime;
								for (auto ho : bmp.HitObjects)
								{
									dce.length = std::max(ho.StartTime, dce.length);
								}
								dce.length -= first;
								dce.keys = bmp.CircleSize;
								dce.od = bmp.OverallDifficulty;
								dce.name = bmp.Version;
								dce.path = diff.path().string();
								dce.nps = bmp.HitObjects.size() / dce.length * 1000;
								dce.preview = bmp.PreviewTime;
								cache.difficulties.emplace_back(dce);
							}
						}
					}
					std::sort(cache.difficulties.begin(), cache.difficulties.end(), [](const auto& a, const auto& b) {return a.nps < b.nps; });
					if (!cache.difficulties.empty())
						screa.Songs.push_back(cache);
				}
			}
			pthis->parent->Raise("songs_cache_ready", screa);
			return;
		}
		pthis->parent->Raise("require", "songs_path");
	}
	const char* songs_path;
	std::string songs_path_storage;
	// Í¨¹ý Component ¼Ì³Ð
	virtual void ProcessEvent(const char* evt, const void* evtargs)
	{
		if (strcmp(evt, "set") == 0)
		{
			auto sea = *(SetEventArgs*)evtargs;
			if (strcmp(sea.Key, "songs_path") == 0)
			{
				if (sea.Value.has_value() && sea.Value.type() == typeid(std::string))
				{
					auto path = std::any_cast<std::string>(sea.Value);
					if (std::filesystem::exists(path))
					{
						songs_path_storage = path;
						songs_path = songs_path_storage.c_str();
					}
				}
			}
		}
		if (strcmp(evt, "resolve_beatmap") == 0)
		{
			std::thread(Resolve, this).detach();
		}
		if (strcmp(evt, "get_songs_cache") == 0)
		{
			std::thread(GetSongsCache, this).detach();
		}
	}
};