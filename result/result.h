#ifndef RESULT_H_1de851ad_b9f8_434d_b73d_f06a6b5daa9a
#define RESULT_H_1de851ad_b9f8_434d_b73d_f06a6b5daa9a

#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <type_traits>
#include <utility>

template <typename T>
using optional = std::optional<T>;
using nullopt_t = std::nullopt_t;
inline constexpr nullopt_t nullopt = std::nullopt;

namespace result {

template <typename T>
inline std::ostream& operator<<(
        std::ostream& stream, const std::reference_wrapper<T>& obj);

enum class ResultKind : uint8_t {
    Ok = 0,
    Err = 1,
};

struct ok_tag_t {};
struct err_tag_t {};
struct unit_t {};

inline constexpr ok_tag_t ok_tag = ok_tag_t{};
inline constexpr err_tag_t err_tag = err_tag_t{};
inline constexpr unit_t unit = unit_t{};

template <typename T>
class Err {
public:
    using value_type = T;

    explicit constexpr Err(const T& val) : m_value(val) {}
    explicit constexpr Err(T&& val) : m_value(std::move(val)) {}

    constexpr const T& value() const& { return m_value; }
    constexpr T&& value() && { return std::move(m_value); }


private:
    T m_value;
};

template <typename T>
class Ok {
public:
    using value_type = T;

    explicit constexpr Ok(const T& val) : m_value(val) {}
    explicit constexpr Ok(T&& val) : m_value(std::move(val)) {}

    constexpr const T& value() const& { return m_value; }
    constexpr T&& value() && { return std::move(m_value); }


private:
    T m_value;
};

template <>
class Ok<unit_t> {
public:
    using value_type = unit_t;

    constexpr Ok() = default;
    constexpr Ok(unit_t) {}

    constexpr const unit_t value() const { return {}; }
};

Ok()->Ok<unit_t>;


namespace details {

inline void terminate(const std::string& msg) {
    std::cerr << msg << std::endl;
    std::terminate();
}

template <typename T, typename E>
class ResultStorage {
    using DecayT = std::decay_t<T>;
    using DecayE = std::decay_t<E>;

public:
    using value_type = T;
    using error_type = E;
    using data_type = std::aligned_union_t<1, T, E>;

    ResultStorage() = delete;

    constexpr ResultStorage(Ok<T> val) {
        if constexpr(!std::is_same<T, unit_t>::value) {
            new(&m_data) DecayT(std::move(val).value());
        }
        m_tag = ResultKind::Ok;
    }
    constexpr ResultStorage(Err<E> val) {
        new(&m_data) DecayE(std::move(val).value());
        m_tag = ResultKind::Err;
    }

    constexpr ResultStorage(const ResultStorage<T, E>& rhs) : m_tag(rhs.m_tag) {
        if(kind() == ResultKind::Ok) {
            if constexpr(!std::is_same<T, unit_t>::value) {
                new(&m_data) DecayT(rhs.get<T>());
            }
        } else {
            new(&m_data) DecayE(rhs.get<E>());
        }
    }
    constexpr ResultStorage(ResultStorage<T, E>&& rhs) : m_tag(rhs.m_tag) {
        if(kind() == ResultKind::Ok) {
            if constexpr(!std::is_same<T, unit_t>::value) {
                new(&m_data) DecayT(std::move(rhs).template get<T>());
            }
        } else {
            new(&m_data) DecayE(std::move(rhs).template get<E>());
        }
    }
    constexpr ResultStorage& operator=(const ResultStorage<T, E>& rhs) {
        destroy();
        m_tag = rhs.m_tag;

        if(kind() == ResultKind::Ok) {
            T& val = get<T>();
            val = rhs.get<T>();
        } else {
            E& val = get<E>();
            val = rhs.get<E>();
        }
    }
    constexpr ResultStorage& operator=(ResultStorage<T, E>&& rhs) {
        destroy();
        m_tag = rhs.m_tag;

        if(kind() == ResultKind::Ok) {
            T& val = get<T>();
            val = std::move(rhs).template get<T>();
        } else {
            E& val = get<E>();
            val = std::move(rhs).template get<E>();
        }
    }

