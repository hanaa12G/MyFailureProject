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
				for (auto it = vert_ptr->m_children.begin(); it != vert_ptr->m_children.end(); ++it)
				{
					Widget* widget = it->get();
					ret = FindId(widget, id);
					if (ret) return ret;
				}
			}

			HorizontalContainer* hor_ptr = dynamic_cast<HorizontalContainer*>(root_widget);
			if (hor_ptr)
			{
				for (Widget* widget : hor_ptr->m_children)
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
					ret = FindId(it->second.get(), id);
					if (ret) return ret;
				}
			}

			return NULL;
		}

		Application::Application()
		{
			InitLayout();
			LoadFile();

			m_application_path = platform::CurrentPath();
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
			button->SetText("Save");
			button->SetColor(Color(1.0, 1.0, 0.0, 1.0));
			button->SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));
			button->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
			button->SetTextActiveColor(Color(0.0, 0.0, 0.0, 1.0));
			button->SetOnClicked(std::bind(&Application::SaveButtonClicked, this, button, std::placeholders::_1));
			button_horizontal_list->PushBack(button);

			auto button2 = dynamic_cast<Button*>(platform::NewWidget(WidgetType::ButtonType));
			assert(button2);
			button2->SetId("OpenButton");
			button2->SetWidth(WidgetSize(WidgetSize::Type::Percent, 20));
			button2->SetHeight(WidgetSize(WidgetSize::Type::Percent, 100));
			button2->SetText("Open");
			button2->SetColor(Color(1.0, 1.0, 0.0, 1.0));
			button2->SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));
			button2->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
			button2->SetTextActiveColor(Color(0.0, 0.0, 0.0, 1.0));
			button2->SetOnClicked(std::bind(&Application::OpenButtonClicked, this, button2, std::placeholders::_1));
			button_horizontal_list->PushBack(button2);

			vertical_container->PushBack(button_horizontal_list);

			auto textbox = dynamic_cast<TextBox*>(platform::NewWidget(WidgetType::TextBoxType));
			assert(textbox);
			textbox->SetId("TextBox");
			textbox->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1));
			textbox->SetHeight(WidgetSize(WidgetSize::Type::Ratio, 0.8));
			textbox->SetColor(Color(1.0, 1.0, 1.0, 1.0));
			textbox->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
			textbox->SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));
			vertical_container->PushBack(textbox);




			layers->SetLayer(0, std::shared_ptr<Widget>(vertical_container));
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
			bool last_mouse_dragged = m_last_mouse_dragged;

			Widget* interacting_widget = NULL;
			if (event->type == UserEvent::Type::MouseEventType)
			{
				MouseEvent e = event->mouse_event;

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
					mouse_moving = true;
					mouse_down = true;
					if (mouse_dragged == false)
					{
						printf("start_x:%d, now_x: %d\n", m_mouse_drag_start_x, e.x);
						if (!m_last_mouse_dragged)
						{
							m_mouse_drag_start_x = e.x;
							m_mouse_drag_start_y = e.y;
						}
						else
						{
							mouse_dragged_dx = e.x - m_mouse_drag_start_x;
							mouse_dragged_dy = e.y - m_mouse_drag_start_y;
						}
					}
					mouse_dragged = true;
					m_last_mouse_dragged = true;
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
			printf("%s\n", interacting_widget ? interacting_widget->GetId().c_str() : "NULL");

			if (mouse_click && interacting_widget)
			{
				interacting_widget->OnClick();
			}

		}

		void Application::SaveFileSuccessfullyCallback()
		{
			//logger::Info("OKOKOK");

			//auto ok_textbox = dynamic_cast<TextBox*>(platform::NewWidget(WidgetType::TextBoxType));
			//assert(ok_textbox);

			//ok_textbox->SetId("OkTextBox");
			//ok_textbox->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1));
			//ok_textbox->SetHeight(WidgetSize(WidgetSize::Type::Ratio, 1));

			//ok_textbox->SetColor(Color(1.0, 0.0, 0.0, 1.0));
			//ok_textbox->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
			//ok_textbox->SetActiveColor(Color(1.0, 0.0, 0.0, 1.0));

			//auto layers = dynamic_cast<Layers*>(m_widget);
			//assert(layers);
			//layers->SetLayer(1, ok_textbox);
		}

		void Application::Render(LayoutConstraint* constraint, platform::RenderContext* render_context)
		{
			if (m_widget)
			{
				m_widget->Layout(*constraint, m_interaction_context);
				m_widget->Draw(render_context, m_interaction_context);
			}
		}


		void Application::SaveToFile(std::string const& content, std::function<void()> callback)
		{
			if (platform::WriteFile("test.txt", content))
				callback();
		}

		void Application::LoadFile()
		{
			auto filename_box = dynamic_cast<TextBox*>(FindId(m_widget, "FilenameBox"));
			if (!filename_box) return;

			std::string filename = filename_box->GetText();

			if (filename.empty()) return;


			std::string s = platform::ReadFile(filename);
			auto textbox = dynamic_cast<TextBox*>(FindId(m_widget, "TextBox"));
			if (!textbox) return;

			textbox->SetText(s);
		}

		void Application::SaveButtonClicked(Widget*, void*)
		{
		}

		void Application::OpenButtonClicked(Widget*, void*)
		{
			if (m_widget->GetType() != WidgetType::LayersType)
				throw std::logic_error("Unable to create popup item from current layout");
			Layers& layers = *dynamic_cast<Layers*>(m_widget);
			int level = layers.GetLevel();

			auto file_opener = std::shared_ptr<Widget>(platform::NewWidget(WidgetType::FileSelectorType));
			auto& fo = *dynamic_cast<FileSelector*>(file_opener.get());
			
			fo.SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1.0));
			fo.SetHeight(WidgetSize(WidgetSize::Type::Ratio, 1.0));
			fo.SetPath(m_application_path);
			fo.SetId("FileSelector");
      fo.SetOnDestroyed(std::bind(&Application::FileSelectionFinished, this, file_opener.get(), std::placeholders::_1, std::placeholders::_2));
			

			layers.SetLayer(level + 1, file_opener);

		}

    void Application::FileSelectionFinished(Widget*, void*, std::string file) 
    {
      printf("FIle open : %s\n", file.c_str());

			Layers& layers = *dynamic_cast<Layers*>(m_widget);
      layers.PopLayer();
    }

	} // namespace gui

} // namespace application
