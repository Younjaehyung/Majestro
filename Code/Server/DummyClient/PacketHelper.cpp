#include "pch.h"
#include "PacketHelper.h"
#include "SendBuffer.h"


bool SendRequestPacket::SerializePacket(SendRequest& pkt, SendBuffer* sendBuffer) {

	switch (pkt.Type) {
	case PKT_Type::PKT_TCP: {
		SerializeTcpPacket(pkt, sendBuffer);
		break;
	}
	case PKT_Type::PKT_UDP: {
		SerializeUdpPacket(pkt, sendBuffer);
		break;
					   }
	case PKT_Type::S2C_PKT_SYNC: {
		SerializeSyncPacket(pkt, sendBuffer);
		break;
	}
	default :
		return false;
	}


	return true;
}

void SendRequestPacket::SerializeTcpPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{
	sendBuffer->SetData(&pkt.tcpHeader, sizeof(PacketTcpHeader), TCP);
}

void SendRequestPacket::SerializeUdpPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{
	sendBuffer->SetData(&pkt.udpHeader, sizeof(PacketUdpHeader), UDP);
}

void SendRequestPacket::SerializeSyncPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{
	// Copy header
	sendBuffer->SetData(&pkt.sync, sizeof(S2C_SyncPacket),UDP);
}

bool ProcessPacket::ProcessPackets(InputCommand& inputCommand, BYTE* buffer)
{
	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));

	switch (header.PacketType) {
	case PKT_Type::PKT_TCP: {
		ProcessTcpPackets(inputCommand, buffer);
		break;
	}
	 case PKT_Type::PKT_UDP: {
		ProcessUdpPackets(inputCommand, buffer);
		break;
	}
	case PKT_Type::S2C_PKT_SYNC: {
		ProcessSyncPacket(inputCommand, buffer);
		break;
	}
	case PKT_Type::PKT_LOGIN: {
		ProcessLoginPacket(inputCommand, buffer);
		// std::cout << "Processed Login Packet for Client ID: " << inputCommand.SessionId << std::endl;
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
	PacketTcpHeader tcpPacket;
	::memcpy(&tcpPacket, buffer, sizeof(PacketTcpHeader));
	// For demonstration, we assume the TCP packet contains SyncPacketData
	std::cout << "Processed TCP Packet of size: " << tcpPacket.Header.Size << std::endl;
}

void ProcessPacket::ProcessUdpPackets(InputCommand& inputCommand, BYTE* buffer)
{
	PacketUdpHeader udpPacket;
	::memcpy(&udpPacket, buffer, sizeof(PacketUdpHeader));
	// For demonstration, we assume the UDP packet contains SyncPacketData
	std::cout << "Processed UDP Packet for Session ID: " << udpPacket.SessionId << std::endl;
}

void ProcessPacket::ProcessSyncPacket(InputCommand& inputCommand, BYTE* buffer)
{
	S2C_SyncPacket syncPacket;
	::memcpy(&syncPacket, buffer, sizeof(S2C_SyncPacket));

	inputCommand.SessionId = syncPacket.clientId;
	inputCommand.moveX = syncPacket.rhythmTime;
	std::cout << "Processed SyncPacket for Client ID: " << syncPacket.clientId << " with Rhythm Time: " << syncPacket.rhythmTime << std::endl;
}

void ProcessPacket::ProcessLoginPacket(InputCommand& inputCommand, BYTE* buffer)
{
	LoginPacket loginPacket;
	::memcpy(&loginPacket, buffer, sizeof(LoginPacket));
	inputCommand.SessionId = loginPacket.clientId;
	inputCommand.Type = loginPacket.Header.PacketType;
	std::cout << "Processed Login Packet for Client ID: " << loginPacket.clientId << std::endl;
}