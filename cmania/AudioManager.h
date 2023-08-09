#pragma once
#include <istream>
#include <chrono>
#include <vector>
#include <exception>

class IAudioManager {
public:
	struct IChannel {
		virtual ~IChannel() = default;
		virtual int getId() const = 0;
		virtual double getVolume() const = 0;
		virtual void setVolume(double volume) = 0;
		virtual bool play() = 0;
		virtual bool stop() = 0;
		virtual bool pause(bool pause = true) = 0;
		virtual double getPlaybackRate() const = 0;
		virtual void setPlaybackRate(double rate) = 0;
		virtual double getCurrent() const = 0;
		virtual void setCurrent(double time) = 0;
		virtual bool isStopped() const = 0;
		virtual bool isPaused() const = 0;
		virtual bool isBuffering() const = 0;
		virtual bool isPlaying() const = 0;
		virtual double getDuration() const = 0;
	};

	struct IAudioStream : public IChannel {
		// No additional audio writing required
	};

	struct ISample {
		virtual ~ISample() = default;
		virtual int getId() const = 0;
		virtual IAudioStream* generateStream() = 0;
	};

	struct IAudioDevice {
		virtual ~IAudioDevice() = default;
		virtual std::string getName() const = 0;
		virtual std::string getType() const = 0;
		virtual int getId() const = 0;
		virtual bool isDefault() const = 0;
		virtual bool isEnabled() const = 0;
		virtual bool isInitiated() const = 0;
		virtual bool isLoopbackDevice() const = 0;
	};

	virtual ~IAudioManager() = default;
	virtual IAudioStream* load(std::istream& stream) = 0;
	virtual IAudioStream* load(const void* data, size_t size) = 0;
	virtual ISample* loadSample(const void* data, size_t size) = 0;
	virtual std::vector<IAudioDevice*> getAudioDevices() = 0;
	virtual IAudioDevice* getCurrent() = 0;
	virtual bool isDeviceOpened() const = 0;
	virtual void openDevice(IAudioDevice* device) = 0;
	virtual void close() = 0;
};
class AudioManagerExtensions {
public:
	static IAudioManager::IAudioDevice* getDefaultDevice(IAudioManager* am) {
		auto audioDevices = am->getAudioDevices();
		auto it = std::find_if(audioDevices.begin(), audioDevices.end(), [](const auto& device) {
			return device->isDefault();
		});
		return (it != audioDevices.end()) ? *it : nullptr;
	}
};