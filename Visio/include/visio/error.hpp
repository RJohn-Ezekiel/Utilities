#pragma once

#include <exception>
#include <format>
#include <memory>
#include <string>
#include <system_error>

namespace visio {

enum class [[nodiscard]] ErrorCode
{
    Ok = 0,
    NetworkError,
    HttpError,
    ParseError,
    NotFound,
    InvalidInput,
    SubprocessError,
    CacheError,
    PlaybackError,
    DownloadError,
    SubscriptionError,
    PlaylistError,
};

[[nodiscard]] constexpr std::string_view toString(ErrorCode ec) noexcept
{
    switch (ec) {
    case ErrorCode::Ok:               return "Ok";
    case ErrorCode::NetworkError:     return "network error";
    case ErrorCode::HttpError:        return "HTTP error";
    case ErrorCode::ParseError:       return "parse error";
    case ErrorCode::NotFound:         return "not found";
    case ErrorCode::InvalidInput:     return "invalid input";
    case ErrorCode::SubprocessError:  return "subprocess error";
    case ErrorCode::CacheError:       return "cache error";
    case ErrorCode::PlaybackError:    return "playback error";
    case ErrorCode::DownloadError:    return "download error";
    case ErrorCode::SubscriptionError: return "subscription error";
    case ErrorCode::PlaylistError:    return "playlist error";
    }
    return "unknown error";
}

class Error
{
public:
    Error(ErrorCode code, std::string message) noexcept
        : m_code(code)
        , m_message(std::move(message))
    {}

    [[nodiscard]] ErrorCode code() const noexcept { return m_code; }
    [[nodiscard]] const std::string& message() const noexcept { return m_message; }
    [[nodiscard]] std::string what() const
    {
        return std::format("[visio] {}: {}", toString(m_code), m_message);
    }

private:
    ErrorCode m_code;
    std::string m_message;
};

class VisioException : public std::exception
{
public:
    explicit VisioException(Error err) noexcept
        : m_error(std::move(err))
        , m_what(m_error.what())
    {}

    [[nodiscard]] const Error& error() const noexcept { return m_error; }
    [[nodiscard]] const char* what() const noexcept override
    {
        return m_what.c_str();
    }

private:
    Error m_error;
    std::string m_what;
};

template <typename T>
class [[nodiscard]] Result
{
public:
    using value_type = T;
    using error_type = Error;

    Result(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : m_has_value(true)
    {
        std::construct_at(&m_storage.value, std::move(value));
    }

    Result(Error err) noexcept
        : m_has_value(false)
    {
        std::construct_at(&m_storage.error, std::move(err));
    }

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : m_has_value(other.m_has_value)
    {
        if (m_has_value) {
            std::construct_at(&m_storage.value, std::move(other.m_storage.value));
        } else {
            std::construct_at(&m_storage.error, std::move(other.m_storage.error));
        }
    }
    Result& operator=(Result&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (this != &other) {
            this->~Result();
            if (other.m_has_value) {
                std::construct_at(&m_storage.value, std::move(other.m_storage.value));
            } else {
                std::construct_at(&m_storage.error, std::move(other.m_storage.error));
            }
            m_has_value = other.m_has_value;
        }
        return *this;
    }

    ~Result()
    {
        if (m_has_value) {
            m_storage.value.~T();
        } else {
            m_storage.error.~Error();
        }
    }

    [[nodiscard]] explicit operator bool() const noexcept { return m_has_value; }
    [[nodiscard]] bool hasValue() const noexcept { return m_has_value; }

    [[nodiscard]] T& value() &
    {
        if (!m_has_value) {
            throw VisioException(m_storage.error);
        }
        return m_storage.value;
    }

    [[nodiscard]] const T& value() const&
    {
        if (!m_has_value) {
            throw VisioException(m_storage.error);
        }
        return m_storage.value;
    }

    [[nodiscard]] T&& value() &&
    {
        if (!m_has_value) {
            throw VisioException(m_storage.error);
        }
        return std::move(m_storage.value);
    }

    [[nodiscard]] Error error() const
    {
        if (m_has_value) {
            throw VisioException(Error(ErrorCode::Ok, "no error"));
        }
        return m_storage.error;
    }

    [[nodiscard]] T valueOr(T&& fallback) const&
    {
        if (m_has_value) {
            return m_storage.value;
        }
        return std::forward<T>(fallback);
    }

    [[nodiscard]] T valueOr(T&& fallback) &&
    {
        if (m_has_value) {
            return std::move(m_storage.value);
        }
        return std::forward<T>(fallback);
    }

    template <typename Func>
    [[nodiscard]] auto andThen(Func&& f) -> Result<decltype(f(std::declval<T&>()))>
    {
        using U = decltype(f(std::declval<T&>()));
        if (m_has_value) {
            return f(m_storage.value);
        }
        return U(m_storage.error);
    }

    template <typename Func>
    [[nodiscard]] auto map(Func&& f) -> Result<decltype(f(std::declval<T&>()))>
    {
        using U = decltype(f(std::declval<T&>()));
        if (m_has_value) {
            return U(f(m_storage.value));
        }
        return U(m_storage.error);
    }

private:
    bool m_has_value;
    union Storage {
        T value;
        Error error;
        Storage() noexcept {}
        ~Storage() noexcept {}
    } m_storage;
};

template <>
class [[nodiscard]] Result<void>
{
public:
    Result() noexcept = default;
    Result(Error err) noexcept
        : m_error(std::make_unique<Error>(std::move(err)))
    {}

    [[nodiscard]] explicit operator bool() const noexcept { return !m_error; }
    [[nodiscard]] bool hasValue() const noexcept { return !m_error; }

    void value() const
    {
        if (m_error) {
            throw VisioException(*m_error);
        }
    }

    [[nodiscard]] Error error() const
    {
        if (!m_error) {
            throw VisioException(Error(ErrorCode::Ok, "no error"));
        }
        return *m_error;
    }

private:
    std::unique_ptr<Error> m_error;
};

template <typename T>
[[nodiscard]] auto makeError(ErrorCode code, std::string message) -> Result<T>
{
    return Result<T>(Error(code, std::move(message)));
}

} // namespace visio
