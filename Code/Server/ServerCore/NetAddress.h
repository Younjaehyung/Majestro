#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
/*--------------
	NetAddress
---------------*/

class NetAddress
{
public:
	NetAddress() = default;
	NetAddress(sockaddr_in sockAddr);
	NetAddress(std::wstring ip, uint16 port);

	sockaddr_in&	GetSockAddr() { return mSockAddr; }
	std::wstring	GetIpAddress();
	std::string		GetIpAddressA();
	uint16			GetPort() { return ::ntohs(mSockAddr.sin_port); }

public:
	static IN_ADDR	Ip2Address(const WCHAR* ip);

private:
	sockaddr_in		mSockAddr = {};
};

