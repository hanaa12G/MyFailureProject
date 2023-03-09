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


namespace logger
{
  static const int debug=4;
  static const int info=3;
  static const int normal=2;
  static const int warning=1;
  static const int error=0;
  static int current_log_level = normal;

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

struct WidgetContext
{
  ID2D1HwndRenderTarget* render_target = NULL; 
  IDWriteFactory* dwrite_factory = NULL;
  IDWriteTextFormat* text_format = NULL;
  ID2D1SolidColorBrush* text_brush = NULL;
};
struct LayoutConstraint
{
  std::optional<int> max_width;
  std::optional<int> max_height;
  int x;
  int y;
};

struct LayoutInfo
{
  int x;
  int y;
  int width;
  int height;
};


namespace widget
{

struct Widget
{
  virtual ~Widget() {}
  virtual LayoutInfo Layout(LayoutConstraint) = 0;
  virtual void SaveLayout(LayoutInfo) = 0;
  virtual void Draw(WidgetContext*) = 0;
};

struct WidgetSize
{
  enum Type {
    Undefined,
    Fixed,
    Ratio,
  };

  Type  type = Type::Undefined;
  float value = 0.0f;

  operator bool()
  {
    return type != Type::Undefined;
  }

  static WidgetSize FixSize(float pixel)
  {
    return WidgetSize {
      Type::Fixed,
      pixel
    };
  }
  static WidgetSize RatioSize(float ratio)
  {
    return WidgetSize {
      Type::Ratio,
      ratio
    };
  }
};

WidgetSize operator ""_px(long double pixel)
{
  return WidgetSize::FixSize(pixel);
}
WidgetSize operator ""_px(unsigned long long int pixel)
{
  return WidgetSize::FixSize(pixel);
}

WidgetSize operator ""_pc(unsigned long long int pixel)
{
  return WidgetSize::RatioSize(pixel);
}
WidgetSize operator ""_pc(long double pixel)
{
  return WidgetSize::RatioSize(pixel);
}


struct Rectangle : public Widget 
{
  std::optional<LayoutInfo> m_info; 

  WidgetSize m_width;
  WidgetSize m_height;
  D2D1_COLOR_F m_color;

  virtual ~Rectangle()
  {
  }

  Rectangle(WidgetSize p_width, WidgetSize p_height, D2D1_COLOR_F p_color)
  : m_width (p_width),
    m_height (p_height),
    m_color (p_color)
  {
  }

  virtual LayoutInfo Layout(LayoutConstraint c) override
  {
    LayoutInfo info = {};
    info.x = c.x;
    info.y = c.y;

    if (c.max_width)
    {
      info.width = c.max_width.value() > m_width ? m_width : c.max_width.value();
    }
    else
    {
      info.width = m_width;
    }

    if (c.max_height)
    {
      info.height = c.max_height.value() > m_height ? m_height : c.max_height.value();
    }
    else
    {
      info.height = m_height;
    }

    logger::Debug("INFO (Rectangle): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

    return info;
  }

  virtual void SaveLayout(LayoutInfo info) override
  {
    m_info = info;
  }

  virtual void Draw(WidgetContext* context) override
  {
    logger::Debug("Rectangle::Draw");
    if (!m_info) return;
    LayoutInfo layout = m_info.value();

    ID2D1SolidColorBrush* brush;
    HRESULT hr = S_OK;
    hr = context->render_target->CreateSolidColorBrush(m_color, &brush);

    if (FAILED(hr)) return;

    D2D1_RECT_F rec = D2D1::RectF(
      0, 0,
      layout.width, layout.height);

    D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(layout.x, layout.y);

    D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();

    context->render_target->GetTransform(&parent_translation);

    my_translation = parent_translation * my_translation;

    context->render_target->SetTransform(my_translation);

    context->render_target->FillRectangle(
      rec,
      brush
    );

    context->render_target->SetTransform(parent_translation);

  }
};

struct VerticalContainer : public Widget
{
  std::vector<Widget*> m_children;
  std::vector<LayoutInfo> m_children_layout;

  int m_width;
  int m_height;
  std::optional<LayoutInfo> m_info;


  virtual ~VerticalContainer()
  {
  }

