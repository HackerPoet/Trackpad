#include "TrackpadDLL.h"

UINT const WM_HOOK = WM_APP + 1;
HWND hwndServer = NULL;
HINSTANCE instanceHandle = NULL;
HHOOK hookHandle = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
    instanceHandle = hModule;
    hookHandle = NULL;
    break;
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}

//Keyboard Hook procedure
static LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
  if (code < 0) {
    return CallNextHookEx(hookHandle, code, wParam, lParam);
  }

  //Report the event to the main window, but do not pass it to the hook chain
  SendMessage(hwndServer, WM_HOOK, wParam, lParam);
  return 1;
}

BOOL InstallHook(HWND hwndParent) {
  //Check if already hooked
  if (hwndServer != NULL) {
    return FALSE;
  }

  //Register keyboard Hook
  hookHandle = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)KeyboardProc, instanceHandle, 0);
  if (hookHandle == NULL) {
    return FALSE;
  }
  hwndServer = hwndParent;
  return TRUE;
}

BOOL UninstallHook() {
  //Check if already unhooked
  if (hookHandle == NULL) {
    return TRUE;
  }

  //If unhook attempt fails, check whether it is because of invalid handle (in that case continue)
  if (!UnhookWindowsHookEx(hookHandle)) {
    DWORD error = GetLastError();
    if (error != ERROR_INVALID_HOOK_HANDLE) {
      return FALSE;
    }
  }
  hwndServer = NULL;
  hookHandle = NULL;
  return TRUE;
}
