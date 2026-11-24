#pragma once

// 서버 MCP 상태 조회. 로직 스레드에서만 부른다 (McpBridge::Poll).
//   McpListRooms:       게임 World 가 있는 방 목록
//   McpQueryRoomState:  요청 {room (생략 시 첫 방), enemies (기본 20)}
//     응답: room, scene, phase, entities, songPos, conquest, escort, players[], enemies{alive, list}
//     players 의 action/movement 는 서버가 클라에 복제하는 값과 같다 (클라 query_state 와 비교 가능).
//     netId 는 클라·서버가 같은 엔티티를 가리키는 id (NetEntityComponent::mNetEntityId).
json McpListRooms();
json McpQueryRoomState(const json& request);

// 요청의 room (생략 시 첫 방) 의 게임 씬. 없으면 nullptr 이고 outError 에 응답할 오류를 채운다. (McpCommands 도 쓴다)
class Scene;
Scene* McpFindRoom(const json& request, uint32& outRoomId, json& outError);
