#include "pch.h"
#include "CrashHandler.h"
#include <DbgHelp.h>
#include <cstdio>
#pragma comment(lib, "dbghelp.lib")

// Unity 빌드를 고려해 익명 namespace 대신 이름을 붙인다.
namespace CrashHandlerDetail
{
	wchar_t gName[32] = L"app";

	void WriteMiniDump(const wchar_t* path, EXCEPTION_POINTERS* info)
	{
		HANDLE file = ::CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
			return;
		MINIDUMP_EXCEPTION_INFORMATION exception{ ::GetCurrentThreadId(), info, FALSE };
		const auto type = static_cast<MINIDUMP_TYPE>(
			MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory | MiniDumpWithThreadInfo);
		::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), file, type, &exception, nullptr, nullptr);
		::CloseHandle(file);
	}

	void WriteStack(const wchar_t* path, EXCEPTION_POINTERS* info)
	{
		FILE* out = nullptr;
		if (::_wfopen_s(&out, path, L"w") != 0 || out == nullptr)
			return;

		const EXCEPTION_RECORD* record = info->ExceptionRecord;
		std::fprintf(out, "exception=0x%08lX address=%p thread=%lu\n",
			record->ExceptionCode, record->ExceptionAddress, ::GetCurrentThreadId());
		if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
			std::fprintf(out, "access-violation %s address=0x%llX\n",
				record->ExceptionInformation[0] == 0 ? "read" : record->ExceptionInformation[0] == 1 ? "write" : "execute",
				static_cast<unsigned long long>(record->ExceptionInformation[1]));

		HANDLE process = ::GetCurrentProcess();
		::SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
		::SymInitialize(process, nullptr, TRUE);	// exe 옆의 PDB 를 쓴다

		CONTEXT context = *info->ContextRecord;
		STACKFRAME64 frame{};
		frame.AddrPC.Offset = context.Rip;
		frame.AddrFrame.Offset = context.Rbp;
		frame.AddrStack.Offset = context.Rsp;
		frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;

		for (int i = 0; i < 64; ++i)
		{
			if (!::StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, ::GetCurrentThread(), &frame, &context,
					nullptr, ::SymFunctionTableAccess64, ::SymGetModuleBase64, nullptr) || frame.AddrPC.Offset == 0)
				break;

			const DWORD64 address = frame.AddrPC.Offset;
			char module[MAX_PATH] = "?";
			HMODULE handle = nullptr;
			if (::GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
					reinterpret_cast<LPCSTR>(address), &handle))
				::GetModuleFileNameA(handle, module, MAX_PATH);
			const char* moduleName = std::strrchr(module, '\\') ? std::strrchr(module, '\\') + 1 : module;

			alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + 512]{};
			auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = 511;
			DWORD64 displacement = 0;
			IMAGEHLP_LINE64 line{};
			line.SizeOfStruct = sizeof(line);
			DWORD lineDisplacement = 0;

			if (::SymFromAddr(process, address, &displacement, symbol))
			{
				if (::SymGetLineFromAddr64(process, address, &lineDisplacement, &line))
					std::fprintf(out, "#%02d %s!%s  %s:%lu\n", i, moduleName, symbol->Name, line.FileName, line.LineNumber);
				else
					std::fprintf(out, "#%02d %s!%s+0x%llX\n", i, moduleName, symbol->Name, displacement);
			}
			else
			{
				std::fprintf(out, "#%02d %s+0x%llX\n", i, moduleName,
					address - reinterpret_cast<DWORD64>(handle));
			}
		}
		::SymCleanup(process);
		std::fclose(out);
	}

	LONG WINAPI OnCrash(EXCEPTION_POINTERS* info)
	{
		// 덤프를 쓰다 또 죽으면 그냥 끝낸다
		static volatile LONG sEntered = 0;
		if (::InterlockedExchange(&sEntered, 1) != 0)
			return EXCEPTION_EXECUTE_HANDLER;

		::CreateDirectoryW(L"Dumps", nullptr);
		SYSTEMTIME now{};
		::GetLocalTime(&now);
		wchar_t base[MAX_PATH];
		::swprintf_s(base, L"Dumps\\%s_%04u%02u%02u_%02u%02u%02u", gName,
			now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);

		wchar_t path[MAX_PATH];
		::swprintf_s(path, L"%s.txt", base);	// 스택 먼저: 미니덤프 쓰다 실패해도 이것은 남긴다
		WriteStack(path, info);
		::swprintf_s(path, L"%s.dmp", base);
		WriteMiniDump(path, info);

		// 오류 보고 창을 띄우지 않고 종료한다 (무인 QA 가 멈추지 않게)
		return EXCEPTION_EXECUTE_HANDLER;
	}
}

void CrashHandler::Install(const wchar_t* name)
{
	::wcsncpy_s(CrashHandlerDetail::gName, name, _TRUNCATE);
	::SetUnhandledExceptionFilter(CrashHandlerDetail::OnCrash);
}
