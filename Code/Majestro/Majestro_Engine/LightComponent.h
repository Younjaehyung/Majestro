#pragma once
#include "Component.h"
class World;

enum class LIGHT_TYPE : uint8
{
	DIRECTIONAL_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT,
};

struct LightColor	//빛의 3개 속성
{
	Vec4	Diffuse;
	Vec4	Ambient;
	Vec4	Specular;
};

struct LightParams	//빛과 관련된 정보
{
	LightColor	Color;
	Vec4		Position;	//DIRECTIONAL_LIGHT은 사실상 필요 없음
	Vec4		Direction;	//POINT_LIGHT은 사실상 필요 없음
	int32		LightType;	//LIGHT_TYPE
	float		Range;
	float		Angle;
	int32		Padding;	//데이터 사이즈용 padding

	Matrix MatWorld;
	Matrix MatView;
	Matrix MatProjection;
	Matrix MatViewInv;
	Matrix MatProjectionInv;

};

struct LightInfo	//빛과 관련된 정보
{
	LightColor	Color;
	Vec4		Position;	//DIRECTIONAL_LIGHT은 사실상 필요 없음
	Vec4		Direction;	//POINT_LIGHT은 사실상 필요 없음
	int32		LightType;	//LIGHT_TYPE
	float		Range;
	float		Angle;

};

class LightFactory
{
public:
	static Entity CreateLight(World* world, LIGHT_TYPE type, class LightComponent& l);
};


class LightComponent : public Component<LightComponent>
{
public:
	void SetLightIndex(int8 index) { mLightIndex = index; }
	LIGHT_TYPE GetLightType() { return static_cast<LIGHT_TYPE>(mLightInfo.LightType); }


	void SetLightDirection(Vec3 direction);	//빛의 방향과 Transform의 방향의 동기화를 위한 함수

	void SetDiffuse(const Vec3& diffuse) { mLightInfo.Color.Diffuse = diffuse; }
	void SetAmbient(const Vec3& ambient) { mLightInfo.Color.Ambient = ambient; }
	void SetSpecular(const Vec3& specular) { mLightInfo.Color.Specular = specular; }

	//void SetLightType(LIGHT_TYPE type);
	void SetLightRange(float range) { mLightInfo.Range = range; }
	void SetLightAngle(float angle) { mLightInfo.Angle = angle; }


	

public:


	bool mStaticLight = true; //정적물체인지 동적 물체인지 확인

	LightInfo mLightInfo = {};

	int8 mLightIndex = -1;
	//shared_ptr<class Mesh> mVolumeMesh;
	//shared_ptr<class Material> mLightMaterial;


};

