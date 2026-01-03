#include "pch.h"
#include "PacketHelper.h"

ProcessPacket::ProcessPacket()
{
}

void ProcessPacket::Process(BYTE* buffer, int32 len)
{
	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));

	BYTE* payload = buffer + sizeof(PacketHeader);
	int32 payloadSize = header.Size - sizeof(PacketHeader);

	switch (header.PacketType)
	{
	case PKT_Type::KSYNC:
		//ProcessSyncPacket(buffer, len);
		break;
	case PKT_Type::KINPUT:
		//ProcessInputPacket(buffer, len);
		break;
	case PKT_Type::KACTION:
		//ProcessActionPacket(buffer, len);
		break;
	case PKT_Type::KPOSITION:
		//ProcessPositionPacket(buffer, len);
		break;
	case PKT_Type::KMSG:
		//ProcessMsgPacket(buffer, len);
		break;
	default:
		LOG_ERROR("Unknown Packet Type: {}", static_cast<uint32>(header.PacketType));
		break;
	}

}

InputCommand* ProcessPacket::PopCommand()
{
	std::lock_guard<std::mutex> lock(mPopMutex);
	if (!mCommandQueue.empty())
	{
		InputCommand* cmd = mCommandQueue.front();
		mCommandQueue.pop();
		return cmd;
	}
}
