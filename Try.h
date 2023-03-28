#ifndef TINY_HTTP_SERVER_TRY_H
#define TINY_HTTP_SERVER_TRY_H

#include <cassert>
#include <concepts>
#include <exception>
#include <utility>
#include <variant>

#include "Common.h"
#include "Uint.h"

/// Try contains an instance of T or an exception.
template <typename T>
class Try {
public:
    explicit Try(const T& val) : _value(val) {}

    explicit Try(T&& val) : _value(std::move(val)) {}

    explicit Try(std::exception_ptr err) : _value(std::move(err)) {}

    Try(const Try&) = delete;

    Try& operator=(const Try&) = delete;

    template <typename T2 = T>
    explicit Try(std::enable_if_t<std::is_same_v<Uint, T2>, const Try<void>&> other) {
        if (other.hasError()) {
            _value = std::exception_ptr(other._err);
        } else if (other.available()) {
            _value = T();
        }
    }

    Try(Try<T>&& other) noexcept { _value = std::move(other._value); }

    Try& operator=(Try<T>&& other) noexcept {
        if (&other == this) return *this;
        _value = std::move(other._value);
        return *this;
    }

    Try& operator=(const std::exception_ptr& err) {
        if (_value == err) return *this;
        _value = std::make_exception_ptr(err);
        return *this;
    }

    [[nodiscard]] bool hasError() const {
        return std::holds_alternative<std::exception_ptr>(_value);
    }

    T& value() & {
        T* v = std::get_if<T>(_value);
        if (!v) std::rethrow_exception(std::get<std::exception_ptr>(_value));
        return *v;
    }

    T&& value() && {
        T* v = std::get_if<T>(_value);
        if (!v) std::rethrow_exception(std::get<std::exception_ptr>(_value));
        return std::move(*v);
    }

    const T&& value() const&& {
        T* v = std::get_if<T>(_value);
        if (!v) std::rethrow_exception(std::get<std::exception_ptr>(_value));
        return std::move(*v);
    }

    [[nodiscard]] std::exception_ptr getException() {
        logicAssert(hasError(), "Try object does not has an error");
        return std::get<std::exception_ptr>(_value);
    }

private:
    std::variant<T, std::exception_ptr> _value;
};

template <>
class Try<void> {
public:
    Try() = default;

    explicit Try(std::exception_ptr err) : _err(std::move(err)) {}

    Try& operator=(std::exception_ptr err) {
        _err = std::move(err);
        return *this;
    }

    Try(Try&& other) noexcept : _err(std::move(other._err)) {}

    Try& operator=(Try&& other) noexcept {
        if (this != &other) _err = std::exchange(other._err, nullptr);
        return *this;
    }

    void value() {
        if (_err) std::rethrow_exception(_err);
    }

    [[nodiscard]] bool hasError() const { return _err.operator bool(); }

    [[nodiscard]] std::exception_ptr getException() { return _err; }

private:
    std::exception_ptr _err;
};

#endif  // TINY_HTTP_SERVER_TRY_H
