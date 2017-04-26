#ifndef glossy_util_hpp_included
#define glossy_util_hpp_included

#include <SFML/Graphics.hpp>
#include <ostream>
#include <string>

namespace glossy {
	using vec2 = sf::Vector2f;
	std::ostream& operator<<( std::ostream& stream, vec2 const& vec );
	using vec3 = sf::Vector3f;
	std::ostream& operator<<( std::ostream& stream, vec3 const& vec );

	struct strvec3 {
		std::string x, y, z;
	};
	std::ostream& operator<<( std::ostream& stream, strvec3 const& vec );

	vec3 cross( vec3 u, vec3 v );
	float dot( vec3 u, vec3 v );
	float norm_sq( vec3 v );
	float norm( vec3 v );
	vec3 normalize( vec3 v );

	constexpr float pi = 3.14159265359f;
	constexpr float two_pi = 6.28318530718f;

	constexpr float deg2rad( float deg ) {
		return deg / 360.0f * two_pi;
	}

	template< typename T >
	T clamp( T const& min, T const& max, T const& value ) {
		return value < min ? min : value > max ? max : value;
	}
}

#endif // !glossy_util_hpp_included
