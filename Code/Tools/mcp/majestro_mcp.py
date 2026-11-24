# /// script
# requires-python = ">=3.11"
# dependencies = ["mcp>=2.3"]
# ///
"""Majestro MCP 서버.

실행 중인 Majestro.exe(-mcp) 의 McpBridge(127.0.0.1:9300) 에 붙어 명령을 전달한다.
호출마다 접속 -> 요청 한 줄 -> 응답 한 줄 -> 종료. 게임이 꺼져 있으면 오류를 돌려준다.

등록:  claude mcp add majestro -- uv run <이 파일 경로>
포트:  환경변수 MAJESTRO_MCP_PORT (기본 9300). 게임은 -mcp=포트 로 맞춘다.
"""

import itertools
import json
import os
import socket

from mcp.server import MCPServer

HOST = "127.0.0.1"
PORT = int(os.environ.get("MAJESTRO_MCP_PORT", "9300"))
TIMEOUT = 10.0  # 게임 스레드가 프레임 시작에 처리한다. 로딩 중에는 몇 초 걸릴 수 있다.

mcp = MCPServer("majestro")
_ids = itertools.count(1)


def call(cmd: str, **args) -> dict:
    request = {"id": next(_ids), "cmd": cmd, **args}
    try:
        with socket.create_connection((HOST, PORT), timeout=TIMEOUT) as sock:
            sock.sendall((json.dumps(request) + "\n").encode())
            data = b""
            while not data.endswith(b"\n"):
                chunk = sock.recv(65536)
                if not chunk:
                    break
                data += chunk
    except OSError as e:
        return {"ok": False, "error": f"Majestro({HOST}:{PORT}) 에 연결 못함: {e}. 게임을 -mcp 로 실행했는지 확인"}
    if not data:
        return {"ok": False, "error": "응답 없음 (게임이 접속을 닫음)"}
    return json.loads(data)


@mcp.tool()
def ping() -> dict:
    """게임이 MCP 로 연결되는지 확인한다. 프로세스 id 를 돌려준다."""
    return call("ping")


@mcp.tool()
def log_domains() -> dict:
    """클라이언트 로그 도메인 목록과 각각 지금 켜져 있는지."""
    return call("log_domains")


@mcp.tool()
def set_log(domain: str, on: bool = True) -> dict:
    """로그 도메인을 실행 중에 켜거나 끈다 (리빌드 없음). 예: audio-runtime, network-runtime."""
    return call("set_log", domain=domain, on=on)


@mcp.tool()
def get_logs(since: int = 0, max_bytes: int = 65536) -> dict:
    """engine_log.txt 를 since 바이트부터 읽는다. 응답의 next 를 다음 호출의 since 로 넘기면 새 줄만 받는다.
    more 가 true 면 아직 남은 로그가 있다. max_bytes 상한은 256KB."""
    return call("get_logs", since=since, max_bytes=max_bytes)


@mcp.tool()
def query_state(enemies: int = 20) -> dict:
    """지금 게임 상태: 씬, 웨이브 페이즈, 점령/호위 진행도, 로컬 플레이어(위치 cm·hp·action·movement·flags·리듬 포인트),
    살아 있는 적 수와 가까운 순 목록(enemies 마리까지, 0 이면 개수만).
    action 은 Protocol ReplicatedActionState, movement 는 ReplicatedMovementMode, 적 type 은 EnemyComponent.h EnemyType 값."""
    return call("query_state", enemies=enemies)


if __name__ == "__main__":
    mcp.run()
