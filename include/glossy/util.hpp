#ifndef glossy_util_hpp_included
#define glossy_util_hpp_included

#include <SFML/Graphics.hpp>
#include <cmath>

namespace glossy {
	using vec2 = sf::Vector2f;
	inline std::ostream& operator<<( std::ostream& stream, vec2 const& vec ) {
		return stream << "( " << vec.x << ", " << vec.y << " )";
	}
	using vec3 = sf::Vector3f;
	inline std::ostream& operator<<( std::ostream& stream, vec3 const& vec ) {
		return stream << "( " << vec.x << ", " << vec.y << ", " << vec.z << " )";
	}

	struct strvec3 {
		std::string x, y, z;
	};
	inline std::ostream& operator<<( std::ostream& stream, strvec3 const& vec ) {
		return stream << "( (" << vec.x << "), (" << vec.y << "), (" << vec.z << ") )";
	}

	inline vec3 cross( vec3 u, vec3 v ) {
		return {
			u.y * v.z - u.z * v.y,
			u.z * v.x - u.x * v.z,
			u.x * v.y - u.y * v.x
		};
	}
	inline float dot( vec3 u, vec3 v ) {
		return u.x * v.x + u.y * v.y + u.z * v.z;
	}
	inline float norm_sq( vec3 v ) {
		return dot( v, v );
	}
	inline float norm( vec3 v ) {
		return std::sqrt( norm_sq( v ) );
	}
	inline vec3 normalize( vec3 v ) {
		return v / norm( v );
	}

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
