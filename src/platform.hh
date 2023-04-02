#pragma once

#include "logger.hh"
#include <chrono>

namespace application::gui
{
  struct Widget;
}

namespace platform
{
  struct RenderContext;

  application::gui::Widget* NewWidget(int);
  
  
  bool WriteFile(std::string name, std::wstring content);
  std::wstring ReadFile(std::wstring name);

  std::vector<std::string> ReadPath(std::string);
  std::string CurrentPath();



  using Timestamp = std::chrono::steady_clock::time_point;
  using Duration  = std::chrono::duration<double, std::milli>;

  Timestamp CurrentTimestamp();
  Duration  DurationFrom(Timestamp, Timestamp); 
}

