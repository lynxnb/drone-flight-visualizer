#include <chrono>

namespace dfv {
    using clock = std::chrono::steady_clock;
    using nanoseconds = std::chrono::nanoseconds; //!< Integer nanoseconds
    using milliseconds = std::chrono::milliseconds; //!< Integer milliseconds
    using milliseconds_f = std::chrono::duration<float, std::milli>; //!< Floating-point milliseconds
    using seconds_f = std::chrono::duration<float>; //!< Floating-point seconds
}
