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
import math
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
SERVER_PORT = int(os.environ.get("MAJESTRO_SERVER_MCP_PORT", "9310"))  # GameServer -mcp=포트
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


def call(cmd: str, _port: int = PORT, **args) -> dict:
    """게임(기본) 또는 서버(_port=SERVER_PORT) McpBridge 에 명령 한 줄을 보내고 응답 한 줄을 받는다."""
    request = {"id": next(_ids), "cmd": cmd, **args}
    target = "Majestro" if _port == PORT else "GameServer"
    try:
        with socket.create_connection((HOST, _port), timeout=TIMEOUT) as sock:
            sock.sendall((json.dumps(request) + "\n").encode())
            data = b""
            while not data.endswith(b"\n"):
                chunk = sock.recv(65536)
                if not chunk:
                    break
                data += chunk
    except OSError as e:
        return {"ok": False, "error": f"{target}({HOST}:{_port}) 에 연결 못함: {e}. -mcp 로 실행했는지 확인"}
    if not data:
        return {"ok": False, "error": f"응답 없음 ({target} 가 접속을 닫음)"}
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


# ---- 서버 (GameServer -mcp. launch 로 띄운 서버는 자동으로 켜져 있다) ----

@mcp.tool()
def server_rooms() -> dict:
    """서버에서 게임 World 가 있는 방 목록 (room, scene, phase, players). 광장·레벨에 들어간 방만 나온다."""
    return call("rooms", _port=SERVER_PORT)


@mcp.tool()
def server_query_state(room: int | None = None, enemies: int = 20) -> dict:
    """서버가 보는 방 상태 (판정 기준). room 생략 시 첫 방.
    songPos(서버 곡 위치 초), conquest(playersInZone·enemiesInZone 포함), escort,
    players[](pos·hp·action·movement·fsmState·rhythmPoints·lastJudgedSongPos·netId·session),
    enemies{alive, list(netId 포함)}. action/movement 는 클라 query_state 의 값과 같은 체계라 바로 비교할 수 있다."""
    args = {"enemies": enemies} if room is None else {"room": room, "enemies": enemies}
    return call("query_state", _port=SERVER_PORT, **args)


@mcp.tool()
def server_get_logs(contains: str = "", tail: int = 100, since: int = 0, file: str = "") -> dict:
    """서버 로그 파일을 읽는다. 기본은 가장 최근 GameServer/Logs/server_*.log (launch·run_qa·직접 실행 모두).
    contains: 이 문자열이 든 줄만 (예: "[combat-detail]", "WARN", "ERROR"). tail: 마지막 N 줄 (0 이면 최대 200).
    since: 이 바이트부터 (응답의 next 를 넘기면 새 줄만). file: 다른 로그 파일 경로를 직접 지정.
    결과: file, lines, matched(전체 일치 수), next, size."""
    log_dir = SERVER_DIR / "Logs"
    path = Path(file) if file else max(log_dir.glob("server_*.log"), key=lambda p: p.stat().st_mtime, default=None)
    if path is None or not path.exists():
        return {"ok": False, "error": f"서버 로그 파일이 없음 ({file or log_dir})"}
    data = path.read_bytes()
    size = len(data)
    if since > size:
        since = 0  # 새 파일이면 처음부터
    end = data.rfind(b"\n") + 1  # 쓰는 중인 마지막 줄은 다음 호출로 미룬다
    if end <= since:
        return {"ok": True, "file": str(path), "lines": [], "matched": 0, "next": since, "size": size}
    lines = data[since:end].decode("utf-8", "replace").replace("\r\n", "\n").split("\n")[:-1]
    if since > 0 and data[since - 1:since] != b"\n" and lines:
        lines = lines[1:]  # since 가 줄 중간이면 잘린 첫 줄은 버린다
    if contains:
        lines = [line for line in lines if contains in line]
    keep = tail if tail > 0 else 200
    return {"ok": True, "file": str(path), "lines": lines[-keep:], "matched": len(lines), "next": end, "size": size}


