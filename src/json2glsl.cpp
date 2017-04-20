#include <SFML/System.hpp>
#include <glossy/json2glsl.hpp>
#include <glossy/entities.hpp>
#include <glossy/util.hpp>
#include <json.hpp>
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
			read( i, variable->material, "material" ) ||
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

	std::ostringstream code;
	code << "#version 130\n";

	// uniforms
	code << "uniform vec2 resolution;";
	code << "uniform float global_time;";
	code << "uniform vec3 pos;";
	code << "uniform vec3 at;";
	code << "uniform vec3 up;";
	code << "uniform vec3 right;";

	// constants
	code << "const int SS = " << SS << ";";
	code << "const float fovy = " << deg2rad( fovy ) << ";";
	code << "const float fovh = " << std::tan( deg2rad( fovy ) / 2.0 ) << ";";
	code << "const float no_hit = 1.0 / 0.0;";

	// background color
	code << "const vec3 background = vec3" << background << ";";

	// util funs
	code << "float sq( float x ) {"
			"	return x * x;"
			"}";
	code << "float normsq( vec3 v ) {"
			"	return dot( v, v );"
			"}";
	code << "void swap( inout float x, inout float y ) {"
			"	float temp = x;"
			"	x = y;"
			"	y = temp;"
			"}";

	// ray class
	code << "struct ray {"
			"	vec3 o;"
			"	vec3 d;"
			"};";
	code << "vec3 propagate( ray r, float dist ) {"
			"	return r.o + r.d * dist;"
			"}";

	// light class
	code << "struct light {"
			"	vec3 p;"
			"	vec3 col;"
			"};";

	// light description
	if( !lights.empty() ) {
		code << "light lights[ " << lights.size() << " ] = light[ " << lights.size() << " ](";
		for( std::size_t i = 0;; ) {
			code << "	";
			lights[ i ].print( code );
			if( ++i >= lights.size() )
				break;
			code << ",";
		}
		code << ");";
	}

	// forwards
	code << "vec3 pathtrace( ray r );";
	for( unsigned i = 1; i <= recursion; ++i )
		code << "vec3 pathtrace" << i << "( ray r );";
	code << "vec3 pathtracedummy( ray r ) {"
			"	return vec3( 1.0, 1.0, 1.0 );"
			"}"
			"bool visible( ray r, light l );";

	// diffuse lighting
	code << "vec3 diffuse( light l, vec3 col, vec3 p, vec3 n ) {"
			"	vec3 path = l.p - p;"
			"	float len = length( path );"
			"	path /= len;"
			"	ray lr = ray( p, path );"
			"	lr.o = propagate( lr, 1.0e-2 );"
			"	return float( visible( lr, l ) ) * l.col * col / sq( len ) * dot( path, n );"
			"}";

	// material class
	code << "struct material {"
			"	uint type;"
			"	vec3 col;"
			"};"
			"const uint mat_checkered	= 0x01u;"
			"const uint mat_diffuse		= 0x02u;"
			"const uint mat_specular	= 0x04u;";

	// materialize funs
	code << "\n"
			"#define gen_materialize( name, next )"
			"vec3 materialize##name( ray r, const material mat, vec3 glob, vec3 rel, vec3 n ) {"
			"	vec3 col = mat.col;"
			"	vec3 result = vec3( 0.0 );"
			"	float denom = 0.0;"
			"	if( ( mat.type & mat_checkered ) != 0u ) {"
			"		if( ( mod( rel.x, 2.0 ) < 1.0 ) ^^ ( mod( rel.z, 2.0 ) < 1.0 ) )"
			"			col *= 0.5;"
			"	}"
			"	if( ( mat.type & mat_diffuse ) != 0u ) {";
	if( lights.empty() ) {
		code << "		result += col * background;";
	} else {
		code << "		vec3 diff = vec3( 0.0 );"
				"		for( int i = 0; i < lights.length(); ++i )"
				"			diff += diffuse( lights[ i ], col, glob, n );"
				"		result += diff * 1.0;";
	}
	code << "		++denom;"
			"	}"
			"	if( ( mat.type & mat_specular ) != 0u ) {"
			"		ray ref = ray( glob, reflect( r.d, n ) );"
			"		ref.o += ref.d * 1.0e-3;"
			"		result += col * pathtrace##next( ref );"
			"		++denom;"
			"	}"
			"	return result / denom;"
			"}\n";
	for( unsigned i = 0; i <= recursion; ++i ) {
		code << "gen_materialize( ";
		if( i != 0 )
			code << i;
		code << ", ";
		if( i != recursion )
			code << i + 1;
		else
			code << "dummy";
		code << " );";
	}

	// eval funs
	code << "\n"
			"#define gen_eval_fun( type, name )"
			"void eval##name( ray r, inout vec3 color, inout float dist, const type obj ) {"
			"	float d = intersect( r, obj );"
			"	if( d != no_hit && d < dist && d < " << rendering_distance << " ) {"
			"		vec3 i = propagate( r, d );"
			"		color = materialize##name( r, obj.mat, i, i - obj.p, normal( i, obj ) );"
			"		dist = d;"
			"	}"
			"}\n"
			"#define gen_eval_funs( type )"
			"bool eval_occ( ray r, float dist, const type obj ) {"
			"	float d = intersect( r, obj );"
			"	return d != no_hit && d < dist;"
			"}";
	for( unsigned i = 0; i <= recursion; ++i ) {
		code << "gen_eval_fun( type, ";
		if( i != 0 )
			code << i;
		code << " )";
	}
	code << "\n";

	// sphere class
	code << "struct sphere {"
			"	vec3 p;"
			"	float r;"
			"	material mat;"
			"};"
			"float intersect( ray r, const sphere obj ) {"
			"	float ang = dot( r.d, r.o - obj.p );"
			"	float radicand = sq( ang ) - normsq( r.o - obj.p ) + sq( obj.r );"
			"	if( radicand < 0.0 )"
			"		return no_hit;"
			"	radicand = sqrt( radicand );"
			"	float r1 = -ang + radicand;"
			"	float r2 = -ang - radicand;"
			"	if( r1 > r2 )"
			"		swap( r1, r2 );"
			"	if( r2 < 0.0 ) {"
			"		return no_hit;"
			"	} else {"
			"		if( r1 < 0.0 )"
			"			return r2;"
			"		else"
			"			return r1;"
			"	}"
			"}"
			"vec3 normal( vec3 i, const sphere obj ) {"
			"	return normalize( i - obj.p );"
			"}"
			"gen_eval_funs( sphere )";

	// plane class
	code << "struct plane {"
			"	vec3 p;"
			"	vec3 n;"
			"	material mat;"
			"};"
			"float intersect( ray r, const plane obj ) {"
			"	float denom = dot( r.d, obj.n );"
			"	if( denom != 0.0 ) {"
			"		float result = dot( obj.p - r.o, obj.n ) / denom;"
			"		if( result > 0.0 )"
			"			return result;"
			"	}"
			"	return no_hit;"
			"}"
			"vec3 normal( vec3 i, const plane obj ) {"
			"	return normalize( obj.n );"
			"}"
			"gen_eval_funs( plane )";

	// scene description
	for( std::size_t i = 0; i < objects.size(); ++i ) {
		code << "const " << objects[ i ]->type() << " obj" << i << " = ";
		objects[ i ]->print( code );
		code << ";";
	}

	// pathtracing funs
	code << "\n"
			"#define gen_pathtrace( name )"
			"vec3 pathtrace##name( ray r ) {"
			"	vec3 col = background;"
			"	float dist = no_hit;";
	for( std::size_t i = 0; i < objects.size(); ++i ) {
		code << "	eval##name( r, col, dist, obj" << i << " );";
	}
	code << "	return col;"
			"}\n";
	for( unsigned i = 0; i <= recursion; ++i ) {
		code << "gen_pathtrace(";
		if( i != 0 )
			code << " " << i << " ";
		code << " )";
	}

	// visibility checker function
	if( objects.empty() ) {
		code << "bool visible( ray r, light l ) {"
				"	return true;"
				"}";
	} else {
		code << "bool visible( ray r, light l ) {"
				"	float dist = length( r.o - l.p );"
				"	return";
		std::size_t i;
		for( i = 0; i < objects.size() - 1; ++i )
			code << "		!eval_occ( r, dist, obj" << i << " ) &&";
		code << "		!eval_occ( r, dist, obj" << i << " );";
		code << "}";
	}

	code << "vec3 calc( vec2 screen_coord ) {"
			"	vec2 normalized = ( screen_coord - resolution / 2.0 ) * 2.0 / resolution.y * fovh;"
			"	ray pixelray;"
			"	pixelray.o = pos;"
			"	pixelray.d = normalize( at + normalized.x * right + normalized.y * up );"
			"	return pathtrace( pixelray );"
			"}";

	const vec2 sub{ 1.0f / SS, 1.0f / SS };
	const vec2 off = sub / 2.0f;

	// main function
	code << "void main() {"
			"	vec3 result = vec3( 0.0 );"
			"	for( int y = 0; y < SS; ++y ) {"
			"		for( int x = 0; x < SS; ++x ) {"
			"			vec2 subpix = gl_FragCoord.xy + vec2" << off << " + vec2" << sub << " * vec2( x, y );"
			"			result += calc( subpix );"
			"		}"
			"	}"
			"	gl_FragColor = vec4( result / sq( SS ), 1.0 );"
			"}";

	return code.str();
}
