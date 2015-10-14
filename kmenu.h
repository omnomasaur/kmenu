
#ifndef KUI_H
#define KUI_H

#include <vector>
#include <string>

namespace km
{
	static inline float round(float number)
	{
		return number < 0.0 ? ceil(number - 0.5f) : floor(number + 0.5f);
	}

	static inline float round_to_nearest(float x, float factor)
	{
		return round(x / factor) * factor;
	}

	class Rect
	{
	public:
		Rect(float left, float top, float width, float height)
			:left(left), top(top), width(width), height(height)
		{}

		float left, top, width, height;
	};

	class Node;
	typedef void(*MenuNodeRenderFunc)(const Node* const cpc_node, const bool& is_selected, const Rect&, const float& alpha);
	typedef void(*MenuActionFunc)(const Node* const cpc_node);

//defines the arguments that all Node subclasses will need to take in thier constructors
#define NODE_ARGS const std::string& text = "", const std::string& data = "", MenuActionFunc fp_action = NULL, MenuNodeRenderFunc fp_render = NULL
//defines the paramaters necessary to pass to the Node constructor, from NODE_ARGS
#define NODE_PARAMS text, data, fp_action, fp_render

	class Menu;
	class Node
	{
		friend class Menu;
	public:
		Node(NODE_ARGS)
			:mp_parent(NULL)
			, m_text(text)
			, m_data(data)
			, mfp_render(fp_render)
			, mfp_action(fp_action)
		{}

		virtual bool up() { return false; }
		virtual bool down() { return false; }
		virtual bool press() 
		{ 
			if (mfp_action)
			{
				mfp_action(this);
				return true;
			}
			return false;
		}
		virtual bool back() { return false; }

		const std::string& get_text() const { return m_text; }
		void set_text(const std::string& text){ m_text = text; }

		const std::string& get_data() const { return m_data; }
		void set_data(const std::string& data){ m_data = data; }

	protected:
		Node* mp_parent;
		std::string m_text;
		std::string m_data;
		MenuNodeRenderFunc mfp_render;
		MenuActionFunc mfp_action;

		virtual void update(const float& delta_time){}

		virtual void render(const bool& is_selected, const Rect& rect, const float& alpha, const float& off_x, const float& off_y)
		{
			if (mfp_render)
			{
				mfp_render(this, is_selected, Rect(rect.left + off_x, rect.top + off_y,
					rect.width, rect.height), alpha);
			}
		}

		virtual void shift(const bool& away)
		{
			if (mp_parent)
				mp_parent->shift(away);
		}

		virtual Node* get_parent() const { return mp_parent; }
		virtual void set_parent(Node* const parent){ mp_parent = parent; }
	};


	typedef void(*MenuRenderFunc)(const Menu* const cpc_item, const Rect&, const float& alpha, const std::string& text);

#define MENU_ARGS MenuNodeRenderFunc fp_node_render = NULL, MenuRenderFunc fp_menu_render = NULL, NODE_ARGS
#define MENU_PARAMS fp_node_render, fp_menu_render, NODE_PARAMS

	class Menu : public Node
	{
	public:
		Menu(const Rect& rect, const float& cell_height, const bool& shift = false, MENU_ARGS)
		:Node(NODE_PARAMS)
		, m_active(false)
		, m_shift(shift)
		, m_rect(rect)
		, m_target_x(0)
		, m_x(0)
		, m_alpha(0.0f)
		, m_target_alpha(1.0f)
		, m_cell_height(cell_height)
		, m_index(0)
		, mfp_node_render(fp_node_render)
		, mfp_menu_render(fp_menu_render)
		{}

		~Menu()
		{
			for (unsigned int i = 0; i < m_nodes.size(); i++)
			{
				delete m_nodes[i];
			}
		}

		//takes ownership of the node, and puts it in the menu
		virtual void take_node(Node * const p_item)
		{
			m_nodes.push_back(p_item);
			m_nodes.back()->set_parent(this);

			if (!m_nodes.back()->mfp_render)
				m_nodes.back()->mfp_render = mfp_node_render;
		}

		virtual void update(const float& delta_time)
		{
			m_x += (m_target_x - m_x) * (1.0f / 0.1f) * delta_time;
			m_alpha += (m_target_alpha - m_alpha) * (1.0f / 0.1f) * delta_time;

			for (unsigned int i = 0; i < m_nodes.size(); i++)
			{
				m_nodes[i]->update(delta_time);
			}
		}

