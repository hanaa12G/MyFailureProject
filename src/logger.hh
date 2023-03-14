#pragma once

#include <cstdio>

namespace logger
{
  static const int debug=4;
  static const int info=3;
  static const int normal=2;
  static const int warning=1;
  static const int error=0;
  static int current_log_level = normal;

  void Init();

  template<typename ...T>
  void Debug(char const* s, T...t)
  {
    if (current_log_level >= debug)
    {
      std::printf(s, t...);
      std::printf("\n");
    }

  }

  template<typename ...T>
  void Info(char const* s, T...t)
  {
    if (current_log_level >= info)
    {
      std::printf(s, t...);
      std::printf("\n");
    }
  }

  template<typename ...T>
  void Warning(char const* s, T...t)
  {
    if (current_log_level >= warning)
    {
      std::printf(s, t...);
      std::printf("\n");
    }
  }

  template<typename ...T>
  void Error(char const* s, T...t)
  {
    if (current_log_level >= error)
    {
      std::printf(s, t...);
      std::printf("\n");
    }
  }
}
