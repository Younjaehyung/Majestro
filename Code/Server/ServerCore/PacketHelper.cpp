#include "pch.h"
#include "PacketHelper.h"
#include "SendBuffer.h"


/// <summary>
///	SerializePacket
/// </summary>

bool SendRequestPacket::SerializePacket(SendRequest& pkt, SendBuffer* sendBuffer) {

	switch (pkt.Type) {
	case PKT_Type::PKT_TCP:
	case PKT_Type::PKT_LOGIN:
	case PKT_Type::S2C_PKT_SYNC:
	case PKT_Type::S2C_PKT_LOGIN:
	case PKT_Type::S2C_GAME_START:
	case PKT_Type::S2C_SCENE_CHANGE_RESULT:
	case PKT_Type::S2C_PKT_SPAWN:
	case PKT_Type::S2C_PKT_SPAWNS:
	case PKT_Type::S2C_PKT_STATE:
	case PKT_Type::S2C_PKT_COLLISION:
	{
		SerializeTcpPacket(pkt, sendBuffer);
		break;
	}
	case PKT_Type::PKT_UDP:
	case PKT_Type::S2C_PKT_MOVE:
	{
		SerializeUdpPacket(pkt, sendBuffer);
		break;
	}
	
	default :
		return false;
	}


	return true;
}

void SendRequestPacket::SerializeTcpPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{
	sendBuffer->SetData(pkt.MsgBuffer.data(), pkt.Size, TCP);
}

void SendRequestPacket::SerializeUdpPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{
	sendBuffer->SetData(pkt.MsgBuffer.data(), pkt.Size, UDP);
}




/// <summary>
///	ProcessPackets
/// </summary>


bool ProcessPacket::ProcessPackets(InputCommand& inputCommand, BYTE* buffer)
{
	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));
	
	inputCommand.Type = header.PacketType;
	inputCommand.SIze = header.Size;
	//inputCommand.Kind = MsgKind::KNONE;

	switch (header.PacketType) {
	case PKT_Type::PKT_TCP:
	case PKT_Type::PKT_LOGIN:
	case PKT_Type::C2S_SCENE_CHANGE:
	case PKT_Type::C2S_PKT_ACTION:{
		ProcessTcpPackets(inputCommand, buffer , header.Size);
		break;
	}
	case PKT_Type::PKT_UDP:
	case PKT_Type::C2S_PKT_INPUT:
	case PKT_Type::C2S_PKT_LOGIN:
	case PKT_Type::C2S_GAME_START: {
		ProcessUdpPackets(inputCommand, buffer, header.Size);
		break;
	}
	default:
		LOG_ERROR("Unknown Packet Type: {}", static_cast<uint32>(header.PacketType));
		return false;
	}
	return true;
}

void ProcessPacket::ProcessTcpPackets(InputCommand& inputCommand, BYTE* buffer, uint32 size)
{
	::memcpy(&inputCommand.MsgBuffer, buffer, size);
}

void ProcessPacket::ProcessUdpPackets(InputCommand& inputCommand, BYTE* buffer, uint32 size)
{
	
	::memcpy(&inputCommand.MsgBuffer, buffer, size);
}
