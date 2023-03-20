#pragma once

#include "logger.hh"

namespace application::gui
{
  struct Widget;
}

namespace platform
{
  struct RenderContext;

  application::gui::Widget* NewWidget(int);
  
  

  bool WriteFile(std::string name, std::wstring content);
}

