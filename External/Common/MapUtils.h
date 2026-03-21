#pragma once
//==================================
// **** 클라/서버 공동 참조 파일 ****
//==================================

std::string   GetVariantFileName(EModelVariant variant);

EModelVariant PickRandomVariant(const std::string& key);