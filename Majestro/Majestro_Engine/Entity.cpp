#include "pch.h"
#include "Entity.h"

Entity::Entity()
{
	static uint32 idEntityGenerator = 1;
	mID = idEntityGenerator;
	idEntityGenerator++;
}
