#pragma once

#include <string>
#include "Sound.h"

#include <FMOD/fmod.h>
#include <FMOD/fmod.hpp>
#include <FMOD/fmod_codec.h>

#ifdef _DEBUG
#pragma comment(lib, "FMOD/fmodL_vc.lib")
#else
#pragma comment(lib, "FMOD/fmod_vc.lib")
#endif

class CSound;

class CSoundManager
{
private:
	CSoundManager() = default;
	CSoundManager(const CSoundManager&) = delete;

public:
	~CSoundManager();

	static CSoundManager& GetInstance()
	{
		static CSoundManager instance;
		return instance;
	}

public:
	void Init();
	void Tick();

	// loopCount: 1 = 1회, 0 = 무한반복
	void LoadSound(const std::string& key, const std::string& path);
	int  Play(const std::string& key, int loopCount = 1, float volume = 1.f, bool overlap = false);
	void Stop(const std::string& key);
	void StopAll();
	void SetVolume(const std::string& key, float volume, int channelIdx);

	FMOD::System* GetFMODSystem() { return fmod_system; }

private:
	FMOD::System*                                        fmod_system = nullptr;
	std::unordered_map<std::string, std::unique_ptr<CSound>> sound_map;
};
