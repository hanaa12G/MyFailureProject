#pragma once

#include <utility>
#include <string>
#include <vector>
#include "logger.hh"
#include "platform.hh"
#include <functional>
#include <map>

namespace application
{

namespace gui
{

  enum WidgetType
  {
    RectangleType,
    VerticalContainerType,
    HorizontalContainerType,
    ButtonType,
    TextBoxType,
    LayersType
  };

  struct Color
  {
    float r;
    float g;
    float b;
    float a;

    Color(float pr, float pg, float pb, float pa)
    : r(pr), g(pg), b(pb), a(pa)
    {}

    Color()
    : r(0.0), g(0.0), b(0.0), a(1.0)
    {
    }
  };

  struct WidgetSize
  {
    enum Type
    {
      Undefined,
      Fixed,
      Percent,
      Ratio,
    };

    Type type = Type::Undefined;
    float value = 0;
  };

  struct LayoutConstraint
  {
    int max_width = 0;
    int max_height = 0;

    int x = 0;
    int y = 0;
  };

  struct LayoutInfo
  {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
  };

  struct InteractionContext
  {
    Widget* active;
    Widget* hot;
    Widget* about_to_active;

    std::vector<wchar_t> keys_pressed;
  };
  inline bool IsLayoutInfoValid(LayoutInfo info)
  {
    return info.x != 0 ||
           info.y != 0 ||
           info.width != 0 ||
           info.height != 0;
  }

  struct Widget
  {
    virtual ~Widget() {}
    virtual void Layout(LayoutConstraint const&) = 0;
    virtual void Draw(platform::RenderContext*, InteractionContext) = 0;
    virtual Widget* HitTest(int, int) = 0;
    virtual LayoutInfo& GetLayout() = 0;

    virtual std::string const& GetId()
    {
      return m_id;
    }
    virtual void SetId(std::string const& id)
    {
      m_id = id;
    }


    std::string m_id;
  };


  struct Rectangle : public Widget 
  {
    WidgetSize m_width = WidgetSize();
    WidgetSize m_height = WidgetSize();
    Color m_bg_default_color = {};
    Color m_bg_active_color = {};

    LayoutInfo m_layout;


    void SetColor(Color c)
    {
      m_bg_default_color = c;
    }

    void SetActiveColor(Color c)
    {
      m_bg_active_color = c;
    }

    void SetWidth(WidgetSize w)
    {
      m_width = w;
    }

    void SetHeight(WidgetSize w)
    {
      m_height = w;
    }

    virtual void Layout(LayoutConstraint const& c) override
    {
      LayoutInfo info = {};
      info.x = c.x;
      info.y = c.y;


      info.width = FindFixSize(m_width, c.max_width);
      info.height = FindFixSize(m_height, c.max_height);

      logger::Debug("INFO (Rectangle): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

      m_layout = info;
    }

    virtual LayoutInfo& GetLayout() override { return m_layout; }

    int FindFixSize(WidgetSize size, int max_size)
    {
      switch (size.type)
      {
        case WidgetSize::Type::Undefined:
        {
          logger::Warning("[GUI] [Rectangle] size is set to 0");
          return 0;
        } break;
        case WidgetSize::Type::Ratio:
        {
          return max_size * size.value;
        } break;
        case WidgetSize::Type::Percent:
        {
          return max_size * size.value / 100.0f;
        } break;
        case WidgetSize::Type::Fixed:
        {
          int width = 0;
          if (max_size < size.value)
          {
            width = max_size;
          }
          else
          {
            width = size.value;
          }
          return width;
        } break;

        default:
        {
          std::unreachable();
        }
      }
      return 0;
    }

    virtual Widget* HitTest(int x, int y) override
    {
      if (IsLayoutInfoValid(m_layout))
      {
        if (x > m_layout.x && x < m_layout.x + m_layout.width &&
            y > m_layout.y && y < m_layout.y + m_layout.height)
        {
          return this;
        }
      }
      return NULL;
    }
  };

  struct VerticalContainer : public Widget
  {
    std::vector<Widget*> m_children;

    WidgetSize m_width = {};
    WidgetSize m_height = {};
    LayoutInfo m_layout = {};

    virtual ~VerticalContainer()
    {
      for (Widget* w : m_children)
      {
        delete w;
      }
    }

    void PushBack(Widget* w)
    {
      m_children.push_back(w);
    }

    void SetWidth(WidgetSize w)
    {
      m_width = w;
    }

    void SetHeight(WidgetSize w)
    {
      m_height = w;
    }

