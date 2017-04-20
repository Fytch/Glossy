#include <glossy/entities.hpp>

std::ostream& glossy::material::print( std::ostream& stream ) const {
	stream << "material( ";
	if( !checkered && !diffuse && !specular ) {
		stream << "0u";
	} else {
		bool separator = false;
		if( checkered ) {
			separator = true;
			stream << "mat_checkered";
		}
		if( diffuse ) {
			if( separator )
				stream << " | ";
			else
				separator = true;
			stream << "mat_diffuse";
		}
		if( specular ) {
			if( separator )
				stream << " | ";
			else
				separator = true;
			stream << "mat_specular";
		}
	}
	stream << ", vec3" << color << " )";
	return stream;
}

std::ostream& glossy::sphere::print( std::ostream& stream ) const {
	stream << type() << "( vec3" << position << ", " << radius << ", ";
	mat.print( stream );
	stream << " )";
	return stream;
}
char const* glossy::sphere::type() const {
	return "sphere";
}

std::ostream& glossy::plane::print( std::ostream& stream ) const {
	stream << type() << "( vec3" << position << ", vec3" << normal << ", ";
	mat.print( stream );
	stream << " )";
	return stream;
}
char const* glossy::plane::type() const {
	return "plane";
}

std::ostream& glossy::light::print( std::ostream& stream ) const {
	stream << "light( vec3" << position << ", vec3" << color << " )";
	return stream;
}
