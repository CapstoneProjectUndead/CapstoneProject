#include "stdafx.h"
#include "SoundManager.h"
#include "Sound.h"

CSoundManager::~CSoundManager()
{
	sound_map.clear();

	if (fmod_system)
	{
		fmod_system->release();
		fmod_system = nullptr;
	}
}

void CSoundManager::Init()
{
	FMOD::System_Create(&fmod_system);
	assert(fmod_system);
	fmod_system->init(32, FMOD_DEFAULT, nullptr);
}

void CSoundManager::Tick()
{
	fmod_system->update();
}

void CSoundManager::LoadSound(const std::string& key, const std::string& path)
{
	auto sound = std::make_unique<CSound>();
	sound->Load(path);
	sound_map.emplace(key, std::move(sound));
}

int CSoundManager::Play(const std::string& key, int loopCount, float volume, bool overlap)
{
	auto it = sound_map.find(key);
	if (it == sound_map.end())
		return -1;
	return it->second->Play(loopCount, volume, overlap);
}

void CSoundManager::Stop(const std::string& key)
{
	auto it = sound_map.find(key);
	if (it == sound_map.end())
		return;
	it->second->Stop();
}

void CSoundManager::StopAll()
{
	for (auto& [key, sound] : sound_map)
		sound->Stop();
}

void CSoundManager::SetVolume(const std::string& key, float volume, int channelIdx)
{
	auto it = sound_map.find(key);
	if (it == sound_map.end())
		return;
	it->second->SetVolume(volume, channelIdx);
}
