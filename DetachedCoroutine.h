#ifndef TINY_HTTP_SERVER_DETACHED_COROUTINE_H
#define TINY_HTTP_SERVER_DETACHED_COROUTINE_H

#include <coroutine>
#include <exception>
#include <iostream>

struct DetachedCoroutine {
    struct promise_type {
        static std::suspend_never initial_suspend() noexcept { return {}; }
        static std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        static DetachedCoroutine get_return_object() noexcept { return {}; }
        static void unhandled_exception() {
            try {
                std::rethrow_exception(std::current_exception());
            } catch (const std::exception& e) {
                std::cerr << "Find an exception: " << e.what() << std::endl;
                std::rethrow_exception(std::current_exception());
            }
        }

        std::coroutine_handle<> _continuation = nullptr;
    };
};

#endif  // TINY_HTTP_SERVER_DETACHED_COROUTINE_H
