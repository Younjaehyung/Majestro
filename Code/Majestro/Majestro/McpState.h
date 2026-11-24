#pragma once

// MCP query_state: 지금 씬의 게임 상태를 JSON 으로 만든다. 게임 스레드에서만 부른다.
//   요청 인자: enemies (기본 20) - 가까운 순으로 몇 마리까지 목록에 넣을지. 0 이면 개수만.
//   응답: scene, phase, entities, player, conquest, escort, enemies
json McpQueryState(const json& request);
