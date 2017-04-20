#include <glossy/stopwatch.hpp>

#ifdef glossy_windows
LARGE_INTEGER glossy::stopwatch::m_frequency{};
#endif // glossy_windows
