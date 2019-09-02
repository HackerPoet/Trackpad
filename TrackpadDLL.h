#pragma once
#include <Windows.h>

#ifdef HOOKIGNOREKEYBOARD_EXPORTS
#define HOOKIGNOREKEYBOARD_API __declspec(dllexport)
#else
#define HOOKIGNOREKEYBOARD_API __declspec(dllimport)
#endif

HOOKIGNOREKEYBOARD_API BOOL InstallHook(HWND hwndParent);
HOOKIGNOREKEYBOARD_API BOOL UninstallHook();
