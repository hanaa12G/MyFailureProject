#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
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
#include <cmath>
#include <cassert>
#include <array>


namespace gui
{
  template<typename Interface>
  void SafeRelease(Interface** i)
  {
    (*i)->Release();
    (*i) = 0;
  }


  D2D1_COLOR_F btn_bg_default = D2D1::ColorF(0.3, 0.3, 0.3, 1.0);
  D2D1_COLOR_F btn_fg_default = D2D1::ColorF(0.0, 0.0, 0.0, 1.0);
  D2D1_COLOR_F btn_bg_hot     = D2D1::ColorF(0.5, 0.5, 0.5, 1.0);
  D2D1_COLOR_F btn_bg_click     = D2D1::ColorF(0.4, 0.4, 0.6, 1.0);
  D2D1_COLOR_F textbox_bg_default = D2D1::ColorF(1.0, 1.0, 1.0, 1.0);
  D2D1_COLOR_F textbox_fg_default = D2D1::ColorF(0.0, 0.0, 0.0, 1.0);

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
    float value = NAN;

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

  struct Widget;
  struct InteractionContext
  {
    Widget* active = NULL;
    Widget* about_to_active = NULL;
    Widget* hot = {};

    std::vector<char> keys_pressed = {};
  };


  bool IsLayoutInfoValid(LayoutInfo info)
  {
    return info.x != 0 ||
           info.y != 0 ||
           info.width != 0 ||
           info.height != 0;
  }

  struct RenderContext
  {
    ID2D1HwndRenderTarget* render_target = NULL; 
    IDWriteFactory* dwrite_factory = NULL;
    IDWriteTextFormat* text_format = NULL;
    ID2D1SolidColorBrush* text_brush = NULL;
  };

  struct Widget
  {
    virtual ~Widget() {}
    virtual LayoutInfo Layout(LayoutConstraint const*) = 0;
    virtual void SaveLayout(LayoutInfo) = 0;
    virtual void Draw(RenderContext*, InteractionContext) = 0;
    virtual Widget* HitTest(int, int) = 0;
  };

  struct Rectangle : public Widget 
  {
    LayoutInfo* m_info = nullptr; 

    WidgetSize m_width = WidgetSize();
    WidgetSize m_height = WidgetSize();
    D2D1_COLOR_F m_color;
    D2D1_COLOR_F m_highlight_color;

    virtual ~Rectangle()
    {
      delete m_info;
    }

    Rectangle(WidgetSize p_width, WidgetSize p_height, D2D1_COLOR_F p_color)
    : m_width (p_width),
      m_height (p_height),
      m_color (p_color),
      m_highlight_color(D2D1::ColorF(0.7, 0.7, 0.7, 1.0))
    {
    }

    virtual LayoutInfo Layout(LayoutConstraint const* pc) override
    {
      LayoutConstraint const& c = *pc;

      LayoutInfo info = {};
      info.x = c.x;
      info.y = c.y;


      info.width = FindFixSize(m_width, c.max_width);
      info.height = FindFixSize(m_height, c.max_height);

      logger::Debug("INFO (Rectangle): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

      return info;
    }

    virtual void SaveLayout(LayoutInfo info) override
    {
      if (m_info)
        *m_info = info;
      else
        m_info = new LayoutInfo(info);
    }

    virtual void Draw(RenderContext* context, InteractionContext interaction_context) override
    {
      logger::Debug("Rectangle::Draw");
      if (!m_info)
      {
        logger::Error("Call draw without layout");
        return;
      }
      LayoutInfo& layout = *m_info;

      ID2D1SolidColorBrush* brush;
      HRESULT hr = S_OK;
      if (interaction_context.active == this)
      {
        hr = context->render_target->CreateSolidColorBrush(D2D1::ColorF(1.0, 0.0, 0.0, 1.0), &brush);
      }
      else if (interaction_context.hot == this)
      {
        hr = context->render_target->CreateSolidColorBrush(m_highlight_color, &brush);
      }
      else
      {
        hr = context->render_target->CreateSolidColorBrush(m_color, &brush);
      }

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

      SafeRelease(&brush);
    }


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
      }
      assert(false);
      return 0;
    }


