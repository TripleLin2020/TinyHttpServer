#ifndef TINY_HTTP_SERVER_UINT_H
#define TINY_HTTP_SERVER_UINT_H

/// Unit plays the role of a simplest type in case we couldn't use void directly.
struct Uint {
    constexpr bool operator==(const Uint&) const { return true; }
    constexpr bool operator!=(const Uint&) const { return false; }
};

#endif  // TINY_HTTP_SERVER_UINT_H
