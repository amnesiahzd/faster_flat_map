/*
 * Copyright 2023 AmnesiaHzd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS," WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <utility>
#include <stddef.h>
#include <stdint.h>

namespace ddaof {

static constexpr uint8_t min_lookups = 4;

// function wrapper
template<typename Result, typename Functor>
struct function_wrapper {
    function_wrapper() = default;
    function_wrapper(const Functor& funtor) : Functor(funtor) {}

    template<typename... Args>
    Result operator()(Args&& ...args) {
        return static_cast<Functor&>(*this)(std::forward<Args>(args)...);
    }

    template<typename... Args>
    Result operator()(Args&& ...args) const {
        return static_cast<const Functor&>(*this)(std::forward<Args>(args)...);
    }
};

template<typename Result, typename... Args>
struct function_wrapper<Result, Result(*)(Args...)> { // AmnesiaHzd: its line32
    typedef Result (*function_ptr)(Args...);
    function_ptr _funtion;

    function_wrapper(function_ptr function) : _funtion(function) {}

    Result operator()(Args... args) {
        return _funtion(std::forward<Args>(args)...);
    }

    friend function_ptr operator&() {
        return *_funtion;
    }

    friend const function_ptr operator&() {
        return *_funtion;
    }
};  

template<typename KeyType, typename ValueType, typename Hasher> // ValueType: map value type
struct KeyOrValueHasher : function_wrapper<size_t, Hasher> {
    typedef function_wrapper<size_t, Hasher> hasher_storage;

    KeyOrValueHasher() = default;
    KeyOrValueHasher(hasher_storage& hash) : hasher_storage(hash) {}

    size_t operator()(const KeyType& key) {
        return static_cast<hasher_storage&>(*this)(key);
    }

    size_t operator()(const KeyType& key) const {
        return static_cast<hasher_storage&>(const *this)(key);
    }

    size_t operator()(const ValueType& value) {
        return static_cast<hasher_storage&>(*this)(value.first);
    }

    size_t operator()(const ValueType& value) const {
        return static_cast<hasher_storage&>(const *this)(value.first);
    }

    template<typename First, typename Second>
    size_t operator()(const std::pair<First, Second>& pair_value) {
        return static_cast<hasher_storage&>(*this)(pair_value.first);
    }

    template<typename First, typename Second>
    size_t operator()(const std::pair<First, Second>& pair_value) const {
        return static_cast<hasher_storage&>(const *this)(pair_value.first);
    }
};

template<typename key_type, typename value_type, typename key_equal>
struct KeyOrValueEquality : functor_storage<bool, key_equal> {
    typedef functor_storage<bool, key_equal> equality_storage;
    // why cant use equality_storage = functor_storage<bool, key_equal>
    KeyOrValueEquality() = default;
    KeyOrValueEquality(const key_equal& equality)
            : equality_storage(equality) {}

    bool operator()(const key_type& lhs, const key_type& rhs) {
        return static_cast<equality_storage&>(*this)(lhs, rhs);
    }

    bool operator()(const key_type& lhs, const value_type& rhs) {
        return static_cast<equality_storage&>(*this)(lhs, rhs.first);
    }

    bool operator()(const value_type& lhs, const key_type& rhs) {
        return static_cast<equality_storage&>(*this)(lhs.first, rhs);
    }

    bool operator()(const value_type& lhs, const value_type& rhs) {
        return static_cast<equality_storage&>(*this)(lhs.first, rhs.first);
    }

    template<typename First, typename Second>
    bool operator()(const key_type& lhs, const std::pair<First, Second>& rhs) {
        return static_cast<equality_storage&>(*this)(lhs, rhs.first);
    }

    template<typename First, typename Second>
    bool operator()(const std::pair<First, Second>& lhs, const key_type& rhs) {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs);
    }

    template<typename First, typename Second>
    bool operator()(const value_type& lhs, const std::pair<First, Second>& rhs) {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }

    template<typename First, typename Second>
    bool operator()(const std::pair<First, Second>& lhs, const value_type& rhs) {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }

    template<typename FL, typename SL, typename FR, typename SR>
    bool operator()(const std::pair<FL, SL>& lhs, const std::pair<FR, SR>& rhs) {
        return static_cast<equality_storage &>(*this)(lhs.first, rhs.first);
    }
};

template<typename T>
struct faster_hashtable_entry {
    faster_hashtable_entry() {};
    faster_hashtable_entry(uint8_t distance_from_desired) : _distance_from_desired(distance_from_desired) {};
    ~faster_hashtable_entry() {};

    static faster_hashtable_entry* get_defualt_table() {
        static faster_hashtable_entry res[min_lookups] = { {}, {}, {}, {_special_end_value}};
        return res;
    }

    bool is_empty() {
        return _distance_from_desired < 0;
    }

    bool has_value() {
        return _distance_from_desired >= 0;
    }

    bool is_at_desired_position() {
        return _distance_from_desired <= 0;
    }

    template<typename... Args>
    void emplace(uint8_t distance_from_desired, Args&&... args) {
        _distance_from_desired = distance_from_desired;
        new (std::addressof()) T(std::forward<Args>(args)...);
    }

    void destroy_value() {
        ~T();
        _distance_from_desired = -1;
    }
    
    
    uint8_t _distance_from_desired = -1;
    static constexpr int8_t _special_end_value = 0;
    union { T _value; };
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

template<typename T, bool>
struct AssignIfTrue {
    void operator()(T& lhs, T& rhs) {
        lhs = rhs;
    }

    void operator()(T&& lhs, T&& rhs) {
        lhs = std::move(rhs);
    }
};

template<typename T>
struct AssignIfTrue<T, false> {
    void operator()(T& lhs, T& rhs) {}
    void operator()(T&& lhs, T&& rhs) {}
};

inline size_t next_power_of_two(size_t i) {
    --i;
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    i |= i >> 32;
    ++i;
    return i;
}

template<typename...> 
using void_t = void;

template<typename T, typename = void>
struct HashPolicySelector {
    typedef fibonacci_hash_policy type;
};

template<typename T>
struct HashPolicySelector<T, void_t<typename T::hash_policy>> {
    typedef typename T::hash_policy type;
};



} // endnamespace ddaof