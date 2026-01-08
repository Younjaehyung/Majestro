#include "pch.h"
#include "PacketHelper.h"
#include "SendBuffer.h"


bool SendRequestPacket::SerializePacket(SendRequest& pkt, SendBuffer* sendBuffer) {

	switch (pkt.Type) {
	case PKT_Type::KSYNC: {
		SerializeSyncPacket(pkt, sendBuffer);
		break;
	}
	default :
		return false;
	}


	return true;
}

void SendRequestPacket::SerializeSyncPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{

	// Copy header
	sendBuffer->SetData(&pkt.sync, sizeof(SyncPacketData),UDP);

	
}

bool ProcessPacket::ProcessPackets(InputCommand& inputCommand, BYTE* buffer)
{
	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));

	switch (header.PacketType) {
	case PKT_Type::KSYNC: {
		ProcessSyncPacket(inputCommand, buffer);
		break;
	}
	default:
		LOG_ERROR("Unknown Packet Type: {}", static_cast<uint32>(header.PacketType));
		return false;
	}
	return true;
}

void ProcessPacket::ProcessSyncPacket(InputCommand& inputCommand, BYTE* buffer)
{
	SyncPacketData syncPacket;
	::memcpy(&syncPacket, buffer, sizeof(SyncPacketData));

	inputCommand.SessionId = syncPacket.clientId;
	inputCommand.moveX = syncPacket.rhythmTime;
	std::cout << "Processed SyncPacket for Client ID: " << syncPacket.clientId << " with Rhythm Time: " << syncPacket.rhythmTime << std::endl;
}
