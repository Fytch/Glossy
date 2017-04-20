#ifndef glossy_stopwatch_hpp_included
#define glossy_stopwatch_hpp_included

#if defined( _WIN32 ) || defined( WIN32 )
	#define glossy_windows
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN 1
	#endif // !WIN32_LEAN_AND_MEAN
	#ifndef NOMINMAX
		#define NOMINMAX 1
	#endif // !NOMINMAX
	#ifndef STRICT
		#define STRICT 1
	#endif // !STRICT
	#include <windows.h>
#endif

#include <chrono>
#include <limits>
#include <cassert>

namespace glossy {
	class stopwatch {
#ifdef glossy_windows
		static LARGE_INTEGER m_frequency;
		LARGE_INTEGER m_start, m_end;
#else // glossy_windows
		typedef std::chrono::high_resolution_clock clock_t;
		std::chrono::time_point< clock_t > m_start, m_end;
#endif // glossy_windows

		bool m_running = false;

	public:
#ifdef glossy_windows
		stopwatch() {
			if( m_frequency.QuadPart == 0 ) {
				const BOOL result = QueryPerformanceFrequency( &m_frequency );
				assert( result );
				( void )result; // -Wunused-variable
			}
		}
#endif // glossy_windows

		void start() {
#ifdef glossy_windows
			const BOOL result = QueryPerformanceCounter( &m_start );
			assert( result );
			( void )result; // -Wunused-variable
#else // glossy_windows
			m_start = clock_t::now();
#endif // glossy_windows

			m_running = true;
		}
		void stop() {
			assert( is_running() );

#ifdef glossy_windows
			const BOOL result = QueryPerformanceCounter( &m_end );
			assert( result );
			( void )result; // -Wunused-variable
#else // glossy_windows
			m_end = clock_t::now();
#endif // glossy_windows

			m_running = false;
		}
		bool is_running() const noexcept {
			return m_running;
		}

		template< typename duration_t >
		duration_t elapsed() const {
#ifdef glossy_windows
			LARGE_INTEGER end;
			if( is_running() ) {
				const BOOL result = QueryPerformanceCounter( &end );
				assert( result );
				( void )result; // -Wunused-variable
			}
			else {
				end = m_end;
			}
			return std::chrono::duration_cast< duration_t >( std::chrono::duration< long double >{ static_cast< long double >( end.QuadPart - m_start.QuadPart ) / m_frequency.QuadPart } );
#else // glossy_windows
			std::chrono::time_point< clock_t > end;
			if( is_running() )
				end = clock_t::now();
			else
				end = m_end;
			return std::chrono::duration_cast< duration_t >( end - m_start );
#endif // glossy_windows
		}
		auto elapsed_ns() const {
			return elapsed< std::chrono::nanoseconds >().count();
		}
		auto elapsed_ns_flt() const {
			return elapsed< std::chrono::duration< long double, std::chrono::nanoseconds::period > >().count();
		}
		auto elapsed_us() const {
			return elapsed< std::chrono::microseconds >().count();
		}
		auto elapsed_us_flt() const {
			return elapsed< std::chrono::duration< long double, std::chrono::microseconds::period > >().count();
		}
		auto elapsed_ms() const {
			return elapsed< std::chrono::milliseconds >().count();
		}
		auto elapsed_ms_flt() const {
			return elapsed< std::chrono::duration< long double, std::chrono::milliseconds::period > >().count();
		}
		auto elapsed_s() const {
			return elapsed< std::chrono::seconds >().count();
		}
		auto elapsed_s_flt() const {
			return elapsed< std::chrono::duration< long double, std::chrono::seconds::period > >().count();
		}
	};
}

#endif // !glossy_stopwatch_hpp_included
