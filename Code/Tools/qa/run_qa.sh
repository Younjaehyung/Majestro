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
#   QA_WALL_TIMEOUT=초  클라이언트 강제 종료 시간 (기본 2100). 봇 자체 제한(-qatimeout, 기본 1800)보다 길게 둔다.
#
# 결과: results/<시각>/ 에 qa_log.txt, perf_log.txt, server.log
# 종료 코드: 0 = PASS, 1 = FAIL 또는 비정상 종료, 2 = 서버 기동 실패
#
# 사전 조건: Majestro.sln 과 Server.sln 을 Release x64 로 빌드해 둔다.

HERE="$(cd "$(dirname "$0")" && pwd)"
CODE="$(cd "$HERE/../.." && pwd)"
OUT="$HERE/results/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUT"

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
rm -f qa_log.txt perf_log.txt
timeout "${QA_WALL_TIMEOUT:-2100}" ./Majestro.exe -qa -statlog "$@"
CLIENT_EXIT=$?

SERVER_ALIVE=1
tasklist | grep -qi "GameServer.exe" || SERVER_ALIVE=0
taskkill //F //IM GameServer.exe > /dev/null 2>&1

cp qa_log.txt perf_log.txt "$OUT/" 2> /dev/null

echo "==== QA 요약 ($OUT) ===="
echo "client_exit=$CLIENT_EXIT server_alive_at_end=$SERVER_ALIVE"
grep -a -E " (FAIL|WARN) " "$OUT/qa_log.txt" 2> /dev/null | grep -v SUMMARY
grep -a -E "SUMMARY|STAGE |MOVE" "$OUT/qa_log.txt" 2> /dev/null
END_LINE="$(grep -a " END " "$OUT/qa_log.txt" 2> /dev/null)"
if [ -z "$END_LINE" ]; then
	echo "QA: END 줄 없음 -> 클라이언트가 크래시했거나 강제 종료됨 (마지막 기록:)"
	tail -3 "$OUT/qa_log.txt" 2> /dev/null
	exit 1
fi
echo "$END_LINE"
[ "$SERVER_ALIVE" = "1" ] || { echo "QA: 실행 중 서버가 죽음"; exit 1; }
echo "$END_LINE" | grep -q "result=PASS" && exit 0
exit 1
