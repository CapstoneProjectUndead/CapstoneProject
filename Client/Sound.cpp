#include "stdafx.h"
#include "Sound.h"
#include "SoundManager.h"

CSound::~CSound()
{
	if (fmod_sound) {
		fmod_sound->release();
		fmod_sound = nullptr;
	}
}

void CSound::Load(const std::string& path)
{
	FMOD::System* system = CSoundManager::GetInstance().GetFMODSystem();
	assert(system);
	FMOD_RESULT result = system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &fmod_sound);
	assert(result == FMOD_OK);
}

int CSound::Play(int loopCount, float volume, bool overlap)
{
	if (!fmod_sound)
		return -1;

	if (!overlap && !channel_list.empty())
		return -1;

	FMOD::Channel* pChannel = nullptr;
	CSoundManager::GetInstance().GetFMODSystem()->playSound(fmod_sound, nullptr, false, &pChannel);

	if (!pChannel)
		return -1;

	pChannel->setVolume(volume);
	pChannel->setCallback(CHANNEL_CALLBACK);
	pChannel->setUserData(this);
	pChannel->setMode(FMOD_LOOP_NORMAL);
	pChannel->setLoopCount(loopCount - 1); // 1->0(1회), 0->-1(무한)

	channel_list.push_back(pChannel);

	int idx = -1;
	pChannel->getIndex(&idx);
	return idx;
}

void CSound::Stop()
{
	while (!channel_list.empty()) {
		auto iter = channel_list.begin();
		(*iter)->stop(); // stop() 호출 시 콜백이 동기적으로 발생해 RemoveChannel 처리
	}
}

void CSound::SetVolume(float volume, int channelIdx)
{
	for (auto* pChannel : channel_list) {
		int idx = -1;
		pChannel->getIndex(&idx);

		if (idx == channelIdx) {
			pChannel->setVolume(volume);
			return;
		}
	}
}

void CSound::RemoveChannel(FMOD::Channel* pChannel)
{
	channel_list.remove(pChannel);
}

// =========
// Call Back
// =========
FMOD_RESULT CHANNEL_CALLBACK(FMOD_CHANNELCONTROL* channelcontrol, FMOD_CHANNELCONTROL_TYPE controltype
	, FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype
	, void* commanddata1, void* commanddata2)
{
	FMOD::Channel* cppchannel = (FMOD::Channel*)channelcontrol;
	CSound* pSound = nullptr;

	switch (controltype)
	{
	case FMOD_CHANNELCONTROL_CALLBACK_END:
	{
		cppchannel->getUserData((void**)&pSound);
		pSound->RemoveChannel(cppchannel);
	}
	break;
	}

	return FMOD_OK;
}