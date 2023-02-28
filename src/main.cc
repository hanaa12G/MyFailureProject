#include <windows.h>

#include <stdio.h>
#include <d2d1.h>
#include <exception>
#include <stdexcept>

template<typename T>
concept WindowInterface = requires (T t)
{
  t.HandleMessage(HWND ,UINT ,WPARAM, LPARAM);
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

  template<typename Interface>
  void SafeRelease(Interface** i)
  {
    (*i)->Release();
    (*i) = 0;
  }
};

class MainWindow
{
private:
  HWND m_hwnd;
  ID2D1Factory* m_direct2d_factory;
  ID2D1HwndRenderTarget* m_render_target; 

  ID2D1SolidColorBrush* m_defautl_color_brush;
public:
  MainWindow()
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
      this);

    if (!hwnd)
      throw std::runtime_error("Can't create window");
    m_hwnd = hwnd;
  }

  ~MainWindow()
  {
    UnregisterClassW(L"MyFailureProject", GetModuleHandle(NULL));
  }


  void CreateGraphicResources()
  {
  }

  void CleanupGraphicResources()
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
    MainWindow t;
    t.Show();
    return 0;
  }
  catch (std::exception e)
  {
    printf("Exception: %s\n", e.what());
    return 1;
  }
}
