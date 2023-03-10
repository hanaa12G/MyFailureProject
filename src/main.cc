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
#include <optional>
#include <thread>

#include "logger.hh"
#include "application.hh"

#include "widget.cc"

using IntervalClock = std::chrono::steady_clock;
using Interval = std::chrono::milliseconds;
using IntervalTimePoint = std::chrono::time_point<IntervalClock, Interval>;


template<typename Interface>
void SafeRelease(Interface** i)
{
  (*i)->Release();
  (*i) = 0;
}

std::set<int> g_instances;

struct UserInput
{

  int x;
  int y; 
  int state;

};

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

  gui::Widget* m_layout;

  bool m_running = true;

  UserInput m_user_input = {};
  gui::InteractionContext m_interaction_context = {};

  int m_width = 0;
  int m_height = 0;


public:
  MainWindow(MainWindow const&) = delete;
  MainWindow(MainWindow&&) = default;
  MainWindow()
  {
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
    SafeRelease(&m_render_target);

    SafeRelease(&m_text_format);
    SafeRelease(&m_dwrite_factory);

    SafeRelease(&m_direct2d_factory);
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



    m_render_target->BeginDraw();

    gui::RenderContext wc = CreateContext(this);
    gui::LayoutConstraint constraint;
    constraint.max_width = m_width;
    constraint.max_height = m_height;
    constraint.x = 0;
    constraint.y = 0;

    gui::LayoutInfo info = m_layout->Layout(&constraint);

    m_layout->SaveLayout(info);
    m_layout->Draw(&wc, m_interaction_context);


    m_render_target->EndDraw();

    SafeRelease(&brush);
    
  }

  void UpdateLayout()
  {
    m_layout = new gui::VerticalContainer();
    auto layout = dynamic_cast<gui::VerticalContainer*>(m_layout);
    layout->PushBack(new gui::Button (
      { gui::WidgetSize::Type::Fixed, 100}, 
      { gui::WidgetSize::Type::Fixed, 100},
      D2D1::ColorF(1.0, 0.2, 0.8, 1.0),
      L"Click me"));

    auto horizontal_items = new gui::HorizontalContainer();

    horizontal_items->PushBack(new gui::Rectangle (
      { gui::WidgetSize::Type::Fixed, 20}, 
      { gui::WidgetSize::Type::Fixed, 20},
      D2D1::ColorF(0.0, 0.4, 0.8, 1.0)));
    horizontal_items->PushBack(new gui::Rectangle (
      { gui::WidgetSize::Type::Fixed, 20}, 
      { gui::WidgetSize::Type::Fixed, 10},
      D2D1::ColorF(0.3, 0.3, 0.2, 1.0)));
    horizontal_items->PushBack(new gui::Rectangle (
      { gui::WidgetSize::Type::Fixed, 20}, 
      { gui::WidgetSize::Type::Fixed, 20},
      D2D1::ColorF(0.3, 0.7, 0.1, 1.0)));
    horizontal_items->PushBack(new gui::Rectangle (
      { gui::WidgetSize::Type::Fixed, 20}, 
      { gui::WidgetSize::Type::Fixed, 10},
      D2D1::ColorF(0.8, 0.3, 0.5, 1.0)));
    horizontal_items->PushBack(new gui::Rectangle (
      { gui::WidgetSize::Type::Fixed, 20}, 
      { gui::WidgetSize::Type::Fixed, 20},
      D2D1::ColorF(0.5, 0.3, 0.5, 1.0)));

    layout->PushBack(horizontal_items);

    layout->PushBack(new gui::TextBox (
      { gui::WidgetSize::Type::Percent, 100},
      { gui::WidgetSize::Type::Percent, 70},
      D2D1::ColorF(0.8, 0.8, 0.8, 1.0)));

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
          SafeRelease(&m_render_target);
    
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
        UpdateLayout();
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
      m_interaction_context.keys_pressed.clear();
      BOOL r = 0;
      while ((r = PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE)) != 0)
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }


      UpdateWidgetStatus();
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

  void UpdateWidgetStatus()
  {
    logger::Debug("Hightlight widget is %p", m_interaction_context.hot);
  }

  void OnMouseHover(int x, int y)
  {
    if (!m_layout)
      return;
    
    gui::Widget* w = m_layout->HitTest(x, y);

    if (w)
    {
      m_interaction_context.hot = w;
    }
    else
    {
      m_interaction_context.hot = NULL;
    }
  }

  void OnMouseDown(int x, int y)
  {
    if (!m_layout)
      return;
    
    gui::Widget* w = m_layout->HitTest(x, y);

    if (w)
    {
      m_interaction_context.about_to_active = w;
    }
    else
    {
      m_interaction_context.about_to_active = NULL;
    }
  }

  void OnMouseUp(int x, int y)
  {
    if (!m_layout)
      return;
    
    gui::Widget* w = m_layout->HitTest(x, y);

    if (w)
    {
      m_interaction_context.about_to_active = w;
      if (w == m_interaction_context.about_to_active)
      {
        m_interaction_context.active = w;
      }
    }
    m_interaction_context.about_to_active = NULL;
  }

  void OnKeyboartEvent(char c)
  {
    if (!m_layout)
      return;
    
    m_interaction_context.keys_pressed.push_back(c);
  }

  static gui::RenderContext CreateContext(MainWindow* m)
  {
    gui::RenderContext w;
    w.render_target = m->m_render_target;
    w.dwrite_factory = m->m_dwrite_factory;
    w.text_format = m->m_text_format;
    w.text_brush = m->m_text_brush;

    return w;
  }
};

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

