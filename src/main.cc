#define NOMINMAX
#include <windows.h>

#include <stdio.h>
#include <d2d1.h>
#include <dwrite.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>



template<typename Interface>
void SafeRelease(Interface** i)
{
  (*i)->Release();
  (*i) = 0;
}



template<typename ChildWindow>
struct Window
{
  static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
  {
    printf("msg: %u\n", msg);
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

  struct WidgetContext
  {
    ID2D1HwndRenderTarget* render_target = NULL; 
    IDWriteFactory* dwrite_factory = NULL;
    IDWriteTextFormat* text_format = NULL;
    ID2D1SolidColorBrush* text_brush = NULL;


    static WidgetContext CreateContext(MainWindow* m)
    {
      WidgetContext w;
      w.render_target = m->m_render_target;
      w.dwrite_factory = m->m_dwrite_factory;
      w.text_format = m->m_text_format;
      w.text_brush = m->m_text_brush;

      return w;
    }
  };

  struct Widget
  {
    int width;
    int height;
    D2D1_COLOR_F color;

    Widget(int p_width, int p_height, D2D1_COLOR_F p_color)
    : width (p_width),
      height (p_height),
      color (p_color)
    {
    }

    virtual ~Widget() {}

    virtual void Draw(WidgetContext context)
    {
      printf("Widget::Draw\n");
      ID2D1SolidColorBrush* brush;
      HRESULT hr = S_OK;
      hr = context.render_target->CreateSolidColorBrush(color, &brush);

      if (FAILED(hr)) return;

      D2D1_RECT_F rec = D2D1::RectF(
        0, 0, width, height);

      context.render_target->FillRectangle(
        rec,
        brush);
    }
  };

  struct Button: public Widget
  {
    std::wstring const mk_text;

    virtual ~Button() {}
    
    Button(int p_width, int p_height, std::wstring p_text, D2D1_COLOR_F p_color)
    : Widget(p_width, p_height, p_color),
      mk_text (p_text)
    {
    }

    virtual void Draw(WidgetContext context) override
    {
      printf("Button::Draw\n");
      ID2D1SolidColorBrush* brush;
      HRESULT hr = S_OK;
      hr = context.render_target->CreateSolidColorBrush(color, &brush);

      if (FAILED(hr)) return;

      D2D1_RECT_F rec = D2D1::RectF(
        0, 0, width, height);

      context.render_target->DrawText(
        mk_text.c_str(),
        mk_text.size(),
        context.text_format,
        rec,
        brush);
    }
  };

  class Layout 
  {
  private:
    std::vector<std::vector<Widget*>> m_widgets;
  public:

    void InitLayout()
    {
      std::vector<Widget*> rows;

      rows.push_back(new Widget(10, 50,
            D2D1::ColorF(0.8, 0.2, 0.2, 1.0)
            ));

      rows.push_back(new Button(70, 70,
        L"Click me",
        D2D1::ColorF(1.0, 1.0, 1.0, 1.0)));

      m_widgets.push_back(rows);

      rows.clear();

      rows.push_back(new Widget(30, 60,
            D2D1::ColorF(0.1, 0.2, 0.5, 1.0)));

      rows.push_back(new Widget(60, 20,
            D2D1::ColorF(0.4, 0.4, 0.4, 1.0)));

      m_widgets.push_back(rows);
    }

    ~Layout()
    {
      for (auto i = m_widgets.begin(); i != m_widgets.end(); ++i)
        for (auto j = i->begin(); j != i->end(); ++j)
        {
          Widget* w = *j;
          if (w) delete w;
        }
    }


    void Draw(WidgetContext context)
    {
      int x = 0;
      int y = 0;
      for (int i = 0; i < m_widgets.size(); ++i)
      {
        int max_y = 0;
        x = 0;
        for (int j = 0; j < m_widgets.at(i).size(); ++j)
        {
          D2D1_MATRIX_3X2_F translation = D2D1::Matrix3x2F::Translation (x, y);

          context.render_target->SetTransform(translation);

          Widget* w = m_widgets[i][j];
          w->Draw(context);
          max_y = std::max(w->height, max_y);
          x +=  w->width;
        }

        y += max_y;
      }
    }
  };


  HWND m_hwnd = NULL;
  ID2D1Factory* m_direct2d_factory = NULL;
  ID2D1HwndRenderTarget* m_render_target = NULL; 

  IDWriteFactory* m_dwrite_factory = NULL;
  IDWriteTextFormat* m_text_format = NULL;
  ID2D1SolidColorBrush* m_text_brush = NULL;

  Layout m_layout;

public:
  MainWindow(MainWindow const&) = delete;
  MainWindow(MainWindow&&) = default;
  MainWindow() = default;

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
      CW_USEDEFAULT, CW_USEDEFAULT,
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

    WidgetContext wc = WidgetContext::CreateContext(this);
    m_layout.Draw(wc);

    m_render_target->EndDraw();

    SafeRelease(&brush);
    
  }

  LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
  {
    printf("Child msg: %u, %d\n", msg, GetLastError());
    switch (msg)
    {
      case WM_CREATE:
      {
        try {
          CreateGraphicResources();
          m_layout.InitLayout();
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

      case WM_DESTROY:
      {
        CleanupGraphicResources();
        PostQuitMessage(0);
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
    BOOL r;
    while ((r = GetMessage(&msg, m_hwnd, 0, 0)) != 0)
    {
      if (r == -1)
      {
        return;
      }
      else
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }
};

int main()
{
  try
  {
    MainWindow::Register();
    MainWindow t = MainWindow::Instance();


    t.Show();
  }
  catch (std::exception e)
  {
    printf("Exception: %s\n", e.what());
    return 1;
  }
  printf("Final last error %d\n", GetLastError());
  return 0;
}
//