		virtual void render()
		{
			m_active = true;
			render(true, Rect(0, 0, 0, 0), m_alpha, 0, 0);
		}

		virtual bool up()
		{
			if (m_active)
			{
				//pass action to existing item
				if (m_nodes.size() > 0 &&
					!m_nodes[m_index]->up())
				{
					//if the action was not handled below, handle it here
					if (m_index > 0)
					{
						m_index--;
					}
					else
					{
						m_index = m_nodes.size() - 1;
					}
				}
				return true;
			}
			return false;
		}

		virtual bool down()
		{
			if (m_active)
			{
				//pass action to existing item
				if (m_nodes.size() > 0 &&
					!m_nodes[m_index]->down())
				{
					//if action was not handled below, handle it here
					if (m_index < m_nodes.size() - 1)
					{
						m_index++;
					}
					else
					{
						m_index = 0;
					}
				}
				return true;
			}
			return false;
		}

		virtual bool press()
		{
			if (m_active)
			{
				//pass action to existing item
				if (m_nodes.size() > 0 &&
					!m_nodes[m_index]->press())
				{
					//if action was not handled below, handle it here
					//nothing to handle actually
				}
			}
			else
			{
				m_active = true;
				m_x = m_rect.width;
				m_target_x = 0.0f;
				m_alpha = 0.0f;
				m_target_alpha = 1.0f;
				if (mp_parent)
				{
					if (m_shift)
					{
						mp_parent->shift(true);
					}
				}
			}

			return true;
		}

		virtual bool back()
		{
			if (m_active)
			{
				//pass action to existing item
				if (m_nodes.size() > 0 &&
					!m_nodes[m_index]->back())
				{
					//if action was not handled below, handle it here
					if (mp_parent)
					{
						m_active = false;
						m_target_x = m_rect.width;
						m_target_alpha = 0.0f;
						if (m_shift)
						{
							mp_parent->shift(false);
						}
					}
				}

				return true;
			}

			return false;
		}

	protected:
		bool m_active;
		bool m_shift;
		Rect m_rect;
		float m_target_x, m_x, m_alpha, m_target_alpha;
		float m_cell_height;
		unsigned int m_index;
		std::vector<Node*> m_nodes;
		MenuNodeRenderFunc mfp_node_render;
		MenuRenderFunc mfp_menu_render;

		virtual void render(const bool& is_selected, const Rect& rect, const float& alpha, const float& off_x, const float& off_y)
		{
			if (rect.width != 0 && rect.height != 0)
				Node::render(is_selected, rect, alpha, off_x, off_y);

			if (!is_selected)
				return;

			Rect rRect(rect.left, rect.top,
				m_rect.width, m_rect.height);

			if (m_shift == false)
			{
				rRect.left += m_rect.left;
				rRect.top += m_rect.top;
			}
			else
			{
				Node* p_head = mp_parent;
				while (p_head)
				{
					if (!p_head->mp_parent)
						break;

					p_head = p_head->mp_parent;
				}

				//top will always be a Menu, since we can't get into render any other way
				//ergo, this is actually safe
				rRect.left = ((Menu*)p_head)->m_rect.left;
				rRect.top = ((Menu*)p_head)->m_rect.top;
			}

			if ((!m_active &&
				((m_target_x > 0 && m_x > m_target_x - 1.0f) ||
				(m_target_x < 0 && m_x < m_target_x + 1.0f) ||
				(m_target_x == 0 && m_x == 0)))
				|| m_nodes.size() == 0)
				return;

			if (mfp_menu_render)
			{
				Rect offsetRect(rRect);
				offsetRect.left += m_x;
				mfp_menu_render(this, offsetRect, m_alpha, m_text);
			}

			rRect.height = m_cell_height;
			for (unsigned int i = 0; i < m_nodes.size(); i++)
			{
				if (i != m_index)
				{
					m_nodes[i]->render(false, rRect, m_alpha, m_x, rRect.height * i);
				}
			}

			m_nodes[m_index]->render(true, rRect, m_alpha, m_x, rRect.height * m_index);
		}

