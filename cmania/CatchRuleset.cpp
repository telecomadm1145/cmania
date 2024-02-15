#include "Defines.h"
#include "CatchRuleset.h"
class CatchRuleset : public Ruleset {
	// Í¨¹ý Ruleset ¼Ì³Ð
	virtual void Init(BinaryStorage& settings) {
	}
	virtual std::string Id() {
		return "osucatch";
	}
	virtual std::string DisplayName() {
		return "Catch";
	}
	virtual Beatmap* LoadBeatmap(path beatmap_path, bool load_samples = true) {
		return nullptr;
	}
	virtual GameplayBase* GenerateGameplay() {
		return nullptr;
	}
	virtual double CalculateDifficulty(Beatmap* bmp, OsuMods mods) {
		return 0.0;
	}
	virtual DifficultyInfo PopulateDifficultyInfo(Beatmap* bmp) {
		return DifficultyInfo();
	}
};

Ruleset* MakeCatchRuleset() {
	return new CatchRuleset();
}
