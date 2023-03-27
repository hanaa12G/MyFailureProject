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

  void Application::ProcessEvent(UserEvent* event)
  {
    bool mouse_click = false;
    bool mouse_moving = false;
    bool mouse_dragged = false;
    bool mouse_double_clicked = false;
    bool mouse_down = false;
    bool text_event = false;
    bool shortcut_event = false;
    int  mouse_dragged_dx = 0;
    int  mouse_dragged_dy = 0;

    Widget* interacting_widget = NULL;
    if (event->type == UserEvent::Type::MouseEventType)
    {
      MouseEvent e  = event->mouse_event;

      if (e.state == MouseState::Down && m_last_mouse_event.state == MouseState::Up)
      {
        mouse_down = true;
        m_last_mouse_down_timestamp = platform::CurrentTimestamp();
        m_last_mouse_event = e;
      }
      else if (e.state == MouseState::Up && m_last_mouse_event.state == MouseState::Down)
      {
        // let's check double click
        if (platform::DurationFrom(e.timestamp, m_last_mouse_click_timestamp) < m_mouse_double_click_duration)
        {
          mouse_double_clicked = true;
        }

        mouse_down = false;
        mouse_click = true;
        m_last_mouse_click_timestamp = platform::CurrentTimestamp();
        m_last_mouse_event = e;
        m_mouse_drag_start_x = 0;
        m_mouse_drag_start_y = 0;
      }
      else if (e.state == MouseState::Move && m_last_mouse_event.state == MouseState::Down)
      {
        printf("Event mouse move\n");
        mouse_moving = true;
        mouse_down = true;
        if(mouse_dragged == false)
        {
          m_mouse_drag_start_x = e.x;
          m_mouse_drag_start_y = e.y;
          printf("start_x:%d, now_x: %d\n", m_mouse_drag_start_x, e.x);
          mouse_dragged_dx = e.x - m_mouse_drag_start_x;
          mouse_dragged_dy = e.y - m_mouse_drag_start_y;
        }
        mouse_dragged = true;
      }
      else if (e.state == MouseState::Move)
      {
        mouse_moving = true;
      }

      interacting_widget = m_widget->HitTest(e.x, e.y);
    }
    else if (event->type == UserEvent::Type::KeyboardEventType)
    {
      KeyboardEvent e = event->keyboard_event;
    }

    // mouse_click: check duration < hold_duration
    // mouse_drag: check_duration
    // mouse_double_click: save last click timestamp and check
    printf("move: %d, down: %d, click: %d, dclick: %d, drag: %d\n", mouse_moving, mouse_down, mouse_click, mouse_double_clicked, mouse_dragged);

    if (mouse_dragged)
    {
      m_interaction_context.dragging = interacting_widget;
    }
    else
    {
      m_interaction_context.dragging = NULL;
    }

    if (m_interaction_context.dragging)
    {
      m_interaction_context.dragging_dx = mouse_dragged_dx;
      m_interaction_context.dragging_dy = mouse_dragged_dy;
      printf("%s is dragging, dx = %d, dy=%d\n", m_interaction_context.dragging->GetId().c_str(),
       m_interaction_context.dragging_dx, m_interaction_context.dragging_dy );
    }


    if ((mouse_click && !mouse_dragged) && interacting_widget) 
    {
      if (interacting_widget->GetId() == "SaveButton")
      {
        printf("Save button\n");
      }
      else if (interacting_widget->GetId() == "OpenButton")
      {
        printf("Open button\n");
      }
    }



    

  }

  // void Application::ProcessEvent(UserInput* input)
  // {
  //   Widget* interacting_widget = NULL;
  //   if (m_widget)
  //   {
  //     interacting_widget = m_widget->HitTest(input->mouse_x, input->mouse_y);
  //   }

  //   m_interaction_context.hot = interacting_widget;
  //   m_interaction_context.keys_pressed = input->keys_pressed;


  //   // Update interaction context, use for drawing
  //   if (!interacting_widget) return;

  //   if (input->mouse_half_transitions == 1 && input->mouse_state == MouseState::MouseUp)
  //   {
  //     m_interaction_context.active = interacting_widget;
  //     printf("-----------------------------%s", m_interaction_context.active->GetId().c_str());
  //   }
  //   else if (input->mouse_half_transitions == 1 && input->mouse_state == MouseState::MouseDown)
  //   {
  //     m_interaction_context.about_to_active = interacting_widget;
  //   }
  //   else if (input->mouse_half_transitions == 0 && input->mouse_state == MouseSate::MouseUp &&
  //     input->mouse_scroll_distance != 0)
  //   {
  //   }


  //   // NOTE (hanasou): Only process event at frame of interaction, since interaction_context
  //   // is persist between frame that can cause calling process event multiple times

  //   if (interacting_widget->GetId() == "SaveButton" && m_interaction_context.active == interacting_widget)
  //   {
  //     TextBox* textbox = dynamic_cast<TextBox*>(FindId(m_widget, "TextBox"));

  //     if (!textbox) 
  //     {
  //       return;
  //     }

  //     std::wstring const& content = textbox->GetText();

  //     SaveToFile(content, std::bind(SaveFileSuccessfullyCallback, this));
  //   }
  //   else if (interacting_widget->GetId() == "OpenButton" && m_interaction_context.active == interacting_widget)
  //   {
  //     LoadFile();
  //   }
  //   if (interacting_widget->GetId() == "OkTextBox" && m_interaction_context.active == interacting_widget)
  //   {
  //     auto layers = dynamic_cast<Layers*>(m_widget);
  //     assert(layers);
  //     layers->m_layers.erase(1);
  //   }
  // }

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
      m_widget->Layout(*constraint, m_interaction_context);
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
    auto filename_box = dynamic_cast<TextBox*>(FindId(m_widget, "FilenameBox"));
    if (!filename_box) return;

    std::wstring filename = filename_box->GetText();

    if (filename.empty()) return;


    std::wstring s = platform::ReadFile(filename);
    auto textbox = dynamic_cast<TextBox*>(FindId(m_widget, "TextBox"));
    if (!textbox) return;

    textbox->SetText(s);
  }

} // namespace gui

} // namespace application
