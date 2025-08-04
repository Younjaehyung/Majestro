#pragma once
#include "Object.h"

class Shader;
class Texture;

enum
{
	MATERIAL_INT_COUNT = 4,
	MATERIAL_FLOAT_COUNT = 4,
	MATERIAL_TEXTURE_COUNT = 4,
	MATERIAL_VECTOR2_COUNT = 4,
	MATERIAL_VECTOR4_COUNT = 4,
	MATERIAL_MATRIX_COUNT = 4
};

struct MaterialParams
{
	void SetInt(uint8 index, int32 value) { IntParams[index] = value; }
	void SetFloat(uint8 index, float value) { FloatParams[index] = value; }
	void SetTexOn(uint8 index, int32 value) { TexOnParams[index] = value; }
	void SetVec2(uint8 index, Vec2 value) { Vec2Params[index] = value; }
	void SetVec4(uint8 index, Vec4 value) { Vec4Params[index] = value; }
	void SetMatrix(uint8 index, Matrix& value) { MatrixParams[index] = value; }

	array<int32, MATERIAL_INT_COUNT> IntParams;
	array<float, MATERIAL_FLOAT_COUNT> FloatParams;
	array<int32, MATERIAL_TEXTURE_COUNT> TexOnParams;
	array<Vec2, MATERIAL_VECTOR2_COUNT> Vec2Params;
	array<Vec4, MATERIAL_VECTOR4_COUNT> Vec4Params;
	array<Matrix, MATERIAL_MATRIX_COUNT> MatrixParams;
};

class Material : public Object
{

public:
	Material();
	virtual ~Material();

	shared_ptr<Shader> GetShader() { return mShader; }
	wstring& GetShaderID() { return mShaderID; }


	void SetShader(std::wstring name);

	void SetInt(uint8 index, int32 value) { mParams.SetInt(index, value); }
	void SetFloat(uint8 index, float value) { mParams.SetFloat(index, value); }
	void SetTexture(uint8 index, shared_ptr<Texture> texture)
	{
		mTextures[index] = texture;
		mParams.SetTexOn(index, (texture == nullptr ? 0 : 1));
	}

	void SetVec2(uint8 index, Vec2 value) { mParams.SetVec2(index, value); }
	void SetVec4(uint8 index, Vec4 value) { mParams.SetVec4(index, value); }
	void SetMatrix(uint8 index, Matrix& value) { mParams.SetMatrix(index, value); }


	void PushComputeData();
	void Dispatch(uint32 x, uint32 y, uint32 z);

	shared_ptr<Material> Clone();


	MaterialParams& GetParams() { return mParams; }
private:
	wstring mShaderID;

	shared_ptr<Shader>	mShader;	//쉐이더 지울 예정
	MaterialParams		mParams;	//머테리얼 parm
	array<shared_ptr<Texture>, MATERIAL_TEXTURE_COUNT> mTextures;	//텍스쳐들
};


