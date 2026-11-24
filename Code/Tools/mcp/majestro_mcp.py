# /// script
# requires-python = ">=3.11"
# dependencies = ["mcp>=2.3"]
# ///
"""Majestro MCP 서버.

실행 중인 Majestro.exe(-mcp) 의 McpBridge(127.0.0.1:9300) 에 붙어 명령을 전달한다.
호출마다 접속 -> 요청 한 줄 -> 응답 한 줄 -> 종료. 게임이 꺼져 있으면 오류를 돌려준다.

실행 제어 도구(launch / stop / run_qa)는 게임과 서버 프로세스를 직접 띄우고 끈다 (Release 빌드 기준).

등록:  claude mcp add majestro -- uv run <이 파일 경로>
포트:  환경변수 MAJESTRO_MCP_PORT (기본 9300). 게임은 -mcp=포트 로 맞춘다.
"""

import itertools
import json
import os
import shlex
import shutil
import socket
import subprocess
import tempfile
import time
from pathlib import Path

from mcp.server import MCPServer
from mcp.server.mcpserver import Image

HOST = "127.0.0.1"
PORT = int(os.environ.get("MAJESTRO_MCP_PORT", "9300"))
TIMEOUT = 10.0  # 게임 스레드가 프레임 시작에 처리한다. 로딩 중에는 몇 초 걸릴 수 있다.

CODE = Path(__file__).resolve().parents[2]
CLIENT_DIR = CODE / "Majestro" / "Executable"
SERVER_DIR = CODE / "Server" / "GameServer"
SERVER_EXE = CODE / "Server" / "Binary" / "Release" / "GameServer.exe"
SERVER_STDOUT = Path(tempfile.gettempdir()) / "majestro_mcp_server.log"
SERVER_READY = "게임 데이터 로드 완료"  # GameCore.cpp: 포트를 연 뒤 데이터 로드가 끝나면 찍힌다
QA_SCRIPT = CODE / "Tools" / "qa" / "run_qa.sh"
CAPTURE_SCRIPT = CODE / "Tools" / "qa" / "capture.ps1"
SHOT_DIR = Path(tempfile.gettempdir()) / "majestro_shots"

mcp = MCPServer("majestro")
_ids = itertools.count(1)
_server: subprocess.Popen | None = None
_client: subprocess.Popen | None = None


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


def _scan_logs(since: int, contains: str, keep: int) -> dict:
    """since 부터 끝까지 256KB 씩 읽으며 contains 가 든 줄만 모아 마지막 keep 줄을 남긴다."""
    lines: list[str] = []
    matched = 0
    first = True
    while True:
        chunk = call("get_logs", since=since, max_bytes=262144)
        if not chunk.get("ok"):
            return chunk
        text = chunk["text"].replace("\r\n", "\n")
        part = text.split("\n")[:-1]  # 마지막은 빈 문자열 (엔진이 줄 단위로 끊어 보낸다)
        if first and since > 0 and part:
            part = part[1:]  # since 가 줄 중간이면 첫 줄은 잘린 줄이라 버린다
        first = False
        for line in part:
            if contains in line:
                matched += 1
                lines.append(line)
        del lines[:-keep]
        since = chunk["next"]
        if not chunk.get("more") or not text:
            return {"ok": True, "lines": lines, "matched": matched, "next": since, "size": chunk["size"]}


@mcp.tool()
def get_logs(since: int = 0, max_bytes: int = 65536, contains: str = "", tail: int = 0) -> dict:
    """engine_log.txt 를 읽는다. 응답의 next 를 다음 호출의 since 로 넘기면 새 줄만 받는다.
    - 그대로: since 바이트부터 max_bytes(상한 256KB) 만큼 원문. more 가 true 면 남은 로그가 있다.
    - contains: 이 문자열이 든 줄만 (예: "audio-runtime", "[Drift]", "fail"). since 부터 끝까지 훑는다.
    - tail: 마지막 N 줄만 (contains 와 같이 쓰면 걸러진 줄 중 마지막 N 줄).
    contains/tail 을 쓰면 text 대신 lines(최대 tail 또는 200줄)와 matched(전체 일치 수)를 돌려준다."""
    if not contains and tail <= 0:
        return call("get_logs", since=since, max_bytes=max_bytes)
    keep = tail if tail > 0 else 200
    if tail > 0 and not contains:
        # 끝부분만 읽는다. 줄 길이를 넉넉히 300바이트로 잡는다.
        size = call("get_logs", since=0, max_bytes=0)
        if not size.get("ok"):
            return size
        since = max(since, size["size"] - keep * 300)
    return _scan_logs(since, contains, keep)


