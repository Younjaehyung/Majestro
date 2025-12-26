#include "pch.h"
#include "PacketHelper.h"



PacketBlock* PacketPool::Acquire()
{
    return nullptr;
}

void PacketPool::Release(PacketBlock* block)
{
}




PacketBlock* PacketHelper::Allocate(uint16 size)
{
    if (size <= 64)   return pool64.Acquire();
    if (size <= 128)  return pool128.Acquire();
    if (size <= 256)  return pool256.Acquire();
    if (size <= 512)  return pool512.Acquire();
    return pool1024.Acquire();
}

void PacketHelper::Free(PacketBlock* block)
{
    switch (block->capacity)
    {
    case 64:   pool64.Release(block); break;
    case 128:  pool128.Release(block); break;
    }
}
