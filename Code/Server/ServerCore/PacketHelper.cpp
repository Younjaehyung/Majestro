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
	case PKT_Type::S2C_PKT_LOGIN:{
		SerializeTcpPacket(pkt, sendBuffer);
		break;
	}
	case PKT_Type::PKT_UDP: {
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
	
	

	switch (header.PacketType) {
	case PKT_Type::PKT_TCP:
	case PKT_Type::PKT_LOGIN:
	case PKT_Type::C2S_PKT_ACTION:
	case PKT_Type::C2S_PKT_LOGIN:{
		ProcessTcpPackets(inputCommand, buffer , header.Size);
		break;
	}
	case PKT_Type::PKT_UDP:
	case PKT_Type::C2S_PKT_INPUT: {
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
