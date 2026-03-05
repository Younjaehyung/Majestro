#include "pch.h"
#include "PacketHelper.h"
#include "SendBuffer.h"

/// <summary>
///	SerializePacket
/// </summary>


bool SendRequestPacket::SerializePacket(SendRequest& pkt, SendBuffer* sendBuffer) {

	switch (pkt.Type) {
	//TCP
	case PKT_Type::PKT_TCP:
	case PKT_Type::PKT_LOGIN:
	case PKT_Type::PKT_SERVER:
	case PKT_Type::C2S_PKT_LOGIN:
	case PKT_Type::C2S_PKT_ACTION:
	case PKT_Type::C2S_SCENE_CHANGE:
		SerializeTcpPacket(pkt, sendBuffer);
		break;
	

	// UDP
	case PKT_Type::PKT_UDP:
	case PKT_Type::C2S_GAME_START:
	case PKT_Type::C2S_PKT_INPUT:
		SerializeUdpPacket(pkt, sendBuffer);
		break;
	default :
		return false;
	}


	return true;
}

void SendRequestPacket::SerializeTcpPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{
	sendBuffer->SetData(pkt.MsgBuffer.data(), pkt.SIze, TCP);
}

void SendRequestPacket::SerializeUdpPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{
	sendBuffer->SetData(pkt.MsgBuffer.data(), pkt.SIze, UDP);
}




/// <summary>
///	ProcessPackets
/// </summary>


bool ProcessPacket::ProcessPackets(InputCommand& inputCommand, BYTE* buffer)
{
	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));

	inputCommand.Type = header.PacketType;
	inputCommand.Size = header.Size;

	switch (header.PacketType) {
	// TCP
	case PKT_Type::PKT_TCP:
	case PKT_Type::PKT_LOGIN: 
	case PKT_Type::S2C_PKT_SYNC:
	case PKT_Type::S2C_PKT_LOGIN:
	case PKT_Type::S2C_GAME_START:
	case PKT_Type::S2C_SCENE_CHANGE_RESULT:
	case PKT_Type::S2C_PKT_SPAWN:
	case PKT_Type::S2C_PKT_STATE:
	case PKT_Type::S2C_PKT_COLLISION:
	case PKT_Type::S2C_PKT_BULLET_ACTIVATE:
	case PKT_Type::S2C_PKT_HEALTH:
	{
		ProcessTcpPackets(inputCommand, buffer);
		break;
	}

	// UDP
	case PKT_Type::PKT_UDP:
	case PKT_Type::S2C_PKT_MOVE: {
		ProcessUdpPackets(inputCommand, buffer);
		break;
	}
	default:
		//LOG_ERROR("Unknown Packet Type: {}", static_cast<uint32>(header.PacketType));
		return false;
	}
	return true;
}

void ProcessPacket::ProcessTcpPackets(InputCommand& inputCommand, BYTE* buffer)
{
	inputCommand.Kind = MsgKind::KNONE;
	::memcpy(&inputCommand.MsgBuffer, buffer, inputCommand.Size);
}

void ProcessPacket::ProcessUdpPackets(InputCommand& inputCommand, BYTE* buffer)
{
	inputCommand.Kind = MsgKind::KNONE;
	::memcpy(&inputCommand.MsgBuffer, buffer, inputCommand.Size);
}