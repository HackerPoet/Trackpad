#pragma once
#include <vector>

class WinLowLevel {
public:
  static void ForceOnTop();
  static void MakeMessage(const char* msg);
  static void CreateHook();
  static void DestroyHook();
  static void Move(int dx, int dy);
  static void Click(bool pressed);
  static void Scroll(int dx, int dy);
};
