#ifndef TINY_HTTP_SERVER_COMMON_H
#define TINY_HTTP_SERVER_COMMON_H

#include <stdexcept>

#if defined(__clang__)
#define INLINE [[clang::always_inline]]
#elif defined(__GNUC__)
#define INLINE [[gnu::always_inline]]
#else
#define INLINE inline
#endif

inline void logicAssert(bool b, const char* errorMsg) {
    if (!b) [[unlikely]] {
        throw std::logic_error(errorMsg);
    }
}

#endif //TINY_HTTP_SERVER_COMMON_H
