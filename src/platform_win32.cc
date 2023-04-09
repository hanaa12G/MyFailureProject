#define NOMINMAX
#include <windows.h>

#include <sys/stat.h>

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
#include <fstream>
#include <thread>
#include <filesystem>
#include <codecvt>
#include "logger.hh"
#include "platform.hh"
#include "application.hh"


namespace platform
{
  Timestamp CurrentTimestamp()
  {
    return std::chrono::steady_clock::now();
  }
  Duration DurationFrom(Timestamp to, Timestamp from)
  {
    return std::chrono::duration_cast<Duration>(to - from);
  }

  std::vector<std::string> ReadPath(std::string path)
  {
      namespace fs = std::filesystem;

      fs::path p{ path };
      if (!fs::is_directory(p))
          return {};
      fs::directory_iterator di{ p };
      std::vector<std::string> out;
      for (auto const& item : di)
      {
          out.push_back(item.path().filename().string());
      }
      out.push_back("..");
      return out;
  }


  std::string CurrentPath()
  {
      return std::filesystem::current_path().string();
  }

  bool IsFile(std::string path)
  {
    return std::filesystem::is_regular_file(path);
  }
  bool IsDirectory(std::string path)
  {
    return std::filesystem::is_directory(path);
  }

  std::string AppendSegment(std::string basepath, std::string name)
  {
    namespace fs = std::filesystem;
    fs::path base {basepath};
    base /= name;

    return fs::absolute(base).string();
  }

  struct RenderContext
  {
    ID2D1HwndRenderTarget* render_target = NULL; 
    IDWriteFactory* dwrite_factory = NULL;
    IDWriteTextFormat* text_format = NULL;
    ID2D1SolidColorBrush* text_brush = NULL;
  };

  template<typename Interface>
  void SafeRelease(Interface** i)
  {
    (*i)->Release();
    (*i) = 0;
  }

  D2D1_COLOR_F ConvertToD2D1Color(application::gui::Color color)
  {
    return D2D1::ColorF(
      color.r,
      color.g,
      color.b,
      color.a);
  }

  struct PlatformRectangle : public application::gui::Rectangle
  {
    virtual void Draw(RenderContext* render_context, application::gui::InteractionContext interaction_context) override
    {
      logger::Debug("Rectangle::Draw");
      if (!IsLayoutInfoValid(m_layout))
      {
        logger::Error("Call draw without layout");
      }

      ID2D1SolidColorBrush* brush;
      HRESULT hr = S_OK;
      if (interaction_context.active == this ||
          interaction_context.hot    == this)
      {
        hr = render_context->render_target->CreateSolidColorBrush(ConvertToD2D1Color(m_bg_active_color), &brush);
      }
      else
      {
        hr = render_context->render_target->CreateSolidColorBrush(ConvertToD2D1Color(m_bg_default_color), &brush);
      }

      if (FAILED(hr)) return;

      D2D1_RECT_F rec = D2D1::RectF(
        0, 0,
        m_layout.width, m_layout.height);

      D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(m_layout.x, m_layout.y);

      D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();

      render_context->render_target->GetTransform(&parent_translation);

      my_translation = parent_translation * my_translation;

      render_context->render_target->SetTransform(my_translation);

      render_context->render_target->FillRectangle(
        rec,
        brush
      );

      render_context->render_target->SetTransform(parent_translation);

      SafeRelease(&brush);
    }
  };

  struct PlatformVerticalContainer: public application::gui::VerticalContainer
  {
    virtual void Draw(RenderContext* render_context, application::gui::InteractionContext interaction_context) override
    {
      logger::Debug("VerticalContainer::Draw");
      if (!IsLayoutInfoValid(m_layout)) return;

      D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(m_layout.x, m_layout.y);
      D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();
      render_context->render_target->GetTransform(&parent_translation);

      my_translation = parent_translation * my_translation;

      render_context->render_target->SetTransform(my_translation);
      for (auto it = m_children.begin(); it != m_children.end(); ++it)
      {
          Widget* widget = it->get();
        widget->Draw(render_context, interaction_context);
      }
      render_context->render_target->SetTransform(parent_translation);

    }
  };

