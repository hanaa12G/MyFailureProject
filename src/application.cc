#include "application.hh"

#include <cassert>
#include <functional>
#include <set>
#include <sstream>


namespace application
{

	namespace gui
	{
    std::set<std::string> g_id_list;

    std::string AddUniqueId(std::string id)
    {
      std::cout << "Add: " << id << std::endl;
      if (g_id_list.find(id) != g_id_list.end())
        throw std::runtime_error("Logic error: Two widget has the same id: " + id);
      g_id_list.insert(id);
      return id;
    }

    void RemoveUniqueId(std::string id)
    {
      g_id_list.erase(id);
    }

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
				for (auto it = hor_ptr->m_children.begin(); it != hor_ptr->m_children.end(); ++it)
				{
					Widget* widget = it->get();
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

		Widget::Widget(WidgetType type)
    : m_type { type },
      m_id   { "" },
      m_width { WidgetSize::Type::Undefined, 0},
      m_height { WidgetSize::Type::Undefined, 0},
      m_layout {}
		{
      assert(type != WidgetType::InvalidType);
			SetId();
		}

    Widget::Widget(const Widget& other)
    {
    }

		Widget::~Widget()
		{
      RemoveUniqueId(m_id);
		}

		std::string const& Widget::GetId()
		{
			return m_id;
		}

		void Widget::SetId(std::string s)
		{
      if (!m_id.empty())
        RemoveUniqueId(m_id);
			m_id = AddUniqueId(s);
    }

    void Widget::SetId()
    {
      std::stringstream ss;
      ss << m_type << ":" << this;
      m_id = AddUniqueId(ss.rdbuf()->str());
    }

		WidgetType Widget::GetType()
		{
			return m_type;
		}

    void Widget::SetWidth(WidgetSize w)
    {
      m_width = w;
    }

    void Widget::SetHeight(WidgetSize h)
    {
      m_height = h;
    }

    LayoutInfo& Widget::GetLayout()
    {
      return m_layout;
    }

		void Widget::OnClick()
		{
		}

		void Widget::OnChar(KeyboardEvent e)
		{
		}



		Rectangle::Rectangle(): 
      Widget(WidgetType::RectangleType)
		{
		}

		void Rectangle::SetColor(Color c)
		{
			m_bg_default_color = c;
		}

		void Rectangle::SetActiveColor(Color c)
		{
			m_bg_active_color = c;
		}

		void Rectangle::Layout(LayoutConstraint const& c, InteractionContext& interaction_context)
		{
			LayoutInfo info = {};
			info.x = c.x;
			info.y = c.y;


			info.width = FindFixSize(m_width, c.max_width);
			info.height = FindFixSize(m_height, c.max_height);

			if (interaction_context.dragging == this)
			{
				info.x = interaction_context.dragging_dx;
				info.y = interaction_context.dragging_dy;
			}

			logger::Debug("INFO (Rectangle): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

			m_layout = info;
		}

		Widget* Rectangle::HitTest(int x, int y)
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


		VerticalContainer::VerticalContainer()
      : Widget(WidgetType::VerticalContainerType)
		{
		}

		void VerticalContainer::PushBack(Widget* w)
		{
			m_children.push_back(std::shared_ptr<Widget>(w));
		}
		void VerticalContainer::PushBack(std::shared_ptr<Widget> w)
		{
			m_children.push_back(w);
		}

		void VerticalContainer::Clear()
		{
			m_children.clear();
		}

		void VerticalContainer::Layout(LayoutConstraint const& c, InteractionContext& interaction_context)
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

			size_t i = 0;
			for (auto it = m_children.begin(); it != m_children.end(); ++it)
			{
				Widget* w = it->get();
				LayoutConstraint child_constraint;
				child_constraint.max_width = info.width;
				child_constraint.max_height = info.height;
				child_constraint.x = 0;
				child_constraint.y = y;

				w->Layout(child_constraint, interaction_context);
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

			m_layout = info;
		}

		Widget* VerticalContainer::HitTest(int x, int y)
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
			for (auto it = m_children.begin(); it != m_children.end(); ++it)
			{
				Widget* widget = it->get();
				if (IsLayoutInfoValid(widget->GetLayout()))
				{
					w = widget->HitTest(x, y);
					if (w) break;
				}
			}

			return w ? w : hit;
		}





		HorizontalContainer::HorizontalContainer()
    : Widget(WidgetType::HorizontalContainerType)
		{
		}

		void HorizontalContainer::PushBack(Widget* w)
		{
			m_children.push_back(std::shared_ptr<Widget>(w));
		}

		void HorizontalContainer::PushBack(std::shared_ptr<Widget> w)
		{
			m_children.push_back(w);
		}

		void HorizontalContainer::Layout(LayoutConstraint const& c, InteractionContext& interaction_context)
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

			int i = 0;
			for (auto it = m_children.begin(); it != m_children.end(); ++it)
			{
				Widget* w = it->get();
				LayoutConstraint child_constraint;
				child_constraint.max_width = info.width;
				child_constraint.max_height = info.height;
				child_constraint.x = x;
				child_constraint.y = 0;

				w->Layout(child_constraint, interaction_context);
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
			m_layout = info;
		}

		Widget* HorizontalContainer::HitTest(int x, int y)
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
			for (auto it = m_children.begin(); it != m_children.end(); ++it)
			{
				Widget* widget = it->get();
				if (IsLayoutInfoValid(widget->GetLayout()))
				{
					w = widget->HitTest(x, y);
					if (w) break;
				}
			}

			return w ? w : hit;
		}




