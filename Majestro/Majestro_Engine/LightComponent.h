#pragma once


enum class LIGHT_TYPE : uint8
{
	DIRECTIONAL_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT,
};

struct LightColor	//빛의 3개 속성
{
	Vec4	diffuse;
	Vec4	ambient;
	Vec4	specular;
};

struct LightInfo	//빛과 관련된 정보
{
	LightColor	color;
	Vec4		position;	//DIRECTIONAL_LIGHT은 사실상 필요 없음
	Vec4		direction;	//POINT_LIGHT은 사실상 필요 없음
	int32		LightType;	//LIGHT_TYPE
	float		range;
	float		angle;
	int32		padding;	//데이터 사이즈용 padding
};

struct LightParams
{
	uint32		lightCount;
	Vec3		padding;	//데이터 사이즈용 padding
	LightInfo	lights[50];


};


class LightComponent
{
public:

		bool GetCheckFrustum() { return _checkFrustum; }

public:


	bool _static = true; //정적물체인지 동적 물체인지 확인

	LightInfo mLightInfo = {};

	int8 _lightIndex = -1;
	shared_ptr<class Mesh> _volumeMesh;
	shared_ptr<class Material> _lightMaterial;


};

