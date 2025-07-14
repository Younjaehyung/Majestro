#include "pch.h"
#include "Object.h"


Object::Object(OBJECT_TYPE type) : mObjectType(type)
{
	static uint32 idGenerator = 1;
	mId = idGenerator;
	idGenerator++;
}

Object::~Object()
{

}