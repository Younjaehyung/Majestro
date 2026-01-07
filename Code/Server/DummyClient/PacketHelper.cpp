#include "pch.h"
#include "PacketHelper.h"
#include "SendBuffer.h"


bool SendRequestPacket::SerializePacket(SendRequest& pkt, SendBuffer* sendBuffer)
{

    
    switch (pkt.Type)
    {
    case PKT_Type::KSYNC:
        SerializeSyncPacket(pkt, sendBuffer);
        break;
    case PKT_Type::KINPUT:
        SerializeInputPacket(pkt, sendBuffer);
        break;
    case PKT_Type::KACTION:
        SerializeActionPacket(pkt, sendBuffer);
        break;
    default:
        // Unknown packet type
        return false;
    }
    
    return true;

}

void SendRequestPacket::SerializeSyncPacket(SendRequest& pkt, SendBuffer* sendBuffer)
{

	// Copy header
	sendBuffer->SetData(&pkt.sync, sizeof(SyncPacketData),TCP);

	
}

bool ProcessPacket::ProcessPackets(InputCommand& inputCommand, BYTE* buffer)
{

    PacketTcpHeader header;
	::memcpy(&header, buffer, sizeof(PacketTcpHeader));


    switch (header.Header.PacketType) {
    case PKT_Type::KSYNC: {
        ProcessSyncPacket(inputCommand, buffer);
        std::cout << "Processed Sync Packet for Client ID: " << inputCommand.SessionId << " with Rhythm Time: " << inputCommand.moveX << std::endl;
        break;
    }
    case PKT_Type::KLOGIN: {
        ProcessLoginPacket(inputCommand, buffer);
        std::cout << "Processed Login Packet for Client ID: " << inputCommand.SessionId << std::endl;
        break;
	}
    default:
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
	//std::cout << "Processed SyncPacket for Client ID: " << inputCommand->SessionId << " with Rhythm Time: " << inputCommand->moveX << std::endl;
    // 여기서 inputCmd의 다른 필드를 설정할 수 있습니다.
    // 수신된 명령을 링 버퍼에 푸시
   
}

void ProcessPacket::ProcessLoginPacket(InputCommand& inputCommand, BYTE* buffer)
{
	KLoginPacket loginPacket;
	::memcpy(&loginPacket, buffer, sizeof(KLoginPacket));
	inputCommand.SessionId = loginPacket.clientId;
    
}
