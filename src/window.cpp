#include <glossy/window.hpp>
#include <glossy/default_scene.hpp>
#include <glossy/json2glsl.hpp>
#include <glossy/stopwatch.hpp>
#include <glossy/util.hpp>
#include <string>
#include <stdexcept>
#include <cmath>
#include <iostream>

void glossy::window::update_resolution( unsigned int width, unsigned int height ) {
	m_size.x = width;
	m_size.y = height;
	m_shader.setUniform( "resolution", sf::Glsl::Vec2{ static_cast< float >( m_size.x ), static_cast< float >( m_size.y ) } );
}

sf::Vector3f glossy::window::get_position() const {
	return m_pos;
}
void glossy::window::set_position( sf::Vector3f const& p ) {
	m_pos = p;
	m_shader.setUniform( "pos", m_pos );
}
void glossy::window::move( sf::Vector3f const& v ) {
	set_position( get_position() + v );
}

void glossy::window::rotate_x( float angle ) {
	m_alpha += angle;
	if( m_alpha < 0.0f )
		m_alpha += two_pi;
	if( m_alpha >= two_pi )
		m_alpha -= two_pi;
}
void glossy::window::rotate_y( float angle ) {
	m_beta += angle;
	m_beta = clamp( deg2rad( -90 ), deg2rad( 90 ), m_beta );
}
void glossy::window::update_camera() {
	const float sin_x = std::sin( m_alpha );
	const float cos_x = std::cos( m_alpha );
	const float sin_y = std::sin( m_beta );
	const float cos_y = std::cos( m_beta );
	m_at.x = sin_x *  cos_y;
	m_at.y =          sin_y;
	m_at.z = cos_x *  cos_y;
	m_up.x = sin_x * -sin_y;
	m_up.y =          cos_y;
	m_up.z = cos_x * -sin_y;
	m_right = cross( m_up, m_at );
	m_shader.setUniform( "at", m_at );
	m_shader.setUniform( "up", m_up );
	m_shader.setUniform( "right", m_right );
}

glossy::window::window( int argc, char** argv ) {
	if( argc > 2 )
		throw std::runtime_error{ "too many program options provided" };
	if( !sf::Shader::isAvailable() )
		throw std::runtime_error{ "sf::Shader not available!" };
	std::string code;
	if( argc == 2 )
		code = json2glsl( argv[ 1 ] );
	else
		code = json2glsl( default_scene );
	if( !m_shader.loadFromMemory( code, sf::Shader::Fragment ) )
		throw std::runtime_error{ "unable to process shader" };
	update_resolution( 1280, 720 );
	m_window.create( sf::VideoMode{ static_cast< unsigned >( m_size.x ), static_cast< unsigned >( m_size.y ) }, m_title, sf::Style::Default, sf::ContextSettings{ 0, 0, 0, 3, 0 } );
	m_window.setView( sf::View{ { 0, 1, 1, -1 } } );
	m_window.setMouseCursorVisible( false );
	m_window.setMouseCursorGrabbed( true );
	sf::Mouse::setPosition( m_size / 2, m_window );
	m_window.setKeyRepeatEnabled( false );
	// m_window.setVerticalSyncEnabled( true );
	set_position( { 0, 1, 0 } );
	update_camera();
}

int glossy::window::run() {
	stopwatch global_timer;
	global_timer.start();

	stopwatch frame_timer;
	frame_timer.start();

	stopwatch fps_timer;
	fps_timer.start();
	int frames = 0;

	bool forwards = false;
	bool left = false;
	bool backwards = false;
	bool right = false;
	bool up = false;
	bool down = false;

	while( m_window.isOpen() ) {
		++frames;
		const auto fps_elapsed = fps_timer.elapsed_s_flt();
		if( fps_elapsed >= 1.0 ) {
			fps_timer.start();
			m_window.setTitle( m_title + ( " - " + std::to_string( static_cast< unsigned >( frames / fps_elapsed + 0.5 ) ) + " FPS" ) );
			frames = 0;
		}

		for( sf::Event event; m_window.pollEvent( event ); ) {
			bool pressing = false;
			switch( event.type ) {
			case sf::Event::Closed:
				m_window.close();
				break;
			case sf::Event::Resized:
				update_resolution( event.size.width, event.size.height );
				break;
			case sf::Event::KeyPressed:
				pressing = true;
			case sf::Event::KeyReleased:
				switch( event.key.code ) {
				case sf::Keyboard::W:
				case sf::Keyboard::Up:
					forwards = pressing;
					break;
				case sf::Keyboard::A:
				case sf::Keyboard::Left:
					left = pressing;
					break;
				case sf::Keyboard::S:
				case sf::Keyboard::Down:
					backwards = pressing;
					break;
				case sf::Keyboard::D:
				case sf::Keyboard::Right:
					right = pressing;
					break;
				case sf::Keyboard::Space:
					up = pressing;
					break;
				case sf::Keyboard::LControl:
					down = pressing;
					break;
				case sf::Keyboard::F12:
					{
						sf::Texture tex;
						tex.create( m_size.x, m_size.y );
						tex.update( m_window );
						tex.copyToImage().saveToFile( "scrot.png" );
					}
				default:
					; // -Wswitch
				}
				break;
			case sf::Event::MouseMoved:
				if( m_window.hasFocus() ) {
					const sf::Vector2i center = m_size / 2;
					const int delta_x = -( center.x - event.mouseMove.x );
					const int delta_y =    center.y - event.mouseMove.y;
					constexpr float factor = 1.0f / 1000.0f;
					if( delta_x )
						rotate_x( delta_x * factor );
					if( delta_y )
						rotate_y( delta_y * factor );
					if( delta_x || delta_y ) {
						update_camera();
						sf::Mouse::setPosition( center, m_window );
					}
				}
				break;
			default:
				; // -Wswitch
			}
		}

		const auto frame_elapsed = frame_timer.elapsed_s_flt();
		frame_timer.start();

		if( forwards || left || backwards || right || up || down ) {
			sf::Vector3f offset = m_at * static_cast< float >( forwards - backwards ) + m_right * static_cast< float >( right - left ) + m_up * static_cast< float >( up - down );
			float speed = 5;
			if( sf::Keyboard::isKeyPressed( sf::Keyboard::LShift ) )
				speed *= 3;
			speed *= frame_elapsed;
			offset *= speed;
			move( offset );
		}

		m_shader.setUniform( "global_time", static_cast< float >( global_timer.elapsed_s_flt() ) );

		m_window.clear();
		m_window.draw( m_shape, &m_shader );
		m_window.display();
	}
	return 0;
}