@mcp.tool()
def query_state(enemies: int = 20) -> dict:
    """지금 게임 상태: 씬, 웨이브 페이즈, 점령/호위 진행도, 로컬 플레이어(위치 cm·hp·action·movement·flags·리듬 포인트),
    살아 있는 적 수와 가까운 순 목록(enemies 마리까지, 0 이면 개수만).
    action 은 Protocol ReplicatedActionState, movement 는 ReplicatedMovementMode, 적 type 은 EnemyComponent.h EnemyType 값."""
    return call("query_state", enemies=enemies)


# ---- 입력 (게임을 -mcpinput 또는 -qa 로 띄웠을 때만. launch 는 기본으로 -mcpinput 을 붙인다) ----

def _wait_input(timeout: float) -> dict:
    """input_status 가 끝날 때까지 기다린다."""
    deadline = time.monotonic() + timeout
    status = call("input_status")
    while status.get("ok") and status.get("active") and time.monotonic() < deadline:
        time.sleep(0.2)
        status = call("input_status")
    return status


@mcp.tool()
def press(keys: list[str] | None = None, seconds: float = 0.1, mouse_left: bool = False, mouse_right: bool = False,
          mouse_dx: int = 0, mouse_dy: int = 0) -> dict:
    """키·마우스를 seconds 동안 누르고 있다가 뗀다 (끝날 때까지 기다림, 최대 30초).
    keys: Q W E R ... A S D ... SPACE SHIFT ESC TAB 0-9 F1-F8 (InputManager eKeyCode 이름).
    mouse_dx/dy: 매 프레임 넣을 마우스 이동(픽셀). 1픽셀 = 카메라 약 0.5도.
    예) 앞으로 1초: keys=["W"], seconds=1 / 공격: mouse_left=True / 대시: keys=["SHIFT"]"""
    started = call("press", keys=keys or [], seconds=seconds, mouse_left=mouse_left, mouse_right=mouse_right,
                   mouse_dx=mouse_dx, mouse_dy=mouse_dy)
    return started if not started.get("ok") else _wait_input(seconds + 5)


@mcp.tool()
def move_to(x: float, y: float, z: float, arrive_cm: float = 150, timeout_seconds: float = 30) -> dict:
    """로컬 플레이어를 (x,y,z) cm 까지 걷게 한다. 스테이지는 NavMesh 경로, 광장 등은 직선. 끝날 때까지 기다린다.
    result: arrived / timeout / cancelled / no player. distance 는 마지막 수평 거리(cm). 좌표는 query_state 의 pos 와 같은 계."""
    started = call("move_to", pos=[x, y, z], arrive_cm=arrive_cm, timeout=timeout_seconds)
    return started if not started.get("ok") else _wait_input(timeout_seconds + 5)


@mcp.tool()
def cancel_input() -> dict:
    """진행 중인 press / move_to 를 멈춘다."""
    return call("input_cancel")


@mcp.tool()
def screenshot(max_width: int = 1280) -> list:
    """Majestro 창을 지금 찍어 이미지로 돌려준다 (Tools/qa/capture.ps1 -Once). 창이 여러 개면 먼저 뜬 순서로 전부.
    max_width 로 줄인다 (0 = 원본). 파일은 임시 폴더 majestro_shots 에 남는다."""
    SHOT_DIR.mkdir(exist_ok=True)
    name = time.strftime("shot_%Y%m%d_%H%M%S") + f"_{int(time.time() * 1000) % 1000:03d}.jpg"  # 1초 안에 여러 번 찍어도 안 겹치게
    run = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(CAPTURE_SCRIPT),
                          "-Once", "-Dir", str(SHOT_DIR), "-Name", name, "-MaxWidth", str(max_width)],
                         stdin=subprocess.DEVNULL, capture_output=True, timeout=30)
    shots = sorted(SHOT_DIR.glob(name.replace(".jpg", "*.jpg")))
    if run.returncode != 0 or not shots:
        return [f"찍을 Majestro 창이 없음 (exit={run.returncode}) {run.stderr.decode('cp949', 'replace')[-500:]}"]
    return [Image(path=p) for p in shots] + ["\n".join(str(p) for p in shots)]


# ---- 실행 제어 ----
# 자식 프로세스는 이 MCP 서버의 stdin/stdout(=MCP 통신선)을 물려받으면 안 된다. 항상 DEVNULL 이나 파일로 돌린다.

def _running(image: str) -> bool:
    out = subprocess.run(["tasklist", "/FI", f"IMAGENAME eq {image}", "/NH"],
                         capture_output=True, text=True, errors="replace").stdout
    return image.lower() in out.lower()


def _wait(check, seconds: float) -> bool:
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if check():
            return True
        time.sleep(0.5)
    return False


