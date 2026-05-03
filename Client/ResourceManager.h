#pragma once

// ImGui���� ����ϴ� ��� �ؽ�ó�� �� ������ �ε��Ѵ�.
// ������ �Լ��� �и��Ͽ� �����Ѵ�.

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
    // �� ���� �� �� �� ȣ��. ��� ���� �ؽ�ó�� ���ε��Ѵ�.
    void LoadAll(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);

private:
    void LoadTitleSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
    void LoadGameSceneTextures(ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
    void LoadSounds();
};
