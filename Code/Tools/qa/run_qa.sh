#!/bin/bash
# 무인 QA 1회 실행: 서버 실행 -> 클라이언트(-qa -statlog) 실행 -> 로그 수집 -> 요약 출력
#
# 사용:  bash run_qa.sh [클라이언트 추가 인자...]
#   예)  bash run_qa.sh                     # 전체 스테이지
#        bash run_qa.sh -qastages=1         # 1스테이지만
#        QA_GOD=0 bash run_qa.sh            # 서버 무적 끄기 (봇이 죽을 수 있음)
#
# 환경변수:
#   QA_GOD=1            서버를 -qagod 로 띄워 플레이어가 피해를 받지 않게 한다 (기본 1)
#   QA_PLAYERS=N        봇 N명(1~3)으로 실행. 방장은 캐릭터 1, 입장하는 봇은 캐릭터 2·3 (-qachar 를 따로 주지 않는다)
#   QA_SHOTS=0          화면 캡처 끄기
#   QA_BASELINE=save    이번 실행을 이 인자 조합의 기준으로 저장 (이후 실행은 기준과 비교)
#   QA_ACCEPT_WARNINGS=1 이번에 나온 서버 경고 종류를 알려진 목록에 추가
#   QA_WALL_TIMEOUT=초 클라이언트 강제 종료 시간 (기본 2100). 봇 자체 제한(-qatimeout, 기본 1800)보다 길게 둔다.
#
# 결과: results/<시각>/ 에 qa_log.txt, perf_log.txt, engine_log.txt, server.log
# 종료 코드: 0 = PASS, 1 = FAIL 또는 비정상 종료, 2 = 서버 기동 실패
#
# 사전 조건: Majestro.sln 과 Server.sln 을 Release x64 로 빌드해 둔다.

HERE="$(cd "$(dirname "$0")" && pwd)"
CODE="$(cd "$HERE/../.." && pwd)"
OUT="$HERE/results/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUT"
touch "$OUT/.start"	# 이 시각 이후에 생긴 크래시 기록만 이번 실행 것으로 본다

SERVER_ARGS=""
[ "${QA_GOD:-1}" = "1" ] && SERVER_ARGS="-qagod"

taskkill //F //IM GameServer.exe > /dev/null 2>&1
( cd "$CODE/Server/GameServer" && ../Binary/Release/GameServer.exe $SERVER_ARGS > "$OUT/server.log" 2>&1 ) &
for i in $(seq 1 60); do
	netstat -ano | grep -q ":9000 .*LISTENING" && break
	ping -n 2 127.0.0.1 > /dev/null
done
if ! netstat -ano | grep -q ":9000 .*LISTENING"; then
	echo "QA: 서버가 9000 포트를 열지 못함 (server.log 확인)"
	taskkill //F //IM GameServer.exe > /dev/null 2>&1
	exit 2
fi
ping -n 12 127.0.0.1 > /dev/null	# 서버 리소스 로딩 대기

cd "$CODE/Majestro/Executable"
rm -f qa_log.txt qa_log_join*.txt perf_log.txt engine_log*.txt
# 화면 캡처: 게임 밖에서 창을 찍는다 (20초마다 + FAIL/WARN 이 새로 기록될 때). QA_SHOTS=0 으로 끈다.
[ "${QA_SHOTS:-1}" = "1" ] && powershell -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$HERE/capture.ps1")" -Dir "$(cygpath -w "$OUT/shots")" -Log "$(cygpath -w "$PWD/qa_log.txt")" > /dev/null 2>&1 &
# 멀티: 방장(캐릭터 1) + 입장하는 봇(캐릭터 2, 3). 입장하는 쪽은 방장이 방을 만든 뒤에 띄운다.
PLAYERS=${QA_PLAYERS:-1}
for i in $(seq 2 "$PLAYERS"); do
	( ping -n $((4 + i * 2)) 127.0.0.1 > /dev/null
	  timeout "${QA_WALL_TIMEOUT:-2100}" ./Majestro.exe -qa -qajoin -qachar=$i -logfile=engine_log_join$i.txt "$@" ) &