  struct PlatformHorizontalContainer: public application::gui::HorizontalContainer
  {
    virtual void Draw(RenderContext* render_context, application::gui::InteractionContext interaction_context) override
    {
      if (!IsLayoutInfoValid(m_layout)) return;
      logger::Debug("HorizontalContainer::Draw");

      D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(m_layout.x, m_layout.y);
      D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();
      render_context->render_target->GetTransform(&parent_translation);

      my_translation = parent_translation * my_translation;

      render_context->render_target->SetTransform(my_translation);
      for (auto it = m_children.begin(); it != m_children.end(); ++it)
      {
        Widget* widget = it->get();
        widget->Draw(render_context, interaction_context);
      }
      render_context->render_target->SetTransform(parent_translation);

    }
  };

  struct PlatformButton: public application::gui::Button
  {
    virtual void Draw(RenderContext* render_context, application::gui::InteractionContext interaction_context) override
    {
      logger::Debug("Button::Draw");
      if (!IsLayoutInfoValid(m_layout))
      {
        logger::Error("Call draw without layout");
        return;
      }

      ID2D1SolidColorBrush* brush;
      HRESULT hr = S_OK;
      if (interaction_context.about_to_active == this)
      {
        hr = render_context->render_target->CreateSolidColorBrush(ConvertToD2D1Color(m_bg_active_color), &brush);
      }
      else if (interaction_context.hot == this)
      {
        hr = render_context->render_target->CreateSolidColorBrush(ConvertToD2D1Color(m_bg_active_color), &brush);
      }
      else
      {
        hr = render_context->render_target->CreateSolidColorBrush(ConvertToD2D1Color(m_bg_default_color), &brush);
      }

      if (FAILED(hr)) return;

      ID2D1SolidColorBrush* text_brush = NULL;
      hr = render_context->render_target->CreateSolidColorBrush(
        ConvertToD2D1Color(m_fg_default_color),
        &text_brush);
      if (FAILED(hr)) return;

      D2D1_RECT_F rec = D2D1::RectF(
        0, 0,
        m_layout.width, m_layout.height);


      D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(m_layout.x, m_layout.y);

      D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();

      render_context->render_target->GetTransform(&parent_translation);

      my_translation = parent_translation * my_translation;

      render_context->render_target->SetTransform(my_translation);

      render_context->render_target->FillRectangle(
        rec,
        brush
      );

      std::wstring_convert<std::codecvt_utf8<wchar_t>> converter{};

      auto text = converter.from_bytes(m_text);
      render_context->render_target->DrawText(
        text.c_str(),
        text.size(),
        render_context->text_format,
        rec,
        text_brush);

      render_context->render_target->SetTransform(parent_translation);

      SafeRelease(&brush);
      SafeRelease(&text_brush);
    }
  };

  struct PlatformTextBox: public application::gui::TextBox
  {
    IDWriteTextFormat* m_text_format = NULL;

    virtual void Draw(RenderContext* render_context, application::gui::InteractionContext interaction_context) override
    {
      if (!m_text_format)
      {
        InitTextFormat(render_context);
      }
      if (!m_text_format)
      {
        logger::Error("Can't create text format");
        return;
      }

      logger::Debug("TextBox::Draw");
      if (!IsLayoutInfoValid(m_layout))
      {
        logger::Error("Call draw without layout");
        return;
      }

      if (interaction_context.active == this)
      {
        for (char c : interaction_context.keys_pressed)
        {
          m_text.push_back((wchar_t) c);
        }
      }


      ID2D1SolidColorBrush* brush;
      HRESULT hr = S_OK;

      hr = render_context->render_target->CreateSolidColorBrush(ConvertToD2D1Color(m_bg_default_color), &brush);

      if (FAILED(hr)) return;

      ID2D1SolidColorBrush* text_brush = NULL;
      hr = render_context->render_target->CreateSolidColorBrush(
        ConvertToD2D1Color(m_fg_default_color),
        &text_brush);
      if (FAILED(hr)) return;

      D2D1_RECT_F rec = D2D1::RectF(
        0, 0,
        m_layout.width, m_layout.height);


      D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(m_layout.x, m_layout.y);

      D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();

      render_context->render_target->GetTransform(&parent_translation);

      my_translation = parent_translation * my_translation;

      render_context->render_target->SetTransform(my_translation);

      render_context->render_target->FillRectangle(
        rec,
        brush
      );

      static unsigned long n = 0;

      std::string tmp_text = m_text;

      if (interaction_context.active == this)
      {
        if (interaction_context.keys_pressed.empty())
        {
          n++;
        }
        else
        {
          n = 0;
        }
        if (n % 60 < 30)
        {
          tmp_text += '_';
        }
      }

      IDWriteTextLayout* layout;


      std::wstring_convert<std::codecvt_utf8<wchar_t>> converter{};

      auto text = converter.from_bytes(tmp_text);
      hr = render_context->dwrite_factory->CreateTextLayout(
        text.c_str(),
        text.size(),
        m_text_format,
        m_layout.width,
        m_layout.height,
        &layout);
      if (FAILED(hr)) return;

      render_context->render_target->DrawTextLayout(
        D2D1::Point2F(0, 0),
        layout,
        text_brush);


      render_context->render_target->SetTransform(parent_translation);

      SafeRelease(&brush);
      SafeRelease(&text_brush);
    }

