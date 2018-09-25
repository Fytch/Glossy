#include <SFML/System.hpp>
#include <glossy/json2glsl.hpp>
#include <glossy/entities.hpp>
#include <glossy/util.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <sstream>

using namespace nlohmann;

namespace {
	using namespace glossy;

	using obj_t = std::unique_ptr< object >;
	using objs_t = std::vector< std::unique_ptr< object > >;

	using lights_t = std::vector< light >;

	template< typename iter_t >
	bool discard( iter_t const& iter, std::string const& name ) {
		return iter.key() == name;
	}
	template< typename iter_t >
	bool read( iter_t const& iter, bool& variable, std::string const& name ) {
		const bool result = iter.key() == name;
		if( result ) {
			if( !iter->is_boolean() )
				throw std::runtime_error{ name + " must be of type bool" };
			variable = iter->template get< bool >();
		}
		return result;
	}
	template< typename iter_t >
	bool read( iter_t const& iter, unsigned& variable, std::string const& name ) {
		const bool result = iter.key() == name;
		if( result ) {
			if( !iter->is_number_unsigned() )
				throw std::runtime_error{ name + " must be of type unsigned" };
			variable = iter->template get< unsigned >();
		}
		return result;
	}
	template< typename iter_t >
	bool read( iter_t const& iter, float& variable, std::string const& name ) {
		const bool result = iter.key() == name;
		if( result ) {
			if( !iter->is_number_float() )
				throw std::runtime_error{ name + " must be of type float" };
			variable = iter->template get< float >();
		}
		return result;
	}
	template< typename iter_t >
	bool read( iter_t const& iter, vec3& variable, std::string const& name ) {
		const bool result = iter.key() == name;
		if( result ) {
			if( !iter->is_array() || iter->size() != 3 )
				throw std::runtime_error{ name + " must be of type vec3" };
			for( std::size_t i = 0; i < 3; ++i ) {
				auto& source = ( *iter )[ i ];
				auto& target = ( i == 0 ? variable.x : ( i == 1 ? variable.y : variable.z ) );
				if( source.is_number() )
					target = source.template get< float >();
				else
					throw std::runtime_error{ name + "'s components must be numbers" };
			}
		}
		return result;
	}
	template< typename iter_t >
	bool read( iter_t const& iter, strvec3& variable, std::string const& name ) {
		const bool result = iter.key() == name;
		if( result ) {
			if( !iter->is_array() || iter->size() != 3 )
				throw std::runtime_error{ name + " must be of type strvec3" };
			for( std::size_t i = 0; i < 3; ++i ) {
				auto& source = ( *iter )[ i ];
				auto& target = ( i == 0 ? variable.x : ( i == 1 ? variable.y : variable.z ) );
				if( source.is_number() )
					target = std::to_string( source.template get< float >() );
				else if( source.is_string() )
					target = source.template get< std::string >();
				else
					throw std::runtime_error{ name + "'s components must be primitives" };
			}
		}
		return result;
	}
	template< typename iter_t >
	bool read( iter_t const& iter, material& variable, std::string const& name ) {
		const bool result = iter.key() == name;
		if( result ) {
			if( !iter->is_object() )
				throw std::runtime_error{ name + " must be of type object" };
			for( auto i = iter->cbegin(); i != iter->cend(); ++i ) {
				read( i, variable.color, "color" ) ||
				read( i, variable.checkered, "checkered" ) ||
				read( i, variable.diffuse, "diffuse" ) ||
				read( i, variable.specular, "specular" ) ||
				( throw std::runtime_error{ "unrecognized material property: " + i.key() }, false );
			}
		}
		return result;
	}
	bool read( json const& j, obj_t& variable ) {
		if( !j.is_object() )
			return false;
		const auto shape_iter = j.find( "shape" );
		if( shape_iter == j.end() )
			throw std::runtime_error{ "objects must define the shape property" };
		if( !shape_iter->is_string() )
			throw std::runtime_error{ "shape must be a string" };
		const auto&& shape = shape_iter->get< std::string >();
		bool is_sphere = false;
		bool is_plane = false;
		if( ( is_sphere = ( shape == "sphere" ) ) ) // -Wparentheses
			variable = std::make_unique< sphere >();
		else if( ( is_plane = ( shape == "plane" ) ) ) // -Wparentheses
			variable = std::make_unique< plane >();
		else
			throw std::runtime_error{ "unrecognized shape: " + shape };
		for( auto i = j.cbegin(); i != j.cend(); ++i ) {
			discard( i, "shape" ) ||
			read( i, variable->position, "position" ) ||
			( is_sphere && read( i, static_cast< sphere* >( &*variable )->radius, "radius" ) ) ||
			( is_plane && read( i, static_cast< plane* >( &*variable )->normal, "normal" ) ) ||
			read( i, variable->mat, "material" ) ||
			( throw std::runtime_error{ "unrecognized object property: " + i.key() }, false );
		}

		return true;
	}
	bool read( json const& j, light& variable ) {
		if( !j.is_object() )
			return false;
		for( auto i = j.cbegin(); i != j.cend(); ++i ) {
			read( i, variable.position, "position" ) ||
			read( i, variable.color, "color" ) ||
			( throw std::runtime_error{ "unrecognized light property: " + i.key() }, false );
		}

		return true;
	}
	template< typename iter_t, typename T >
	bool read( iter_t const& iter, std::vector< T >& container, std::string const& name ) {
		const bool result = iter.key() == name;
		if( result ) {
			if( !iter->is_array() )
				throw std::runtime_error{ name + " must be an array" };
			for( auto const& i : *iter ) {
				T entity;
				if( !read( i, entity ) )
					throw std::runtime_error{ name + " must only contain valid objects" };
				container.emplace_back( std::move( entity ) );
			}
		}
		return result;
	}
}

