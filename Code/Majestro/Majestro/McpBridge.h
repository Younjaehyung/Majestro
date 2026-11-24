#pragma once
#include <string>

// 실행 인자 -mcp 또는 -mcp=포트 로 켜는 MCP 명령 통로 (기본 127.0.0.1:9300, 로컬 접속만 받는다).
//   요청과 응답은 한 줄에 JSON 하나다.
//     {"id":1,"cmd":"ping"}  ->  {"id":1,"ok":true,"pid":1234}
//   게임 스레드(Game::Update)에서 매 프레임 Poll 하므로 명령 처리에 잠금이 필요 없다.
//   MCP 서버(Tools/mcp/majestro_mcp.py)가 이 포트에 붙는다.
//
//   명령:  ping
//          log_domains                       도메인 이름과 현재 켜짐 여부
//          set_log   {domain, on}            EngineLog::SetEnabled
//          get_logs  {since, max_bytes}      engine_log.txt 의 since 바이트부터. 응답의 next 를 다음 since 로 쓴다
//          query_state {enemies}             씬·페이즈·플레이어·목표·적 상태 (McpState.h)
//          press {keys, seconds, mouse_left, mouse_right, mouse_dx, mouse_dy}   (McpInput.h, -mcpinput 또는 -qa 필요)
//          move_to {pos:[x,y,z], arrive_cm, timeout}
//          input_status / input_cancel
//
//   엔진은 MCP 를 모른다. 배선은 실행 파일(Game.cpp)이 한다.
class McpBridge
{
public:
	static bool Requested();
	// -mcpinput: MCP 가 입력 소스를 차지한다 (하드웨어 키보드·마우스 입력은 꺼진다).
	static bool InputRequested();
	static McpBridge& Get();

	// 접속 수락, 수신, 명령 처리, 응답까지 한 번에. 매 프레임 1회 호출한다.
	void Poll();

private:
	McpBridge();
	void CloseClient();
	std::string Handle(const std::string& line);

	SOCKET mListen = INVALID_SOCKET;
	SOCKET mClient = INVALID_SOCKET;
	std::string mPending;
};
