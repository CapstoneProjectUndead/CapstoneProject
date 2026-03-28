#include "stdafx.h"
#include "DataManager.h"
#include "UIComponent.h"

void CDataManager::LoadScripts(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    json j;
    file >> j;

    for (auto& [npcName, situations] : j.items()) {
        for (auto& [tag, lines] : situations.items()) {
            for (const auto& line : lines) {
                npc_scripts[npcName].dialogues[tag].push_back(Utf8ToWstring(line.get<std::string>()));
            }
        }
    }
}

std::wstring CDataManager::GetDialogue(const std::string& npcName, const std::string& tag)
{
    if (npc_scripts.count(npcName) && npc_scripts[npcName].dialogues.count(tag)) {
        auto& lines = npc_scripts[npcName].dialogues[tag];
        if (lines.empty()) return L"";
        return lines[rand() % lines.size()];
    }
    return L"Error: Script Not Found";
}

void CDataManager::SaveToFile(std::shared_ptr<CUICanvas>& editorCanvas, char savePath[256])
{
    if (!editorCanvas) return;

    // 전체 계층을 json 객체로 변환
    json rootData = editorCanvas->Serialize();

    // 파일 쓰기
    std::ofstream file(savePath);
    if (file.is_open()) {
        file << rootData.dump(4); // 4칸 들여쓰기로 보기 좋게 저장
        file.close();
    }
}

std::shared_ptr<CUICanvas> CDataManager::LoadFromFile(char savePath[256])
{
    std::ifstream file(savePath);
    if (!file.is_open()) return nullptr;

    json rootData;
    file >> rootData;
    file.close();
    std::shared_ptr<CUICanvas> canvas = std::make_shared<CUICanvas>();

    // 캔버스 자체 데이터 복구
    canvas->Deserialize(rootData);

    // 자식들 생성 (재귀 함수 호출)
    if (rootData.contains("Children")) {
        for (const auto& childData : rootData["Children"]) {
            LoadRecursive(canvas, childData);
        }
    }

    return canvas;
}

void CDataManager::LoadRecursive(std::shared_ptr<CUIComponent> parent, const json& data)
{
    std::string type = data["Type"];
    std::shared_ptr<CUIComponent> newUI = nullptr;

    if (type == "Image") newUI = std::make_shared<CUIImage>();
    else if (type == "Button") newUI = std::make_shared<CUIButton>();
    else if (type == "Text") newUI = std::make_shared<CUIText>();
    else newUI = std::make_shared<CUIComponent>();

    // 데이터 세팅
    newUI->Deserialize(data);

    // 계층 구조 연결
    parent->AddChild(newUI);

    if (data.contains("Children")) {
        for (const auto& childData : data["Children"]) {
            LoadRecursive(newUI, childData);
        }
    }
}

std::wstring CDataManager::Utf8ToWstring(const std::string& str)
{
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}