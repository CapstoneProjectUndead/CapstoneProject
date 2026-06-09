#pragma once

// ImGui에서 사용하는 모든 텍스처를 한 곳에서 로드한다.
// 씬별로 함수를 분리하여 관리한다.

class CResourceManager
{
private:
    CResourceManager() = default;
    CResourceManager(const CResourceManager&) = delete;

public:
    ~CResourceManager() = default;

    static CResourceManager& GetInstance() {
        static CResourceManager instance;
        return instance;
    }

public:
    // 클라이언트 실행 시 한 번 호출. 모든 씬의 텍스처를 업로드한다.
    void LoadAll(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);

public:
    const char* PlayerImageKey(PLAYER_IMAGE img)
    {
        switch (img)
        {
        case PLAYER_IMAGE::DOG:
            return "portrait_dog";
        case PLAYER_IMAGE::CAT:
            return "portrait_cat";
        case PLAYER_IMAGE::BUNNY:
            return "portrait_bunny";
        case PLAYER_IMAGE::DOG2:
            return "portrait_dog2";
        case PLAYER_IMAGE::CAT2:
            return "portrait_cat2";
        case PLAYER_IMAGE::BUNNY2:
            return "portrait_bunny2";
            default:
                return nullptr;
        }
    }

private:
    void LoadTitleSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
    void LoadCustomSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
    void LoadLobbySceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
    void LoadGameSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
    void LoadSounds();
};