		virtual void shift(const bool& away)
		{
			if (!m_active)
				return;

			if (mp_parent)
			{
				mp_parent->shift(away);
			}

			if (away)
			{
				m_target_x = -m_rect.width;
				m_target_alpha = 0.0f;
			}
			else
			{
				m_target_x = 0;
				m_target_alpha = 1.0f;
			}
		}
	};

	class Menu_Select : public Menu
	{
	public:
		Menu_Select(const Rect& rect, const float& cell_height, const bool& shift = false, MENU_ARGS)
			:Menu(rect, cell_height, shift, MENU_PARAMS)
			, m_last_pressed_index(m_index)
		{}

		virtual bool press()
		{
			if (m_active)
			{
				//If we have an action, run it with the data of our selected item.  
				if (mfp_action)
				{
					mfp_action(m_nodes[m_index]);
				}

				//set our data equal to that of the selected item
				m_data = m_nodes[m_index]->get_data();

				//set index
				m_last_pressed_index = m_index;

				//go back
				back();

				//move to the right instead of the left to signify selection
				m_target_x = -m_rect.width;
				m_target_alpha = 0.0f;
			}
			else
			{
				Menu::press();
			}

			return true;
		}

		virtual bool back()
		{
			//set our index to the last pressed index
			m_index = m_last_pressed_index;

			return Menu::back();
		}

		virtual void set_data(const std::string& data)
		{
			Node::set_data(data);

			//set our index/last_press to the first item which has said data if it exists
			for (unsigned int i = 0; i < m_nodes.size(); i++)
			{
				if (m_nodes[i]->get_data() == data)
				{
					m_index = m_last_pressed_index = i;
					break;
				}
			}
		}

	protected:
		unsigned int m_last_pressed_index;
	};

	class Menu_Bool : public Menu_Select
	{
	public:
		Menu_Bool(const float& width, const float& cell_height, const std::string& true_text = "true", const std::string& false_text = "false", MENU_ARGS)
			:Menu_Select(Rect(0, 0, width, cell_height * 2), cell_height, false, MENU_PARAMS)
		{
			take_node(new Node(true_text, "true", NULL, mfp_render));
			take_node(new Node(false_text, "false", NULL, mfp_render));
		}
		
		virtual void render(const bool& is_selected, const Rect& rect, const float& alpha, const float& off_x, const float& off_y)
		{
			if (rect.width != 0 && rect.height != 0)
				Node::render(is_selected, rect, alpha, off_x, off_y);

			if (!is_selected)
				return;

			Menu::render(true, Rect(rect.left + rect.width + off_x - m_rect.width / 2.0f, rect.top + off_y - m_cell_height / 2.0f, 0, 0), alpha, 0, 0);
		}
	};

	class Menu_Bool_Display : public Menu_Bool
	{
	public:
		Menu_Bool_Display(const float& width, const float& cell_height, const std::string& true_text = "true", const std::string& false_text = "false", MENU_ARGS)
			:Menu_Bool(width, cell_height, true_text, false_text, MENU_PARAMS)
			, m_base_text(m_text)
		{
			m_text = m_base_text + ": " + m_nodes[m_index]->get_text();
		}

		virtual bool press()
		{
			Menu_Bool::press();

			m_text = m_base_text + ": " + m_nodes[m_index]->get_text();

			return true;
		}

		virtual void set_data(const std::string& data)
		{
			Menu_Bool::set_data(data);

			m_text = m_base_text + ": " + m_nodes[m_index]->get_text();
		}

	protected:
		std::string m_base_text;
	};

	class Menu_Number : public Menu
	{
	public:
		Menu_Number(const float& width, const float& height, const float& start, const float& min, const float& max, const float& interval, MENU_ARGS)
		:Menu(Rect(0, 0, width, height), height, false, MENU_PARAMS)
		, m_num(start)
		, m_last_num(start)
		, m_min(min)
		, m_max(max)
		, m_interval(interval)
		{
			take_node(new Node(std::to_string((long double)m_num), std::to_string((long double)m_num), NULL, mfp_render));
		}

		virtual bool up()
		{
			if (m_active)
			{
				m_num += m_interval;
				m_num = m_num > m_max ? m_max : m_num;
				m_num = round_to_nearest(m_num, m_interval);
				m_nodes[m_index]->set_text(std::to_string((long double)m_num));
				m_nodes[m_index]->set_data(std::to_string((long double)m_num));
				return true;
			}
			return false;
		}