    virtual Widget* HitTest(int x, int y) override
    {
      if (!m_info || !IsLayoutInfoValid(*m_info))
        return NULL;
      
      if (x > m_info->x && x < m_info->x + m_info->width &&
          y > m_info->y && y < m_info->y + m_info->height)
      {
        return this;
      }

      return NULL;
    }
  };

  struct VerticalContainer : public Widget
  {
    struct ChildNode {
      Widget* widget;
      LayoutInfo layout;
    };
    std::vector<ChildNode> m_children;

    WidgetSize m_width = {};
    WidgetSize m_height = {};
    LayoutInfo m_info = {};

    virtual ~VerticalContainer()
    {
      for (ChildNode const& w : m_children)
      {
        delete w.widget;
      }
    }

    void PushBack(Widget* w)
    {
      m_children.push_back( {w, {}});
    }

    virtual LayoutInfo Layout(LayoutConstraint const* pc) override
    {
      LayoutConstraint const& c = *pc;

      LayoutInfo info = {};
      info.x = c.x;
      info.y = c.y;
      info.width = 0;
      info.height = 0;

      int x = 0;
      int y = 0;

      info.width = FindMaxSize(m_width, c.max_width);
      info.height = FindMaxSize(m_height, c.max_height);
      
      for (size_t i = 0; ChildNode& w: m_children)
      {
        LayoutConstraint child_constraint;
        child_constraint.max_width = info.width;
        child_constraint.max_height = info.height;
        child_constraint.x = 0;
        child_constraint.y = y;

        LayoutInfo child_layout = w.widget->Layout(&child_constraint);
        w.layout = child_layout;

        x = std::max(x, child_layout.x + child_layout.width);
        y = child_layout.y + child_layout.height;

        i++;
      }

      if (m_width.type != WidgetSize::Type::Fixed)
        info.width = x < info.width ? x : info.width;
      if (m_height.type != WidgetSize::Type::Fixed)
        info.height = y < info.height ? y : info.height;
      logger::Debug("INFO (VerticalContainer): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

      return info;
    }



    virtual void SaveLayout(LayoutInfo info) override
    {
      m_info = info;
      for (ChildNode& node: m_children)
      {
        node.widget->SaveLayout(node.layout);
      }
    }

    virtual void Draw(RenderContext* context, InteractionContext interaction_context) override
    {
      logger::Debug("VerticalContainer::Draw");
      if (!IsLayoutInfoValid(m_info)) return;

      LayoutInfo layout = m_info;


      D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(layout.x, layout.y);
      D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();
      context->render_target->GetTransform(&parent_translation);

      my_translation = parent_translation * my_translation;

      context->render_target->SetTransform(my_translation);
      for (ChildNode& node : m_children)
      {
        node.widget->Draw(context, interaction_context);
      }
      context->render_target->SetTransform(parent_translation);

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
          assert(false);
        } break;
      }
      return 0;
    }


    virtual Widget* HitTest(int x, int y) override
    {
      if (!IsLayoutInfoValid(m_info))
        return NULL;

      Widget* hit = NULL;

      if (x > m_info.x && x < m_info.x + m_info.width &&
          y > m_info.y && y < m_info.y + m_info.height)
      {
        hit = this;
      }

      

      Widget* w = NULL;
      x -= m_info.x;
      y -= m_info.y;
      for (ChildNode node: m_children)
      {
        if (IsLayoutInfoValid(node.layout))
        {
          w = node.widget->HitTest(x, y);
          if (w) break;
        }
      }

      return w ? w : hit;
    }


  };

  struct HorizontalContainer : public Widget
  {
    struct ChildNode {
      Widget* widget;
      LayoutInfo layout;
    };

    std::vector<ChildNode> m_children;

    WidgetSize m_width = {};
    WidgetSize m_height = {};
    LayoutInfo m_info = {};

    virtual ~HorizontalContainer()
    {
      for (ChildNode const& w : m_children)
      {
        delete w.widget;
      }
    }

    void PushBack(Widget* w)
    {
      m_children.push_back( {w, {}});
    }

    virtual LayoutInfo Layout(LayoutConstraint const* pc) override
    {
      LayoutConstraint const& c = *pc;

      LayoutInfo info = {};
      info.x = c.x;
      info.y = c.y;
      info.width = 0;
      info.height = 0;

      int x = 0;
      int y = 0;

      info.width = FindMaxSize(m_width, c.max_width);
      info.height = FindMaxSize(m_height, c.max_height);
      
      for (size_t i = 0; ChildNode& w: m_children)
      {
        LayoutConstraint child_constraint;
        child_constraint.max_width = info.width;
        child_constraint.max_height = info.height;
        child_constraint.x = x;
        child_constraint.y = 0;

        LayoutInfo child_layout = w.widget->Layout(&child_constraint);
        w.layout = child_layout;

        x = child_layout.x + child_layout.width;
        y = std::max(y, child_layout.y + child_layout.height);

        i++;
      }

      if (m_width.type != WidgetSize::Type::Fixed)
        info.width = x < info.width ? x : info.width;
      if (m_height.type != WidgetSize::Type::Fixed)
        info.height = y < info.height ? y : info.height;
      logger::Debug("INFO (HorizontalContainer): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

      return info;
    }



    virtual void SaveLayout(LayoutInfo info) override
    {
      m_info = info;
      for (ChildNode& node: m_children)
      {
        node.widget->SaveLayout(node.layout);
      }
    }

    virtual void Draw(RenderContext* context, InteractionContext interaction_context) override
    {
      logger::Debug("HorizontalContainer::Draw");
      if (!IsLayoutInfoValid(m_info)) return;

      LayoutInfo layout = m_info;


      D2D1_MATRIX_3X2_F my_translation = D2D1::Matrix3x2F::Translation(layout.x, layout.y);
      D2D1_MATRIX_3X2_F parent_translation = D2D1::Matrix3x2F::Identity();
      context->render_target->GetTransform(&parent_translation);

      my_translation = parent_translation * my_translation;

      context->render_target->SetTransform(my_translation);
      for (ChildNode& node : m_children)
      {
        node.widget->Draw(context, interaction_context);
      }
      context->render_target->SetTransform(parent_translation);

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
          assert(false);
        } break;
      }
      return 0;
    }

    virtual Widget* HitTest(int x, int y) override
    {
      if (!IsLayoutInfoValid(m_info))
        return NULL;

      Widget* hit = NULL;

      if (x > m_info.x && x < m_info.x + m_info.width &&
          y > m_info.y && y < m_info.y + m_info.height)
      {
        hit = this;
      }

      Widget* w = NULL;
      x -= m_info.x;
      y -= m_info.y;
      for (ChildNode node: m_children)
      {
        if (IsLayoutInfoValid(node.layout))
        {
          w = node.widget->HitTest(x, y);
          if (w) break;
        }
      }

      return w ? w : hit;
    }


  };

  struct Button: public Rectangle 
  {
    std::wstring text;
    D2D1_COLOR_F fg = btn_fg_default;
    D2D1_COLOR_F bg = btn_bg_default;

    virtual ~Button()
    {
    }

    Button(WidgetSize p_width, WidgetSize p_height, D2D1_COLOR_F p_color, std::wstring txt)
    : Rectangle(p_width, p_height, p_color)
    {
      text = txt;
    }

    virtual void Draw(RenderContext* context, InteractionContext interaction_context) override
    {
      logger::Debug("Button::Draw");
      if (!m_info)
      {
        logger::Error("Call draw without layout");
        return;
      }
      LayoutInfo& layout = *m_info;

      ID2D1SolidColorBrush* brush;
      HRESULT hr = S_OK;
      if (interaction_context.about_to_active == this)
      {
        hr = context->render_target->CreateSolidColorBrush(btn_bg_click, &brush);
      }
      else if (interaction_context.hot == this)
      {
        hr = context->render_target->CreateSolidColorBrush(btn_bg_hot, &brush);
      }
      else
      {
        hr = context->render_target->CreateSolidColorBrush(btn_bg_default, &brush);
      }

      if (FAILED(hr)) return;

      ID2D1SolidColorBrush* text_brush = NULL;
      hr = context->render_target->CreateSolidColorBrush(
        fg,
        &text_brush);
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
      context->render_target->DrawText(
        text.c_str(),
        text.size(),
        context->text_format,
        rec,
        text_brush);

      context->render_target->SetTransform(parent_translation);

      SafeRelease(&brush);
      SafeRelease(&text_brush);
    }
  };

  struct TextBox: public Rectangle 
  {
    std::wstring text;

    virtual ~TextBox()
    {
    }

    TextBox(WidgetSize p_width, WidgetSize p_height, D2D1_COLOR_F p_color)
    : Rectangle(p_width, p_height, p_color),
      text()
    {
    }

    virtual void Draw(RenderContext* context, InteractionContext interaction_context) override
    {
      logger::Debug("TextBox::Draw");
      if (!m_info)
      {
        logger::Error("Call draw without layout");
        return;
      }
      LayoutInfo& layout = *m_info;

      for (char c : interaction_context.keys_pressed)
      {
        text.push_back((wchar_t) c);
      }


      ID2D1SolidColorBrush* brush;
      HRESULT hr = S_OK;

      hr = context->render_target->CreateSolidColorBrush(textbox_bg_default, &brush);

      if (FAILED(hr)) return;

      ID2D1SolidColorBrush* text_brush = NULL;
      hr = context->render_target->CreateSolidColorBrush(
        textbox_fg_default,
        &text_brush);
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

      static unsigned long n = 0;

      std::wstring tmp_text = text;

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

      context->render_target->DrawText(
        tmp_text.c_str(),
        tmp_text.size(),
        context->text_format,
        rec,
        text_brush);

      context->render_target->SetTransform(parent_translation);

      SafeRelease(&brush);
      SafeRelease(&text_brush);
    }
  };


}

