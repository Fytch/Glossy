#include <iostream>
#include <exception>
#include <glossy/window.hpp>

int main( int argc, char** argv )
try {
	using namespace glossy;
	window w{ argc, argv };
	return w.run();
} catch( std::exception const& e ) {
	std::cerr << e.what() << '\n';
}
