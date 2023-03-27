#ifndef TINY_HTTP_SERVER_TRAITS_H
#define TINY_HTTP_SERVER_TRAITS_H

#include <utility>

template <typename, typename = void>
struct HasCoAwaitMethod : std::false_type {};

template <typename T>
struct HasCoAwaitMethod<T, std::void_t<decltype(std::declval<T>().coAwait(nullptr))>>
    : std::true_type {};

template <typename, typename = void>
struct HasMemberCoAwaitOperator : std::false_type {};

template <typename T>
struct HasMemberCoAwaitOperator<T, std::void_t<decltype(std::declval<T>().operator co_await())>>
    : std::true_type {};

template <class, class = void>
struct HasGlobalCoAwaitOperator : std::false_type {};

template <typename T>
struct HasGlobalCoAwaitOperator<T, std::void_t<decltype(operator co_await(std::declval<T>()))>>
    : std::true_type {};

template <typename Awaitable, std::enable_if_t<HasMemberCoAwaitOperator<Awaitable>::value, int> = 0>
auto getAwaiter(Awaitable&& awaitable) {
    return std::forward<Awaitable>(awaitable).operator co_await();
}

template <typename Awaitable, std::enable_if_t<HasGlobalCoAwaitOperator<Awaitable>::value, int> = 0>
auto getAwaiter(Awaitable&& awaitable) {
    return operator co_await(std::forward<Awaitable>(awaitable));
}

template <typename Awaitable,
          std::enable_if_t<!HasMemberCoAwaitOperator<Awaitable>::value, int> = 0,
          std::enable_if_t<!HasGlobalCoAwaitOperator<Awaitable>::value, int> = 0>
auto getAwaiter(Awaitable&& awaitable) {
    return std::forward<Awaitable>(awaitable);
}

#endif  // TINY_HTTP_SERVER_TRAITS_H
