#pragma once
#include <string>

// 실행 인자 -mcp 또는 -mcp=포트 로 켜는 서버 MCP 명령 통로 (기본 127.0.0.1:9310, 로컬 접속만).
//   클라 McpBridge 와 같은 규약: 한 줄에 JSON 하나. {"id":1,"cmd":"ping"} -> {"id":1,"ok":true,...}
//   로직 스레드(main 루프, gGameCore->Update 직전)에서 매 틱 Poll 하므로 World 를 잠금 없이 읽는다.
//
//   명령:  ping
//          rooms                         게임 World 가 있는 방 목록 (roomId, scene, phase, players)
//          query_state {room, enemies}   그 방 World 의 서버 상태 (McpState.h). room 생략 시 첫 방
//          set_log {domain, on}          ServerLog::SetEnabled
//
//   배선은 GameServer.cpp main. 게임 로직(GameCore)은 MCP 를 모른다.
//   ponytail: 소켓 처리부는 클라 McpBridge 와 같은 코드를 복사해 둔 것이다 (클라·서버가 공유하는 빌드 단위가 Protocol 뿐).
//             한쪽 규약을 바꾸면 반대쪽도 같이 바꾼다.
class McpBridge
{
public:
	static bool Requested();
	static McpBridge& Get();

	void Poll();

private:
	McpBridge();
	void CloseClient();
	std::string Handle(const std::string& line);

	SOCKET mListen = INVALID_SOCKET;
	SOCKET mClient = INVALID_SOCKET;
	std::string mPending;
};
