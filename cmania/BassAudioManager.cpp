#include "BassAudioManager.h"
#include "EnumFlag.h"
#include <bass.h>

enum class StreamSystem
{
	NoBuffer,
	Buffer,
	BufferPush
};
enum class PlaybackState
{
	// Token: 0x040001F0 RID: 496
	Stopped,
	// Token: 0x040001F1 RID: 497
	Playing,
	// Token: 0x040001F2 RID: 498
	Stalled,
	// Token: 0x040001F3 RID: 499
	Paused
};
inline float BASS_ChannelGetAttribute(DWORD handle, DWORD attrib)
{
	float res;
	BASS_ChannelGetAttribute(handle, attrib, &res);
	return res;
}
class BassAudioManager : public IAudioManager {
public:
	BassAudioManager() = default;
	virtual ~BassAudioManager() {
		close();
	}

	IAudioManager::IAudioDevice* getCurrent() override {
		return (deviceOpened) ? createBassDevice(BASS_GetDevice()) : nullptr;
	}

	bool isDeviceOpened() const override {
		return deviceOpened;
	}

	void openDevice(IAudioManager::IAudioDevice* device) override {
		if (!BASS_Init(device->getId(), 44100, 0, 0, 0))
			throw BassException(BASS_ErrorGetCode());

		deviceOpened = true;
	}

	void close() override {
		if (deviceOpened) {
			if (!BASS_Free())
				throw BassException(BASS_ErrorGetCode());

			deviceOpened = false;
		}
	}

	IAudioManager::IAudioStream* load(std::istream& fileStream) {
		int sid;
		auto fileproc = createFileProcedures();
		if ((sid = BASS_StreamCreateFileUser((DWORD)StreamSystem::NoBuffer, 0, &fileproc, &fileStream)) == 0)
			throw BassException(BASS_ErrorGetCode());
		return new BassAudioStream(sid,0);
	}

	IAudioManager::IAudioStream* load(const void* data, size_t size) override {
		auto block = malloc(size);
		std::copy((const char*)data, (const char*)data + size, (char*)block);
		DWORD sid = BASS_StreamCreateFile(true, block, 0, size , 0);
		if (sid == 0)
			throw BassException(BASS_ErrorGetCode());

		return new BassAudioStream(sid, block);
	}

	IAudioManager::ISample* loadSample(const void* data, size_t size) override {
		auto block = malloc(size);
		std::copy((const char*)data, (const char*)data + size, (char*)block);
		int sid = BASS_SampleLoad(true, block, 0, size, 10240, 0);
		if (sid == 0)
			throw BassException(BASS_ErrorGetCode());

		return new BassAudioSample(sid,block);
	}

	std::vector<IAudioManager::IAudioDevice*> getAudioDevices() override {
		std::vector<IAudioManager::IAudioDevice*> devices;
		IAudioManager::IAudioDevice* dvc;
		int i = 0;
		while ((dvc = createBassDevice(i++)) != 0)
			devices.push_back(dvc);
		return devices;
	}

private:
	class BassException : public std::exception {
	public:
		explicit BassException(DWORD err) : error(err) {
		
		}

		const char* what() const noexcept override {
			return "Bass returned an error.";
		}

	private:
		DWORD error;
	};

	class BassAudioDevice : public IAudioManager::IAudioDevice {
	public:
		std::string getName() const override {
			return name;
		}

		std::string getType() const override {
			return "NotImpl";
		}

		int getId() const override {
			return id;
		}

		bool isDefault() const override {
			return isDefaultDevice;
		}

		bool isEnabled() const override {
			return isEnabledDevice;
		}

		bool isInitiated() const override {
			return isInitiatedDevice;
		}

		bool isLoopbackDevice() const override {
			return isLoopback;
		}

	private:
		BassAudioDevice(const char* Name, DWORD Type, int Id, bool def, bool ena, bool init, bool loop)
		{
			name = Name;
			type = Type;
			id = Id;
			isDefaultDevice = def;
			isEnabledDevice = ena;
			isInitiatedDevice = init;
			isLoopback = loop;
		}

		const char* name;
		DWORD type;
		int id;
		bool isDefaultDevice;
		bool isEnabledDevice;
		bool isInitiatedDevice;
		bool isLoopback;
		friend BassAudioManager;
	};

	class BassAudioStream : public IAudioManager::IAudioStream {
	public:
		BassAudioStream(int sid,void* ptr) : id(sid),ptr(ptr) {}

		int getId() const override {
			return id;
		}

		double getVolume() const override {
			return BASS_ChannelGetAttribute(id, 2);
		}

