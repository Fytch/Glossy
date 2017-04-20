#ifndef glossy_json2glsl_hpp_included
#define glossy_json2glsl_hpp_included

#include <istream>
#include <string>

namespace glossy {
	std::string json2glsl( std::string const& filename );
	std::string json2glsl( char const* filename );
	std::string json2glsl( std::istream& stream );
}

#endif // !glossy_json2glsl_hpp_included
