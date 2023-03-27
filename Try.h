#ifndef TINY_HTTP_SERVER_TRY_H
#define TINY_HTTP_SERVER_TRY_H

#include <cassert>
#include <concepts>
#include <exception>

#include "Common.h"
#include "Uint.h"

/// Try contains either an instance of T, an exception, or nothing.
template <typename T>
class Try {
private:
    enum class Contains { VALUE, EXCEPTION, NOTHING };

public:
//    Try() : _contains(Contains::NOTHING) {}

    ~Try() { destroy(); }

    Try(const T& val) : _contains(Contains::VALUE), _value(val) {}

    Try(T&& val) : _contains(Contains::VALUE), _value(std::move(val)) {}

    Try(std::exception_ptr err) : _contains(Contains::EXCEPTION), _err(err) {}

    Try(const Try&) = delete;

    Try& operator=(const Try&) = delete;

    template <typename T2 = T>
    Try(std::enable_if_t<std::is_same_v<Uint, T2>, const Try<void>&> other) {
        if (other.hasError()) {
            _contains = Contains::EXCEPTION;
            new (&_err) std::exception_ptr(other._err);
        } else if (other.available()) {
            _contains = Contains::VALUE;
            new (&_value) T();
        }
    }

    Try(Try<T>&& other) noexcept : _contains(other._contains) {
        if (_contains == Contains::VALUE) {
            new (&_value) T(std::move(other._value));
        } else if (_contains == Contains::EXCEPTION) {
            new (&_err) std::exception_ptr(other._err);
        }
    }

    Try& operator=(Try<T>&& other) noexcept {
        if (&other == this) return *this;
        destroy();
        _contains = other._contains;
        if (_contains == Contains::VALUE) {
            new (&_value) T(std::move(other._value));
        } else if (_contains == Contains::EXCEPTION) {
            new (&_err) std::exception_ptr(other._err);
        }
        return *this;
    }

    Try& operator=(const std::exception_ptr& err) {
        if (_contains == Contains::EXCEPTION && _err == err) {
            return *this;
        }
        destroy();
        _contains = Contains::EXCEPTION;
        new (&_err) std::exception_ptr(err);
        return *this;
    }

    [[nodiscard]] bool available() const { return _contains != Contains::NOTHING; }

    [[nodiscard]] bool hasError() const { return _contains == Contains::EXCEPTION; }

    T& value() & {
        checkHasTry();
        return _value;
    }

    T&& value() && {
        checkHasTry();
        return _value;
    }

    const T&& value() const&& {
        checkHasTry();
        return std::move(_value);
    }

    void setException(std::exception_ptr err) {
        if (_contains == Contains::EXCEPTION && _err == err) return;
        destroy();
        _contains = Contains::EXCEPTION;
        new (&_err) std::exception_ptr(err);
    }

    [[nodiscard]] std::exception_ptr getException() {
        logicAssert(_contains == Contains::EXCEPTION, "Try object does not has an error");
        return _err;
    }

private:
    INLINE void checkHasTry() const {
        switch (_contains) {
            case Contains::VALUE:
                [[likely]] return;
            case Contains::EXCEPTION:
                std::rethrow_exception(_err);
            case Contains::NOTHING:
                throw std::logic_error("Try object is empty");
            default:
                assert(false);
        }
    }

    void destroy() {
        if (_contains == Contains::VALUE) {
            _value.~T();
        } else if (_contains == Contains::EXCEPTION) {
            _err.~exception_ptr();
        }
        _contains = Contains::NOTHING;
    }

private:
    Contains _contains = Contains::NOTHING;
    union {
        T _value;
        std::exception_ptr _err;
    };
};

template <>
class Try<void> {
public:
    Try() = default;

    explicit Try(std::exception_ptr err) : _err(std::move(err)) {}

    Try& operator=(std::exception_ptr err) {
        _err = err;
        return *this;
    }

    Try(Try&& other) noexcept : _err(std::move(other._err)) {}

    Try& operator=(Try&& other) noexcept {
        if (this != &other) std::swap(_err, other._err);
        return *this;
    }

public:
    void value() {
        if (_err) std::rethrow_exception(_err);
    }

    [[nodiscard]] bool hasError() const { return _err.operator bool(); }

    void setException(std::exception_ptr err) { _err = std::move(err); }

    [[nodiscard]] std::exception_ptr getException() { return _err; }

private:
    std::exception_ptr _err;
};

#endif  // TINY_HTTP_SERVER_TRY_H
