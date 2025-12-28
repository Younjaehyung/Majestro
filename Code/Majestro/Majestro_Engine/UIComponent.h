#pragma once
#include "Entity.h"
#include "Component.h"

class Mesh;
class Material;


class UIComponent : public Component<UIComponent>
{
public:
	UIComponent() {}

public:
	//Anchor mAnchor = Anchor::TopLeft;


    Vec2 position;     // anchor 기준 오프셋 (pixel)
    Vec2 size;         // pixel
    Vec2 pivot;        // (0~1)
	Vec2 finalPixelPos;   // 최종 화면 픽셀 좌표



	uint8 mUILayerIndex = 0;
	bool mUIVisibility = true;
	vector<shared_ptr<Material>> mMaterials;
};