# 서버 디버그 명령: 버그 재현용으로 방의 판정 상태를 바꾼다. 모두 게임의 기존 경로를 타므로 클라 복제·사망 처리가 평소와 같다.
# netId 생략 = 방의 첫 플레이어. room 생략 = 첫 방. netId 는 server_query_state / compare_state 에서 얻는다.

def _room(args: dict, room: int | None) -> dict:
    return args if room is None else {**args, "room": room}


@mcp.tool()
def server_teleport(x: float, y: float, z: float, net_id: int | None = None, room: int | None = None) -> dict:
    """플레이어를 (x,y,z) cm 로 옮긴다 (스폰 배치와 같은 경로: NavMesh 위로 보정). 실제로 놓인 pos 를 돌려준다."""
    args = {"pos": [x, y, z]} if net_id is None else {"pos": [x, y, z], "netId": net_id}
    return call("teleport", _port=SERVER_PORT, **_room(args, room))


@mcp.tool()
def server_set_hp(hp: int, net_id: int | None = None, room: int | None = None) -> dict:
    """체력을 hp 로 (0..maxHp). 0 이면 적은 처치, 플레이어는 평소 사망 처리. net_id 생략 = 첫 플레이어.
    (무적 중이어도 직접 설정이라 적용된다)"""
    args = {"hp": hp} if net_id is None else {"hp": hp, "netId": net_id}
    return call("set_hp", _port=SERVER_PORT, **_room(args, room))


@mcp.tool()
def server_kill_enemies(net_id: int | None = None, radius_cm: float = 0, room: int | None = None) -> dict:
    """적을 처치한다 (치명 피해로, 평소 처치와 같은 처리). net_id 를 주면 그 적만,
    아니면 radius_cm 안(첫 플레이어 기준, 0 이면 전부)의 적 전부. 결과: killed 수."""
    args = {"netId": net_id} if net_id is not None else ({"radius": radius_cm} if radius_cm > 0 else {})
    return call("kill_enemies", _port=SERVER_PORT, **_room(args, room))


@mcp.tool()
def server_spawn_enemy(type: int, count: int = 1, x: float | None = None, y: float | None = None,
                       z: float | None = None, offset_x: float = 0, offset_z: float = 500,
                       room: int | None = None) -> dict:
    """적을 만든다 (보스 소환과 같은 경로: 지면 보정 + 클라에 스폰 알림).
    type: 0 HornMan, 1 Pianoman, 2 Bongoman, 3 Obelisk, 4 Fly, 5 Brass, 6 Slime, 7 Dragon.
    위치: x,y,z 를 주면 그곳, 아니면 첫 플레이어 + (offset_x, 0, offset_z). count 마리는 x 로 150cm 씩 벌린다 (최대 20).
    결과: spawned[{entity, netId}]."""
    args: dict = {"type": type, "count": count}
    if x is not None and y is not None and z is not None:
        args["pos"] = [x, y, z]
    else:
        args["offset"] = [offset_x, 0, offset_z]
    return call("spawn_enemy", _port=SERVER_PORT, **_room(args, room))


@mcp.tool()
def server_god(on: bool = True) -> dict:
    """플레이어 무적을 켜고 끈다 (서버 전체, -qagod 와 같은 플래그). launch 기본은 god=True 로 켜져 있다."""
    return call("god", _port=SERVER_PORT, on=on)


@mcp.tool()
def server_set_log(domain: str, on: bool = True) -> dict:
    """서버 로그 도메인을 실행 중에 켜거나 끈다. 예: combat-detail, packet-flow, player-state, ai, perf."""
    return call("set_log", _port=SERVER_PORT, domain=domain, on=on)


