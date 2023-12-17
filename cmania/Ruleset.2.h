#pragma once
#include <memory>
#include <string>
#include <filesystem>
#include <typeinfo>
#include "Stopwatch.h"
#include "InputHandler.h"
#include "GameBuffer.h"
#include "ScoreProcessor.h"
#include "Record.h"
#include "HitObject.h"

// Abstract base class that defines the interface for gameplay mechanics
class GameplayBase {
protected:
	Stopwatch Clock;								   // Presumably a timer to keep track of the game's progress
	std::unique_ptr<InputHandler> RulesetInputHandler; // Input handler for the game
	bool GameEnded;									   // Flag to indicate if the game has ended
	bool GameStarted;								   // Flag to indicate if the game has started

public:
	GameplayBase() = default;  // Default constructor
	virtual ~GameplayBase() {} // Virtual destructor for proper cleanup in derived classes

	// Pure virtual methods that must be implemented in derived classes
	virtual void Update() = 0;								   // Called every frame to update game logic
	virtual void Render(GameBuffer&) = 0;					   // Called every frame to render the game
	virtual void Skip() = 0;								   // Method to skip parts of the gameplay (e.g., intros)
	virtual double GetCurrentTime() const = 0;				   // Get the current time in the game
	virtual double GetDuration() const = 0;					   // Get the duration of the game
	virtual ScoreProcessorBase* GetScoreProcessor() const = 0; // Get the score processor
	virtual std::string GetBgPath() const = 0;				   // Get the path to the background image
	virtual Record GetAutoplayRecord() = 0;					   // Get the autoplay record
};

// Struct to hold beatmap metadata
class BeatmapMetadata {
public:
	std::string Artist;		// The artist of the song
	std::string Title;		// The title of the song
	std::string Mapper;		// The creator of the beatmap
	std::string Difficulty; // The difficulty level of the beatmap
};

class DifficultyCalculator {

};

// Abstract base class that defines the interface for beatmaps
class BeatmapBase {
public:
	virtual ~BeatmapBase() {} // Virtual destructor for proper cleanup in derived classes

	// Pure virtual methods that must be implemented in derived classes
	virtual const HitObject& At(size_t i) const = 0;		  // Get a hit object at a certain index
	virtual size_t Size() const = 0;						  // Get the number of hit objects
	virtual BeatmapMetadata Metadata() const = 0;			  // Get the beatmap's metadata
	virtual std::filesystem::path AudioPath() const = 0;	  // Get the path to the audio file
	virtual std::filesystem::path BackgroundPath() const = 0; // Get the path to the background image
	virtual std::type_info GetHitobjectType() const = 0;	  // Get the type info of the hit objects
};

// Abstract base class that defines the interface for rulesets
class RulesetBase {
public:
	virtual ~RulesetBase() {} // Virtual destructor for proper cleanup in derived classes

	// Pure virtual methods that must be implemented in derived classes
	virtual void LoadSettings(BinaryStorage& settings) = 0;							   // Load settings from a binary storage
	virtual std::unique_ptr<BeatmapBase> Load(std::filesystem::path beatmap_path) = 0; // Load a beatmap from a path
	virtual std::unique_ptr<GameplayBase> CreateGameplay() = 0;						   // Create a gameplay instance
};