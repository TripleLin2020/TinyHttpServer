#ifndef TINY_HTTP_SERVER_LAZY_H
#define TINY_HTTP_SERVER_LAZY_H

#include <cassert>
#include <coroutine>
#include <variant>

#include "Common.h"
#include "DetachedCoroutine.h"
#include "Executor.h"
#include "Try.h"

template <typename T>
class Lazy;

class LazyPromiseBase {
public:
    class FinalAwaiter {
    public:
        static bool await_ready() noexcept { return false; }

        template <typename PromiseType>
        auto await_suspend(std::coroutine_handle<PromiseType> h) noexcept {
            return h.promise()._handle;
        }

        void await_resume() noexcept {}
    };

    class YieldAwaiter {
    public:
        YieldAwaiter(Executor* executor) : _executor(executor) {}

        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> handle) {
            logicAssert(_executor, "Yielding is only meaningful with an executor!");
            _executor->schedule([handle]() { handle.resume(); });
        }

        void await_resume() noexcept {}

    private:
        Executor* _executor;
    };

public:
    LazyPromiseBase() : _executor(nullptr) {}

    std::suspend_always initial_suspend() noexcept { return {}; }

    FinalAwaiter final_suspend() noexcept { return {}; }

public:
    std::coroutine_handle<> _handle;
    Executor* _executor;
};

template <typename T>
class LazyPromise : public LazyPromiseBase {
public:
    LazyPromise() noexcept = default;

    ~LazyPromise() noexcept = default;

    Lazy<T> get_return_object() noexcept { return Lazy<T>(Lazy<T>::Handle::from_promise(*this)); }

    template <typename V>
        requires std::is_convertible_v<V&&, T>
    void return_value(V&& value) noexcept(std::is_nothrow_constructible_v<T, V&&>) {
        _value.template emplace<T>(std::forward<V>(value));
    }

    void unhandled_exception() noexcept {
        _value.template emplace<std::exception_ptr>(std::current_exception());
    }

    T& result() & {
        if (std::holds_alternative<std::exception_ptr>(_value)) [[unlikely]] {
            std::rethrow_exception(std::get<std::exception_ptr>(_value));
        }
        assert(std::holds_alternative<T>(_value));
        return std::get<T>(_value);
    }

    T&& result() && {
        if (std::holds_alternative<std::exception_ptr>(_value)) [[unlikely]] {
            std::rethrow_exception(std::get<std::exception_ptr>(_value));
        }
        assert(std::holds_alternative<T>(_value));
        return std::move(std::get<T>(_value));
    }

    Try<T> tryResult() noexcept {
        if (std::holds_alternative<std::exception_ptr>(_value)) [[unlikely]] {
            return Try<T>(std::get<std::exception_ptr>(_value));
        } else {
            assert(std::holds_alternative<T>(_value));
            return Try<T>(std::move(std::get<T>(_value)));
        }
    }

public:
    std::variant<std::monostate, T, std::exception_ptr> _value;
};

template <>
class LazyPromise<void> : public LazyPromiseBase {
public:
    Lazy<void> get_return_object() noexcept;

    void return_void() noexcept {}

    void unhandled_exception() noexcept { _exception = std::current_exception(); }

    void result() {
        if (_exception) [[unlikely]]
            std::rethrow_exception(_exception);
    }

    Try<void> tryResult() noexcept { return Try<void>(_exception); }

public:
    std::exception_ptr _exception{nullptr};
};

template <typename T>
class RescheduleLazy;

template <typename T>
class LazyAwaiterBase {
public:
    using Handle = std::coroutine_handle<LazyPromise<T>>;

public:
    LazyAwaiterBase(LazyAwaiterBase& other) = delete;

    LazyAwaiterBase& operator=(LazyAwaiterBase& other) = delete;

    LazyAwaiterBase(LazyAwaiterBase&& other) : _handle(std::exchange(other._handle, nullptr)) {}

    LazyAwaiterBase& operator=(LazyAwaiterBase&& other) noexcept {
        std::swap(_handle, other._handle);
        return *this;
    }

    LazyAwaiterBase(Handle co) : _handle(co) {}

    ~LazyAwaiterBase() {
        if (_handle) {
            _handle.destroy();
            _handle = nullptr;
        }
    }

    bool await_ready() const noexcept { return false; }

    template <typename T2 = T>
        requires std::is_void_v<T2>
    void awaitResume() {
        _handle.promise().result();
        _handle.destroy();
        _handle = nullptr;
    }

