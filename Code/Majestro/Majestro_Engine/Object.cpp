#include "pch.h"
#include "Object.h"


Object::Object(OBJECT_TYPE type) : mObjectType(type)
{
	static uint32 idGenerator = 0;
	mId = idGenerator;
	idGenerator++;
}

Object::~Object()
{

}