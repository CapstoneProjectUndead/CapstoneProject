#pragma once

#include <list>
#include "SoundManager.h"

class CSound
{
public:
	CSound() = default;
	~CSound();

	void Load(const std::string& path);

	// loopCount: 1 = 1회, 0 = 무한반복 / overlap: 중복 재생 허용
	int  Play(int loopCount, float volume = 1.f, bool overlap = false);
	void Stop();
	void SetVolume(float volume, int channelIdx);

private:
	void RemoveChannel(FMOD::Channel* pChannel);

	friend FMOD_RESULT CHANNEL_CALLBACK(FMOD_CHANNELCONTROL* channelcontrol, FMOD_CHANNELCONTROL_TYPE controltype
		, FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype
		, void* commanddata1, void* commanddata2);

private:
	FMOD::Sound*              fmod_sound   = nullptr;
	std::list<FMOD::Channel*> channel_list;
};
