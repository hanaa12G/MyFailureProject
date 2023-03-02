#include <windows.h>

#include <stdio.h>
#include <d2d1.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <format>

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

  ID2D1SolidColorBrush* m_defautl_color_brush = NULL;
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

    if (FAILED(hr)) throw std::runtime_error(std::format("Failed to create graphic: {:x}", hr));

    RECT rc;
    GetClientRect(m_hwnd, &rc);

    D2D1_SIZE_U size = D2D1::SizeU(
      rc.right - rc.left,
      rc.bottom - rc.top);

    hr = m_direct2d_factory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(),
      D2D1::HwndRenderTargetProperties(m_hwnd, size),
      &m_render_target);

    if (FAILED(hr)) throw std::runtime_error(std::format("Failed to create render target: {:x}", hr));
  }

  void CleanupGraphicResources()
  {
    SafeRelease(&m_render_target);
    SafeRelease(&m_direct2d_factory);
  }

  void OnPaint()
  {
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
    ShowWindow(m_hwnd, SW_RESTORE);
    
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
    return 0;
  }
  catch (std::exception e)
  {
    printf("Exception: %s\n", e.what());
    return 1;
  }
}
//
