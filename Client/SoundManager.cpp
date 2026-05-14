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

void CSoundManager::LoadSound(SOUND_ID key, const std::string& path)
{
	auto sound = std::make_unique<CSound>();
	sound->Load(path);
	sound_map.emplace(key, std::move(sound));
}

int CSoundManager::Play(SOUND_ID key, int loopCount, float volume, bool overlap)
{
	auto it = sound_map.find(key);
	if (it == sound_map.end())
		return -1;

	return it->second->Play(loopCount, volume * master_volume, overlap);
}

void CSoundManager::Stop(SOUND_ID key)
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

void CSoundManager::SetVolume(SOUND_ID key, float volume, int channelIdx)
{
	auto it = sound_map.find(key);
	if (it == sound_map.end())
		return;

	it->second->SetVolume(volume, channelIdx);
}