    void InitTextFormat(RenderContext* render_context)
    {
      HRESULT hr = render_context->dwrite_factory->CreateTextFormat(
        L"Cascadia Code",
        NULL,
        DWRITE_FONT_WEIGHT_REGULAR,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        14.0f,
        L"en-us",
        &m_text_format);
      if (FAILED(hr))
      {
        logger::Warning("Failed to create text format, will use default\n");
        m_text_format = render_context->text_format;
      }
      else
      {
        m_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
      }
    }
  };

  struct PlatformLayers : public application::gui::Layers
  {
    virtual void Draw(RenderContext* render_context, application::gui::InteractionContext interaction_context) override
    {
      logger::Debug("Layers::Draw");
      if (!IsLayoutInfoValid(m_layout))
      {
        logger::Error("Call draw without layout");
      }

      D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(m_layout.x, m_layout.y);
      D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();
      render_context->render_target->GetTransform(&parent_translation);

      my_translation = parent_translation * my_translation;

      render_context->render_target->SetTransform(my_translation);

      int d = 0;
      for (auto it = m_layers.begin(); it != m_layers.end(); ++it)
      {
        logger::Debug("Draw layer %d", d++);
        it->second->Draw(render_context, interaction_context);
      }
      render_context->render_target->SetTransform(parent_translation);
    }
  };

  struct PlatformFileSelector : public application::gui::FileSelector
  {
      void Draw(RenderContext* render_context, application::gui::InteractionContext interaction_context) override
      {
        logger::Debug("FileSelector::Draw");
        if (!IsLayoutInfoValid(m_layout)) return;

        D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(m_layout.x, m_layout.y);
        D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();
        render_context->render_target->GetTransform(&parent_translation);

        my_translation = parent_translation * my_translation;

        render_context->render_target->SetTransform(my_translation);

        m_container->Draw(render_context, interaction_context);
        
        render_context->render_target->SetTransform(parent_translation);

      }
  };

  application::gui::Widget*
  NewWidget(int type)
  {
      switch (type)
      {
      case application::gui::WidgetType::RectangleType:
      {
          return new PlatformRectangle();
      } break;
      case application::gui::WidgetType::VerticalContainerType:
      {
          return new PlatformVerticalContainer();
      } break;
      case application::gui::WidgetType::HorizontalContainerType:
      {
          return new PlatformHorizontalContainer();
      } break;
      case application::gui::WidgetType::ButtonType:
      {
          return new PlatformButton();
      } break;
      case application::gui::WidgetType::TextBoxType:
      {
          return new PlatformTextBox();
      } break;
      case application::gui::WidgetType::LayersType:
      {
          return new PlatformLayers();
      } break;
      case application::gui::WidgetType::FileSelectorType:
      {
          return new PlatformFileSelector();
      } break;
      default:
      {
        std::unreachable();
      }
    }
  }


  bool WriteFile(std::string filename, std::string content)
  {
    logger::Debug("I'm writting");
    std::wfstream f(filename, std::ios::out | std::ios::trunc);
    if (!f)
    {
      logger::Error("Cant write to file");
      return false;
    }

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter{};

    auto text = converter.from_bytes(content);
    f << text;


    f.flush();
    if (f) return true;
    return false;
  }

