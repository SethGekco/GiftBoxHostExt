// GiftBoxHost — standalone Yuri's Revenge Syringe DLL
// Stage 0: minimal bootstrap. Proves the DLL is injected by Syringe and that
// its hooks execute, with zero dependency on the Kratos framework.
//
// Everything here is intentionally tiny and self-contained. Feature code
// (Host / GiftBox spawning) is added in later stages on top of this skeleton.

#include <Syringe.h>   // DEFINE_HOOK, REGISTERS, EXPORT_FUNC (hook -> .syhks00 section)

#include <windows.h>
#include <cstdio>

namespace GiftBoxHost
{
	// Dead-simple file logger so we can confirm load without any game classes.
	// Writes next to gamemd.exe (current working directory at runtime).
	static void Log(const char* msg)
	{
		FILE* f = nullptr;
		if (fopen_s(&f, "GiftBoxHost.log", "a") == 0 && f)
		{
			fputs(msg, f);
			fputc('\n', f);
			fclose(f);
		}
	}
}

// Fires once early in YR's boot sequence. Reusing an address+size Kratos uses,
// so alignment/size are known-good. If this line appears in GiftBoxHost.log,
// Syringe loaded the DLL and executed our hook.
DEFINE_HOOK(0x52BA60, GiftBoxHost_Boot, 0x5)
{
	static bool logged = false;
	if (!logged)
	{
		logged = true;
		GiftBoxHost::Log("[GiftBoxHost] Stage 0 skeleton: boot hook fired, DLL is live.");
	}
	return 0; // 0 = execute the original overwritten instructions and continue
}

// Standard Syringe/YRpp DLL entry. Mirrors Kratos's signature exactly.
bool __stdcall DllMain(HANDLE hInstance, DWORD dwReason, LPVOID /*reserved*/)
{
	(void)hInstance;
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		GiftBoxHost::Log("[GiftBoxHost] DllMain: DLL_PROCESS_ATTACH");
	}
	return true;
}