done
timeout "${QA_WALL_TIMEOUT:-2100}" ./Majestro.exe -qa -statlog -qaplayers=$PLAYERS "$@"
CLIENT_EXIT=$?
if [ "$PLAYERS" -gt 1 ]; then	# 입장한 봇이 스스로 끝낼 시간을 주고, 남아 있으면 정리한다
	for i in $(seq 1 30); do tasklist | grep -qi "Majestro.exe" || break; ping -n 2 127.0.0.1 > /dev/null; done
	taskkill //F //IM Majestro.exe > /dev/null 2>&1
fi

SERVER_ALIVE=1
tasklist | grep -qi "GameServer.exe" || SERVER_ALIVE=0
taskkill //F //IM GameServer.exe > /dev/null 2>&1

cp qa_log.txt qa_log_join*.txt perf_log.txt engine_log*.txt "$OUT/" 2> /dev/null
# 이번 실행 중 클라·서버가 남긴 크래시 기록(CrashHandler: Dumps/*.txt 호출 스택, *.dmp 미니덤프)을 모은다
CRASHES=$(find "$CODE/Majestro/Executable/Dumps" "$CODE/Server/GameServer/Dumps" -type f -newer "$OUT/.start" 2> /dev/null)
[ -n "$CRASHES" ] && mkdir -p "$OUT/dumps" && cp $CRASHES "$OUT/dumps/"

echo "==== QA 요약 ($OUT) ===="
echo "client_exit=$CLIENT_EXIT server_alive_at_end=$SERVER_ALIVE"
for f in "$OUT"/dumps/*.txt; do
	[ -f "$f" ] || continue
	echo "QA: 크래시 기록 $(basename "$f") (스택 앞부분:)"
	head -8 "$f"
done
grep -a -E " (FAIL|WARN) " "$OUT/qa_log.txt" 2> /dev/null | grep -v SUMMARY
grep -a -E "SUMMARY|STAGE |MOVE" "$OUT/qa_log.txt" 2> /dev/null
END_LINE="$(grep -a " END " "$OUT/qa_log.txt" 2> /dev/null)"
if [ -z "$END_LINE" ]; then
	echo "QA: END 줄 없음 -> 클라이언트가 크래시했거나 강제 종료됨 (마지막 기록:)"
	tail -3 "$OUT/qa_log.txt" 2> /dev/null
	exit 1
fi
echo "$END_LINE"

# 서버 경고 분류 + 기준 실행과의 비교. 기준은 같은 인자로 돌린 실행끼리만 비교한다.
#   QA_BASELINE=save  이번 실행을 이 인자 조합의 기준으로 저장
BASE="$HERE/baseline/$(echo "god${QA_GOD:-1} $*" | tr -c 'A-Za-z0-9=.,\n-' '_')"
if [ "${QA_BASELINE:-}" = "save" ]; then
	mkdir -p "$BASE" && cp "$OUT/qa_log.txt" "$OUT/perf_log.txt" "$BASE/" && echo "QA: 기준 저장 -> $BASE"
fi
bash "$HERE/report.sh" "$OUT" "$BASE" 2>&1 | tee "$OUT/report.txt"

[ "$SERVER_ALIVE" = "1" ] || { echo "QA: 실행 중 서버가 죽음"; exit 1; }
# 입장한 봇들도 각자 PASS 로 끝나야 한다.
JOIN_OK=1
for i in $(seq 2 "$PLAYERS"); do
	JOIN_LOG="$OUT/qa_log_join$i.txt"
	echo "---- 입장 봇 $i ----"
	grep -a -E " (FAIL|WARN) " "$JOIN_LOG" 2> /dev/null | grep -v SUMMARY
	JOIN_END="$(grep -a " END " "$JOIN_LOG" 2> /dev/null)"
	echo "${JOIN_END:-END 줄 없음 (크래시했거나 방장이 끝난 뒤 정리됨)}"
	echo "$JOIN_END" | grep -q "result=PASS" || JOIN_OK=0
done
[ "$JOIN_OK" = "1" ] || exit 1
echo "$END_LINE" | grep -q "result=PASS" && exit 0
exit 1