@mcp.tool()
def crash_reports(limit: int = 3, stack_lines: int = 25) -> dict:
    """클라·서버 크래시 기록 (CrashHandler 가 남긴 Dumps/<이름>_<시각>.txt/.dmp), 최근 것부터.
    각 항목: 시각, 대상(client/server), 예외 코드·접근 위반 주소, 호출 스택 앞부분(stack_lines 줄), .dmp 경로.
    .dmp 는 VS 에서 같은 빌드의 PDB 와 함께 열어 변수까지 본다."""
    reports = []
    for who, folder in (("client", CLIENT_DIR / "Dumps"), ("server", SERVER_DIR / "Dumps")):
        for txt in folder.glob("*.txt") if folder.exists() else []:
            reports.append((txt.stat().st_mtime, who, txt))
    reports.sort(reverse=True)
    out = []
    for mtime, who, txt in reports[:limit]:
        lines = txt.read_text("utf-8", "replace").splitlines()
        dmp = txt.with_suffix(".dmp")
        out.append({"time": time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(mtime)), "target": who,
                    "header": [l for l in lines if not l.startswith("#")],
                    "stack": [l for l in lines if l.startswith("#")][:stack_lines],
                    "txt": str(txt), "dmp": str(dmp) if dmp.exists() else None})
    return {"ok": True, "total": len(reports), "reports": out}


@mcp.tool()
def wait_until(scene: str | None = None, phase: str | None = None, player: bool = False,
               timeout_seconds: float = 120, server: bool = False) -> dict:
    """조건이 맞을 때까지 기다린다 (0.5초마다 query_state). 예: wait_until(scene="FirstGame", phase="Conquest").
    scene/phase: 이름이 같아야 함 (None 이면 보지 않음). player=True 면 플레이어가 생길 때까지.
    server=True 면 서버 server_query_state 로 본다 (첫 방 기준). 로딩 중 응답이 없어도 timeout 까지 계속 기다린다.
    결과: met(조건 충족 여부), elapsed, 마지막 scene/phase, 못 맞췄으면 마지막 오류."""
    started = time.monotonic()
    last: dict = {}
    while True:
        last = call("query_state", _port=SERVER_PORT, enemies=0) if server else call("query_state", enemies=0)
        has_player = bool(last.get("players")) if server else "player" in last
        met = (last.get("ok") and (scene is None or last.get("scene") == scene)
               and (phase is None or last.get("phase") == phase) and (not player or has_player))
        elapsed = round(time.monotonic() - started, 1)
        if met or elapsed >= timeout_seconds:
            result = {"ok": True, "met": bool(met), "elapsed": elapsed,
                      "scene": last.get("scene"), "phase": last.get("phase")}
            if not last.get("ok"):
                result["last_error"] = last.get("error")
            return result
        time.sleep(0.5)


def _flat(a: list, b: list) -> float:
    return math.hypot(a[0] - b[0], a[2] - b[2])


