#pragma once

#include <utility>
#include <string>
#include <vector>
#include "logger.hh"
#include "platform.hh"
#include <functional>
#include <map>
#include <iostream>


namespace application
{
	inline std::string Tostring(std::string f)
	{
		std::string out(f.size(), L'0');
		for (int i = 0; i < f.size(); ++i)
			out[i] = (wchar_t)f[i];
		return out;
	}
	namespace gui
	{

		enum WidgetType
		{
			InvalidType,
			RectangleType,
			VerticalContainerType,
			HorizontalContainerType,
			ButtonType,
			TextBoxType,
			LayersType,
			FileSelectorType
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

		inline int FindFixSize(WidgetSize size, int max_size)
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
			Widget* active = NULL;
			Widget* hot = NULL;
			Widget* about_to_active = NULL;
			Widget* dragging = NULL;

			int     dragging_dx = 0;
			int     dragging_dy = 0;

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
			Widget()
			{
				SetId();
			}
			virtual ~Widget() {}
			virtual void Layout(LayoutConstraint const&, InteractionContext&) = 0;
			virtual void Draw(platform::RenderContext*, InteractionContext) = 0;
			virtual Widget* HitTest(int, int) = 0;
			virtual LayoutInfo& GetLayout() = 0;
			virtual void OnClick() {}

			virtual std::string const& GetId()
			{
				return m_id;
			}
			virtual void SetId(std::optional<std::string> str = std::nullopt)
			{
				m_id = str.value_or(std::to_string((long)this));
			}

			virtual WidgetType GetType()
			{
				return m_type;
			}


			std::string m_id = "Invalid";
			WidgetType m_type = WidgetType::InvalidType;
		};


		struct Rectangle : public Widget
		{
			WidgetSize m_width = WidgetSize();
			WidgetSize m_height = WidgetSize();
			Color m_bg_default_color = {};
			Color m_bg_active_color = {};
			bool  m_accept_dragging = true;

			Rectangle()
			{
				m_type = WidgetType::RectangleType;
			}

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

			virtual void Layout(LayoutConstraint const& c, InteractionContext& interaction_context) override
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

			virtual LayoutInfo& GetLayout() override { return m_layout; }



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
			std::vector<std::shared_ptr<Widget>> m_children;

			WidgetSize m_width = {};
			WidgetSize m_height = {};
			LayoutInfo m_layout = {};


			VerticalContainer()
			{
				m_type = WidgetType::VerticalContainerType;
			}

			void PushBack(Widget* w)
			{
				m_children.push_back(std::shared_ptr<Widget>(w));
			}
			void PushBack(std::shared_ptr<Widget> w)
			{
				m_children.push_back(w);
			}

			void Clear()
			{
				m_children.clear();
			}

			void SetWidth(WidgetSize w)
			{
				m_width = w;
			}

			void SetHeight(WidgetSize w)
			{
				m_height = w;
			}

			virtual void Layout(LayoutConstraint const& c, InteractionContext& interaction_context) override
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

			HorizontalContainer()
			{
				m_type = WidgetType::HorizontalContainerType;
			}

			void SetWidth(WidgetSize w)
			{
				m_width = w;
			}

			void SetHeight(WidgetSize w)
			{
				m_height = w;
			}

			void PushBack(Widget* w)
			{
				m_children.push_back(w);
			}

			virtual void Layout(LayoutConstraint const& c, InteractionContext& interaction_context) override
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