    template <typename U>
    constexpr const U& get() const& {
        static_assert(std::is_same<T, U>::value || std::is_same<E, U>::value);
        return *reinterpret_cast<const U*>(&m_data);
    }
    template <typename U>
    constexpr U& get() & {
        static_assert(std::is_same<T, U>::value || std::is_same<E, U>::value);
        return *reinterpret_cast<U*>(&m_data);
    }
    template <typename U>
    constexpr U&& get() && {
        static_assert(std::is_same<T, U>::value || std::is_same<E, U>::value);
        return std::move(*reinterpret_cast<U*>(&m_data));
    }

    constexpr ResultKind kind() const { return m_tag; }

    ~ResultStorage() { destroy(); }

private:
    void destroy() {
        switch(m_tag) {
        case ResultKind::Ok:
            get<T>().~T();
            break;
        case ResultKind::Err:
            get<E>().~E();
            break;
        }
    }

    data_type m_data;
    ResultKind m_tag;
};

} // namespace details

template <typename T, typename E>
class [[nodiscard]] Result {
public:
    using value_type = T;
    using error_type = E;

    static_assert(!std::is_same<T, void>::value,
            "Cannot create a Result<T, E> object with T=void. "
            "Introducing `void` to the type causes a lot of problems, "
            "use the type `unit_t` instead");
    static_assert(!std::is_same<E, void>::value,
            "Cannot create a Result<T, E> object with E=void. You want an "
            "optional<T>.");

    constexpr Result() {
        static_assert(std::is_default_constructible<T>::value,
                "Result<T, E> may only be default constructed if T is default "
                "constructible.");
        m_storage = Ok(T());
    }
    constexpr Result(Ok<T> value) : m_storage(std::move(value)) {}
    constexpr Result(Err<E> value) : m_storage(std::move(value)) {}

    template <typename... Args>
    constexpr Result(ok_tag_t, Args... args) : Result(Ok(args...)) {}
    template <typename... Args>
    constexpr Result(err_tag_t, Args... args) : Result(Err(args...)) {}

    constexpr Result(const Result<T, E>& other) = default;
    constexpr Result<T, E>& operator=(const Result<T, E>& other) = default;
    constexpr Result(Result<T, E>&& other) = default;
    constexpr Result<T, E>& operator=(Result<T, E>&& other) = default;

    constexpr bool is_ok() const { return m_storage.kind() == ResultKind::Ok; }
    constexpr bool is_err() const {
        return m_storage.kind() == ResultKind::Err;
    }
    constexpr ResultKind kind() const { return m_storage.kind(); }

    constexpr bool operator==(const Ok<T>& other) const {
        if constexpr(std::is_same<T, unit_t>::value) {
            return true;
        } else {
            return kind() == ResultKind::Ok &&
                    m_storage.template get<T>() == other.value();
        }
    }
    constexpr bool operator!=(const Ok<T>& other) const {
        return !(*this == other);
    }
    constexpr bool operator==(const Err<E>& other) const {
        return kind() == ResultKind::Err &&
                m_storage.template get<E>() == other.value();
    }
    constexpr bool operator!=(const Err<E>& other) const {
        return !(*this == other);
    }
    constexpr bool operator==(const Result<T, E>& other) const {
        if(kind() != other.kind()) {
            return false;
        }
        if(kind() == ResultKind::Ok) {
            if constexpr(std::is_same<T, unit_t>::value) {
                return true;
            } else {
                return m_storage.template get<T>() ==
                        other.m_storage.template get<T>();
            }
        } else {
            return m_storage.template get<E>() ==
                    other.m_storage.template get<E>();
        }
        return false;
    }
    constexpr bool operator!=(const Result<T, E>& other) const {
        return !(*this == other);
    }