  virtual LayoutInfo Layout(LayoutConstraint c) override
  {
    LayoutInfo info = {};
    info.x = c.x;
    info.y = c.y;
    info.width = 0;
    info.height = 0;

    int x = 0;
    int y = 0;

    int max_width = m_width;
    if (c.max_width)
    {
      max_width = c.max_width.value();
    }
    int max_height = m_height;
    if (c.max_height)
    {
      max_height = c.max_height.value();
    }

    m_children_layout.clear();
    for (Widget* w: m_children)
    {
      LayoutConstraint child_constraint;
      child_constraint.max_width = max_width;
      child_constraint.max_height = max_height;
      child_constraint.x = 0;
      child_constraint.y = y;

      LayoutInfo child_layout = w->Layout(child_constraint);
      m_children_layout.push_back(child_layout);


      x = std::max(x, child_layout.x + child_layout.width);
      y = child_layout.y + child_layout.height;
    }

    info.width = x;
    info.height = y;
    logger::Debug("INFO (VerticalContainer): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

    return info;
  }

  virtual void SaveLayout(LayoutInfo info) override
  {
    
    m_info = info;
    for (size_t i = 0; i < m_children.size(); ++i)
    {
      m_children.at(i)->SaveLayout(m_children_layout.at(i));
    }
  }

  virtual void Draw(WidgetContext* context) override
  {
    logger::Debug("VerticalContainer::Draw");
    if (!m_info) return;
    LayoutInfo layout = m_info.value();


    D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(layout.x, layout.y);
    D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();
    context->render_target->GetTransform(&parent_translation);

    my_translation = parent_translation * my_translation;

    context->render_target->SetTransform(my_translation);
    for (Widget* w : m_children)
    {
      w->Draw(context);
    }
    context->render_target->SetTransform(parent_translation);

  }


};

struct HorizontalContainer : public Widget
{
  std::vector<Widget*> m_children;
  std::vector<LayoutInfo> m_children_layout;

  int m_width;
  int m_height;
  std::optional<LayoutInfo> m_info;


  virtual ~HorizontalContainer()
  {
  }

  virtual LayoutInfo Layout(LayoutConstraint c) override
  {
    LayoutInfo info = {};
    info.x = c.x;
    info.y = c.y;
    info.width = 0;
    info.height = 0;

    int x = 0;
    int y = 0;

    int max_width = m_width;
    if (c.max_width)
    {
      max_width = c.max_width.value();
    }
    int max_height = m_height;
    if (c.max_height)
    {
      max_height = c.max_height.value();
    }

    m_children_layout.clear();
    for (Widget* w: m_children)
    {
      LayoutConstraint child_constraint;
      child_constraint.max_width = max_width;
      child_constraint.max_height = max_height;
      child_constraint.x = x;
      child_constraint.y = 0;

      LayoutInfo child_layout = w->Layout(child_constraint);
      m_children_layout.push_back(child_layout);


      x = child_layout.x + child_layout.width;
      y = std::max(y, child_layout.y + child_layout.height);
    }

    info.width = x;
    info.height = y;
    logger::Debug("INFO (HorizontalContainer): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

    return info;
  }

  virtual void SaveLayout(LayoutInfo info) override
  {
    
    m_info = info;
    for (size_t i = 0; i < m_children.size(); ++i)
    {
      m_children.at(i)->SaveLayout(m_children_layout.at(i));
    }
  }

  virtual void Draw(WidgetContext* context) override
  {
    logger::Debug("HorizontalContainer::Draw");
    if (!m_info) return;
    LayoutInfo layout = m_info.value();


    D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(layout.x, layout.y);
    D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();
    context->render_target->GetTransform(&parent_translation);

    my_translation = parent_translation * my_translation;

    context->render_target->SetTransform(my_translation);
    for (Widget* w : m_children)
    {
      w->Draw(context);
    }
    context->render_target->SetTransform(parent_translation);

  }


};
}



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

  UserInput m_user_input;

public:
  MainWindow(MainWindow const&) = delete;
  MainWindow(MainWindow&&) = default;
  MainWindow()
  {
    InitLayout();
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
    constraint.max_width = 640;
    constraint.max_height = 480;
    constraint.x = 0;
    constraint.y = 0;

    gui::LayoutInfo info = m_layout->Layout(&constraint);

    m_layout->SaveLayout(info);
    m_layout->Draw(&wc);


    m_render_target->EndDraw();

    SafeRelease(&brush);
    
  }

  void InitLayout()
  {
    // m_layout = new gui::Rectangle(
    //   {gui::WidgetSize::Type::Fixed, 20},
    //   {gui::WidgetSize::Type::Fixed, 20},
    //   D2D1::ColorF(1.0, 0.2, 0.8, 1.0)
    // );

    m_layout = new gui::VerticalContainer();
    auto layout = dynamic_cast<gui::VerticalContainer*>(m_layout);
    layout->PushBack(new gui::Rectangle (
      { gui::WidgetSize::Type::Fixed, 100}, 
      { gui::WidgetSize::Type::Fixed, 100},
      D2D1::ColorF(1.0, 0.2, 0.8, 1.0)));

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

    layout->PushBack(new gui::Rectangle (
      { gui::WidgetSize::Type::Percent, 100},
      { gui::WidgetSize::Type::Fixed, 30},
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

        return 0;
      } break;
      case WM_LBUTTONDOWN:
      {
        printf("Button down\n");
        return 0;
      } break;
      case WM_LBUTTONUP:
      {
        printf("Button up\n");
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
        end_frame_ts = std::chrono::time_point_cast<Interval>(IntervalClock::now());
      }

      Interval total_duration = end_frame_ts - start_show;
      logger::Info("Frame: %u, Time: %u", frame % 60, total_duration.count() );

      frame += 1;
    }
  }

  void UpdateWidgetStatus()
  {
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

