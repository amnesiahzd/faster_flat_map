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

#include <cmath>
#include <memory>
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

template <typename T, typename TypeKey,         // TODO: to clear why here is different
          typename Hasher, typename HasherArg,
          typename Equal, typename EqualArg,
          typename Alloc, typename AllocArg>
struct faster_hashtable : private Hasher, private Equal, private Alloc {
    using entry = faster_hashtable_entry<T>;
    using AllocatorTraits = std::allocator_traits<Alloc>; // deep look at this
    using EntryPointer = AllocatorTraits::pointer;
public:
    using value_type = T;
    using size_type = size_t;
    using diff_type = std::ptrdiff_t;

    using const_value_type = const value_type;
    using pointer_type = value_type*;
    using const_pointer_type = const value_type*;
    using value_reference_type = value_type&;

    // 1st coding: forget to add bucket_count
    // to clear explicit
    explicit faster_hashtable(size_t bucket_counts, 
                              const HasherArg& hasher_arg = HasherArg(), 
                              const EqualArg& equal_arg = EqualArg(), 
                              const AllocArg& alloc_arg = AllocArg())
            : Hasher(hasher_arg), EqualArg(equal_arg), AllocArg(alloc_arg) {
        rehash(bucket_counts);
    }

    // 因为写去掉后面的arguments的构造函数是没有意义的

    faster_hashtable(size_t bucket_counts, 
                     const HasherArg& hasher_arg = HasherArg(), 
                     const EqualArg& equal_arg = EqualArg(), 
                     const AllocArg& alloc_arg = AllocArg())
            : Hasher(hasher_arg), EqualArg(equal_arg), AllocArg(alloc_arg) {
        rehash(bucket_counts);
    }

    faster_hashtable(size_t bucket_counts,
                     const AllocArg& alloc_arg = AllocArg()) {
        faster_hashtable(bucket_counts, HasherArg(), EqualArg(), alloc_arg);
    }

    faster_hashtable(size_t bucket_counts,
                     const EqualArg& equal_arg = EqualArg()) {
        faster_hashtable(bucket_counts, HasherArg(), equal_arg);
    }

    faster_hashtable(size_t bucket_counts,
                     const EqualArg& equal_arg = EqualArg(),
                     const AllocArg& alloc_arg = AllocArg()) 
            : faster_hashtable(bucket_counts, HasherArg(), equal_arg, alloc_arg) {}

    faster_hashtable(size_t bucket_counts,
                     const HasherArg& hasher_arg = HasherArg(),
                     const AllocArg& alloc_arg = AllocArg())
            : faster_hashtable(bucket_counts, hasher_arg, EqualArg(), alloc_arg) {}

    explicit faster_hashtable(const ArgumentAlloc& alloc)
            : Alloc(alloc) {}

    template<typename It>
    faster_hashtable(It first, It last, size_t bucket_counts,
                     const HasherArg& hasher_arg = HasherArg(), 
                     const EqualArg& equal_arg = EqualArg(), 
                     const AllocArg& alloc_arg = AllocArg())
            : faster_hashtable(bucket_counts, hasher_arg, equal_arg, alloc_arg) {
        insert(first, last);
    }

    template<typename It>
    faster_hashtable(It first, It last, size_t bucket_counts,
                     const AllocArg& alloc = AllocArg())
            : faster_hashtable(bucket_counts, HasherArg(), EqualArg(), alloc) {
        insert(first, last);
    }

    template<typename It>
    faster_hashtable(It first, It last, size_t bucket_counts,
                     const HasherArg& hash = HasherArg(), 
                     const AllocArg& alloc = AllocArg())
            : faster_hashtable(bucket_counts, hash, EqualArg(), alloc) {
        insert(first, last);
    }

    faster_hashtable(std::initializer_list<T>& list, size_t bucket_counts,
                     const HasherArg& hasher_arg = HasherArg(), 
                     const EqualArg& equal_arg = EqualArg(), 
                     const AllocArg& alloc_arg = AllocArg())
            : faster_hashtable(bucket_counts, hasher_arg, equal_arg, alloc_arg) {
        if (bucket_counts == 0) {
            rehash(list.size());
        }
        insert(list.begin(), list.end());
    }

    faster_hashtable(std::initializer_list<T>& list, size_t bucket_counts,
                     const AllocArg& alloc = AllocArg())
            : faster_hashtable(bucket_counts, HasherArg(), EqualArg(), alloc) {
        if (bucket_counts == 0) {
            rehash(list.size());
        }
        insert(list.begin(), list.end());
    }

    faster_hashtable(std::initializer_list<T>& list, size_t bucket_counts,
                     const HasherArg& hash = HasherArg(), 
                     const AllocArg& alloc = AllocArg())
            : faster_hashtable(bucket_counts, hash, EqualArg(), alloc) {
        if (bucket_counts == 0) {
            rehash(list.size());
        }
        insert(list.begin(), list.end());
    }
    
