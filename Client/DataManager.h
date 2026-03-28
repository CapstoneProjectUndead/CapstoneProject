#pragma once

class CUICanvas;
class CUIComponent;

struct DialogueLine {
    std::wstring text;
    std::string next_tag;
    bool has_options = false;
};

struct NPCScript {
    std::map<std::string, std::vector<std::wstring>> dialogues;
};

// 오브젝트의 data, script(대사)를 load 및 관리
class CDataManager {
public:
    // JSON 파일 로드 (인자로 파일 경로 전달)
    void LoadScripts(const std::string& filePath);

    // 대사 리스트 중 하나를 랜덤으로 반환
    std::wstring GetDialogue(const std::string& npcName, const std::string& tag);

    void SaveToFile(std::shared_ptr<CUICanvas>& editorCanvas, char savePath[256]);
    std::shared_ptr<CUICanvas> LoadFromFile(char savePath[256]);
    // 역직렬화 헬퍼 함수: 타입에 맞는 객체를 생성하고 자식을 연결함
    void LoadRecursive(std::shared_ptr<CUIComponent> parent, const json& data);
private:
    // UTF-8(JSON) -> Wstring 변환 헬퍼
    std::wstring Utf8ToWstring(const std::string& str);
private:
    std::map<std::string, NPCScript> npc_scripts;
};

