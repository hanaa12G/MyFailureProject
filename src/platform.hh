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



  using Timestamp = std::chrono::steady_clock::time_point;
  using Duration  = std::chrono::steady_clock::duration;

  Timestamp CurrentTimestamp();
  Duration  DurationFrom(Timestamp); 
}

