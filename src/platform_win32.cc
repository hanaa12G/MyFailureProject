#define NOMINMAX
#include <windows.h>

#include <stdio.h>
#include <d2d1.h>
#include <dwrite.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <utility>
#include <optional>
#include <thread>
#include "logger.hh"
#include "platform.hh"
#include "application.hh"

namespace platform
{
  struct PlatformRectangle : public application::gui::Rectangle
  {
    virtual void Draw(application::gui::RenderContext*, application::gui::InteractionContext) override
    {
    }
  };

  application::gui::Widget*
  NewWidget(int type)
  {
    switch (type)
    {
      case application::gui::WidgetType::Rectangle:
      {
        return new PlatformRectangle();
      }
      default:
      {
        std::unreachable();
      }
    }
  }
}

int main()
{
  try
  {
    logger::Init();

  }
  catch (std::exception e)
  {
    printf("Exception: %s\n", e.what());
    printf("last error %d\n", GetLastError());
    return 1;
  }
  catch (...)
  {
    printf("Unknow exception\n");
    return 1;
  }
  return 0;
}