    virtual void Layout(LayoutConstraint const& c) override
    {
      LayoutInfo info = {};
      info.x = c.x;
      info.y = c.y;
      info.width = 0;
      info.height = 0;

      int x = 0;
      int y = 0;

      info.width = FindMaxSize(m_width, c.max_width);
      info.height = FindMaxSize(m_height, c.max_height);
      
      for (size_t i = 0; Widget* w: m_children)
      {
        LayoutConstraint child_constraint;
        child_constraint.max_width = info.width;
        child_constraint.max_height = info.height;
        child_constraint.x = 0;
        child_constraint.y = y;

        w->Layout(child_constraint);
        LayoutInfo child_layout = w->GetLayout();

        x = std::max(x, child_layout.x + child_layout.width);
        y = child_layout.y + child_layout.height;

        i++;
      }

      if (m_width.type != WidgetSize::Type::Fixed)
        info.width = x < info.width ? x : info.width;
      if (m_height.type != WidgetSize::Type::Fixed)
        info.height = y < info.height ? y : info.height;
      logger::Debug("INFO (VerticalContainer): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

      m_layout=info;
    }
    

    virtual LayoutInfo& GetLayout() override { return m_layout; }

    int FindMaxSize(WidgetSize size, int max_size)
    {
      switch (size.type)
      {
        case WidgetSize::Type::Undefined:
        {
          return max_size;
        } break;
        case WidgetSize::Type::Ratio:
        {
          return max_size * size.value;
        } break;
        case WidgetSize::Type::Percent:
        {
          return max_size * size.value / 100;
        } break;
        case WidgetSize::Type::Fixed:
        {
          return size.value > max_size ? max_size : size.value;
        } break;
        default:
        {
          std::unreachable();
        } break;
      }
      return 0;
    }


    virtual Widget* HitTest(int x, int y) override
    {
      if (!IsLayoutInfoValid(m_layout))
        return NULL;

      Widget* hit = NULL;

      if (x > m_layout.x && x < m_layout.x + m_layout.width &&
          y > m_layout.y && y < m_layout.y + m_layout.height)
      {
        hit = this;
      }

      

      Widget* w = NULL;
      x -= m_layout.x;
      y -= m_layout.y;
      for (Widget* widget: m_children)
      {
        if (IsLayoutInfoValid(widget->GetLayout()))
        {
          w = widget->HitTest(x, y);
          if (w) break;
        }
      }

      return w ? w : hit;
    }


  };

  struct HorizontalContainer : public Widget
  {
    std::vector<Widget*> m_children;

    WidgetSize m_width = {};
    WidgetSize m_height = {};
    LayoutInfo m_layout = {};

    virtual ~HorizontalContainer()
    {
      for (Widget* w : m_children)
      {
        delete w;
      }
    }

    void PushBack(Widget* w)
    {
      m_children.push_back(w);
    }

    virtual void Layout(LayoutConstraint const& c) override
    {
      LayoutInfo info = {};
      info.x = c.x;
      info.y = c.y;
      info.width = 0;
      info.height = 0;

      int x = 0;
      int y = 0;

      info.width = FindMaxSize(m_width, c.max_width);
      info.height = FindMaxSize(m_height, c.max_height);
      
      for (size_t i = 0; Widget* w: m_children)
      {
        LayoutConstraint child_constraint;
        child_constraint.max_width = info.width;
        child_constraint.max_height = info.height;
        child_constraint.x = x;
        child_constraint.y = 0;

        w->Layout(child_constraint);
        LayoutInfo const& child_layout = w->GetLayout();

        x = child_layout.x + child_layout.width;
        y = std::max(y, child_layout.y + child_layout.height);

        i++;
      }

      if (m_width.type != WidgetSize::Type::Fixed)
        info.width = x < info.width ? x : info.width;
      if (m_height.type != WidgetSize::Type::Fixed)
        info.height = y < info.height ? y : info.height;
      logger::Debug("INFO (HorizontalContainer): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);
    }

    virtual LayoutInfo& GetLayout() override { return m_layout; }

    int FindMaxSize(WidgetSize size, int max_size)
    {
      switch (size.type)
      {
        case WidgetSize::Type::Undefined:
        {
          return max_size;
        } break;
        case WidgetSize::Type::Ratio:
        {
          return max_size * size.value;
        } break;
        case WidgetSize::Type::Percent:
        {
          return max_size * size.value / 100;
        } break;
        case WidgetSize::Type::Fixed:
        {
          return size.value > max_size ? max_size : size.value;
        } break;
        default:
        {
          std::unreachable();
        } break;
      }
      return 0;
    }

    virtual Widget* HitTest(int x, int y) override
    {
      if (!IsLayoutInfoValid(m_layout))
        return NULL;

      Widget* hit = NULL;

      if (x > m_layout.x && x < m_layout.x + m_layout.width &&
          y > m_layout.y && y < m_layout.y + m_layout.height)
      {
        hit = this;
      }

      Widget* w = NULL;
      x -= m_layout.x;
      y -= m_layout.y;
      for (Widget* widget: m_children)
      {
        if (IsLayoutInfoValid(widget->GetLayout()))
        {
          w = widget->HitTest(x, y);
          if (w) break;
        }
      }

      return w ? w : hit;
    }


  };