    constexpr optional<std::reference_wrapper<const T>> ok() const& {
        if(is_ok()) {
            return std::cref(m_storage.template get<T>());
        } else {
            return nullopt;
        }
    }
    constexpr optional<std::reference_wrapper<T>> ok()& {
        if(is_ok()) {
            return std::ref(m_storage.template get<T>());
        } else {
            return nullopt;
        }
    }
    constexpr optional<T> ok()&& {
        if(is_ok()) {
            return std::move(m_storage.template get<T>());
        } else {
            return nullopt;
        }
    }
    constexpr optional<std::reference_wrapper<const E>> err() const& {
        if(is_err()) {
            return std::cref(m_storage.template get<E>());
        } else {
            return nullopt;
        }
    }
    constexpr optional<std::reference_wrapper<E>> err() & {
        if(is_err()) {
            return std::ref(m_storage.template get<E>());
        } else {
            return nullopt;
        }
    }
    constexpr optional<E> err() && {
        if(is_err()) {
            return std::move(std::move(m_storage).template get<E>());
        } else {
            return nullopt;
        }
    }

    constexpr const E& try_err() const {
        if(!is_err()) {
            details::terminate("Called try_err on an Ok value");
        }
        return m_storage.template get<E>();
    }
    constexpr E& try_err() {
        if(!is_err()) {
            details::terminate("Called try_err on an Ok value");
        }
        return m_storage.template get<E>();
    }
    constexpr const T& try_ok() const {
        if(!is_ok()) {
            details::terminate("Called try_ok on an Err value");
        }
        return m_storage.template get<T>();
    }
    template <typename U = T,
            std::enable_if_t<!std::is_same<U, void>::value &&
                            std::is_same<T, U>::value,
                    int> = 0>
    constexpr U& try_ok() {
        if(!is_ok()) {
            details::terminate("Called try_ok on an Err value");
        }
        return m_storage.template get<T>();
    }

    template <typename U = T,
            std::enable_if_t<!std::is_same<U, void>::value &&
                            std::is_same<T, U>::value,
                    int> = 0>
    constexpr U&& unwrap() {
        if(!is_ok()) {
            details::terminate("Called unwrap on a Err value");
        }
        return std::move(m_storage).template get<U>();
    }

    template <typename U = T,
            std::enable_if_t<!std::is_same<U, void>::value &&
                            std::is_same<T, U>::value,
                    int> = 0>
    constexpr U&& expect(const std::string& message) {
        if(!is_ok()) {
            details::terminate(message);
        }
        return std::move(m_storage).template get<U>();
    }
    template <typename U = T,
            std::enable_if_t<std::is_same<U, void>::value &&
                            std::is_same<T, U>::value,
                    int> = 0>
    constexpr void expect(const std::string& message) {
        if(!is_ok()) {
            details::terminate(message);
        }
    }

    template <typename U = T,
            std::enable_if_t<!std::is_same<U, void>::value &&
                            std::is_same<T, U>::value,
                    int> = 0>
    constexpr const U& ok_unchecked() const {
        return m_storage.template get<U>();
    }
    constexpr const E& err_unchecked() const {
        return m_storage.template get<E>();
    }

