#include "application.hh"

#include <cassert>



namespace application
{
namespace gui
{

  Widget* FindId(Widget* root_widget, std::string const& id)
  {
    if (id == root_widget->GetId())
      return root_widget;

    VerticalContainer* vert_ptr = dynamic_cast<VerticalContainer*>(root_widget);
    Widget* ret = NULL;

    if (vert_ptr)
    {
      for (Widget* widget: vert_ptr->m_children)
      {
        ret = FindId(widget, id);
        if (ret) return ret;
      }
    }

    HorizontalContainer* hor_ptr = dynamic_cast<HorizontalContainer*>(root_widget);
    if (hor_ptr)
    {
      for (Widget* widget: hor_ptr->m_children)
      {
        ret = FindId(widget, id);
        if (ret) return ret;
      }
    }

    return NULL;
  }

  Application::Application()
  {
    InitLayout();
  }

  void Application::InitLayout()
  {
    m_widget = platform::NewWidget(WidgetType::VerticalContainerType);

    auto vertical_container = dynamic_cast<VerticalContainer*>(m_widget);

    assert(vertical_container);
    vertical_container->SetId("WindowLayout");


    vertical_container->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1.0));

    auto button = dynamic_cast<Button*>(platform::NewWidget(WidgetType::ButtonType));
    assert(button);

    button->SetId("SaveButton");
    button->SetWidth(WidgetSize(WidgetSize::Type::Fixed, 100));
    button->SetHeight(WidgetSize(WidgetSize::Type::Fixed, 100));
    button->SetText(L"Click to say hello");

    button->SetColor(Color(1.0, 1.0, 0.0, 1.0));
    button->SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));
    button->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
    button->SetTextActiveColor(Color(0.0, 0.0, 0.0, 1.0));


    vertical_container->PushBack(button);

    auto textbox = dynamic_cast<TextBox*>(platform::NewWidget(WidgetType::TextBoxType));
    assert(textbox);

    textbox->SetId("TextBox");
    textbox->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1));
    textbox->SetHeight(WidgetSize(WidgetSize::Type::Ratio, 1));

    textbox->SetColor(Color(1.0, 1.0, 1.0, 1.0));
    textbox->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
    textbox->SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));


    vertical_container->PushBack(textbox);


  }

  void Application::ProcessEvent(UserInput* input)
  {
    Widget* interacting_widget = NULL;
    if (m_widget)
    {
      interacting_widget = m_widget->HitTest(input->mouse_x, input->mouse_y);
    }

    m_interaction_context.hot = interacting_widget;
    m_interaction_context.keys_pressed = input->keys_pressed;


    // Update interaction context, use for drawing
    if (!interacting_widget) return;

    if (input->mouse_half_transitions == 1 && input->mouse_state == MouseState::MouseUp)
    {
      m_interaction_context.active = interacting_widget;
    }
    else if (input->mouse_half_transitions == 1 && input->mouse_state == MouseState::MouseDown)
    {
      m_interaction_context.about_to_active = interacting_widget;
    }


    if (interacting_widget->GetId() == "SaveButton" && m_interaction_context.active == interacting_widget)
    {
      TextBox* textbox = dynamic_cast<TextBox*>(FindId(m_widget, "TextBox"));

      if (!textbox) return;

      std::wstring const& content = textbox->GetText();

      SaveToFile(content);
    }
  }

  void Application::Render(LayoutConstraint* constraint, platform::RenderContext* render_context)
  {
    if (m_widget)
    {
      m_widget->Layout(*constraint);
      m_widget->Draw(render_context, m_interaction_context);
    }
  }


  void Application::SaveToFile(std::wstring const& content)
  {
    platform::WriteFile("test.txt", content);
  }

} // namespace gui

} // namespace application
