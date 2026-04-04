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
    // 앱 시작 시 한 번 호출. 모든 씬의 텍스처를 업로드한다.
    void LoadAll(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);

private:
    void LoadTitleSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
    void LoadGameSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
};
