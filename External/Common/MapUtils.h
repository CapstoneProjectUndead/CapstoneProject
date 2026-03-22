#pragma once
//==================================
// **** 클라/서버 공동 참조 파일 ****
//==================================

#include "MapGenerator/MapGenerator.h"

std::vector<std::string> GameSceneTypeToString(const MapGenerator::EModelType& type);

std::string   GetVariantFileName(EModelVariant variant);

EModelVariant PickRandomVariant(const std::string& key);