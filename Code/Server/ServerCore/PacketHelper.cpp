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
	case PKT_Type::S2C_PKT_DESPAWN:
	case PKT_Type::S2C_PKT_STATE:
	case PKT_Type::S2C_PKT_COLLISION:
	case PKT_Type::S2C_PKT_BULLET_ACTIVATE:
	case PKT_Type::S2C_PKT_BULLET_DEACTIVATE:
	case PKT_Type::S2C_PKT_EFFECT_SPAWN:
	case PKT_Type::S2C_PKT_BOSS_TILE:
	case PKT_Type::S2C_PKT_HEALTH:
	case PKT_Type::S2C_PKT_ARMOR:
	case PKT_Type::S2C_PKT_AMMO:
	case PKT_Type::S2C_PKT_PLAYER_STATUS:
	case PKT_Type::S2C_PKT_COOLDOWN:
	case PKT_Type::S2C_PKT_GIMMICK_STATE:
	case PKT_Type::S2C_PKT_RHYTHM_CHANGED:
	case PKT_Type::S2C_PKT_BEAT_JUDGEMENT:
	case PKT_Type::S2C_PKT_COMBO_CHANGED:
	case PKT_Type::S2C_PKT_STICKER:
	case PKT_Type::S2C_PKT_EMOTE:
	case PKT_Type::S2C_PKT_CHAT:
	case PKT_Type::S2C_PKT_HIT_CONFIRM:
	case PKT_Type::S2C_PKT_SCENE_STATE:
	case PKT_Type::S2C_PKT_SCENE_PREPARE:
	case PKT_Type::S2C_PKT_SCENE_CONQUEST:
	case PKT_Type::S2C_PKT_SCENE_ESCORT:
	case PKT_Type::S2C_PKT_SCENE_CLEAR:
	case PKT_Type::S2C_PKT_SCORE_BOARD:
	case PKT_Type::S2C_ROOM_STATE:
	case PKT_Type::S2C_ROOM_ERROR:
	case PKT_Type::S2C_ROOM_LIST:
	case PKT_Type::S2C_ROOM_JOIN_RESULT:
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

	// 패킷 사이즈 검증
	if (header.Size < sizeof(PacketHeader) || header.Size > MAX_PACKET_SIZE)
		return false;

	switch (header.PacketType) {
	case PKT_Type::PKT_TCP:
	case PKT_Type::PKT_LOGIN:
	case PKT_Type::C2S_SCENE_CHANGE:
	case PKT_Type::C2S_PKT_ACTION:
	case PKT_Type::C2S_PKT_RHYTHM_CHANGED:
	case PKT_Type::C2S_PKT_SYNC:
	case PKT_Type::C2S_PKT_STICKER:
	case PKT_Type::C2S_PKT_EMOTE:
	case PKT_Type::C2S_PKT_CHAT:
	case PKT_Type::C2S_PKT_INTRO_DONE:
	case PKT_Type::C2S_ROOM_READY:
	case PKT_Type::C2S_ROOM_CHARACTER_SELECT:
	case PKT_Type::C2S_ROOM_CREATE:
	case PKT_Type::C2S_ROOM_JOIN:
	case PKT_Type::C2S_ROOM_LIST:
	case PKT_Type::C2S_ROOM_LEAVE: {
		ProcessTcpPackets(inputCommand, buffer , header.Size);
		break;
	}
	case PKT_Type::PKT_UDP:
	case PKT_Type::C2S_PKT_MOVE:
	case PKT_Type::C2S_PKT_LOGIN:
	case PKT_Type::C2S_GAME_START: {
		ProcessUdpPackets(inputCommand, buffer, header.Size);
		break;
	}
	default:
		MJLOG_EVERY(Session, Warn,
			"unknown-packet:" + std::to_string(static_cast<uint32>(header.PacketType)), 5.0,
			"알 수 없는 패킷 타입 {}", static_cast<uint32>(header.PacketType));
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
