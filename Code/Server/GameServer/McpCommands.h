#pragma once

// 서버 MCP 디버그 명령: 버그 재현을 위해 방의 판정 상태를 직접 바꾼다. 로직 스레드에서만 부른다 (McpBridge::Poll).
// 모두 기존 게임 경로를 그대로 탄다 — 값만 덮어쓰지 않으므로 클라 복제·사망·처치 처리가 평소와 같다.
//   teleport     {room, netId?, pos:[x,y,z]}         PlacePlayerAt (스폰 배치와 같은 경로, NavMesh 보정)
//   set_hp       {room, netId, hp}                   직접 설정 + EvHealthChanged (회복과 같은 경로).
//                                                    0 이면 적은 kill 경로, 플레이어는 DamageSystem 과 같은 사망 요청
//   kill_enemies {room, netId?, radius?}             치명 EvDamage (낙사 처리와 같은 경로). netId 없으면 radius(첫 플레이어 기준) 안 전부
//   spawn_enemy  {room, type, pos? | offset?, count?} SpawnEnemyAt (보스 소환과 같은 경로). pos 없으면 첫 플레이어 + offset
//   god          {on}                                PlayerGodMode (-qagod 와 같은 플래그)
// netId 생략 = 방의 첫 플레이어.
json McpTeleport(const json& request);
json McpSetHp(const json& request);
json McpKillEnemies(const json& request);
json McpSpawnEnemy(const json& request);
json McpGod(const json& request);
