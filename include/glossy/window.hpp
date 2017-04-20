#ifndef glossy_window_hpp_included
#define glossy_window_hpp_included

#include <SFML/Graphics.hpp>

namespace glossy {
	class window {
		sf::Shader m_shader;
		sf::Vector2i m_size;
		sf::RenderWindow m_window;
		char const* const m_title = "Glossy";
		sf::RectangleShape m_shape{ { 1, 1 } };
		sf::Vector3f m_pos;
		float m_alpha = 0;
		float m_beta = 0;
		sf::Vector3f m_at{ 0, 0, 1 };
		sf::Vector3f m_up{ 0, 1, 0 };
		sf::Vector3f m_right;

	protected:
		void update_resolution( unsigned int width, unsigned int height );

		sf::Vector3f get_position() const;
		void set_position( sf::Vector3f const& p );
		void move( sf::Vector3f const& v );

		void rotate_x( float angle );
		void rotate_y( float angle );
		void update_camera();

	public:
		window( int argc, char** argv );

		int run();
	};
}

#endif // !glossy_window_hpp_included