    template <typename T2 = T>
        requires(!std::is_void_v<T2>)
    T2 awaitResume() {
        auto r = std::move(_handle.promise().result());
        _handle.destroy();
        _handle = nullptr;
        return r;
    }

    Try<T> awaitResumeTry() noexcept {
        Try<T> ret = std::move(_handle.promise().tryResult());
        _handle.destroy();
        _handle = nullptr;
        return ret;
    }

public:
    Handle _handle;
};

template <typename T, bool reschedule>
class LazyBase {
public:
    using promise_type = LazyPromise<T>;
    using Handle = std::coroutine_handle<promise_type>;
    using ValueType = T;

    struct AwaiterBase : public LazyAwaiterBase<T> {
        using Base = LazyAwaiterBase<T>;

        explicit AwaiterBase(Handle co) : Base(co) {}

        INLINE auto await_suspend(std::coroutine_handle<> handle) noexcept {
            this->_handle.promise()._handle = handle;

            using R = std::conditional_t<reschedule, void, std::coroutine_handle<>>;
            return awaitSuspendImpl<R>();
        }

    private:
        template <std::same_as<std::coroutine_handle<>> R>
        auto awaitSuspendImpl() noexcept {
            return this->_handle;
        }

        template <std::same_as<void> R>
        auto awaitSuspendImpl() noexcept {
            auto& pr = this->_handle.promise();
            logicAssert(pr._executor, "RescheduleLazy need executor");
            pr._executor->schedule([h = this->_handle]() { h.resume(); });
        }
    };

    struct TryAwaiter : public AwaiterBase {
        explicit TryAwaiter(Handle h) : AwaiterBase(h) {}

        Try<T> await_resume() noexcept { return AwaiterBase::awaitResumeTry(); }
    };

    struct ValueAwaiter : public AwaiterBase {
        ValueAwaiter(Handle co) : AwaiterBase(co) {}

        INLINE T await_resume() { return AwaiterBase::awaitResume(); }
    };

public:
    explicit LazyBase(Handle coroutine) : _co(coroutine) {}

    LazyBase(LazyBase&& other) : _co(std::move(other._co)) { other._co = nullptr; }

    LazyBase(const LazyBase&) = delete;

    LazyBase& operator=(const LazyBase&) = delete;

    ~LazyBase() {
        if (_co) {
            _co.destroy();
            _co = nullptr;
        }
    }

    Executor* getExecutor() { return _co.promise()._executor; }

    template <class F>
    void start(F&& callback) {
        auto launchCo = [](LazyBase lazy, std::decay_t<F> cb) -> DetachedCoroutine {
            cb(std::move(co_await lazy.coAwaitTry()));
        };
        launchCo(std::move(*this), std::forward<F>(callback));
    }

    [[nodiscard]] bool isReady() const { return !_co || _co.done(); }

    auto operator co_await() { return ValueAwaiter(std::exchange(_co, nullptr)); }

    auto coAwaitTry() { return TryAwaiter(std::exchange(_co, nullptr)); }

protected:
    Handle _co;
};

template <typename T = void>
class [[nodiscard]] Lazy : public LazyBase<T, false> {
    using Base = LazyBase<T, false>;
    using Base::Base;

public:
    /// Bind an executor to a Lazy, and convert it to a RescheduleLazy.
    RescheduleLazy<T> via(Executor* ex) && {
        logicAssert(this->_co.operator bool(), "Lazy does not have a coroutine_handle");
        this->_co.promise()._executor = ex;
        return RescheduleLazy<T>(std::exchange(this->_co, nullptr));
    }

    auto coAwait(Executor* ex) {
        this->_co.promise()._executor = ex;
        return typename Base::ValueAwaiter(std::exchange(this->_co, nullptr));
    }
};

/// RescheduleLazy is a Lazy with an executor.
/// Different from Lazy, when a RescheduleLazy is co_awaited/started/syncAwaited,
/// the RescheduleLazy wouldn't be executed immediately.
/// The RescheduleLazy would submit a task to resume the corresponding Lazy task to the executor.
/// Then the executor would execute the Lazy task later.
template <typename T = void>
class [[nodiscard]] RescheduleLazy : public LazyBase<T, true> {
    using Base = LazyBase<T, true>;
    using Base::Base;

public:
    void detach() {
        this->start([](auto&& t) {
            if (t.hasError()) std::rethrow_exception(t.getException());
        });
    }
};

inline Lazy<void> LazyPromise<void>::get_return_object() noexcept {
    return Lazy<void>(Lazy<void>::Handle::from_promise(*this));
}

#endif  // TINY_HTTP_SERVER_LAZY_H