@mcp.tool()
def launch(client_args: str = "", server: bool = True, god: bool = True, input: bool = True,
           wait_seconds: float = 90) -> dict:
    """GameServer(Release) 와 Majestro.exe 를 -mcp 로 띄우고 ping 이 될 때까지 기다린다.
    client_args: 클라 추가 인자 (예: "-qa -qastages=1 -log=audio-runtime"). server=False 면 서버는 띄우지 않는다.
    god: 서버를 -qagod 로 (플레이어 무적). input: -mcpinput 을 붙여 press/move_to 를 쓸 수 있게 한다
    (이때 하드웨어 키보드·마우스는 꺼진다. 사람이 직접 조작하려면 input=False).
    이미 떠 있는 것은 다시 띄우지 않는다. 끌 때는 stop."""
    global _server, _client
    result: dict = {"ok": True}

    if server:
        if _running("GameServer.exe"):
            result["server"] = "already running"
        else:
            SERVER_STDOUT.unlink(missing_ok=True)
            with open(SERVER_STDOUT, "wb") as log:
                _server = subprocess.Popen([str(SERVER_EXE)] + (["-qagod"] if god else []), cwd=SERVER_DIR,
                                           stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT)
            ready = _wait(lambda: SERVER_READY in SERVER_STDOUT.read_text("utf-8", "replace")
                          or _server.poll() is not None, wait_seconds)
            if not ready or _server.poll() is not None:
                return {"ok": False, "error": "서버가 준비되지 않음", "server_log": str(SERVER_STDOUT),
                        "tail": SERVER_STDOUT.read_text("utf-8", "replace")[-1500:]}
            result["server"] = {"pid": _server.pid, "log": str(SERVER_STDOUT)}

    if call("ping").get("ok"):
        result["client"] = "already running (MCP 응답 중)"
        return result

    args = ([str(CLIENT_DIR / "Majestro.exe"), f"-mcp={PORT}"] + (["-mcpinput"] if input else [])
            + shlex.split(client_args, posix=False))
    _client = subprocess.Popen(args, cwd=CLIENT_DIR, stdin=subprocess.DEVNULL,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if not _wait(lambda: call("ping").get("ok") or _client.poll() is not None, wait_seconds) or _client.poll() is not None:
        return {"ok": False, "error": f"클라가 MCP 에 응답하지 않음 (exit={_client.poll()})", **result}
    result["client"] = {"pid": _client.pid, "args": args[1:], "log": str(CLIENT_DIR / "engine_log.txt")}
    return result


@mcp.tool()
def stop(all_instances: bool = False) -> dict:
    """launch 로 띄운 클라·서버를 끈다. all_instances=True 면 직접 띄운 것까지 Majestro.exe·GameServer.exe 전부 끈다."""
    global _server, _client
    stopped = []
    for name, proc in (("client", _client), ("server", _server)):
        if proc is not None and proc.poll() is None:
            proc.kill()
            proc.wait(10)
            stopped.append(name)
    _server = _client = None
    if all_instances:
        for image in ("Majestro.exe", "GameServer.exe"):
            if _running(image):
                subprocess.run(["taskkill", "/F", "/IM", image], capture_output=True)
                stopped.append(image)
    return {"ok": True, "stopped": stopped}


@mcp.tool()
def run_qa(args: str = "-qastages=1", players: int = 1, timeout_seconds: int = 2400) -> dict:
    """Tools/qa/run_qa.sh 로 무인 QA 1회 (서버+클라 자동 실행·종료). 끝날 때까지 기다린다 (1스테이지 약 3~5분).
    args: 클라 인자 (기본 1스테이지만). players: 봇 수 1~3. 실행 중인 Majestro.exe·GameServer.exe 는 스크립트가 끈다.
    결과: passed, exit_code(0 PASS / 1 FAIL / 2 서버 기동 실패), results 폴더, 요약 출력 끝부분."""
    bash = Path(r"C:\Program Files\Git\bin\bash.exe")
    bash = str(bash) if bash.exists() else (shutil.which("bash") or "bash")
    env = {**os.environ, "QA_PLAYERS": str(players)}
    try:
        run = subprocess.run([bash, str(QA_SCRIPT), *shlex.split(args)], cwd=QA_SCRIPT.parent, env=env,
                             stdin=subprocess.DEVNULL, capture_output=True, timeout=timeout_seconds)
    except subprocess.TimeoutExpired:
        return {"ok": False, "error": f"{timeout_seconds}s 안에 끝나지 않음"}
    out = run.stdout.decode("utf-8", "replace")
    # "==== QA 요약 (/c/.../results/<시각>) ====" -> "C:/.../results/<시각>"
    results = next((line.split("(", 1)[1].split(")", 1)[0] for line in out.splitlines() if "QA 요약 (" in line), None)
    if results and len(results) > 2 and results[0] == "/" and results[2] == "/":
        results = f"{results[1].upper()}:{results[2:]}"
    return {"ok": True, "passed": run.returncode == 0, "exit_code": run.returncode,
            "results": results, "summary": "\n".join(out.splitlines()[-60:])}


if __name__ == "__main__":
    mcp.run()
