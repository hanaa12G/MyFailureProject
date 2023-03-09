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


namespace gui
{


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
    virtual void Draw(RenderContext*) = 0;
  };

  struct Rectangle : public Widget 
  {
    LayoutInfo* m_info = nullptr; 

    WidgetSize m_width = WidgetSize();
    WidgetSize m_height = WidgetSize();
    D2D1_COLOR_F m_color;

    virtual ~Rectangle()
    {
      delete m_info;
    }

    Rectangle(WidgetSize p_width, WidgetSize p_height, D2D1_COLOR_F p_color)
    : m_width (p_width),
      m_height (p_height),
      m_color (p_color)
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

    virtual void Draw(RenderContext* context) override
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

    virtual void Draw(RenderContext* context) override
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
        node.widget->Draw(context);
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

    virtual void Draw(RenderContext* context) override
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
        node.widget->Draw(context);
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


  };


}

