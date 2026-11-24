#include "pch.h"
#include "McpBridge.h"
#include "EngineLog.h"

#include <cwchar>

namespace
{
	constexpr unsigned short kDefaultPort = 9300;
	constexpr size_t kMaxRequestBytes = 64 * 1024;
	constexpr long long kMaxLogBytes = 256 * 1024;

	unsigned short PortFromArgs()
	{
		const wchar_t* p = ::wcsstr(::GetCommandLineW(), L"-mcp=");
		if (p == nullptr)
			return kDefaultPort;
		const long value = ::wcstol(p + 5, nullptr, 10);
		return (value > 0 && value < 65536) ? static_cast<unsigned short>(value) : kDefaultPort;
	}

	json LogDomains()
	{
		json list = json::array();
		for (int i = 0; i < static_cast<int>(EngineLog::Domain::Count); ++i)
		{
			const auto domain = static_cast<EngineLog::Domain>(i);
			list.push_back({ { "name", EngineLog::Name(domain) }, { "on", EngineLog::Enabled(domain) } });
		}
		return list;
	}

	// engine_log.txt 를 since 바이트부터 최대 maxBytes 만큼 읽는다. 줄 중간에서 끊지 않는다.
	json ReadLogs(long long since, long long maxBytes)
	{
		const std::wstring& path = EngineLog::Detail::LogFilePath();
		if (path.empty())
			return { { "ok", false }, { "error", "log file disabled (-logfile=)" } };

		FILE* file = ::_wfsopen(path.c_str(), L"rb", _SH_DENYNO);
		if (file == nullptr)
			return { { "ok", false }, { "error", "cannot open log file" } };

		::_fseeki64(file, 0, SEEK_END);
		const long long size = ::_ftelli64(file);
		if (since < 0 || since > size)
			since = 0;	// 파일이 새로 써졌으면(게임 재시작) 처음부터
		const long long count = (std::min)(size - since, (std::max)(0LL, (std::min)(maxBytes, kMaxLogBytes)));

		std::string text(static_cast<size_t>(count), '\0');
		::_fseeki64(file, since, SEEK_SET);
		text.resize(::fread(text.data(), 1, text.size(), file));
		::fclose(file);

		// 다 읽지 못했으면 마지막 줄바꿈까지만 보낸다 (다음 호출이 그 줄부터 이어 읽는다).
		if (since + static_cast<long long>(text.size()) < size)
		{
			const size_t lastNewline = text.rfind('\n');
			if (lastNewline != std::string::npos)
				text.resize(lastNewline + 1);
		}

		const long long next = since + static_cast<long long>(text.size());
		return { { "ok", true }, { "text", text }, { "next", next }, { "size", size }, { "more", next < size } };
	}
}

bool McpBridge::Requested()
{
	// -mcp 와 -mcp=포트 모두 받는다.
	static const bool sRequested = [] {
		const wchar_t* p = ::wcsstr(::GetCommandLineW(), L"-mcp");
		return p != nullptr && (p[4] == L'\0' || p[4] == L' ' || p[4] == L'=' || p[4] == L'"');
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

	const unsigned short port = PortFromArgs();
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
		EngineLog::Write(EngineLog::Domain::NetworkDiagnostic,
			"[MCP] listen failed port=", port, " code=", ::WSAGetLastError());
		if (mListen != INVALID_SOCKET)
			::closesocket(mListen);
		mListen = INVALID_SOCKET;
		return;
	}
	EngineLog::Write(EngineLog::Domain::NetworkDiagnostic, "[MCP] listening 127.0.0.1:", port);
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
			if (mPending.size() > kMaxRequestBytes)
			{
				CloseClient();
				return;
			}
			continue;
		}
		if (received == 0 || ::WSAGetLastError() != WSAEWOULDBLOCK)
		{
			CloseClient();	// 상대가 끊었거나 오류
			return;
		}
		break;	// 지금 읽을 것 없음
	}

	size_t newline;
	while ((newline = mPending.find('\n')) != std::string::npos)
	{
		const std::string line = mPending.substr(0, newline);
		mPending.erase(0, newline + 1);

		// 응답은 작고 로컬이라 다 보낼 때까지 보낸다. 보내다 실패하면 접속을 닫는다.
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
		reply["id"] = request.value("id", json());
		const std::string cmd = request.value("cmd", "");

		if (cmd == "ping")
		{
			reply["ok"] = true;
			reply["pid"] = ::GetCurrentProcessId();
		}
		else if (cmd == "log_domains")
		{
			reply["ok"] = true;
			reply["domains"] = LogDomains();
		}
		else if (cmd == "set_log")
		{
			const std::string domain = request.value("domain", "");
			const bool on = request.value("on", true);
			reply["ok"] = EngineLog::SetEnabled(domain, on);
			if (!reply["ok"])
				reply["error"] = "unknown domain: " + domain;
		}
		else if (cmd == "get_logs")
		{
			json logs = ReadLogs(request.value("since", 0LL), request.value("max_bytes", 64LL * 1024));
			logs["id"] = reply["id"];
			reply = std::move(logs);
		}
		else
		{
			reply["ok"] = false;
			reply["error"] = "unknown cmd: " + cmd;
		}
	}
	catch (const std::exception& e)
	{
		reply["ok"] = false;
		reply["error"] = std::string("bad request: ") + e.what();
	}
	// 로그에는 CP949 와 UTF-8 이 섞여 있다. 깨진 바이트는 치환해서 JSON 을 항상 만든다.
	return reply.dump(-1, ' ', false, json::error_handler_t::replace);
}