  struct Button: public Rectangle 
  {
    std::wstring m_text;
    Color m_bg_default_color;
    Color m_fg_default_color;
    Color m_bg_active_color;
    Color m_fg_active_color;
    
    void SetColor(Color c)
    {
      m_bg_default_color = c;
    }

    void SetActiveColor(Color c)
    {
      m_bg_active_color = c;
    }

    void SetTextColor(Color c)
    {
      m_fg_default_color = c;
    }
    void SetTextActiveColor(Color c)
    {
      m_fg_active_color = c;
    }

    void SetWidth(WidgetSize w)
    {
      m_width = w;
    }

    void SetHeight(WidgetSize w)
    {
      m_height = w;
    }
    void SetText(std::wstring text)
    {
      m_text = text;
    }
  };

  struct TextBox: public Rectangle 
  {
    std::wstring m_text;

    Color m_fg_default_color;

    void SetColor(Color c)
    {
      m_bg_default_color = c;
    }

    void SetTextColor(Color c)
    {
      m_fg_default_color = c;
    }

    void SetActiveColor(Color c)
    {
      m_bg_active_color = c;
    }

    void SetWidth(WidgetSize w)
    {
      m_width = w;
    }

    void SetHeight(WidgetSize w)
    {
      m_height = w;
    }


    std::wstring const& GetText()
    {
      return m_text;
    }
  };


  struct Layers : public Widget
  {
    std::map<int, Widget*> m_layers;
    LayoutInfo m_layout;
    WidgetSize m_width;
    WidgetSize m_height;


    virtual void Layout(LayoutConstraint const& c)
    {
      LayoutInfo info = {};
      info.x = c.x;
      info.y = c.y;
      info.width = 0;
      info.height = 0;

      int x = 0;
      int y = 0;

      info.width = FindMaxSize(m_width, c.max_width);
      info.height = FindMaxSize(m_height, c.max_height);

      LayoutConstraint layer_constraint = {
        .max_width = info.width,
        .max_height = info.height,
        .x = c.x,
        .y = c.y
      };

      for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
      {
        i->second->Layout(layer_constraint);
      }

      logger::Debug("INFO (Layers): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

      m_layout = info;
    }

    LayoutInfo& GetLayout() override
    {
      return m_layout;
    }

    void SetLayer(int layer, Widget* w)
    {
      m_layers[layer] = w;
    }

    Widget* HitTest(int x, int y) override
    {
      if (m_layers.count(1) != 0)
      {
        return m_layers[1]->HitTest(x, y);
      }
      if (m_layers.count(0) != 0)
      {
        return m_layers[0]->HitTest(x, y);
      }

      return NULL;
    }

    int FindMaxSize(WidgetSize size, int max_size)
    {
      switch (size.type)
      {
        case WidgetSize::Type::Undefined:
        {
          return max_size;
        } break;
        case WidgetSize::Type::Ratio:
        {
          return max_size * size.value;
        } break;
        case WidgetSize::Type::Percent:
        {
          return max_size * size.value / 100;
        } break;
        case WidgetSize::Type::Fixed:
        {
          return size.value > max_size ? max_size : size.value;
        } break;
        default:
        {
          std::unreachable();
        } break;
      }
      return 0;
    }

  };


  enum MouseState
  {
    MouseDown,
    MouseUp
  };

  struct UserInput
  {
    int mouse_x;
    int mouse_y;
    int mouse_state;
    int mouse_half_transitions;

    std::vector<wchar_t> keys_pressed;
  };

  struct Application
  {
    Widget* m_widget = NULL;
    InteractionContext m_interaction_context;

    Application();
    void InitLayout();

    void ProcessEvent(UserInput*);
    void Render(LayoutConstraint*, platform::RenderContext*);

    void SaveToFile(std::wstring const& content, std::function<void()>);
    void SaveFileSuccessfullyCallback();
  };

} // namespace gui



} // namespace application



