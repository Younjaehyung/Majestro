#pragma once

// 처리되지 않은 예외(접근 위반, 잡히지 않은 C++ 예외 등)로 죽을 때 작업 폴더 Dumps/ 에 두 파일을 남긴다.
//   <이름>_<시각>.dmp : 미니덤프. VS 로 열어 디버깅한다 (같은 빌드의 PDB 필요).
//   <이름>_<시각>.txt : 예외 코드와 호출 스택(함수·파일·줄). 디버거 없이 바로 읽는다 (MCP crash_reports).
// 디버거가 붙어 있으면 디버거가 먼저 잡으므로 동작하지 않는다.
// ponytail: 스택 오버플로·__fastfail(순수 가상 호출, 잘못된 CRT 인자)은 못 잡는다. 필요해지면 별도 스레드에서 덤프를 쓴다.
//           클라 Majestro/CrashHandler 와 같은 코드다 (클라·서버가 공유하는 빌드 단위가 Protocol 뿐). 한쪽을 고치면 같이 고친다.
namespace CrashHandler
{
	void Install(const wchar_t* name);
}
