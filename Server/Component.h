#pragma once
// Server쪽 Component

class CObject;

class CComponent {
public:
	virtual void Initialize() {}
	virtual void Update(const float deltaTime) = 0;

	CObject* GetOwner() const { return owner; }

	CObject* owner{};	// 참조용
};
