#pragma once

// Protocol enum 을 로그·QA·MCP 에 찍을 이름으로. (QaBot 에서 옮겨 왔다)

inline const char* SceneName(SceneId id)
{
	switch (id)
	{
	case SceneId::MainMenu:   return "MainMenu";
	case SceneId::Lobby:      return "Lobby";
	case SceneId::Loading:    return "Loading";
	case SceneId::FirstGame:  return "FirstGame";
	case SceneId::SecondGame: return "SecondGame";
	case SceneId::ThirdGame:  return "ThirdGame";
	case SceneId::FourthGame: return "FourthGame";
	case SceneId::VGame:      return "VGame";
	case SceneId::LGame:      return "LGame";
	case SceneId::Plaza:      return "Plaza";
	default:                  return "None";
	}
}

inline const char* PhaseName(uint8 phase)
{
	switch (static_cast<WavePhaseType>(phase))
	{
	case WavePhaseType::Prepare:  return "Prepare";
	case WavePhaseType::Conquest: return "Conquest";
	case WavePhaseType::Escort:   return "Escort";
	case WavePhaseType::Boss:     return "Boss";
	case WavePhaseType::Fail:     return "Fail";
	case WavePhaseType::Clear:    return "Clear";
	default:                      return "None";
	}
}