  std::string ReadFile(std::string name)
  {
    char filepath[1000];

    std::fstream f(filepath);
    if (!f) return {};
    std::stringstream buffer;
    buffer << f.rdbuf();

    return buffer.str();
  }
} // namespace application

using IntervalClock = std::chrono::steady_clock;
using Interval = std::chrono::milliseconds;
using IntervalTimePoint = std::chrono::time_point<IntervalClock, Interval>;


template<typename ChildWindow>
struct Window
{
  static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
  {
    ChildWindow* w = {NULL};
    if (msg == WM_NCCREATE)
    {
      LPCREATESTRUCT s = (LPCREATESTRUCT) lparam;
      w = (ChildWindow*) s->lpCreateParams;
      w->m_hwnd = hwnd;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(w));
    }
    else
    {
      w = (ChildWindow*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (w)
      return w->HandleMessage(hwnd, msg, wparam, lparam);
    else
      return DefWindowProc(hwnd, msg, wparam, lparam);

  }

};

class MainWindow
{
public:
  HWND m_hwnd = NULL;
  ID2D1Factory* m_direct2d_factory = NULL;
  ID2D1HwndRenderTarget* m_render_target = NULL; 

  IDWriteFactory* m_dwrite_factory = NULL;
  IDWriteTextFormat* m_text_format = NULL;
  ID2D1SolidColorBrush* m_text_brush = NULL;

  application::gui::Application* m_app = NULL;
  application::gui::UserInput    m_user_input;
  application::gui::UserInput    m_last_user_input;
  bool m_running = true;

  int m_width = 0;
  int m_height = 0;


public:
  MainWindow(MainWindow const&) = delete;
  MainWindow(MainWindow&&) = default;
  MainWindow()
  {
    m_app = new application::gui::Application();

  }
  ~MainWindow()
  {
    CleanupGraphicResources();
  }

  static void Register()
  {
    WNDCLASSW wclass = {};
    wclass.lpfnWndProc = Window<MainWindow>::WindowProc;
    wclass.hInstance = GetModuleHandle(NULL);
    wclass.lpszClassName = L"MyFailureProject";

    BOOL s = RegisterClassW(&wclass);
    if (!s)
    {
      DWORD err = GetLastError();
      printf("%d\n", err);
      throw std::runtime_error("Can't register window class");
    }
  }

  static MainWindow Instance()
  {
    MainWindow w;
    HWND hwnd = CreateWindowExW(
      0,
      L"MyFailureProject",
      L"Window Name",
      WS_OVERLAPPEDWINDOW,
      640, 480,
      CW_USEDEFAULT, CW_USEDEFAULT,
      0,
      0,
      GetModuleHandle(NULL),
      &w);
     
    if (!hwnd)
      throw std::runtime_error("Can't create window");
    w.m_hwnd = hwnd;

    return w;
  }


  void CreateGraphicResources()
  {
    printf("CreateGraphicResources\n");
    HRESULT hr = S_OK;

    hr = D2D1CreateFactory(
      D2D1_FACTORY_TYPE_SINGLE_THREADED,
      &m_direct2d_factory);

    if (FAILED(hr)) throw std::runtime_error("Failed to create graphic: " + std::to_string(hr));

    RECT rc;
    GetClientRect(m_hwnd, &rc);

    D2D1_SIZE_U size = D2D1::SizeU(
      rc.right - rc.left,
      rc.bottom - rc.top);

    hr = m_direct2d_factory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(),
      D2D1::HwndRenderTargetProperties(m_hwnd, size),
      &m_render_target);

    if (FAILED(hr)) throw std::runtime_error("Failed to create render target: " + std::to_string(hr));


    hr = DWriteCreateFactory(
      DWRITE_FACTORY_TYPE_SHARED,
      __uuidof(IDWriteFactory),
      (IUnknown**) &m_dwrite_factory);

    if (FAILED(hr)) throw std::runtime_error("Faied to create dwrite factory" + std::to_string(hr));


    hr = m_dwrite_factory->CreateTextFormat(
      L"Cascadia Code",
      NULL,
      DWRITE_FONT_WEIGHT_REGULAR,
      DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL,
      13.0f,
      L"en-us",
      &m_text_format);

