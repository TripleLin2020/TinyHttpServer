#ifndef TINY_HTTP_SERVER_UINT_H
#define TINY_HTTP_SERVER_UINT_H

struct Uint {
    constexpr bool operator==(const Uint&) const { return true; }

    constexpr bool operator!=(const Uint&) const { return false; }
};


#endif //TINY_HTTP_SERVER_UINT_H
