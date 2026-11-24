#include "pch.h"
#include "McpBridge.h"
#include "McpState.h"

// Unity 빌드라 익명 namespace 가 다른 .cpp 와 합쳐진다. 이름이 겹치지 않게 따로 둔다.
namespace McpBridgeDetail
{
	constexpr unsigned short kDefaultPort = 9310;
	constexpr size_t kMaxRequestBytes = 64 * 1024;

	unsigned short PortFromArgs()
	{
		const wchar_t* p = ::wcsstr(::GetCommandLineW(), L"-mcp=");
		if (p == nullptr)
			return kDefaultPort;
		const long value = ::wcstol(p + 5, nullptr, 10);
		return (value > 0 && value < 65536) ? static_cast<unsigned short>(value) : kDefaultPort;
	}
}

bool McpBridge::Requested()
{
	static const bool sRequested = [] {
		for (const wchar_t* p = ::wcsstr(::GetCommandLineW(), L"-mcp"); p; p = ::wcsstr(p + 1, L"-mcp"))
		{
			const wchar_t next = p[4];
			if (next == L'\0' || next == L' ' || next == L'\t' || next == L'=' || next == L'"')
				return true;
		}
		return false;
	}();
	return sRequested;
}

McpBridge& McpBridge::Get()
{
	static McpBridge sInstance;
	return sInstance;
}

McpBridge::McpBridge()
{
	WSADATA wsaData{};
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return;

	const unsigned short port = McpBridgeDetail::PortFromArgs();
	mListen = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = ::htons(port);
	address.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);	// 로컬 접속만

	u_long nonBlocking = 1;
	if (mListen == INVALID_SOCKET
		|| ::bind(mListen, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR
		|| ::listen(mListen, 1) == SOCKET_ERROR
		|| ::ioctlsocket(mListen, FIONBIO, &nonBlocking) == SOCKET_ERROR)
	{
		MJLOG_ERROR(Startup, "[MCP] listen 실패 port={} code={}", port, ::WSAGetLastError());
		if (mListen != INVALID_SOCKET)
			::closesocket(mListen);
		mListen = INVALID_SOCKET;
		return;
	}
	MJLOG_INFO(Startup, "[MCP] listening 127.0.0.1:{}", port);
}

void McpBridge::CloseClient()
{
	if (mClient != INVALID_SOCKET)
		::closesocket(mClient);
	mClient = INVALID_SOCKET;
	mPending.clear();
}

void McpBridge::Poll()
{
	if (mListen == INVALID_SOCKET)
		return;

	// ponytail: 한 번에 한 접속만 받는다. MCP 서버는 호출마다 붙었다 끊으므로 충분하다.
	if (mClient == INVALID_SOCKET)
	{
		mClient = ::accept(mListen, nullptr, nullptr);
		if (mClient == INVALID_SOCKET)
			return;
		u_long nonBlocking = 1;
		::ioctlsocket(mClient, FIONBIO, &nonBlocking);
	}

	char buffer[4096];
	for (;;)
	{
		const int received = ::recv(mClient, buffer, sizeof(buffer), 0);
		if (received > 0)
		{
			mPending.append(buffer, static_cast<size_t>(received));
			if (mPending.size() > McpBridgeDetail::kMaxRequestBytes)
			{
				CloseClient();
				return;
			}
			continue;
		}
		if (received == 0 || ::WSAGetLastError() != WSAEWOULDBLOCK)
		{
			CloseClient();
			return;
		}
		break;
	}

	size_t newline;
	while ((newline = mPending.find('\n')) != std::string::npos)
	{
		const std::string line = mPending.substr(0, newline);
		mPending.erase(0, newline + 1);

		const std::string reply = Handle(line) + "\n";
		for (size_t sent = 0; sent < reply.size();)
		{
			const int n = ::send(mClient, reply.data() + sent, static_cast<int>(reply.size() - sent), 0);
			if (n > 0)
				sent += static_cast<size_t>(n);
			else if (n == SOCKET_ERROR && ::WSAGetLastError() == WSAEWOULDBLOCK)
				::Sleep(0);
			else
			{
				CloseClient();
				return;
			}
		}
	}
}

std::string McpBridge::Handle(const std::string& line)
{
	json reply;
	try
	{
		const json request = json::parse(line);
		const json id = request.value("id", json());
		const std::string cmd = request.value("cmd", "");

		if (cmd == "ping")
			reply = { { "ok", true }, { "pid", ::GetCurrentProcessId() } };
		else if (cmd == "rooms")
			reply = McpListRooms();
		else if (cmd == "query_state")
			reply = McpQueryRoomState(request);
		else if (cmd == "set_log")
		{
			const std::string domain = request.value("domain", "");
			const bool ok = ServerLog::SetEnabled(domain, request.value("on", true));
			reply = { { "ok", ok } };
			if (!ok)
				reply["error"] = "unknown domain: " + domain;
		}
		else
			reply = { { "ok", false }, { "error", "unknown cmd: " + cmd } };
		reply["id"] = id;
	}
	catch (const std::exception& e)
	{
		reply = { { "ok", false }, { "error", std::string("bad request: ") + e.what() } };
	}
	return reply.dump(-1, ' ', false, json::error_handler_t::replace);
}
