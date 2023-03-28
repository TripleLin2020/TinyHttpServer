#ifndef TINY_HTTP_SERVER_VIA_COROUTINE_H
#define TINY_HTTP_SERVER_VIA_COROUTINE_H

#include <cassert>
#include <coroutine>
#include <utility>

#include "Executor.h"
#include "Traits.h"

class ViaCoroutine {
public:
    struct promise_type {
        struct FinalAwaiter;
        promise_type(Executor* ex) : _ex(ex), _ctx(Executor::NULLCTX) {}
        ViaCoroutine get_return_object() noexcept;
        void return_void() noexcept {}
        void unhandled_exception() const noexcept { assert(false); }
        std::suspend_always initial_suspend() const noexcept { return {}; }
        FinalAwaiter final_suspend() noexcept { return FinalAwaiter(_ctx); }

        struct FinalAwaiter {
            FinalAwaiter(Executor::Context ctx) : _ctx(ctx) {}
            bool await_ready() const noexcept { return false; }

            template <class PromiseType>
            auto await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
                if (auto& pr = h.promise(); pr._ex) {
                    auto f = [&pr]() { pr._handle.resume(); };
                    pr._ex->checkin(f, _ctx);
                } else {
                    pr._handle.resume();
                }
            }

            void await_resume() const noexcept {}

            Executor::Context _ctx;
        };

        std::coroutine_handle<> _handle;
        Executor* _ex;
        Executor::Context _ctx;
    };

    ViaCoroutine(std::coroutine_handle<promise_type> co) : _co(co) {}
    ~ViaCoroutine() {
        if (_co) {
            _co.destroy();
            _co = nullptr;
        }
    }

    ViaCoroutine(const ViaCoroutine&) = delete;
    ViaCoroutine& operator=(const ViaCoroutine&) = delete;
    ViaCoroutine(ViaCoroutine&& other) : _co(std::exchange(other._co, nullptr)) {}

    static ViaCoroutine create([[maybe_unused]] Executor* ex) { co_return; }

    void checkin() {
        auto& pr = _co.promise();
        if (pr._ex) {
            auto f = []() {};
            pr._ex->checkin(f, pr._ctx);
        }
    }

    std::coroutine_handle<> getWrappedContinuation(std::coroutine_handle<> continuation) {
        assert(_co);
        auto& pr = _co.promise();
        if (pr._ex) pr._ctx = pr._ex->checkout();
        pr._handle = continuation;
        return _co;
    }

private:
    std::coroutine_handle<promise_type> _co;
};

inline ViaCoroutine ViaCoroutine::promise_type::get_return_object() noexcept {
    return {std::coroutine_handle<ViaCoroutine::promise_type>::from_promise(*this)};
}

template <typename Awaiter>
struct [[nodiscard]] ViaAsyncAwaiter {
    template <typename Awaitable>
    ViaAsyncAwaiter(Executor* ex, Awaitable&& awaitable)
        : _ex(ex),
          _awaiter(getAwaiter(std::forward<Awaitable>(awaitable))),
          _viaCoroutine(ViaCoroutine::create(ex)) {}

    using HandleType = std::coroutine_handle<>;
    using AwaitSuspendResultType =
        decltype(std::declval<Awaiter&>().await_suspend(std::declval<HandleType>()));

    bool await_ready() { return _awaiter.await_ready(); }

    AwaitSuspendResultType await_suspend(HandleType continuation) {
        if constexpr (std::is_same_v<AwaitSuspendResultType, bool>) {
            bool shouldSuspend =
                _awaiter.await_suspend(_viaCoroutine.getWrappedContinuation(continuation));
            if (shouldSuspend == false) _viaCoroutine.checkin();
            return shouldSuspend;
        } else {
            return _awaiter.await_suspend(_viaCoroutine.getWrappedContinuation(continuation));
        }
    }

    auto await_resume() { return _awaiter.await_resume(); }

    Executor* _ex;
    Awaiter _awaiter;
    ViaCoroutine _viaCoroutine;
};

template <typename Awaitable, std::enable_if_t<!HasCoAwaitMethod<Awaitable>::value, int> = 0>
inline auto coAwait(Executor* ex, Awaitable&& awaitable) {
    using AwaiterType = decltype(getAwaiter(std::forward<Awaitable>(awaitable)));
    return ViaAsyncAwaiter<std::decay_t<AwaiterType>>(ex, std::forward<Awaitable>(awaitable));
}

template <typename Awaitable, std::enable_if_t<HasCoAwaitMethod<Awaitable>::value, int> = 0>
inline auto coAwait(Executor* ex, Awaitable&& awaitable) {
    return std::forward<Awaitable>(awaitable).coAwait(ex);
}

#endif  // TINY_HTTP_SERVER_VIA_COROUTINE_H