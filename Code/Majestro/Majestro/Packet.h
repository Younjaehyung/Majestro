#pragma once
#include "pch.h"
#pragma pack(push, 1)

enum Type : uint32 {
	KSERVER,
	KINPUT,
	KACTION,
	KMSG,
};


struct PacketDataInfo {
	uint32 Size;
	Type PacketType;
};






#pragma pack(pop)


