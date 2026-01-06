#include "pch.h"
#include "PacketHelper.h"
#include "SendBuffer.h"

void SendRequestPacket::SerializeSyncPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{

	// Copy header
	sendBuffer->SetData(&pkt.sync, sizeof(SyncPacketData));

	
}
