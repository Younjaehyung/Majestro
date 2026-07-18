#pragma once
#include <array>
#include <stack>
#include <queue>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include "Protocol/Packet.h"
//////////////////*
// Single Producer Single Consumer Ring Queue
// LOGIC THREAD <-> NETWORK THREAD
////////////////*/

struct SendBuffer;


// 클라에서는 inputCommand는 S2C 패킷만 담음
struct InputCommand // Packet received (network thread -> logic thread)
{
    PKT_Type Type = PKT_Type::KNONE;
	MsgKind Kind = MsgKind::KNONE;
	uint32  Size{};
	std::array<uint8, MAX_PACKET_SIZE> MsgBuffer{}; // 메시지 버퍼

    void Clear()
    {
        Type = PKT_Type::KNONE;
        Kind = MsgKind::KNONE;
        //SessionId = 0;
        Size = 0;
    }

    template<typename T>
    bool StoreAs(const T& src)
    {
        // [추가] memcpy 저장은 T가 trivially copyable 이어야 안전
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

        if (sizeof(T) > MAX_PACKET_SIZE) return false;
        std::memcpy(MsgBuffer.data(), &src, sizeof(T));
        Size = static_cast<uint16_t>(sizeof(T));
        return true;
    }

    template<typename T>
    const T* ViewAs() const
    {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        if (Size != sizeof(T)) return nullptr;
        return reinterpret_cast<const T*>(MsgBuffer.data());
    }
};


// 클라에서는 sendRequest는 C2S 패킷만 담음
struct SendRequest { // Packet to be sent (logic thread -> network thread)

    //uint32 SessionId{};

    PKT_Type Type = PKT_Type::KNONE;
    uint32  SIze{};
    std::array<uint8, MAX_PACKET_SIZE> MsgBuffer{}; // 메시지 버퍼

    void Clear()
    {
        Type = PKT_Type::KNONE;
        //SessionId = 0;
        SIze = 0;
    }

    template<typename T>
    bool StoreAs(const T& src)
    {
        // [추가] memcpy 저장은 T가 trivially copyable 이어야 안전
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

        if (sizeof(T) > MAX_PACKET_SIZE) return false;
        std::memcpy(MsgBuffer.data(), &src, sizeof(T));
        SIze = static_cast<uint16_t>(sizeof(T));
        return true;
    }

    template<typename T>
    const T* ViewAs() const
    {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        if (SIze != sizeof(T)) return nullptr;
        return reinterpret_cast<const T*>(MsgBuffer.data());
    }

};

class SendRequestPacket
{
public:
    static bool SerializePacket(SendRequest& pkt, SendBuffer*);
    static void SerializeTcpPacket(SendRequest& pkt, SendBuffer*);
    static void SerializeUdpPacket(SendRequest& pkt, SendBuffer*);


    //static void SerializeSyncPacket(SendRequest& pkt, SendBuffer*);
    static void SerializeActionPacket(SendRequest& pkt, SendBuffer*){}
};


class ProcessPacket // Process received packets (network thread -> logic thread)
{
private:
public:
	static bool ProcessPackets(InputCommand& inputCommand, BYTE* buffer);
	static void ProcessTcpPackets(InputCommand& inputCommand, BYTE* buffer);
	static void ProcessUdpPackets(InputCommand& inputCommand, BYTE* buffer);


    static void ProcessPosPacket(InputCommand& inputCommand, BYTE* buffer) {}
    static void ProcessInputPacket(InputCommand& inputCommand, BYTE* buffer) {};
    static void ProcessActionPacket(InputCommand& inputCommand, BYTE* buffer) {};
};