    if (FAILED(hr)) throw std::runtime_error("Faied to create font" + std::to_string(hr));


    m_text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  }

  void CleanupGraphicResources()
  {
    platform::SafeRelease(&m_render_target);

    platform::SafeRelease(&m_text_format);
    platform::SafeRelease(&m_dwrite_factory);

    platform::SafeRelease(&m_direct2d_factory);
  }

  void OnPaint()
  {
    HRESULT hr = S_OK;

    if (!m_direct2d_factory)
      CreateGraphicResources();

    if (!SUCCEEDED(hr)) return;

    ID2D1SolidColorBrush* brush; 
    hr = m_render_target->CreateSolidColorBrush(
      D2D1::ColorF(0.9, 0.1, 0.1, 1.0),
      &brush);

    if (!SUCCEEDED(hr)) return;


    platform::RenderContext wc = CreateContext(this);


    logger::Info("Window: w=%d, h=%d", m_width, m_height);
    application::gui::LayoutConstraint constraint = {
      .max_width = m_width,
      .max_height = m_height,
      .x = 0,
      .y = 0
    };


    m_render_target->BeginDraw();
    m_render_target->Clear(D2D1::ColorF(D2D1::ColorF::Black));
    m_app->Render(&constraint, &wc);
    m_render_target->EndDraw();

    platform::SafeRelease(&brush);
    
  }


  LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
  {
    switch (msg)
    {
      case WM_CREATE:
      {
        try {
          CreateGraphicResources();
        }

        catch (std::exception e)
        {
          printf("Exception: %s\n", e.what());
          PostMessage(hwnd, WM_DESTROY, 1, 0);
        }
      } break;

      case WM_SIZE:
      {
        int width = LOWORD(lparam);
        int height = HIWORD(lparam);

        if (m_width == width && m_height == height)
        {
          return 0;
        }

        if (m_render_target)
        {
          platform::SafeRelease(&m_render_target);
    
          RECT rc;
          GetClientRect(m_hwnd, &rc);

          D2D1_SIZE_U size = D2D1::SizeU(
            rc.right - rc.left,
            rc.bottom - rc.top);

          HRESULT hr = m_direct2d_factory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(m_hwnd, size),
            &m_render_target);

          if (FAILED(hr)) throw std::runtime_error("Failed to create render target: " + std::to_string(hr));
        }

        m_width = width;
        m_height = height;
        return 0;
      } break;

      case WM_PAINT:
      {
        OnPaint();
      } break;

      case WM_CLOSE:
      {
        DestroyWindow(m_hwnd);
        return 0;
      } break;
      case WM_DESTROY:
      {
        m_running = false;
        PostQuitMessage(0);
        return 0;
      } break;
      case WM_MOUSEMOVE:
      {
        int mouse_x = LOWORD(lparam);
        int mouse_y = HIWORD(lparam);

        OnMouseHover(mouse_x, mouse_y);
        return 0;
      } break;
      case WM_LBUTTONDOWN:
      {
        int mouse_x = LOWORD(lparam);
        int mouse_y = HIWORD(lparam);

        OnMouseDown(mouse_x, mouse_y);
        return 0;
      } break;
      case WM_LBUTTONUP:
      {
        int mouse_x = LOWORD(lparam);
        int mouse_y = HIWORD(lparam);

        OnMouseUp(mouse_x, mouse_y);
        return 0;
      } break;
      case WM_CHAR:
      {
        OnKeyboartEvent(wparam);
        return 0;
      } break;
      case WM_MOUSEWHEEL:
      {
        OnMouseWheel(wparam, lparam);
        return 0;
      } break;
      default:
      {
        return DefWindowProc(hwnd, msg, wparam, lparam);
      } break;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }


  void Show()
  {
    ShowWindow(m_hwnd, SW_SHOW);
    
    MSG msg;
    const float ms_per_frame = 1000.0 / 60;
    unsigned long long frame = 0;

    IntervalTimePoint start_show = std::chrono::time_point_cast<Interval>(IntervalClock::now());
    while (m_running)
    {
      IntervalTimePoint start_frame_ts = std::chrono::time_point_cast<Interval>(IntervalClock::now());
      BOOL r = 0;

      // UpdateApplicationLibrary();

      while ((r = PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE)) != 0)
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }


