#include "pch.h"
#include "PacketHelper.h"
#include "SendBuffer.h"

void SendRequestPacket::SerializeSyncPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{

	// Copy header
	sendBuffer->SetData(&pkt.sync, sizeof(SyncPacketData));

	
}

void ProcessPacket::ProcessPackets(BYTE* buffer)
{
	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));


}

void ProcessPacket::ProcessSyncPacket(BYTE* buffer, int32 len)
{
	SyncPacketData syncPacket;
	::memcpy(&syncPacket, buffer, sizeof(SyncPacketData));

	std::cout << "Processed SyncPacket for Client ID: " << syncPacket.clientId << " with Rhythm Time: " << syncPacket.rhythmTime << std::endl;
}
