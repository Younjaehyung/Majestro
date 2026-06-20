#pragma once
#include "UIFeature.h"

class UIScoreBoardFeature : public UIFeature
{
public:
	void Initialize(World* world) override;
	void Update(float dt) override;

private:
	void EnsureTextEntity();
	void UpdateText();
	void SetVisible(bool visible);
	bool CanOpenScoreBoard() const;

private:
	Entity mTextEntity = NULL_ENTITY;
};
