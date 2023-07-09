export module AudioService;
import Game;
import "BassAudioManager.h";
import <map>;
import Settings;
import <functional>;
import <fstream>;
import File;

export struct LoadCompleteEventArgs
{
	union
	{
		IAudioManager::IAudioStream* stream;
		IAudioManager::ISample* sample;
	};
};
export struct LoadEventArgs
{
	union
	{
		const char* requested_path;
		const void* requested_file;
	};
	size_t file_length;
	bool sample;
	bool memory;
	std::function<void(const LoadCompleteEventArgs&)>* callback;
};
export struct EnumDevicesCallbackEventArgs
{
	IAudioManager::IAudioDevice* dev;
};
export struct EnumDevicesEventArgs
{
	std::function<void(const EnumDevicesCallbackEventArgs&)>* callback;
};
// 针对 AudioManager 的简单服务形式(托管 AudioManager )
export class AudioService : public GameComponent
{
	IAudioManager* bam = 0;
public:
	// 通过 Component 继承
	virtual void ProcessEvent(const char* evt, const void* evtargs) override
	{
		if (strcmp(evt, "start") == 0)
		{
			if (!bam)
			{
				bam = CreateBassAudioManager();
				bam->openDevice(AudioManagerExtensions::getDefaultDevice(bam));
			}
		}
		if (strcmp(evt, "set") == 0)
		{
			auto sea = *(SetEventArgs*)evtargs;
			if (strcmp(sea.Key, "device") == 0)
			{
				if (sea.Value.has_value() && sea.Value.type() == typeid(int))
				{
					if (bam)
						delete bam;
					bam = CreateBassAudioManager();
					bam->openDevice(bam->getAudioDevices().at(std::any_cast<int>(sea.Value)));
				}
			}
		}
		if (strcmp(evt, "enum_devices") == 0)
		{
			auto edea = *(EnumDevicesEventArgs*)evtargs;
			if (edea.callback != 0)
			{
				for (auto dev : bam->getAudioDevices())
				{
					EnumDevicesCallbackEventArgs edcea{};
					edcea.dev = dev;
					(*edea.callback)(edcea);
				}
			}
		}
		if (strcmp(evt, "load") == 0)
		{
			auto lea = *(LoadEventArgs*)evtargs;
			IAudioManager::IAudioStream* as = 0;
			IAudioManager::ISample* smp = 0;
			if (lea.sample)
			{
				if (lea.memory)
				{
					smp = bam->loadSample(lea.requested_file,lea.file_length);
				}
			}
			else
			{
				if (lea.memory)
				{
					as = bam->load(lea.requested_file, lea.file_length);
				}
				else
				{
					auto buffer = ReadAllBytes(lea.requested_path);
					as = bam->load(buffer.data(), buffer.size());
				}
			}
			LoadCompleteEventArgs lcea{};
			if (smp != 0)
				lcea.sample = smp;
			if (as != 0)
				lcea.stream = as;
			if (lea.callback != 0)
				(*lea.callback)(lcea);
		}
	}

};