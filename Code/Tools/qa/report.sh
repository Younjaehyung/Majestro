#!/bin/bash
# QA 실행 결과 해석: 서버 로그 경고 분류 + 기준 실행과의 비교. 게임 코드는 이 스크립트를 모른다.
#
# 사용:  bash report.sh <실행 폴더> [기준 폴더]
#   예)  bash report.sh results/20261123_121957 baseline/default
#
# 서버 경고: 숫자를 지운 줄 단위로 종류를 묶고, known_server_warnings.txt 에 없는 종류만 "새 경고"로 보여 준다.
#           QA_ACCEPT_WARNINGS=1 이면 이번에 나온 종류를 알려진 목록에 추가한다.
# 비교:     스테이지 클리어 시간, 판정·실패 수, 프레임 시간을 기준과 나란히 찍고 한계를 넘으면 REGRESS 로 표시한다.
#           한계는 같은 빌드를 반복 실행했을 때의 흔들림(스테이지 시간 ±15%)보다 넓게 잡았다.
# 종료 코드: 0 = 이상 없음, 3 = 새 서버 경고 또는 REGRESS 있음

HERE="$(cd "$(dirname "$0")" && pwd)"
RUN="$1"
BASE="$2"
KNOWN="$HERE/known_server_warnings.txt"
STAGE_LIMIT=${QA_STAGE_LIMIT:-30}	# 스테이지 클리어 시간이 이 % 넘게 늘면 REGRESS
PERF_LIMIT=${QA_PERF_LIMIT:-20}		# 프레임 시간이 이 % 넘게 늘면 REGRESS
STATUS=0

# ---- 서버 경고 분류 ----
kinds() {	# 로그 -> "횟수<TAB>종류" (시각과 숫자를 지워 같은 종류로 묶는다)
	grep -aE "\[(WARN|ERROR|FATAL) *\]" "$1" 2> /dev/null | tr -d '\r\0' \
		| sed -E 's/^\[[^]]*\] *//; s/[0-9]+(\.[0-9]+)?/N/g; s/ +/ /g; s/ $//' \
		| sort | uniq -c | sed -E 's/^ *([0-9]+) /\1\t/'
}
touch "$KNOWN"
ALL="$(kinds "$RUN/server.log")"
NEW="$(echo "$ALL" | grep -v '^$' | while IFS=$'\t' read -r count kind; do
	grep -qxF -- "$kind" "$KNOWN" || printf '%s\t%s\n' "$count" "$kind"
done)"
TOTAL=$(echo "$ALL" | grep -vc '^$')
echo "==== 서버 경고: 종류 $TOTAL 개, 그중 새 종류 $(echo "$NEW" | grep -vc '^$') 개 ===="
if [ -n "$NEW" ]; then
	echo "$NEW" | sort -rn | sed -E 's/^([0-9]+)\t/  NEW x\1  /'
	if [ "${QA_ACCEPT_WARNINGS:-0}" = "1" ]; then
		echo "$NEW" | cut -f2- >> "$KNOWN"
		echo "  -> 알려진 목록에 추가함 ($KNOWN)"
	else
		STATUS=3
	fi
fi

# ---- 지표 추출: "이름 값" 줄 ----
metrics() {
	local dir="$1"
	grep -a " STAGE clear in " "$dir/qa_log.txt" 2> /dev/null | tr -d '\r' \
		| sed -E 's/.*scene=([A-Za-z]+) .*clear in ([0-9.]+)s.*/stage_\1_sec \2/'
	grep -a " END " "$dir/qa_log.txt" 2> /dev/null | tr -d '\r' | grep -oE "(fails|warns|deaths)=[0-9]+" | tr '=' ' '
	grep -a "RHYTHM judgements" "$dir/qa_log.txt" 2> /dev/null | tr -d '\r' | grep -oE "miss=[0-9]+" | tr '=' ' '	# good 은 실행마다 1~7 로 흔들려서 보지 않는다
	# 성능 로그는 60프레임마다 한 줄. 실행 전체 평균과 최댓값만 본다.
	awk '
		/^\[GPU\]/ { for (i = 1; i <= NF; i++) if ($i == "Frame") { v = $(i+1) + 0; g += v; gn++; if (v > gmax) gmax = v } }
		/^\[CPU\]/ { for (i = 1; i <= NF; i++) { if ($i == "Update/logic") { u += $(i+1); un++ } if ($i == "Render/record") { r += $(i+1); rn++ } } }
		END {
			if (gn) { printf "gpu_frame_avg_ms %.3f\ngpu_frame_max_ms %.3f\n", g / gn, gmax }
			if (un) printf "cpu_update_avg_ms %.3f\n", u / un
			if (rn) printf "cpu_render_avg_ms %.3f\n", r / rn
		}' "$dir/perf_log.txt" 2> /dev/null
}

if [ -n "$BASE" ] && [ -f "$BASE/qa_log.txt" ]; then
	echo "==== 기준 대비 ($BASE) ===="
	OUT="$(awk -v stage="$STAGE_LIMIT" -v perf="$PERF_LIMIT" '
		NR == FNR { base[$1] = $2; order[n++] = $1; next }
		{ cur[$1] = $2; if (!($1 in base)) order[n++] = $1 }
		END {
			for (i = 0; i < n; i++) {
				k = order[i]; b = (k in base) ? base[k] : "-"; c = (k in cur) ? cur[k] : "-"
				note = ""
				if (b == "-" || c == "-") {
					if (k ~ /^stage_/ && c == "-") note = "REGRESS 클리어 못 함"
				} else if (k ~ /^stage_/ || k ~ /_ms$/) {
					limit = (k ~ /^stage_/) ? stage : perf
					if (k == "gpu_frame_max_ms") limit = 0		# 최댓값은 한 번 튀면 바뀌므로 참고만
					d = (b > 0) ? (c - b) * 100 / b : 0
					note = sprintf("%+.0f%%", d)
					if (limit > 0 && d > limit) note = note " REGRESS"
				} else if (c > b) note = "REGRESS"	# fails/warns/deaths/miss 는 늘면 표시
				printf "  %-24s %10s -> %-10s %s\n", k, b, c, note
			}
		}' <(metrics "$BASE") <(metrics "$RUN"))"
	echo "$OUT"
	echo "$OUT" | grep -q "REGRESS" && STATUS=3
fi
exit $STATUS