    // copy constructor
    faster_hashtable(const faster_hashtable& other) 
            : Hasher(other), Alloc(other), Equal(other), _max_load_factor(other._max_load_factor) {
        rehash_for_other_container(other);
        try {
            insert(other.begin(), other.end())
        } catch(...) {
            clear();
            deallocate_data();
            throw;
        }
    }

    faster_hashtable(const faster_hashtable&& other) 
            : Hasher(std::move(other)), Alloc(std::move(other)), Equal(std::move(other)) {
        swap_pointers(other);
    }

    faster_hashtable(const faster_hashtable&& other, HasherArg& hash) 
            : Hasher(hash), Alloc(std::move(other)), Equal(std::move(other)) {
        swap_pointers(other);
    }

    //select_on_container_copy_construction
    // 再写一遍
    faster_hashtable& operator=(faster_hashtable& other) { // TODO: whats the difference between  =  and copy constructor / to clear add const if change the behavior of the code
        if(std::addressof(other) == this) {
            return *this;
        }
        clear();

        bool is_can_propagate_assign = AllocatorTraits::propagate_on_container_copy_assignment::value;
        if (is_can_propagate_assign) {
            if (static_cast<Alloc>(*this) != static_cast<const Alloc&>(other)) {
                reset_to_empty_state(); // TODO: get clear why
            }
            AssignIfTrue<EntryAlloc, is_can_propagate_assign>()(*this, other);
        }

        swap_pointers(other);
        static_cast<Hasher>(*this) = other; //
        static_cast<Equal>(*this) = other;  //
        this->_max_load_factor = other._max_load_factor;

        rehash_for_other_container(other);  //
        insert(other.begin(), other.end()); //

        return *this;
    }

    const Alloc& get_alloc() {
        return static_cast<const Alloc&>(*this); // 保证返回类型和设定的返回类型一致，包括const and ref
    }

    const Equal& get_equal() {
        return static_cast<const Equal&>(*this);
    }

    const Hasher& get_hasher() {
        return static_cast<const Hasher&>(*this);
    }

    size_t size() {
        return _num_elements;
    }

    size_t buckets_count() {
        return _slots_num_minus_one ? _slots_num_minus_one + 1 : 0;
    }

    ~faster_hashtable() {
        clear();
        deallocate_data();
    }

private:
    int8_t _max_lookups = ddaof::min_lookups - 1;
    float _max_load_factor = 0.5f;
    size_t _slots_num_minus_one;
    size_t _num_elements; 

    Hasher _hash_policy; // wrong: choose another typename
    Equal _equal;
    Alloc _allocator;
    EntryPointer _entrys = faster_hashtable_entry<T>::get_defualt_table();

    uint8_t compute_max_lookups(size_t bucket) {
        uint8_t desired = log2(bucket);
        return std::max(ddaof::min_lookups, desired);
    }

    // The current factor may be reset every time it is reserved.
    size_t num_buckets_for_reserve(size_t new_elements_count) {
        return static_cast<size_t>(new_elements_count / std::min(0.5f, static_cast<double>(_max_load_factor)));
    }

    void rehash_for_other_container(const faster_hashtable& other) {
        // the num of the elements must minus than the slots, why design this?
        rehash(std::min(num_buckets_for_reserve(other.size()), other.num_buckets_for_reserve + 1));
    }

    void swap_pointers(faster_hashtable& other) {
        using std::swap;
        swap(_max_lookups, other._max_lookups);
        swap(_max_load_factor, other._max_load_factor);
        swap(_slots_num_minus_one, other._slots_num_minus_one);
        swap(_num_elements, other.num_elements);

        swap(_hash_policy, other.hash_policy);
        swap(_max_load_factor, other._max_load_factor);

        // why didnt swap Equal Alloc
    }

    void grow() {
        rehash(std::max(static_cast<size_t>(4), bucket_count() * 2));
    }

    void deallocate_data(EntryPointer entry, size_t num_slots_minus_one, int8_t max_lookups) {
        if (entry != faster_hashtable_entry::empty_default_table()) {
            AllocatorTraits::deallocate(static_cast<Alloc>(*this), entry, num_slots_minus_one + 1 + max_lookups);
        }
    }

    void reset_to_empty_state() {
        deallocate_data(_entrys, _slots_num_minus_one, _max_lookups);

        _slots_num_minus_one = 0;
        _max_lookups = ddaof::min_lookups - 1;
        _entrys = faster_hashtable_entry::empty_default_table();
        _hash_policy.reset();

        return;
    }

    template<typename U>
    value_type& hasher_object(const U& key) {
        return static_cast<Hasher&>(*this)(key);
    }

    template<typename Left, typename Right>
    bool compares_equal(const Left& lhs, const Right& rhs) {
        return static_cast<Equal>(*this)(lhs, rhs);
    }

};

} // endnamespace ddaof