				for (size_t i = 0; Widget * w: m_children)
				{
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
				for (Widget* widget : m_children)
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

		struct Button : public Rectangle
		{
			std::string m_text;
			Color m_bg_default_color;
			Color m_fg_default_color;
			Color m_bg_active_color;
			Color m_fg_active_color;
			Color m_border_color;
			std::function<void(void*)> m_on_clicked;

			Button()
			{
				m_type = WidgetType::ButtonType;
			}

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
			void SetText(std::string text)
			{
				m_text = text;
			}

			void SetOnClicked(std::function<void(void*)> fn)
			{
				m_on_clicked = fn;
			}

			void OnClick()
			{
				m_on_clicked(this);
			}

			void SetBorderColor(Color c) {
				m_border_color = c;
			}
		};

		struct TextBox : public Rectangle
		{
			std::string m_text;

			Color m_fg_default_color;

			TextBox()
			{
				m_type = WidgetType::TextBoxType;
			}

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


			std::string const& GetText()
			{
				return m_text;
			}

			void SetText(std::string text)
			{
				m_text = text;
			}
		};


		struct Layers : public Widget
		{
			std::map<int, std::shared_ptr<Widget>> m_layers;
			LayoutInfo m_layout;
			WidgetSize m_width;
			WidgetSize m_height;

			Layers()
			{
				m_type = WidgetType::LayersType;
			}


			virtual void Layout(LayoutConstraint const& c, InteractionContext& interaction_context)
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

			LayoutInfo& GetLayout() override
			{
				return m_layout;
			}

			void SetLayer(int layer, std::shared_ptr<Widget> w)
			{
				m_layers[layer] = w;
			}

      void PopLayer()
      {
        if (m_layers.size() <= 1) return;
        auto nit = m_layers.begin();
        nit++;
        m_layers.erase(nit, m_layers.end());
      }

			int GetLevel() { return m_layers.rbegin()->first; }


			Widget* HitTest(int x, int y) override
			{
				int number_of_layers = m_layers.size();
				if (number_of_layers > 0)
				{
					Widget* hit = m_layers.rbegin()->second->HitTest(x, y);
					if (hit) return hit;
				}

				return this;
			}

			void OnClick() override
			{
				auto it = m_layers.lower_bound(1);
				m_layers.erase(it, m_layers.end());
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

		struct FileSelector : public Widget
		{
			std::string m_current_path;
			std::vector<std::string> m_file_names;

			std::shared_ptr<Widget> m_container;
			WidgetSize m_width;
			WidgetSize m_height;
			LayoutInfo m_layout;

      std::function<void(void*, std::string)> m_on_destroyed_fn;

			FileSelector()
			{
				m_container = std::shared_ptr<Widget>(platform::NewWidget(WidgetType::VerticalContainerType));
				VerticalContainer& container = dynamic_cast<VerticalContainer&>(*m_container);
				container.SetWidth(WidgetSize(WidgetSize::Type::Ratio, 0.6));
				container.SetHeight(WidgetSize(WidgetSize::Type::Ratio, 0.6));
				container.SetId("FileSelector>Container");
			}

			void SetWidth(WidgetSize size)
			{
				m_width = size;
			}
			void SetHeight(WidgetSize size)
			{
				m_height = size;
			}

			void SetPath(std::string path)
			{
				if (m_current_path != path)
				{
					auto container = dynamic_cast<VerticalContainer*>(m_container.get());
					container->Clear();

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

						container->PushBack(btnw);
					}
				}
			}


			void Layout(LayoutConstraint const& layout, InteractionContext& interaction_context) override
			{
        m_layout.x = layout.x;
        m_layout.y = layout.y;
        m_layout.width = FindFixSize(m_width, layout.max_width);
        m_layout.height = FindFixSize(m_height, layout.max_height);


        // get some space for padding
        int child_max_width = m_layout.width * 0.6;
        int child_max_height = m_layout.height * 0.6;
        int x = (m_layout.width - child_max_width) / 2;
        int y = (m_layout.height - child_max_height) / 2;



        LayoutConstraint constraint {
          .max_width = child_max_width,
          .max_height = child_max_height,
          .x = x,
          .y = y,
        };

				m_container.get()->Layout(constraint, interaction_context);
			}

			Widget* HitTest(int x, int y) override
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

			LayoutInfo& GetLayout() override
			{
				return m_layout;
			}


			std::vector<std::string> ReadPath(std::string p)
			{
				if (p.empty()) return {};

				return platform::ReadPath(p);
			}

      void OnClick() override
      {
        OnDestroyed("");
      }

      void OnDestroyed(std::string s)
      {
        m_on_destroyed_fn (this, s);
      }
      void SetOnDestroyed(std::function<void (void*, std::string)> fn)
      {
        m_on_destroyed_fn = fn;
      }


		};


		enum MouseState
		{
			Down,
			Up,
			Move
		};


		struct KeyboardEvent
		{
			int key_press;
			int modifier;
		};

		struct MouseEvent
		{
			int x;
			int y;
			int state;
			platform::Timestamp timestamp;
		};

		struct UserEvent
		{
			enum Type {
				KeyboardEventType,
				MouseEventType,
			};
			Type type;

			union {
				KeyboardEvent keyboard_event;
				MouseEvent    mouse_event;
			};

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

			MouseEvent m_last_mouse_event = {
			  .state = MouseState::Up,
			};

			bool m_last_mouse_dragged = false;
			int m_mouse_drag_start_x;
			int m_mouse_drag_start_y;
			platform::Timestamp m_last_mouse_down_timestamp;
			platform::Timestamp m_last_mouse_click_timestamp;
			platform::Duration  m_mouse_drag_duration;
			platform::Duration  m_mouse_double_click_duration{ 500.0 };

			std::string  m_application_path;


			Application();

			void InitLayout();

			void ProcessEvent(UserEvent*);
			void Render(LayoutConstraint*, platform::RenderContext*);

			void SaveToFile(std::string const& content, std::function<void()>);
			void SaveFileSuccessfullyCallback();
			void LoadFile();

			void SaveButtonClicked(Widget*, void*);
			void OpenButtonClicked(Widget*, void*);
      void FileSelectionFinished(Widget*, void*, std::string);
		};

	} // namespace gui



} // namespace application