		Button::Button()
    : Widget(WidgetType::ButtonType)
		{
			SetColor(Color(1.0, 1.0, 0.0, 1.0));
			SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));
			SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
			SetTextActiveColor(Color(0.0, 0.0, 0.0, 1.0));
		}

		void Button::SetColor(Color c)
		{
			m_bg_default_color = c;
		}

		void Button::SetActiveColor(Color c)
		{
			m_bg_active_color = c;
		}

		void Button::SetTextColor(Color c)
		{
			m_fg_default_color = c;
		}
		void Button::SetTextActiveColor(Color c)
		{
			m_fg_active_color = c;
		}

		void Button::SetText(std::string text)
		{
			m_text = text;
		}

		std::string Button::GetText()
		{
			return m_text;
		}

		void Button::SetOnClicked(std::function<void(void*)> fn)
		{
			m_on_clicked = fn;
		}

		void Button::OnClick()
		{
			m_on_clicked(this);
		}

		void Button::SetBorderColor(Color c)
		{
			m_border_color = c;
		}

    void Button::Layout(LayoutConstraint const& c, InteractionContext& interaction_context)
    {
			LayoutInfo info = {};
			info.x = c.x;
			info.y = c.y;

			info.width = FindFixSize(m_width, c.max_width);
			info.height = FindFixSize(m_height, c.max_height);

			if (interaction_context.dragging == this)
			{
				info.x = interaction_context.dragging_dx;
				info.y = interaction_context.dragging_dy;
			}

			logger::Debug("INFO (Button): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

			m_layout = info;
    }

    Widget* Button::HitTest(int x, int y)
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

		TextBox::TextBox()
    : Widget(WidgetType::TextBoxType)
		{

			SetColor(Color(1.0, 1.0, 1.0, 1.0));
			SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
			SetActiveColor(Color(1.0, 1.0, 1.0, 1.0));
		}

		void TextBox::SetColor(Color c)
		{
			m_bg_default_color = c;
		}

		void TextBox::SetTextColor(Color c)
		{
			m_fg_default_color = c;
		}

		void TextBox::SetActiveColor(Color c)
		{
			m_bg_active_color = c;
		}

		std::string const& TextBox::GetText()
		{
			return m_text;
		}

		void TextBox::SetText(std::string text)
		{
			m_text = text;
		}


		void TextBox::OnChar(KeyboardEvent e)
		{
			static int last_width = 1;

			printf("%s\n", m_text.data());
			if (e.key_press == 8)
			{

				while (!m_text.empty())
				{
					char c = m_text.back();
					if (! (c & 0b10000000))
					{
						m_text.pop_back();
						return;
					}
					else if (!(c &0b01000000))
					{
						m_text.pop_back();
					}
					else if (c & 0b01000000)
					{
						m_text.pop_back();
						return;
					}

					else assert(false);
				}
				return;
			}

			char* p = (char*)&e.key_press;
			for (int i = 0; i < e.key_length; ++i)
			{
				m_text.push_back(p[i]);
			}
			last_width = e.key_length;
		}



		void TextBox::SetOnChar(std::function<void(void*, int)> fn)
		{
			m_on_char_input_fn = fn;
		}

		void TextBox::Layout(LayoutConstraint const& c, InteractionContext& interaction_context)
		{
			LayoutInfo info = {};
			info.x = c.x;
			info.y = c.y;


			info.width = FindFixSize(m_width, c.max_width);
			info.height = FindFixSize(m_height, c.max_height);

			if (interaction_context.dragging == this)
			{
				info.x = interaction_context.dragging_dx;
				info.y = interaction_context.dragging_dy;
			}

			logger::Debug("INFO (TextBox): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

			m_layout = info;
		}

    Widget* TextBox::HitTest(int x, int y)
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



		Layers::Layers()
    : Widget(WidgetType::LayersType)
		{
			m_type = WidgetType::LayersType;
		}

		void Layers::Layout(LayoutConstraint const& c, InteractionContext& interaction_context)
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
				i->second->Layout(layer_constraint, interaction_context);
			}

			logger::Debug("INFO (Layers): x=%d, y=%d, width=%d, height=%d", info.x, info.y, info.width, info.height);

			m_layout = info;
		}

		void Layers::SetLayer(int layer, std::shared_ptr<Widget> w)
		{
			m_layers[layer] = w;
		}

		void Layers::PopLayer()
		{
			if (m_layers.size() <= 1) return;
			auto nit = m_layers.begin();
			nit++;
			m_layers.erase(nit, m_layers.end());
		}

		int Layers::GetLevel()
		{
			return m_layers.rbegin()->first;
		}

		Widget* Layers::HitTest(int x, int y)
		{
			int number_of_layers = m_layers.size();
			if (number_of_layers > 0)
			{
				Widget* hit = m_layers.rbegin()->second->HitTest(x, y);
				if (hit) return hit;
			}

			return this;
		}

		void Layers::OnClick()
		{
			auto it = m_layers.lower_bound(1);
			m_layers.erase(it, m_layers.end());
		}

		FileSelector::FileSelector()
    : Widget(WidgetType::FileSelectorType)
		{

			m_container = std::shared_ptr<Widget>(platform::NewWidget(WidgetType::VerticalContainerType));


			VerticalContainer& container = dynamic_cast<VerticalContainer&>(*m_container);
			container.SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1.0));
			container.SetHeight(WidgetSize(WidgetSize::Type::Ratio, 1.0));
			container.SetId("FileSelector::Container");

      
      auto file_list_ptr = std::shared_ptr<Widget>(platform::NewWidget(WidgetType::VerticalContainerType));
      VerticalContainer* file_list = dynamic_cast<VerticalContainer*>(file_list_ptr.get());
      file_list->SetId("FileSelector::FileList");
      file_list->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1.0));
      file_list->SetHeight(WidgetSize(WidgetSize::Type::Ratio, 0.8));

      auto horizontal_container = std::shared_ptr<Widget>(platform::NewWidget(WidgetType::HorizontalContainerType));
      auto bottom_row = dynamic_cast<HorizontalContainer*>(horizontal_container.get());

      auto filepath_textbox_ptr = std::shared_ptr<Widget>(platform::NewWidget(WidgetType::TextBoxType));
      auto confirm_button_ptr = std::shared_ptr<Widget>(platform::NewWidget(WidgetType::ButtonType));
      auto cancel_button_ptr = std::shared_ptr<Widget>(platform::NewWidget(WidgetType::ButtonType));


      filepath_textbox_ptr->SetId("FileSelector::TextBox");
      filepath_textbox_ptr->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 0.8));
      filepath_textbox_ptr->SetHeight(WidgetSize(WidgetSize::Type::Fixed, 30));


      auto confirm_button = dynamic_cast<Button*>(confirm_button_ptr.get());
      confirm_button->SetId("FileSelector::ConfirmButton");
      confirm_button->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 0.1));
      confirm_button->SetHeight(WidgetSize(WidgetSize::Type::Fixed, 30));
      confirm_button->SetText("Open");


      auto cancel_button = dynamic_cast<Button*>(cancel_button_ptr.get());
      cancel_button->SetId("FileSelector::CancelButton");
      cancel_button->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 0.1));
      cancel_button->SetHeight(WidgetSize(WidgetSize::Type::Fixed, 30));
      cancel_button->SetText("Cancel");

      bottom_row->PushBack(filepath_textbox_ptr);
      bottom_row->PushBack(cancel_button_ptr);
      bottom_row->PushBack(confirm_button_ptr);


      container.PushBack(file_list_ptr);
      container.PushBack(horizontal_container);

		}

		void FileSelector::SetPath(std::string path)
		{
      auto file_path_textbox = dynamic_cast<TextBox*>(FindId(m_container.get(), "FileSelector::TextBox"));
      if (!file_path_textbox) return;

			if (m_current_path != path)
			{
				auto file_list = dynamic_cast<VerticalContainer*>(FindId(m_container.get(), "FileSelector::FileList"));
				file_list->Clear();

				m_current_path = path;
				m_file_names = ReadPath(m_current_path);
				std::sort(m_file_names.begin(), m_file_names.end());

				for (auto item : m_file_names)
				{
					auto btnw = std::shared_ptr<Widget>(platform::NewWidget(WidgetType::ButtonType));
					auto btn = dynamic_cast<Button*>(btnw.get());
					btn->SetText(item);
					btn->SetId(item);
					btn->SetHeight(WidgetSize(WidgetSize::Type::Fixed, 30));
					btn->SetWidth(WidgetSize(WidgetSize::Type::Percent, 100));
					btn->SetColor(Color(1.0, 1.0, 1.0, 1.0));
					btn->SetTextColor(Color(0.0, 0.0, 0.0, 1.0));
					btn->SetBorderColor(Color(0.7, 0.7, 0.7, 1.0));
					btn->SetOnClicked(std::bind(&FileSelector::PathButtonSelect, this, btn, std::placeholders::_1));

					file_list->PushBack(btnw);
				}
			}

      file_path_textbox->SetText(path);
		}

		void FileSelector::Layout(LayoutConstraint const& layout, InteractionContext& interaction_context)
		{
			m_layout.x = layout.x;
			m_layout.y = layout.y;
			m_layout.width = FindFixSize(m_width, layout.max_width);
			m_layout.height = FindFixSize(m_height, layout.max_height);


			// get some space for padding
			int child_max_width = m_layout.width * 0.6;
			int child_max_height = m_layout.height;
			int x = (m_layout.width - child_max_width) / 2;
			int y = 0;



			LayoutConstraint constraint{
			  .max_width = child_max_width,
			  .max_height = child_max_height,
			  .x = x,
			  .y = y,
			};

			m_container.get()->Layout(constraint, interaction_context);
      LayoutInfo container_layout = m_container.get()->m_layout;

      x = container_layout.x;
      y = container_layout.y + container_layout.height;

      LayoutConstraint textbox_constraint {
        .max_width = child_max_width,
        .max_height = child_max_height - container_layout.height,
        .x = x,
        .y = y
      };
		}

		Widget* FileSelector::HitTest(int x, int y)
		{
			if (IsLayoutInfoValid(m_layout))
			{
				if (x < m_layout.x || x >(m_layout.x + m_layout.width) ||
					y < m_layout.y || y >(m_layout.y + m_layout.height))
					return NULL;

				else
				{
					x -= m_layout.x;
					y -= m_layout.y;
					Widget* child_hit = m_container->HitTest(x, y);
					return child_hit ? child_hit : this;
				}
			}
			return this;
		}

		std::vector<std::string> FileSelector::ReadPath(std::string p)
		{
			if (p.empty()) return {};

			return platform::ReadPath(p);
		}

		void FileSelector::OnClick()
		{
			OnDestroyed("");
		}

		void FileSelector::PathButtonSelect(Widget* w, void*)
		{
			if (!w) return;
			auto widget = (Widget*)w;

			auto& button = dynamic_cast<Button&>(*widget);

			std::string filename = button.GetText();
			std::string newpath = platform::AppendSegment(m_current_path, filename);

			if (platform::IsFile(newpath))
			{
				VerticalContainer* container = dynamic_cast<VerticalContainer*>(m_container.get());
				for (auto it = container->m_children.begin();
					it != container->m_children.end();
					it++)
				{
					auto btn = dynamic_cast<Button*>(it->get());
					if (!btn) continue;

					btn->SetColor(Color(1.0, 1.0, 1.0, 1.0));
				}
				button.SetColor(Color(0.0, 0.3, 0.6, 1.));
			}
			else if (platform::IsDirectory(newpath))
			{
				SetPath(newpath);
			}
		}

		void FileSelector::OnDestroyed(std::string s)
		{
			if (s.empty())
				m_on_destroyed_fn(this, s);

		}

		void FileSelector::SetOnDestroyed(std::function<void(void*, std::string)> fn)
		{
			m_on_destroyed_fn = fn;
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
			button->SetOnClicked(std::bind(&Application::SaveButtonClicked, this, button, std::placeholders::_1));
			button_horizontal_list->PushBack(button);

			auto button2 = dynamic_cast<Button*>(platform::NewWidget(WidgetType::ButtonType));
			assert(button2);
			button2->SetId("OpenButton");
			button2->SetWidth(WidgetSize(WidgetSize::Type::Percent, 20));
			button2->SetHeight(WidgetSize(WidgetSize::Type::Percent, 100));
			button2->SetText("Open");
			button2->SetOnClicked(std::bind(&Application::OpenButtonClicked, this, button2, std::placeholders::_1));
			button_horizontal_list->PushBack(button2);

			vertical_container->PushBack(button_horizontal_list);

			auto textbox = dynamic_cast<TextBox*>(platform::NewWidget(WidgetType::TextBoxType));
			assert(textbox);
			textbox->SetId("TextBox");
			textbox->SetWidth(WidgetSize(WidgetSize::Type::Ratio, 1));
			textbox->SetHeight(WidgetSize(WidgetSize::Type::Ratio, 0.8));
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
			bool key_pressed = false;
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
				m_interaction_context.active = interacting_widget;
			}
			else if (event->type == UserEvent::Type::KeyboardEventType)
			{
				key_pressed = true;
				KeyboardEvent e = event->keyboard_event;
				if (key_pressed && m_interaction_context.active)
				{
					m_interaction_context.active->OnChar(e);
				}
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