    // Combinators and adapters
    template <typename F,
            typename T2 = std::invoke_result_t<F, const T&>,
            std::enable_if_t<std::is_invocable_r<T2, F, const T&>::value, int> =
                    0>
    Result<T2, E> map(F && map_fn) const {
        if(is_ok()) {
            return Result<T2, E>(Ok(map_fn(ok_unchecked())));
        } else {
            return Result<T2, E>(Err(err_unchecked()));
        }
    }

private:
    details::ResultStorage<T, E> m_storage;
};

template <typename T, typename T2, typename E>
inline constexpr bool operator<(
        const Result<T, E>& lhs, const Result<T2, E>& rhs) {
    if(lhs.is_err() && rhs.is_err()) {
        return false;
    }
    if(lhs.is_err()) {
        return true;
    }
    return lhs.try_ok() < rhs.try_ok();
}
template <typename T, typename T2, typename E>
inline constexpr bool operator<=(
        const Result<T, E>& lhs, const Result<T2, E>& rhs) {
    if(lhs.is_err() && rhs.is_err()) {
        return true;
    }
    if(lhs.is_err()) {
        return true;
    }
    return lhs.try_ok() <= rhs.try_ok();
}
template <typename T, typename T2, typename E>
inline constexpr bool operator>(
        const Result<T, E>& lhs, const Result<T2, E>& rhs) {
    return !(lhs <= rhs);
}
template <typename T, typename T2, typename E>
inline constexpr bool operator>=(
        const Result<T, E>& lhs, const Result<T2, E>& rhs) {
    return !(lhs < rhs);
}

template <typename T, typename T2, typename E>
inline constexpr bool operator<(const Result<T, E>& lhs, Ok<T2> rhs) {
    return lhs < Result<T2, E>(std::move(rhs));
}
template <typename T, typename T2, typename E>
inline constexpr bool operator<=(const Result<T, E>& lhs, Ok<T2> rhs) {
    return lhs <= Result<T2, E>(std::move(rhs));
}
template <typename T, typename T2, typename E>
inline constexpr bool operator>(const Result<T, E>& lhs, Ok<T2> rhs) {
    return lhs > Result<T2, E>(std::move(rhs));
}
template <typename T, typename T2, typename E>
inline constexpr bool operator>=(const Result<T, E>& lhs, Ok<T2> rhs) {
    return lhs >= Result<T2, E>(std::move(rhs));
}
template <typename T, typename E>
inline constexpr bool operator<(const Result<T, E>& lhs, Err<E> rhs) {
    return lhs < Result<T, E>(std::move(rhs));
}
template <typename T, typename E>
inline constexpr bool operator<=(const Result<T, E>& lhs, Err<E> rhs) {
    return lhs <= Result<T, E>(std::move(rhs));
}
template <typename T, typename E>
inline constexpr bool operator>(const Result<T, E>& lhs, Err<E> rhs) {
    return lhs > Result<T, E>(std::move(rhs));
}
template <typename T, typename E>
inline constexpr bool operator>=(const Result<T, E>& lhs, Err<E> rhs) {
    return lhs >= Result<T, E>(std::move(rhs));
}

template <typename T>
inline std::ostream& operator<<(std::ostream& stream, unit_t) {
    stream << "()";
    return stream;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& stream, const Ok<T>& ok) {
    if constexpr(std::is_same<T, unit_t>::value) {
        stream << "Ok()" << std::endl;
    } else {
        stream << "Ok(" << ok.value() << ")";
    }
    return stream;
}
template <typename T>
inline std::ostream& operator<<(std::ostream& stream, const Err<T>& err) {
    stream << "Err(" << err.value() << ")";
    return stream;
}

template <typename T, typename E>
inline std::ostream& operator<<(
        std::ostream& stream, const Result<T, E>& result) {
    switch(result.kind()) {
    case ResultKind::Ok: {
        if constexpr(std::is_same<T, unit_t>::value) {
            stream << "Ok()";
        } else {
            stream << "Ok(" << result.ok().value() << ")";
        }
        break;
    }
    case ResultKind::Err: {
        stream << "Err( " << result.err().value() << ")";
        break;
    }
    default:
        stream << "INVALID RESULT";
        break;
    }
    return stream;
}

template <typename T>
inline std::ostream& operator<<(
        std::ostream& stream, const std::reference_wrapper<T>& obj) {
    stream << obj.get();
    return stream;
}

} // namespace result

#define PROPAGATE(res)                                                         \
    {                                                                          \
        auto& r = res;                                                         \
        if(r.is_err()) {                                                       \
            return r;                                                          \
        }                                                                      \
    }

#endif