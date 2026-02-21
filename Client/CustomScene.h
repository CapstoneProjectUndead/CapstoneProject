#pragma once
#include "Scene.h"

class CCustomScene : public CScene
{
public:
	void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;

};

