#include "pch.h"
#include "PacketHelper.h"

void SendRequestPacket::SerializeSyncPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{

	// Copy header
	std::memcpy(&pkt.sync, sendBuffer->Data, sizeof(SyncPacketData));

	// Set total size
	sendBuffer->Capacity = sizeof(SyncPacketData);
}
