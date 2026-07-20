// Tiny self-contained file logger. No game classes, no framework.
#pragma once

#include <windows.h>
#include <cstdio>
#include <cstdarg>

namespace GiftBoxHost
{
	inline void Log(const char* fmt, ...)
	{
		char buffer[512];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, sizeof(buffer), fmt, args);
		va_end(args);

		FILE* f = nullptr;
		if (fopen_s(&f, "GiftBoxHost.log", "a") == 0 && f)
		{
			fputs(buffer, f);
			fputc('\n', f);
			fclose(f);
		}
	}
}
