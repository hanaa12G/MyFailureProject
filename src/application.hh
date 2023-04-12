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
    enum Virtual_KeyCode {
      VirtualKey_a,
      VirtualKey_b,
      VirtualKey_c,
      VirtualKey_d,
      VirtualKey_e,
      VirtualKey_f,
      VirtualKey_g,
      VirtualKey_h,
      VirtualKey_i,
      VirtualKey_j,
      VirtualKey_k,
      VirtualKey_l,
      VirtualKey_m,
      VirtualKey_n,
      VirtualKey_o,
      VirtualKey_p,
      VirtualKey_q,
      VirtualKey_r,
      VirtualKey_s,
      VirtualKey_t,
      VirtualKey_u,
      VirtualKey_v,
      VirtualKey_w,
      VirtualKey_x,
      VirtualKey_y,
      VirtualKey_z
    };

		struct KeyboardEvent
		{
			int key_press;
			int modifier;
			int key_length; // utf-8 can be 1, 2, 3, 4 bytes in length
		};
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
			std::string m_id = "Invalid";
			WidgetType m_type = WidgetType::InvalidType;

			Widget();
			virtual ~Widget();
			virtual void Layout(LayoutConstraint const&, InteractionContext&) = 0;
			virtual void Draw(platform::RenderContext*, InteractionContext) = 0;
			virtual Widget* HitTest(int, int) = 0;
			virtual LayoutInfo& GetLayout() = 0;

			virtual void OnClick();
      virtual void OnChar(KeyboardEvent);
			virtual std::string const& GetId();
			virtual void SetId(std::optional<std::string> str = std::nullopt);
			virtual WidgetType GetType();
		};


		struct Rectangle : public Widget
		{
			WidgetSize m_width = WidgetSize();
			WidgetSize m_height = WidgetSize();
			Color m_bg_default_color = {};
			Color m_bg_active_color = {};
			bool  m_accept_dragging = true;
			LayoutInfo m_layout;

			Rectangle();
			void SetColor(Color c);
			void SetActiveColor(Color c);
			void SetWidth(WidgetSize w);
			void SetHeight(WidgetSize w);
			virtual void Layout(LayoutConstraint const& c, InteractionContext& interaction_context) override;
			virtual LayoutInfo& GetLayout() override;
			virtual Widget* HitTest(int x, int y) override;
		};

		struct VerticalContainer : public Widget
		{
			std::vector<std::shared_ptr<Widget>> m_children;

			WidgetSize m_width = {};
			WidgetSize m_height = {};
			LayoutInfo m_layout = {};


			VerticalContainer();

			void PushBack(Widget* w);
			void PushBack(std::shared_ptr<Widget> w);
			void Clear();
			void SetWidth(WidgetSize w);
			void SetHeight(WidgetSize w);
			virtual void Layout(LayoutConstraint const& c, InteractionContext& interaction_context) override;
			virtual LayoutInfo& GetLayout() override;
			int FindMaxSize(WidgetSize size, int max_size);
			virtual Widget* HitTest(int x, int y) override;

		};

		struct HorizontalContainer : public Widget
		{
      std::vector<std::shared_ptr<Widget>> m_children;

			WidgetSize m_width = {};
			WidgetSize m_height = {};
			LayoutInfo m_layout = {};

			HorizontalContainer();
			virtual void Layout(LayoutConstraint const& c, InteractionContext& interaction_context) override;
		  Widget* HitTest(int x, int y) override;
			virtual LayoutInfo& GetLayout() override;

			void SetWidth(WidgetSize w);
			void SetHeight(WidgetSize w);
			void PushBack(Widget* w);
			int FindMaxSize(WidgetSize size, int max_size);

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

			Button();

			void SetColor(Color c);
			void SetActiveColor(Color c);
			void SetTextColor(Color c);
			void SetTextActiveColor(Color c);
			void SetWidth(WidgetSize w);
			void SetHeight(WidgetSize w);
			void SetText(std::string text);
			std::string GetText();
			void SetOnClicked(std::function<void(void*)> fn);
			void OnClick() override;
			void SetBorderColor(Color c);
		};

		struct TextBox : public Rectangle
		{
			std::string m_text;
			Color m_fg_default_color;

      std::function<void(void*, int)> m_on_char_input_fn;

			TextBox();

			void SetColor(Color c);
			void SetTextColor(Color c);
			void SetActiveColor(Color c);
			void SetWidth(WidgetSize w);
			void SetHeight(WidgetSize w);
			std::string const& GetText();
			void SetText(std::string text);
      void OnChar(KeyboardEvent e) override;
      void SetOnChar(std::function<void(void*, int)> fn);
		};


		struct Layers : public Widget
		{
			std::map<int, std::shared_ptr<Widget>> m_layers;
			LayoutInfo m_layout;
			WidgetSize m_width;
			WidgetSize m_height;

			Layers();

			void Layout(LayoutConstraint const& c, InteractionContext& interaction_context) override;
			Widget* HitTest(int x, int y) override;
			void OnClick() override;
			LayoutInfo& GetLayout() override;

			void SetLayer(int layer, std::shared_ptr<Widget> w);
			void PopLayer();
			int GetLevel();
			int FindMaxSize(WidgetSize size, int max_size);
		};

		struct FileSelector : public Widget
		{
			std::string m_current_path;
			std::vector<std::string> m_file_names;

			std::shared_ptr<Widget> m_container;
      std::shared_ptr<Widget> m_confirm_button;
      std::shared_ptr<Widget> m_cancel_button;
      std::shared_ptr<Widget> m_filepath_textbox;

			WidgetSize m_width;
			WidgetSize m_height;
			LayoutInfo m_layout;

			std::function<void(void*, std::string)> m_on_destroyed_fn;

      FileSelector();

			void SetWidth(WidgetSize size);
			void SetHeight(WidgetSize size);
			LayoutInfo& GetLayout() override;

			void Layout(LayoutConstraint const& layout, InteractionContext& interaction_context) override;
			Widget* HitTest(int x, int y) override;
			void OnClick() override;

			void SetPath(std::string path);
			std::vector<std::string> ReadPath(std::string p);
			void PathButtonSelect(Widget* w, void*);
			void OnDestroyed(std::string s);
			void SetOnDestroyed(std::function<void(void*, std::string)> fn);
		};


		enum MouseState
		{
			Down,
			Up,
			Move
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