std::string glossy::json2glsl( std::string const& filename ) {
	return json2glsl( filename.c_str() );
}
std::string glossy::json2glsl( char const* filename ) {
	std::ifstream file{ filename };
	if( !file )
		throw std::runtime_error{ "unable to load file" };
	return json2glsl( file );
}
std::string glossy::json2glsl( std::istream& stream ) {
	json j;
	stream >> j;

	unsigned SS = 1;
	float fovy = 60.0;
	vec3 background{ 0.0, 0.0, 0.0 };
	unsigned recursion = 0;
	float rendering_distance = 50.0;
	lights_t lights;
	objs_t objects;

	for( auto i = j.cbegin(); i != j.cend(); ++i ) {
		read( i, SS, "SS" ) ||
		read( i, fovy, "fovy" ) ||
		read( i, background, "background" ) ||
		read( i, recursion, "recursion" ) ||
		read( i, rendering_distance, "rendering_distance" ) ||
		read( i, lights, "lights" ) ||
		read( i, objects, "objects" ) ||
		( throw std::runtime_error{ "unrecognized option: " + i.key() }, false );
	}

	if( SS == 0 )
		throw std::range_error{ "SS must be positive" };
	if( fovy <= 0.0 || fovy >= 180.0 )
		throw std::range_error{ "fovy must be in (0, 180)" };
	if( background.x < 0.0 || background.x > 1.0 )
		throw std::range_error{ "background.r must be in [0, 1]" };
	if( background.y < 0.0 || background.y > 1.0 )
		throw std::range_error{ "background.g must be in [0, 1]" };
	if( background.z < 0.0 || background.z > 1.0 )
		throw std::range_error{ "background.b must be in [0, 1]" };
	if( rendering_distance <= 0.0 )
		throw std::range_error{ "rendering_distance must be positive" };

	const auto gen_eval_funs = [ & ]( std::ostream& stream, char const* type ) -> decltype( auto ) {
		stream <<
			"bool eval_occ( ray r, float dist, const " << type << " obj ) {\n"
			"	float d = intersect( r, obj );\n"
			"	return d != no_hit && d < dist;\n"
			"}\n";
		for( unsigned i = 0; i <= recursion; ++i ) {
		stream <<
			"void eval" << i << "( ray r, inout vec3 color, inout float dist, const " << type << " obj ) {\n"
			"	float d = intersect( r, obj );\n"
			"	if( d != no_hit && d < dist && d < " << rendering_distance << " ) {\n"
			"		vec3 i = propagate( r, d );\n"
			"		color = materialize" << i << "( r, obj.mat, i, i - obj.p, normal( i, obj ) );\n"
			"		dist = d;\n"
			"	}\n"
			"}\n";
		}
		return stream;
	};

	std::ostringstream code;
	code << "#version 130\n\n";

	// uniforms
	code << "uniform vec2 resolution;\n";
	code << "uniform float global_time;\n";
	code << "uniform vec3 pos;\n";
	code << "uniform vec3 at;\n";
	code << "uniform vec3 up;\n";
	code << "uniform vec3 right;\n\n";

	// constants
	code << "const int SS = " << SS << ";\n";
	code << "const float fovy = " << deg2rad( fovy ) << ";\n";
	code << "const float fovh = " << std::tan( deg2rad( fovy ) / 2.0 ) << ";\n";
	code << "const float no_hit = 1.0 / 0.0;\n";

	// background color
	code << "const vec3 background = vec3" << background << ";\n\n";

	// util funs
	code << "float sq( float x ) {\n"
			"	return x * x;\n"
			"}\n";
	code << "float normsq( vec3 v ) {\n"
			"	return dot( v, v );\n"
			"}\n";
	code << "void swap( inout float x, inout float y ) {\n"
			"	float temp = x;\n"
			"	x = y;\n"
			"	y = temp;\n"
			"}\n\n";

	// ray class
	code << "struct ray {\n"
			"	vec3 o;\n"
			"	vec3 d;\n"
			"};\n";
	code << "vec3 propagate( ray r, float dist ) {\n"
			"	return r.o + r.d * dist;\n"
			"}\n\n";

	// light class
	code << "struct light {\n"
			"	vec3 p;\n"
			"	vec3 col;\n"
			"};\n";

	// light description
	if( !lights.empty() ) {
		code << "light lights[ " << lights.size() << " ] = light[ " << lights.size() << " ](";
		for( std::size_t i = 0;; ) {
			code << "\n\t";
			lights[ i ].print( code );
			if( ++i >= lights.size() )
				break;
			code << ",";
		}
		code << "\n);\n\n";
	}

	// forwards
	for( unsigned i = 0; i <= recursion; ++i )
		code << "vec3 pathtrace" << i << "( ray r );\n";
	code << "vec3 pathtracedummy( ray r ) {\n"
			"	return vec3( 1.0, 1.0, 1.0 );\n"
			"}\n"
			"bool visible( ray r, light l );\n\n";

	// diffuse lighting
	code << "vec3 diffuse( light l, vec3 col, vec3 p, vec3 n ) {\n"
			"	vec3 path = l.p - p;\n"
			"	float len = length( path );\n"
			"	path /= len;\n"
			"	ray lr = ray( p, path );\n"
			"	lr.o = propagate( lr, 1.0e-2 );\n"
			"	return float( visible( lr, l ) ) * l.col * col / sq( len ) * dot( path, n );\n"
			"}\n\n";

	// material class
	code << "struct material {\n"
			"	uint type;\n"
			"	vec3 col;\n"
			"};\n"
			"const uint mat_checkered = 0x01u;\n"
			"const uint mat_diffuse   = 0x02u;\n"
			"const uint mat_specular  = 0x04u;\n\n";

	// materialize funs
	std::string materialize_name;
	std::string materialize_tail_name = "0";
	for( unsigned i = 0; i <= recursion; ++i ) {
		materialize_name = std::move( materialize_tail_name );
		if( i != recursion )
			materialize_tail_name = std::to_string( i + 1 );
		else
			materialize_tail_name = "dummy";
		code << "vec3 materialize" << materialize_name << "( ray r, const material mat, vec3 glob, vec3 rel, vec3 n ) {\n"
				"	vec3 col = mat.col;\n"
				"	vec3 result = vec3( 0.0 );\n"
				"	float denom = 0.0;\n"
				"	if( ( mat.type & mat_checkered ) != 0u ) {\n"
				"		if( ( mod( rel.x, 2.0 ) < 1.0 ) ^^ ( mod( rel.z, 2.0 ) < 1.0 ) )\n"
				"			col *= 0.5;"
				"	}\n"
				"	if( ( mat.type & mat_diffuse ) != 0u ) {\n";
		if( lights.empty() ) {
			code << "		result += col * background;\n";
		} else {
			code << "		vec3 diff = vec3( 0.0 );\n"
					"		for( int i = 0; i < lights.length(); ++i )\n"
					"			diff += diffuse( lights[ i ], col, glob, n );\n"
					"		result += diff * 1.0;\n";
		}
		code << "		++denom;\n"
				"	}\n"
				"	if( ( mat.type & mat_specular ) != 0u ) {\n"
				"		ray ref = ray( glob, reflect( r.d, n ) );\n"
				"		ref.o += ref.d * 1.0e-3;\n"
				"		result += col * pathtrace" << materialize_tail_name << "( ref );\n"
				"		++denom;\n"
				"	}\n"
				"	return result / denom;\n"
				"}\n\n";
	}

	// sphere class
	code << "struct sphere {\n"
			"	vec3 p;\n"
			"	float r;\n"
			"	material mat;\n"
			"};\n"
			"float intersect( ray r, const sphere obj ) {\n"
			"	float ang = dot( r.d, r.o - obj.p );\n"
			"	float radicand = sq( ang ) - normsq( r.o - obj.p ) + sq( obj.r );\n"
			"	if( radicand < 0.0 )\n"
			"		return no_hit;\n"
			"	radicand = sqrt( radicand );\n"
			"	float r1 = -ang + radicand;\n"
			"	float r2 = -ang - radicand;\n"
			"	if( r1 > r2 )\n"
			"		swap( r1, r2 );\n"
			"	if( r2 < 0.0 ) {\n"
			"		return no_hit;\n"
			"	} else {\n"
			"		if( r1 < 0.0 )\n"
			"			return r2;\n"
			"		else\n"
			"			return r1;\n"
			"	}\n"
			"}\n"
			"vec3 normal( vec3 i, const sphere obj ) {\n"
			"	return normalize( i - obj.p );\n"
			"}\n";
	gen_eval_funs( code, "sphere" ) << '\n';

	// plane class
	code << "struct plane {\n"
			"	vec3 p;\n"
			"	vec3 n;\n"
			"	material mat;\n"
			"};\n"
			"float intersect( ray r, const plane obj ) {\n"
			"	float denom = dot( r.d, obj.n );\n"
			"	if( denom != 0.0 ) {\n"
			"		float result = dot( obj.p - r.o, obj.n ) / denom;\n"
			"		if( result > 0.0 )\n"
			"			return result;\n"
			"	}\n"
			"	return no_hit;\n"
			"}\n"
			"vec3 normal( vec3 i, const plane obj ) {\n"
			"	return normalize( obj.n );\n"
			"}\n";
	gen_eval_funs( code, "plane" ) << '\n';

	// scene description
	for( std::size_t i = 0; i < objects.size(); ++i ) {
		code << "const " << objects[ i ]->type() << " obj" << i << " = ";
		objects[ i ]->print( code );
		code << ";\n";
	}
	code << '\n';

	// pathtracing funs
	for( unsigned i = 0; i <= recursion; ++i ) {
		code << "vec3 pathtrace" << i << "( ray r ) {\n"
				"	vec3 col = background;\n"
				"	float dist = no_hit;\n";
		for( std::size_t j = 0; j < objects.size(); ++j ) {
			code << "	eval" << i << "( r, col, dist, obj" << j << " );\n";
		}
		code << "	return col;\n"
				"}\n";
	}
	code << '\n';

	// visibility checker function
	if( objects.empty() ) {
		code << "bool visible( ray r, light l ) {\n"
				"	return true;\n"
				"}\n\n";
	} else {
		code << "bool visible( ray r, light l ) {\n"
				"	float dist = length( r.o - l.p );\n"
				"	return";
		std::size_t i;
		for( i = 0; i < objects.size() - 1; ++i )
			code << "\n\t\t!eval_occ( r, dist, obj" << i << " ) &&";
		code << "\n\t\t!eval_occ( r, dist, obj" << i << " );\n";
		code << "}\n\n";
	}

	code << "vec3 calc( vec2 screen_coord ) {\n"
			"	vec2 normalized = ( screen_coord - resolution / 2.0 ) * 2.0 / resolution.y * fovh;\n"
			"	ray pixelray;\n"
			"	pixelray.o = pos;\n"
			"	pixelray.d = normalize( at + normalized.x * right + normalized.y * up );\n"
			"	return pathtrace0( pixelray );\n"
			"}\n\n";

	const vec2 sub{ 1.0f / SS, 1.0f / SS };
	const vec2 off = sub / 2.0f;

	// main function
	code << "void main() {\n"
			"	vec3 result = vec3( 0.0 );\n";

	if( SS != 1 ) {
		code << "	for( int y = 0; y < SS; ++y ) {\n"
				"		for( int x = 0; x < SS; ++x ) {\n"
				"			vec2 subpix = gl_FragCoord.xy + vec2" << off << " + vec2" << sub << " * vec2( x, y );\n"
				"			result += calc( subpix );\n"
				"		}\n"
				"	}\n"
				"	gl_FragColor = vec4( result / sq( SS ), 1.0 );\n";
	} else {
		code << "	vec2 subpix = gl_FragCoord.xy + vec2" << off << ";\n"
				"	result += calc( subpix );\n"
				"	gl_FragColor = vec4( result, 1.0 );\n";
	}
	code << "}\n";

	return code.str();
}
