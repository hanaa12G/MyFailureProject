#include "application.hh"

#include <cassert>
#include <functional>


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

    Layers* layers_ptr = dynamic_cast<Layers*>(root_widget);
    if (layers_ptr)
    {
      for (auto it = layers_ptr->m_layers.begin();
           it != layers_ptr->m_layers.end();
           ++it)
      {
        ret = FindId(it->second, id);
        if (ret) return ret;
      }
    }

    return NULL;
  }

  Application::Application()
  {
    InitLayout();


    LoadFile();
  }

  void Application::InitLayout()
  {

    m_widget = platform::NewWidget(WidgetType::LayersType);
    
    auto layers = dynamic_cast<Layers*>(m_widget);
    assert(layers);
    layers->SetId("Layers");

    auto vertical_container = dynamic_cast<VerticalContainer*>(platform::NewWidget(WidgetType::VerticalContainerType));

    assert(vertical_container);
    vertical_container->SetId("WindowLayout");
    vertical_container->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1.0));


    auto button_horizontal_list = dynamic_cast<HorizontalContainer*>(
      platform::NewWidget(WidgetType::HorizontalContainerType));
    assert(button_horizontal_list);
    button_horizontal_list->SetId("ButtonArrays");
    button_horizontal_list->SetWidth(WidgetSize(WidgetSize::Type::Percent, 100));
    button_horizontal_list->SetHeight(WidgetSize(WidgetSize::Type::Fixed, 100));

    auto button = dynamic_cast<Button*>(platform::NewWidget(WidgetType::ButtonType));
    assert(button);
    button->SetId("SaveButton");
    button->SetWidth(WidgetSize(WidgetSize::Type::Percent, 20));
    button->SetHeight(WidgetSize(WidgetSize::Type::Percent, 100));
    button->SetText(L"Save");
    button->SetColor(Color(1.0, 1.0, 0.0, 1.0));
    button->SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));
    button->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
    button->SetTextActiveColor(Color(0.0, 0.0, 0.0, 1.0));
    button_horizontal_list->PushBack(button);

    auto button2 = dynamic_cast<Button*>(platform::NewWidget(WidgetType::ButtonType));
    assert(button2);
    button2->SetId("OpenButton");
    button2->SetWidth(WidgetSize(WidgetSize::Type::Percent, 20));
    button2->SetHeight(WidgetSize(WidgetSize::Type::Percent, 100));
    button2->SetText(L"Open");
    button2->SetColor(Color(1.0, 1.0, 0.0, 1.0));
    button2->SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));
    button2->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
    button2->SetTextActiveColor(Color(0.0, 0.0, 0.0, 1.0));
    button_horizontal_list->PushBack(button2);

    vertical_container->PushBack(button_horizontal_list);

    auto filename_box = dynamic_cast<TextBox*>(platform::NewWidget(WidgetType::TextBoxType));
    assert(filename_box);
    filename_box->SetId("FilenameBox");
    filename_box->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1));
    filename_box->SetHeight(WidgetSize(WidgetSize::Type::Ratio, 0.2));
    filename_box->SetColor(Color(1.0, 1.0, 1.0, 1.0));
    filename_box->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
    filename_box->SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));
    vertical_container->PushBack(filename_box);

    auto textbox = dynamic_cast<TextBox*>(platform::NewWidget(WidgetType::TextBoxType));
    assert(textbox);
    textbox->SetId("TextBox");
    textbox->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1));
    textbox->SetHeight(WidgetSize(WidgetSize::Type::Ratio, 0.8));
    textbox->SetColor(Color(1.0, 1.0, 1.0, 1.0));
    textbox->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
    textbox->SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));
    vertical_container->PushBack(textbox);




    layers->SetLayer(0, vertical_container);
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
      printf("-----------------------------%s", m_interaction_context.active->GetId().c_str());
    }
    else if (input->mouse_half_transitions == 1 && input->mouse_state == MouseState::MouseDown)
    {
      m_interaction_context.about_to_active = interacting_widget;
    }


    // NOTE (hanasou): Only process event at frame of interaction, since interaction_context
    // is persist between frame that can cause calling process event multiple times

    if (interacting_widget->GetId() == "SaveButton" && m_interaction_context.active == interacting_widget)
    {
      TextBox* textbox = dynamic_cast<TextBox*>(FindId(m_widget, "TextBox"));

      if (!textbox) 
      {
        return;
      }

      std::wstring const& content = textbox->GetText();

      SaveToFile(content, std::bind(SaveFileSuccessfullyCallback, this));
    }

    if (interacting_widget->GetId() == "OkTextBox" && m_interaction_context.active == interacting_widget)
    {
      auto layers = dynamic_cast<Layers*>(m_widget);
      assert(layers);
      layers->m_layers.erase(1);
    }
  }

  void Application::SaveFileSuccessfullyCallback()
  {
    logger::Info("OKOKOK");

    auto ok_textbox = dynamic_cast<TextBox*>(platform::NewWidget(WidgetType::TextBoxType));
    assert(ok_textbox);

    ok_textbox->SetId("OkTextBox");
    ok_textbox->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1));
    ok_textbox->SetHeight(WidgetSize(WidgetSize::Type::Ratio, 1));

    ok_textbox->SetColor(Color(1.0, 0.0, 0.0, 1.0));
    ok_textbox->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
    ok_textbox->SetActiveColor(Color(1.0, 0.0, 0.0, 1.0));

    auto layers = dynamic_cast<Layers*>(m_widget);
    assert(layers);
    layers->SetLayer(1, ok_textbox);
  }

  void Application::Render(LayoutConstraint* constraint, platform::RenderContext* render_context)
  {
    if (m_widget)
    {
      m_widget->Layout(*constraint);
      m_widget->Draw(render_context, m_interaction_context);
    }
  }


  void Application::SaveToFile(std::wstring const& content, std::function<void()> callback) 
  {
    if (platform::WriteFile("test.txt", content))
      callback();
  }

  void Application::LoadFile()
  {
    std::wstring s = platform::ReadFile("test.txt");

    auto textbox = dynamic_cast<TextBox*>(FindId(m_widget, "TextBox"));
    if (!textbox) return;

    textbox->SetText(s);
  }

} // namespace gui

} // namespace application