      OnPaint();

      IntervalTimePoint end_frame_ts = std::chrono::time_point_cast<Interval>(IntervalClock::now());

      Interval frame_duration = end_frame_ts - start_frame_ts;
      for (; frame_duration.count() < ms_per_frame; frame_duration =  end_frame_ts - start_frame_ts)
      {
        if (frame_duration.count() < ms_per_frame - 2)
          std::this_thread::sleep_for(Interval(1));
        end_frame_ts = std::chrono::time_point_cast<Interval>(IntervalClock::now());
      }

      Interval total_duration = end_frame_ts - start_show;
      logger::Info("Frame: %u, Time: %u", frame % 60, total_duration.count() );

      frame += 1;
    }
  }

  void OnMouseHover(int x, int y)
  {
    application::gui::MouseEvent e;
    e.state = application::gui::MouseState::Move;
    e.x = x;
    e.y = y;
    e.timestamp = platform::CurrentTimestamp();

    application::gui::UserEvent event {
      .type = application::gui::UserEvent::Type::MouseEventType,
      .mouse_event = e,
    };

    m_app->ProcessEvent(&event);
  }

  void OnMouseDown(int x, int y)
  {
    application::gui::MouseEvent e;
    e.state = application::gui::MouseState::Down;
    e.x = x;
    e.y = y;
    e.timestamp = platform::CurrentTimestamp();

    application::gui::UserEvent event {
      .type = application::gui::UserEvent::Type::MouseEventType,
      .mouse_event = e,
    };

    m_app->ProcessEvent(&event);
  }

  void OnMouseUp(int x, int y)
  {
    application::gui::MouseEvent e;
    e.state = application::gui::MouseState::Up;
    e.x = x;
    e.y = y;
    e.timestamp = platform::CurrentTimestamp();


    application::gui::UserEvent event {
      .type = application::gui::UserEvent::Type::MouseEventType,
      .mouse_event = e,
    };
    m_app->ProcessEvent(&event);
  }

  void OnKeyboartEvent(char c)
  {
  }

  void OnMouseWheel(DWORD wparam, DWORD lparam)
  {
  }

  static platform::RenderContext CreateContext(MainWindow* m)
  {
    platform::RenderContext w;
    w.render_target = m->m_render_target;
    w.dwrite_factory = m->m_dwrite_factory;
    w.text_format = m->m_text_format;
    w.text_brush = m->m_text_brush;

    return w;
  }

  // void UpdateApplicationLibrary()
  // {
  //   char executable_path[MAX_PATH];
  //   DWORD len = GetModuleFileName(NULL, executable_path, sizeof(executable_path));

  //   for (int i = len - 1; i >= 0; --i)
  //   {
  //     if (executable_path[i] == '\\')
  //     {
  //       executable_path[i] = '\0';
  //       break;
  //     }
  //   }
  //   
  //   char lib_app_path_1[MAX_PATH];
  //   char lib_app_path_2[MAX_PATH];
  //   strcpy(lib_app_path_1, executable_path);
  //   strcpy(lib_app_path_2, executable_path);

  //   strcat(lib_app_path_1, "\\libapplication.dll");
  //   strcat(lib_app_path_2, "\\application.dll");

  //   HMODULE new_module = LoadLibrary(lib_app_path_1); 
  //   if (new_module == NULL)
  //     new_module = LoadLibrary(lib_app_path_2);
  //   if (new_module == NULL)
  //     return;
  //   FARPROC addr = GetProcAddress(m_application_handle, "Update");
  // }
};

namespace logger
{
  void Init()
  {
    char* level = getenv("LOG_LEVEL");
    if (!level)
    {
      current_log_level = normal;
    }
    else
    {
      char c = level[0];
      current_log_level = c - '0';
    }
  }
}

int main()
{
  try
  {
    logger::Init();
    MainWindow::Register();
    MainWindow t = MainWindow::Instance();

    t.Show();
  }
  catch (std::exception e)
  {
    printf("Exception: %s\n", e.what());
    printf("last error %d\n", GetLastError());
    return 1;
  }
  return 0;
}

