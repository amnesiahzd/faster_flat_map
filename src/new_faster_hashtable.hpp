#include <cstddef>
#include <cstdint>
#include <utility>

template<typename Functor>
struct function_wrapper : public Functor {
    using Functor::Functor;
    function_wrapper(const Functor& other_functor) : Functor(other_functor) {}

    template<typename... Args>
    decltype(auto) operator()(Args&&... args) {
        return Functor::operator()(std::forward<Args>(args)...);
    }

};

template<typename Result, typename... Args>
struct function_wrapper<Result (*)(Args...)> {
    using function_ptr = Result (*)(Args...);
    function_ptr _function;

    explicit function_wrapper(function_ptr& other_ptr) : _function(other_ptr) {}

    Result operator()(Args&&... args) {
        return _function(std::forward<Args>(args)...);
    }

    operator function_ptr &() {
        return _function;
    }
    operator const function_ptr &() {
        return _function;
    }
};

template<typename KeyType, typename ValueType, typename Hasher>
struct key_or_value_hasher : public function_wrapper<size_t, Hasher> {
    using hasher_storage = function_wrapper<size_t, Hasher>;
    key_or_value_hasher() = default;
    key_or_value_hasher(const Hasher& other) : hasher_storage(other) {}

    size_t operator()(const KeyType& key) { 
        return hasher_storage::operator()(key)
    }

    size_t operator()(const KeyType& key) const {
        return hasher_storage::operator()(key)
    }

    size_t operator()(const ValueType& value) {
        return hasher_storage::operator()(value.first);
    }

    size_t operator()(const ValueType& value) const {
        return hasher_storage::operator()(value.first);
    }

    template<typename First, typename Second>
    size_t operator()(std::pair<First, Second>& value) {
        return hasher_storage::operator()(value.first);
    }

    template<typename First, typename Second>
    size_t operator()(const std::pair<First, Second>& value) const {
        return hasher_storage::operator()(value.first);
    }
};

template<typename KeyType, typename ValueType, typename Equal>
struct key_or_value_equality : public function_wrapper<bool, Equal> {
    using equal_storage = function_wrapper<bool, Equal>;
    key_or_value_equality() = default;
    key_or_value_equality(const Equal& other) : equal_storage(other) {}

    bool operator()(const KeyType& lhs, const KeyType& rhs) {
        return equal_storage::operator()(lhs, rhs);
    }

    bool operator()(const KeyType& lhs, const ValueType& rhs) {
        return equal_storage::operator()(lhs, rhs.first);
    }

    bool operator()(const ValueType& lhs, const KeyType& rhs) {
        return equal_storage::operator()(lhs.first, rhs);
    }

    bool operator()(const ValueType& lhs, const ValueType& rhs) {
        return equal_storage::operator()(lhs.first, rhs.first);
    }

    template<typename First, typename Second>
    bool operator()(const KeyType& lhs, const std::pair<First, Second>& rhs) {
        return equal_storage::operator()(lhs, rhs.first);
    }

    template<typename First, typename Second>
    bool operator()(const std::pair<First, Second>& lhs, const KeyType& rhs) {
        return equal_storage::operator()(lhs.first, rhs);
    }

    template<typename First, typename Second>
    bool operator()(const ValueType& lhs, const std::pair<First, Second>& rhs) {
        return equal_storage::operator()(lhs.first, rhs.first);
    }

    template<typename First, typename Second>
    bool operator()(const std::pair<First, Second>& lhs, const ValueType& rhs) {
        return equal_storage::operator()(lhs.first, rhs.first);
    }

    template<typename FL, typename SL, typename FR, typename SR>
    bool operator()(const std::pair<FL, SL>& lhs, const std::pair<FR, SR>& rhs) {
        return equal_storage::operator()(lhs.first, rhs.first);
    }
};

inline int8_t log2(size_t value) {
    static constexpr int8_t table[64] = {
        63,  0, 58,  1, 59, 47, 53,  2,
        60, 39, 48, 27, 54, 33, 42,  3,
        61, 51, 37, 40, 49, 18, 28, 20,
        55, 30, 34, 11, 43, 14, 22,  4,
        62, 57, 46, 52, 38, 26, 32, 41,
        50, 36, 17, 19, 29, 10, 13, 21,
        56, 45, 25, 31, 35, 16,  9, 12,
        44, 24, 15,  8, 23,  7,  6,  5
    };
    // Set all positions after the highest bit to 1
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return table[((value - (value >> 1)) * 0x07EDD5E59A4E28C2) >> 58]; // AmnesiaHzd : magic number? to find the unique identify？
}


