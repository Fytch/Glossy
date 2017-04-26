#include <glossy/util.hpp>
#include <cmath>

std::ostream& glossy::operator<<( std::ostream& stream, vec2 const& vec ) {
	return stream << "( " << vec.x << ", " << vec.y << " )";
}
std::ostream& glossy::operator<<( std::ostream& stream, vec3 const& vec ) {
	return stream << "( " << vec.x << ", " << vec.y << ", " << vec.z << " )";
}

namespace {
	std::ostream& print_strvec_component( std::ostream& stream, std::string const& component ) {
		const bool parentheses = ( component.find( ',' ) != component.npos );
		if( parentheses )
			stream << "( ";
		stream << component;
		if( parentheses )
			stream << " )";
		return stream;
	}
}

std::ostream& glossy::operator<<( std::ostream& stream, strvec3 const& vec ) {
	stream << "( ";
	::print_strvec_component( stream, vec.x ) << ", ";
	::print_strvec_component( stream, vec.y ) << ", ";
	::print_strvec_component( stream, vec.z );
	stream << " )";
	return stream;
}

glossy::vec3 glossy::cross( vec3 u, vec3 v ) {
	return {
		u.y * v.z - u.z * v.y,
		u.z * v.x - u.x * v.z,
		u.x * v.y - u.y * v.x
	};
}
float glossy::dot( vec3 u, vec3 v ) {
	return u.x * v.x + u.y * v.y + u.z * v.z;
}
float glossy::norm_sq( vec3 v ) {
	return dot( v, v );
}
float glossy::norm( vec3 v ) {
	return std::sqrt( norm_sq( v ) );
}
glossy::vec3 glossy::normalize( vec3 v ) {
	return v / norm( v );
}