def _compare(client: dict, server: dict, pos_tolerance: float, limit: int) -> dict:
    """클라 query_state 와 서버 server_query_state 결과를 netId 로 짝지어 차이를 모은다 (통신 없는 순수 함수)."""
    issues: list[str] = []
    out: dict = {"ok": True, "scene": {"client": client.get("scene"), "server": server.get("scene")},
                 "phase": {"client": client.get("phase"), "server": server.get("phase")}}
    if client.get("scene") != server.get("scene"):
        issues.append(f"씬 다름 client={client.get('scene')} server={server.get('scene')}")
    if client.get("phase") != server.get("phase"):
        issues.append(f"페이즈 다름 client={client.get('phase')} server={server.get('phase')}")

    if "songPos" in client and "songPos" in server:
        diff_ms = (client["songPos"] - server["songPos"]) * 1000
        out["songPos"] = {"client": client["songPos"], "server": server["songPos"], "diff_ms": round(diff_ms, 1)}

    for key, field in (("conquest", "time"), ("escort", "progress")):
        if key in client and key in server:
            out[key] = {"client": client[key][field], "server": server[key][field],
                        "diff": round(client[key][field] - server[key][field], 3)}

    # ---- 로컬 플레이어 ----
    cp = client.get("player")
    sp = next((p for p in server.get("players", []) if cp and p.get("netId") == cp.get("netId")), None)
    if cp and sp:
        player = {"netId": cp.get("netId"), "pos_diff_cm": round(_flat(cp["pos"], sp["pos"]), 1)}
        for field in ("hp", "action", "movement", "rhythmPoints"):
            player[field] = {"client": cp.get(field), "server": sp.get(field)}
            if cp.get(field) != sp.get(field):
                issues.append(f"플레이어 {field} 다름 client={cp.get(field)} server={sp.get(field)}")
        if player["pos_diff_cm"] > pos_tolerance:
            issues.append(f"플레이어 위치 차이 {player['pos_diff_cm']:.0f}cm (> {pos_tolerance:.0f})")
        out["player"] = player
    elif cp:
        issues.append(f"클라 플레이어 netId={cp.get('netId')} 가 서버 방에 없음")

    # ---- 적 ----
    ce = {e["netId"]: e for e in client.get("enemies", {}).get("list", []) if "netId" in e}
    se = {e["netId"]: e for e in server.get("enemies", {}).get("list", []) if "netId" in e}
    only_server = sorted(set(se) - set(ce))
    only_client = sorted(set(ce) - set(se))
    far, hp = [], []
    for net_id in set(ce) & set(se):
        d = _flat(ce[net_id]["pos"], se[net_id]["pos"])
        if d > pos_tolerance:
            far.append({"netId": net_id, "type": se[net_id]["type"], "pos_diff_cm": round(d, 1)})
        if ce[net_id].get("hp") != se[net_id].get("hp"):
            hp.append({"netId": net_id, "client": ce[net_id].get("hp"), "server": se[net_id].get("hp")})
    far.sort(key=lambda e: -e["pos_diff_cm"])
    alive = {"client": client.get("enemies", {}).get("alive"), "server": server.get("enemies", {}).get("alive")}
    if alive["client"] != alive["server"]:
        issues.append(f"살아 있는 적 수 다름 client={alive['client']} server={alive['server']}")
    if only_server:
        issues.append(f"서버에만 있는 적 {len(only_server)}")
    if only_client:
        issues.append(f"클라에만 있는 적 {len(only_client)}")
    if far:
        issues.append(f"위치가 {pos_tolerance:.0f}cm 넘게 다른 적 {len(far)} (최대 {far[0]['pos_diff_cm']:.0f}cm)")
    if hp:
        issues.append(f"hp 가 다른 적 {len(hp)}")
    out["enemies"] = {"alive": alive, "matched": len(set(ce) & set(se)), "only_server": only_server[:limit],
                      "only_client": only_client[:limit], "pos_over_tolerance": far[:limit], "hp_mismatch": hp[:limit]}
    out["issues"] = issues
    return out


@mcp.tool()
def compare_state(room: int | None = None, pos_tolerance_cm: float = 100, limit: int = 10) -> dict:
    """클라가 보는 상태와 서버(판정 기준) 상태를 netId 로 짝지어 차이만 보여준다. 동기화·판정 버그 추적용.
    issues 에 사람이 읽을 요약, 나머지에 항목별 값. 위치는 pos_tolerance_cm 넘게 다를 때만 문제로 본다.
    두 조회 사이 간격(between_ms)만큼은 움직이는 대상의 차이가 자연히 생긴다 (이동 5m/s 면 10ms 당 5cm)."""
    started = time.monotonic()
    client = call("query_state", enemies=100000)
    args = {"enemies": 100000} if room is None else {"room": room, "enemies": 100000}
    server = call("query_state", _port=SERVER_PORT, **args)
    between_ms = round((time.monotonic() - started) * 1000, 1)
    if not client.get("ok") or not server.get("ok"):
        return {"ok": False, "client": client.get("error", "ok"), "server": server.get("error", "ok")}
    result = _compare(client, server, pos_tolerance_cm, limit)
    result["between_ms"] = between_ms
    return result


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
                _server = subprocess.Popen([str(SERVER_EXE), f"-mcp={SERVER_PORT}"] + (["-qagod"] if god else []), cwd=SERVER_DIR,
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