		void setVolume(double volume) override {
			if (!BASS_ChannelSetAttribute(id, 2, volume))
				throw BassException(BASS_ErrorGetCode());
		}

		bool play() override {
			return BASS_ChannelPlay(id, true);
		}

		bool stop() override {
			return BASS_ChannelStop(id);
		}

		bool pause(bool pause = true) override {
			return (pause) ? BASS_ChannelPause(id) : BASS_ChannelPlay(id, true);
		}

		double getPlaybackRate() const override {
			return BASS_ChannelGetAttribute(id, 1) / getInfo().freq;
		}

		void setPlaybackRate(double rate) override {
			if (!BASS_ChannelSetAttribute(id, 1, rate * getInfo().freq))
				throw BassException(BASS_ErrorGetCode());
		}

		double getCurrent() const override {
			return BASS_ChannelBytes2Seconds(id, BASS_ChannelGetPosition(id, 0));
		}

		void setCurrent(double time) override {
			if (!BASS_ChannelSetPosition(id, BASS_ChannelSeconds2Bytes(id, time), 0))
				throw BassException(BASS_ErrorGetCode());
		}

		bool isStopped() const override {
			return BASS_ChannelIsActive(id) == (DWORD)PlaybackState::Stopped;
		}

		bool isPaused() const override {
			return BASS_ChannelIsActive(id) == (DWORD)PlaybackState::Paused;
		}

		bool isBuffering() const override {
			return BASS_ChannelIsActive(id) == (DWORD)PlaybackState::Stalled;
		}

		bool isPlaying() const override {
			return BASS_ChannelIsActive(id) == (DWORD)PlaybackState::Playing;
		}

		double getDuration() const override {
			return BASS_ChannelBytes2Seconds(id, BASS_ChannelGetLength(id, 0));
		}

		~BassAudioStream() override
		{
			if (id != 0)
			{
				BASS_StreamFree(id);
				id = 0;
			}
			if (ptr != 0)
			{
				free(ptr);
				ptr = 0;
			}
		}

	private:
		BASS_CHANNELINFO getInfo() const {
			BASS_CHANNELINFO info;
			if (!BASS_ChannelGetInfo(id, &info))
				throw BassException(BASS_ErrorGetCode());
			return info;
		}

		int id;
		void* ptr;
	};

	class BassAudioSample : public IAudioManager::ISample {
	public:
		BassAudioSample(int sid,void* block) : id(sid),block(block) {}

		~BassAudioSample() override{
			if (id != 0)
			{
				BASS_SampleFree(id);
				id = 0;
			}
			if (block != 0)
			{
				free(block);
				block = 0;
			}
		}

		int getId() const override {
			return id;
		}

		IAudioManager::IAudioStream* generateStream() override {
			int channel = BASS_SampleGetChannel(id, false);
			if (channel == 0)
				throw BassException(BASS_ErrorGetCode());
			return new BassAudioStream(channel,0);
		}

	private:
		int id = 0;
		void* block = 0;
	};

	static BassAudioDevice* createBassDevice(int i) {
		BASS_DEVICEINFO info;
		if (BASS_GetDeviceInfo(i, &info)) {
			return new BassAudioDevice{
				info.name,
				info.flags & 0xFF000000,
				i,
				HasFlag(info.flags,2),
				HasFlag(info.flags,1),
				HasFlag(info.flags,4),
				HasFlag(info.flags,8),
			};
		}
		else {
			return 0;
		}
	}

	static BASS_FILEPROCS createFileProcedures() {
		return BASS_FILEPROCS{
			[](void* user) {
				auto fileStreamPtr = static_cast<std::istream*>(user);
				delete fileStreamPtr;
			},
			[](void* user) {
				auto fileStreamPtr = static_cast<std::istream*>(user);
				auto pos = fileStreamPtr->tellg();
				fileStreamPtr->seekg(0, std::ios::end);
				auto length = fileStreamPtr->tellg();
				fileStreamPtr->seekg(pos,std::ios::beg);
				return (uint64_t)length;
			},
			[](void* buffer, DWORD length, void* user) {
				auto fileStreamPtr = static_cast<std::istream*>(user);
				fileStreamPtr->read(static_cast<char*>(buffer), length);
				return static_cast<DWORD>(fileStreamPtr->gcount());
			},
			[](QWORD seek,void* user) {
				auto fileStreamPtr = static_cast<std::istream*>(user);
				fileStreamPtr->seekg(seek, std::ios::beg);
				return (BOOL)fileStreamPtr->good();
			}
		};
	}

	bool deviceOpened = false;
};
IAudioManager* CreateBassAudioManager()
{
	return new BassAudioManager();
}
