#pragma once
#include <stack>
#include <queue>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include "../../Protocol/Packet.h"
//////////////////*
// Single Producer Single Consumer Ring Queue
// LOGIC THREAD <-> NETWORK THREAD
////////////////*/

struct SendBuffer;



struct InputCommand // Packet received (network thread -> logic thread)
{
    PKT_Type Type;
    uint32 SessionId;
	MsgKind Kind;
    float moveX;
    float moveY;
    bool  action1;
    bool  action2;
};

struct SendRequest { // Packet to be sent (logic thread -> network thread)

    uint32 SessionId{};
    PKT_Type Type{};

    union
    {
		PacketTcpHeader tcpHeader;
		PacketUdpHeader udpHeader;
        
		LoginPacket login;
		ServerPacket server;
        C2S_InputPacket input;
        
		S2C_MovePacket move;
        S2C_SyncPacket sync{};
    };

    SendRequest() :Type(PKT_Type::KNONE) {}
    SendRequest(PKT_Type t) : Type(t) {}
    SendRequest(PKT_Type t, const S2C_SyncPacket& s) : Type(t), sync(s) {}
    SendRequest(PKT_Type t, const PacketTcpHeader& th) : Type(t), tcpHeader(th) {}
    SendRequest(PKT_Type t, const PacketUdpHeader& uh) : Type(t), udpHeader(uh) {}
};

class SendRequestPacket
{
public:
    static bool SerializePacket(SendRequest& pkt, SendBuffer*);
    static void SerializeTcpPacket(SendRequest& pkt, SendBuffer*);
    static void SerializeUdpPacket(SendRequest& pkt, SendBuffer*);


    static void SerializeSyncPacket(SendRequest& pkt, SendBuffer*);
    static void SerializeInputPacket(SendRequest& pkt, SendBuffer*);
    static void SerializeActionPacket(SendRequest& pkt, SendBuffer*){}
};


class ProcessPacket // Process received packets (network thread -> logic thread)
{
private:
public:
	static bool ProcessPackets(InputCommand& inputCommand, BYTE* buffer);
	static void ProcessTcpPackets(InputCommand& inputCommand, BYTE* buffer);
	static void ProcessUdpPackets(InputCommand& inputCommand, BYTE* buffer);
	static void ProcessLoginPacket(InputCommand& inputCommand, BYTE* buffer);
    static void ProcessSyncPacket(InputCommand& inputCommand, BYTE* buffer);
	static void ProcessRespawnPacket(InputCommand& inputCommand, BYTE* buffer);

    static void ProcessPosPacket(InputCommand& inputCommand, BYTE* buffer) {}
    static void ProcessInputPacket(InputCommand& inputCommand, BYTE* buffer) {};
    static void ProcessActionPacket(InputCommand& inputCommand, BYTE* buffer) {};
};



