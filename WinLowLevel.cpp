#include "WinLowLevel.h"
#include "../TrackpadDLL/TrackpadDLL.h"
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "TrackpadDLL.lib")

void WinLowLevel::ForceOnTop() {
  HWND hwnd = GetActiveWindow();
  SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void WinLowLevel::MakeMessage(const char* msg) {
  HWND hwnd = GetActiveWindow();
  MessageBox(hwnd, msg, "ERROR", MB_OK);
}

void WinLowLevel::CreateHook() {
  //Register keyboard Hook
  HWND hwnd = GetActiveWindow();
  InstallHook(hwnd);
}

void WinLowLevel::DestroyHook() {
  //Remove the keyboard Hook
  UninstallHook();
}

void WinLowLevel::Move(int dx, int dy) {
  INPUT input = { 0 };
  ::ZeroMemory(&input, sizeof(INPUT));
  input.type = INPUT_MOUSE;
  input.mi.dwFlags = MOUSEEVENTF_MOVE;
  input.mi.dx = (LONG)dx;
  input.mi.dy = (LONG)dy;
  ::SendInput(1, &input, sizeof(INPUT));
}

void WinLowLevel::Click(bool pressed) {
  INPUT input = { 0 };
  ::ZeroMemory(&input, sizeof(INPUT));
  input.type = INPUT_MOUSE;
  input.mi.dwFlags = (pressed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP);
  ::SendInput(1, &input, sizeof(INPUT));
}

void WinLowLevel::Scroll(int dx, int dy) {
  if (dy != 0) {
    INPUT input = { 0 };
    ::ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = (LONG)dy;
    ::SendInput(1, &input, sizeof(INPUT));
  }
  if (dx != 0) {
    INPUT input = { 0 };
    ::ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
    input.mi.mouseData = (LONG)dx;
    ::SendInput(1, &input, sizeof(INPUT));
  }
}