		virtual bool down()
		{
			if (m_active)
			{
				m_num -= m_interval;
				m_num = m_num < m_min ? m_min : m_num;
				m_num = round_to_nearest(m_num, m_interval);
				m_nodes[m_index]->set_text(std::to_string((long double)m_num));
				m_nodes[m_index]->set_data(std::to_string((long double)m_num));
				return true;
			}
			return false;
		}

		virtual bool press()
		{
			if (m_active)
			{
				//set data
				m_nodes[m_index]->set_text(std::to_string((long double)m_num));
				m_nodes[m_index]->set_data(std::to_string((long double)m_num));
				m_last_num = m_num;

				//If we have an action, run it with the data of our selected item.  
				if (mfp_action)
				{
					mfp_action(m_nodes[m_index]);
				}

				//go back
				back();

				//move to the right instead of the left to signify selection
				m_target_x = -m_rect.width;
				m_target_alpha = 0.0f;
			}
			else
			{
				Menu::press();
			}

			return true;
		}

		virtual bool back()
		{
			//set our number to the last pressed number
			m_num = m_last_num;
			m_nodes[m_index]->set_text(std::to_string((long double)m_num));
			m_nodes[m_index]->set_data(std::to_string((long double)m_num));

			return Menu::back();
		}

		virtual void set_data(const std::string& data)
		{
			Node::set_data(data);

			m_num = m_last_num = (float)atof(data.c_str());
			m_nodes[m_index]->set_text(data);
			m_nodes[m_index]->set_data(data);
		}

	protected:
		float m_num, m_last_num, m_min, m_max, m_interval;

		virtual void render(const bool& is_selected, const Rect& rect, const float& alpha, const float& off_x, const float& off_y)
		{
			if (rect.width != 0 && rect.height != 0)
				Node::render(is_selected, rect, alpha, off_x, off_y);

			if (!is_selected)
				return;

			Menu::render(true, Rect(rect.left + rect.width + off_x - m_rect.width / 2.0f, rect.top + off_y - ((m_rect.height - rect.height) / 2.0f), 0, 0), alpha, 0, 0);
		}
	};

	class Menu_Number_Display : public Menu_Number
	{
	public:
		Menu_Number_Display(const float& width, const float& height, const float& start, const float& min, const float& max, const float& interval, MENU_ARGS)
			:Menu_Number(width, height, start, min, max, interval, MENU_PARAMS)
			, m_base_text(m_text)
		{
			m_text = m_base_text + ": " + std::to_string((long double)m_num);
		}

		virtual bool press()
		{
			Menu_Number::press();

			m_text = m_base_text + ": " + std::to_string((long double)m_num);

			return true;
		}

		virtual void set_data(const std::string& data)
		{
			Menu_Number::set_data(data);

			m_text = m_base_text + ": " + std::to_string((long double)m_num);
		}

	protected:
		std::string m_base_text;
	};

	class Menu_Text : public Menu
	{
	public:
		Menu_Text(const float& width, const float& height, MENU_ARGS)
		:Menu(Rect(0, 0, width, height), height, false, MENU_PARAMS)
		{
			take_node(new Node(m_data, m_data, NULL, mfp_render));
		}

		virtual bool press()
		{
			if (m_active)
			{
				//set data
				m_data = m_nodes[m_index]->get_data();

				//If we have an action, run it with the data of our selected item.  
				if (mfp_action)
				{
					mfp_action(m_nodes[m_index]);
				}

				//go back
				back();

				//move to the right instead of the left to signify selection
				m_target_x = -m_rect.width;
				m_target_alpha = 0.0f;
			}
			else
			{
				Menu::press();
			}

			return true;
		}

		virtual bool back()
		{
			m_nodes[m_index]->set_text(m_data);
			m_nodes[m_index]->set_data(m_data);

			return Menu::back();
		}

	protected:

		virtual void render(const bool& is_selected, const Rect& rect, const float& alpha, const float& off_x, const float& off_y)
		{
			if (rect.width != 0 && rect.height != 0)
				Node::render(is_selected, rect, alpha, off_x, off_y);

			if (!is_selected)
				return;

			Menu::render(true, Rect(rect.left + rect.width + off_x - m_rect.width / 2.0f, rect.top + off_y - ((m_rect.height - rect.height) / 2.0f), 0, 0), alpha, 0, 0);
		}
	};
}

#endif