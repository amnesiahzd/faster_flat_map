#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <iostream>
#include "memory_pool.hpp"

#ifdef _MSC_VER
#define DDAOF_NOINLINE(...) __declspec(noinline) __VA_ARGS__
#else
#define DDAOF_NOINLINE(...) __VA_ARGS__ __attribute__((noinline))
#endif

namespace ddaof {
static constexpr int8_t min_lookups = 4;

struct fibonacci_hash_policy;
struct power_of_two_hash_policy;
struct prime_number_hash_policy;

template<typename Result, typename Functor>
struct function_wrapper : Functor {
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
struct function_wrapper<Result, Result(*)(Args...)> {
    using function_ptr = Result (*)(Args...);
    function_ptr _function;

    explicit function_wrapper(function_ptr other_ptr) : _function(other_ptr) {}

    Result operator()(Args&&... args) {
        return _function(std::forward<Args>(args)...);
    }
};

template<typename KeyType, typename ValueType, typename Hasher>
struct key_or_value_hasher : public function_wrapper<size_t, Hasher> {
    using hasher_storage = function_wrapper<size_t, Hasher>;
    key_or_value_hasher() = default;
    key_or_value_hasher(const Hasher& other) : hasher_storage(other) {}

    size_t operator()(const KeyType& key) { 
        return hasher_storage::operator()(key);
    }

    size_t operator()(const KeyType& key) const {
        return hasher_storage::operator()(key);
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

template<typename T>
struct alignas(16) faster_table_entry {
    faster_table_entry() {}
    faster_table_entry(int distance_from_desired) : _distance_from_desired(distance_from_desired) {}
    faster_table_entry(faster_table_entry& other) : _distance_from_desired(other._distance_from_desired) {}
    ~faster_table_entry() {
        if constexpr (!std::is_trivially_destructible<T>::value) {
            _value.~T(); 
        }
        _distance_from_desired = -1;
    }

    static faster_table_entry* empty_default_table() {
        static faster_table_entry result[min_lookups] = { {}, {}, {}, {_special_end_value} };
        return result;
    }

    bool has_value() const {
        return _distance_from_desired > -1;
    }

    bool is_empty() const {
        return _distance_from_desired == -1;
    }

    bool is_at_desired_pos() const {
        return _distance_from_desired <= 0;
    }

    template<typename ...Args>
    void emplace(int distance, Args&&... args) {
        new (std::addressof(_value)) T(std::forward<Args>(args)...);
        _distance_from_desired = distance;
    }

    void destroy_value() {
        if constexpr (!std::is_trivially_destructible<T>::value) {
            _value.~T(); 
        }
        _distance_from_desired = -1;
    }

    int8_t _distance_from_desired = -1;
    constexpr static int8_t _special_end_value = 0;
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
struct assign_if_true {
    void operator()(T& lhs, const T& rhs) {
        lhs = rhs;
    }

    void operator()(T&& lhs, T&& rhs) {
        lhs = std::move(rhs);
    }
};

template<typename T>
struct assign_if_true<T, false> {
    void operator()(T& lhs, const T& rhs) { /*empty*/ }
    void operator()(const T& lhs, const T& rhs) { /*empty*/ }
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

template<typename T, typename FindKey,
         typename Hasher, typename ArgumentHash,
         typename Equal, typename ArgumentEqual,
         typename EntryAlloc, typename ArgumentAlloc>
class faster_hashtable : private Hasher, private Equal, private EntryAlloc {
    using Entry = faster_table_entry<T>;
    using AllocatorTraits = std::allocator_traits<EntryAlloc>;
    using EntryPointer = typename AllocatorTraits::pointer;
    struct convertible_to_iterator;

public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    
    using hasher = ArgumentHash;
    using key_equal = ArgumentEqual;
    using allocator_type = EntryAlloc;

    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

    faster_hashtable() {}
    faster_hashtable(size_type bucket_count, 
                     const ArgumentHash& hash = ArgumentHash(), 
                     const ArgumentEqual& equal = ArgumentEqual(), 
                     const ArgumentAlloc& alloc = ArgumentAlloc()) : EntryAlloc(alloc), Hasher(hash), Equal(equal) {
        rehash(bucket_count);
    }

    faster_hashtable(size_type bucket_count, const ArgumentAlloc& alloc)
            : faster_hashtable(bucket_count, ArgumentHash(), ArgumentEqual(), alloc) {}

    faster_hashtable(size_type bucket_count, const ArgumentHash& hash, const ArgumentAlloc& alloc)
            : faster_hashtable(bucket_count, hash, ArgumentEqual(), alloc) {};

    explicit faster_hashtable(const ArgumentAlloc& alloc)
            : EntryAlloc(alloc) {}

    template<typename It>
    faster_hashtable(It first, It last, size_type bucket_count = 0, 
                     const ArgumentHash& hash = ArgumentHash(), 
                     const ArgumentEqual& equal = ArgumentEqual(), 
                     const ArgumentAlloc& alloc = ArgumentAlloc())
            : faster_hashtable(bucket_count, hash, equal, alloc) {
        insert(first, last);
    }

    template<typename It>
    faster_hashtable(It first, It last, size_type bucket_count,
                     const ArgumentAlloc& alloc = ArgumentAlloc())
            : faster_hashtable(bucket_count, ArgumentHash(), ArgumentEqual(), alloc) {
        insert(first, last);
    }

    template<typename It>
    faster_hashtable(It first, It last, size_type bucket_count,
                     const ArgumentHash& hash = ArgumentHash(), 
                     const ArgumentAlloc& alloc = ArgumentAlloc())
            : faster_hashtable(bucket_count, hash, ArgumentEqual(), alloc) {
        insert(first, last);
    }

    faster_hashtable(std::initializer_list<T>& initializer_list, // AmnesiaHzd: at origin codes, here isnt a ref
                     size_type bucket_count = 0, 
                     const ArgumentHash& hash = ArgumentHash(),
                     const ArgumentEqual& equal = ArgumentEqual(), 
                     const ArgumentAlloc& alloc = ArgumentAlloc())
            : faster_hashtable(bucket_count, hash, equal, alloc) {
        if (bucket_count == 0) {
            rehash(initializer_list.size());
        }
        insert(initializer_list.begin(), initializer_list.end());
    }

    faster_hashtable(std::initializer_list<T>& initializer_list,
                     size_type bucket_count, 
                     const ArgumentAlloc& alloc = ArgumentAlloc())
            : faster_hashtable(bucket_count, ArgumentHash(), ArgumentEqual(), alloc) {}

    faster_hashtable(std::initializer_list<T>& initializer_list,
                     size_type bucket_count, 
                     const ArgumentHash& hash = ArgumentHash(), 
                     const ArgumentAlloc& alloc = ArgumentAlloc())
            : faster_hashtable(bucket_count, hash, ArgumentEqual(), alloc) {}
    
    faster_hashtable(const faster_hashtable& other, ArgumentAlloc alloc) 
            :EntryAlloc(alloc), Hasher(other), Equal(other),  _max_load_factor(other._max_load_factor) {
        rehash_for_other(other);
        try {
            insert(other.begin(), other.end());
        } catch(...) {
            clear();
            deallocate_data(_entries, _num_slots_minus_one, _max_lookups);
            throw;
        }
    }

    faster_hashtable(const faster_hashtable& other) : 
            faster_hashtable(other, AllocatorTraits::select_on_container_copy_construction(other.get_allocator())) {}

    faster_hashtable(faster_hashtable&& other) noexcept 
            : EntryAlloc(std::move(other)), Hasher(std::move(other)), Equal(std::move(other)) {
        swap_pointers(other);
    }

    faster_hashtable(faster_hashtable&& other, const ArgumentAlloc& alloc) noexcept
            : EntryAlloc(alloc), Hasher(std::move(other)), Equal(std::move(other)) {
        swap_pointers(other);
    }

    faster_hashtable& operator=(const faster_hashtable& other) {
        // 1.判断是否相等
        if (std::addressof(other) == this) {
            return *this;
        }
        // 2.清除自身状态
        clear();
        if constexpr (AllocatorTraits::propagate_on_container_copy_assignment::value) {
            if (static_cast<EntryAlloc>(*this) != static_cast<const EntryAlloc&>(other)) {
                reset_to_empty_state();
            }
            assign_if_true<EntryAlloc, AllocatorTraits::propagate_on_container_copy_assignment::value>()(*this, other);
        }
        // 3.属性赋值
        _max_load_factor = other._max_load_factor;
        static_cast<Hasher&>(*this) = other;
        static_cast<Equal&>(*this) = other;
        // 4.value赋值
        rehash_for_other_container(other);
        insert(other.begin(), other.end());
        return *this;
    }

    faster_hashtable& operator=(faster_hashtable&& other) {
        if (other == *this) {
            return *this;
        } else if (AllocatorTraits::propagate_on_container_copy_assignment::value) {
            clear();
            reset_to_empty_state();
            assign_if_true<EntryAlloc, AllocatorTraits::propagate_on_container_move_assignment::value>()(*this, std::move(other));
            swap_pointer(other);
        } else if (static_cast<EntryAlloc>(other) == static_cast<EntryAlloc>(*this)) {
            swap_pointer(other);
        } else {
            clear();

            _max_load_factor = other._max_load_factor;
            rehash_for_other_container(other);
            for (T& elem : other) {
                emplace(std::move(elem));
            }
    
            other.clear();
        }
        return *this;
    }

    const allocator_type& get_allocator() const {
        return static_cast<const allocator_type&>(*this);
    }

    const ArgumentEqual& key_eq() const {
        return static_cast<const ArgumentEqual&>(*this);
    }

    const ArgumentHash& hash_function() const {
        return static_cast<const ArgumentHash&>(*this);
    }

    ~faster_hashtable() {
        clear();
        deallocate_data(_entries, _num_slots_minus_one, _max_lookups);
    }

    template<typename ValueType>
    struct templated_iterator {
        templated_iterator() = default;
        templated_iterator(EntryPointer current) : _current(current) {}
        EntryPointer _current = EntryPointer();

        using iterator_category = std::forward_iterator_tag;
        using value_type = ValueType;
        using difference_type = ptrdiff_t;
        using pointer = ValueType*;
        using reference = ValueType&;

        friend bool operator==(const templated_iterator& lhs, const templated_iterator& rhs) {
            return lhs._current == rhs._current;
        }

        friend bool operator!=(const templated_iterator& lhs, const templated_iterator& rhs) {
            return !(lhs == rhs);
        }

        templated_iterator& operator++() {
            do {
                ++_current;
            } while (_current->is_empty());
            return *this;
        }

        templated_iterator operator++(int) {
            templated_iterator copy(*this);
            ++(*this);

            return copy;
        }

        reference operator*() {
            return _current->value;
        }

        pointer operator->() {
            return std::addressof(_current->_value);
        }

        operator templated_iterator<const value_type>() const {
            return { _current };
        }
    };

    using iterator = templated_iterator<value_type>;
    using const_iterator = templated_iterator<const value_type>;

    iterator begin() {
        for (EntryPointer iter = _entries; ; ++iter) {
            if (iter->has_value()) {
                return {iter};
            }
        }
    }

    const_iterator cbegin() {
        for (EntryPointer iter = _entries; ; ++iter) {
            if (iter->has_value()) {
                return {iter};
            }
        }
    }

    const_iterator cbegin() const {
        return begin();
    }

    iterator end() {
        return { _entries + static_cast<ptrdiff_t>(_num_slots_minus_one + _max_lookups) };
    }

    const_iterator end() const {
        return { _entries + static_cast<ptrdiff_t>(_num_slots_minus_one + _max_lookups) };
    }

    const_iterator cend() const {
        return end();
    }

    iterator find(const FindKey& key) {
        size_t index = _hash_policy.index_for_hash(hash_object(key), _num_slots_minus_one);
        EntryPointer iter = _entries + ptrdiff_t(index);
        for (uint8_t distance = 0; iter->_distance_from_desired >= distance; ++distance, ++iter) {
            if (compares_equal(key, iter->_value)) {
                return { iter };
            }
        }
        return end();
    }

    const_iterator find(const FindKey& key) const {
        return const_cast<faster_hashtable*>(this)->find(key); // ?
    }

    size_t count(const FindKey& key) {
        return find(key) == end() ? 1 : 0;
    }

    std::pair<iterator, iterator> equal_range(const FindKey& key) {
        iterator iter = find(key);
        if (iter == end()) {
            return std::make_pair(iter, iter);
        } else {
            return std::make_pair(iter, std::next(iter));
        }
    }

    std::pair<iterator, iterator> equal_range(const FindKey& key) const {
        iterator found = find(key);
        if (found == end()) {
            return std::make_pair(found, found);
        } else {
            return std::make_pair(found, std::next(found));
        } 
    }

    void rehash(size_t num_buckets) {
        // 1.计算当前所需slots数量
        num_buckets = std::max(num_buckets, static_cast<size_t>(std::ceil(_num_elements / static_cast<double>(_max_load_factor))));
        if (num_buckets == 0) {
            reset_to_empty_state();
            return;
        } 
        if (num_buckets == bucket_count()) {
            return;
        }
        // 2.计算下一个新的质数作为桶的slot num、计算新的max_look_up
        auto next_prime_index = _hash_policy.next_size_over(num_buckets);
        auto new_max_lookups = compute_max_lookups(num_buckets);

        // 3.创建新的桶
        EntryPointer new_buckets(AllocatorTraits::allocate(*this, num_buckets + new_max_lookups));
        EntryPointer special_end_item = new_buckets + static_cast<ptrdiff_t>(num_buckets + new_max_lookups - 1);
        
        // 4.复制数据
        for (EntryPointer iter = new_buckets; iter != special_end_item; ++iter) {
            iter->_distance_from_desired = -1;
        }
        std::swap(_entries, new_buckets);
        std::swap(_num_slots_minus_one, num_buckets);
        --_num_slots_minus_one;
        _hash_policy.commit(next_prime_index); 

        int8_t old_max_lookups = _max_lookups;
        _max_lookups = new_max_lookups; 
        _num_elements = 0;

        // 5.删除原来的表的数据
        for (EntryPointer it = new_buckets, end = it + static_cast<ptrdiff_t>(num_buckets + old_max_lookups); it != end; ++it) {
            if (it->has_value()) {
                emplace(std::move(it->_value));
                it->destroy_value();
            }
        }
        deallocate_data(new_buckets, num_buckets, old_max_lookups);
    }

//     void rehash(size_t num_buckets) {
//     // step1 calculate the new number of buckets
//     num_buckets = std::max(num_buckets, static_cast<size_t>(std::ceil(_num_elements / _max_load_factor)));
//     if (num_buckets == 0) {
//         reset_to_empty_state();
//         return;
//     }
//     if (num_buckets == bucket_count()) { // Do not need to do anything
//         return;
//     }

//     // step2 calculate new prime number and new max lookups
//     auto new_prime_index = _hash_policy.next_size_over(num_buckets);
//     auto new_max_lookups = compute_max_lookups(num_buckets);

//     // Using smart pointers for exception safety
//     std::unique_ptr<Entry[], std::function<void(Entry*)>> new_buckets(
//         AllocatorTraits::allocate(*this, num_buckets + new_max_lookups),
//         [this, num_buckets, new_max_lookups](Entry* ptr) {
//             deallocate_data(ptr, num_buckets, new_max_lookups);
//         }
//     );
//     Entry* special_end_item = new_buckets.get() + num_buckets + new_max_lookups - 1;

//     // Initialize new buckets
//     for (Entry* it = new_buckets.get(); it != special_end_item; ++it) {
//         it->distance_from_desired = -1;
//     }

//     // Swap new and old buckets
//     std::unique_ptr<Entry[], std::function<void(Entry*)>> old_buckets(
//         _entries,
//         [this, num_buckets = _num_slots_minus_one + 1, old_max_lookups = _max_lookups](Entry* ptr) {
//             deallocate_data(ptr, num_buckets, old_max_lookups);
//         }
//     );
//     _entries = new_buckets.release();
//     _num_slots_minus_one = num_buckets - 1;
//     _hash_policy.commit(new_prime_index);

//     auto old_max_lookups = _max_lookups;
//     _max_lookups = new_max_lookups; 
//     _num_elements = 0;

//     // Rehash elements
//     for (Entry* it = old_buckets.get(), *end = it + num_buckets + old_max_lookups; it != end; ++it) {
//         if (it->has_value()) {
//             emplace(std::move(it->value()));
//             it->destroy_value();
//         }
//     }
//     // Old buckets will be automatically deallocated when `old_buckets` goes out of scope
// }

    void reserve(size_t num_elements) {
        auto required_buckets = num_buckets_for_reserve(num_elements);
        if (required_buckets > bucket_count()) {
            rehash(num_elements);
        }
    }

    convertible_to_iterator erase(const_iterator to_erase) {
        EntryPointer current = to_erase._current;
        current->destroy_value();
        --_num_elements;

        for (EntryPointer next = current + ptrdiff_t(1); !next->is_at_desired_pos(); ++current, ++next) {
            current->emplace(next->_distance_from_desired - 1, std::move(next->_value));
            next->destroy_value();
        }
        return { to_erase._current };
    }

    void clear() {
        for (EntryPointer it = _entries, end = it + static_cast<ptrdiff_t>(_num_slots_minus_one + _max_lookups); it != end; ++it) { // 为什么不是除以空槽和有数据的槽的综合？
            if (it->has_value()) {
                it->destroy_value();
            }
        }

        _num_elements = 0;
    }

    size_t erase(const FindKey& key) {
        auto found = find(key);
        if (found == end()) {
            return 0;
        } else {
            erase(found);
            return 1;
        }
    }

    void shrink_to_fit() {
        rehash_for_other_container(*this);
    }

    size_t size() {
        return _num_elements;
    }

    size_t max_size() {
        return (AllocatorTraits::max_size(*this)) / sizeof(Entry);
    }

    size_type max_bucket_count() const {
        return (AllocatorTraits::max_size(*this) - min_lookups) / sizeof(Entry);
    }

    size_t bucket(const FindKey& key) const {
        return _hash_policy.index_for_hash(hash_object(key), _num_slots_minus_one);
    }

    size_t bucket_count() const {
        return _num_slots_minus_one ? _num_slots_minus_one + 1 : 0;
    }

    float load_factor() const {
        size_t buckets = bucket_count();
        if (buckets) {
            return static_cast<float>(_num_elements) / bucket_count();
        } else {
            return 0;
        }     
    }

    void max_load_factor(float value) {
        _max_load_factor = value;
    }

    float max_load_factor() const {
        return _max_load_factor;
    }

    bool empty() const {
        return _num_elements == 0;
    }

    template<typename Key, typename... Args>
    std::pair<iterator, bool> emplace(Key&& key, Args... args) {
        // 1. 计算当前的index
        auto index = _hash_policy.index_for_hash(hash_object(key), _num_slots_minus_one);

        // 2. 比较是否有相同的key
        auto current_entry = _entries + ptrdiff_t(index);
        uint8_t distance_from_desired = 0;
        for ( ; current_entry->_distance_from_desired >= distance_from_desired; ++current_entry, ++distance_from_desired) {
            if (compares_equal(key, current_entry->_value)) {
                return std::make_pair(current_entry, false);
            }
        }

        return emplace_new_key(distance_from_desired, current_entry, std::forward<Key>(key), std::forward<Args>(args)...);
    }

    template<typename... Args>
    iterator emplace_hint(const_iterator, Args& ...args) {
        return emplace(std::forward<Args>(args)...).first;
    }

    std::pair<iterator, bool> insert(const value_type& value) {
        return emplace(value);
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        return emplace(std::move(value));
    }

    iterator insert(const_iterator, const value_type& value) {
        return emplace(value).first;
    }

    iterator insert(const_iterator, value_type && value) {
        return emplace(std::move(value)).first;
    }


private:
    EntryPointer _entries = Entry::empty_default_table();
    size_t _num_slots_minus_one = 0;
    typename HashPolicySelector<ArgumentHash>::type _hash_policy;
    int8_t _max_lookups = ddaof::min_lookups - 1;
    float _max_load_factor = 0.5f;
    size_t _num_elements = 0;

    static int8_t compute_max_lookups(size_t num_buckets) {
        int8_t desired = log2(num_buckets);
        return std::max(desired, ddaof::min_lookups);
    }

    size_t num_buckets_for_reserve(size_t num_elements) const {
        return static_cast<size_t>(std::ceil(num_elements / std::min(0.5f, static_cast<float>(_max_load_factor))));
    }

    void rehash_for_other_container(const faster_hashtable& other) { // ?
        rehash(std::min(other.bucket_count(), num_buckets_for_reserve(other.size())));
    }

    void swap_pointers(faster_hashtable& other) {
        using std::swap;
        swap(_hash_policy, other.hash_policy);
        swap(_entries, other.entries);
        swap(_num_slots_minus_one, other.num_slots_minus_one);
        swap(_num_elements, other.num_elements);
        swap(_max_lookups, other.max_lookups);
        swap(_max_load_factor, other._max_load_factor);
    }

    template<typename Key, typename... Args>
    DDAOF_NOINLINE(std::pair<iterator, bool>) 
    emplace_new_key(int8_t distance_from_desired, EntryPointer current_entry, Key&& key, Args&&... args) noexcept {
        using std::swap;
        // 当容器增长时的条件判断
        if (_num_slots_minus_one == 0
                || distance_from_desired == _max_lookups
                || _num_elements + 1 > (_num_slots_minus_one + 1) * static_cast<double>(_max_load_factor)) {
            grow();
            return emplace(std::forward<Key>(key), std::forward<Args>(args)...);
        } else if (current_entry->is_empty()) {
            current_entry->emplace(distance_from_desired, std::forward<Key>(key), std::forward<Args>(args)...);
            ++_num_elements;
            return std::make_pair(current_entry, true);
        } else {/* empty */}

        auto to_insert = value_type(std::forward<Key>(key), std::forward<Args>(args)...);
        swap(distance_from_desired, current_entry->_distance_from_desired);
        swap(to_insert, current_entry->_value);
        iterator result = { current_entry };

        for (++distance_from_desired, ++current_entry; ; ++current_entry) {
            if (current_entry->is_empty()) {
                current_entry->emplace(distance_from_desired, std::move(to_insert));
                ++_num_elements;
                return { result, true };
            } else if (current_entry->_distance_from_desired < distance_from_desired) {
                swap(distance_from_desired, current_entry->_distance_from_desired);
                swap(to_insert, current_entry->_value);
                ++distance_from_desired;
            } else {
                ++distance_from_desired;
                if (distance_from_desired == _max_lookups) {
                    swap(to_insert, result._current->_value);
                    grow(); // 需要增长容器
                    return emplace(std::move(to_insert)); // 重新插入对象
                }
            }
        }
    }

    void grow() {
        rehash(std::max(size_t(4), 2 * bucket_count()));
    }

    void deallocate_data(EntryPointer begin, size_t num_slots_minus_one, int8_t max_lookups) {
        if (begin != Entry::empty_default_table()) {
            AllocatorTraits::deallocate(*this, begin, num_slots_minus_one + 1 + max_lookups);
        }
    }

    void reset_to_empty_state() {
        deallocate_data(_entries, _num_slots_minus_one, _max_lookups);
        _entries = Entry::empty_default_table();
        _num_slots_minus_one = 0;
        _hash_policy.reset();
        _max_lookups = ddaof::min_lookups - 1;
        return;
    }

    template<typename U>
    size_t hash_object(const U& key) {
        return static_cast<Hasher&>(*this)(key);
    }

    template<typename U>
    size_t hash_object(const U& key) const {
        return static_cast<const Hasher&>(*this)(key);
    }

    template<typename L, typename R>
    bool compares_equal(const L& lhs, const R& rhs) {
        return static_cast<Equal&>(*this)(lhs, rhs);
    }

    struct convertible_to_iterator {
        iterator it;

        operator iterator() {
            if (it->has_value()) {
                return { it };
            } else {
                return  ++iterator{it};
            }
        }

        operator const_iterator() {
            if (it->has_value()) {
                return { it };
            } else {
                return ++const_iterator{it};
            }
        }
    };

};

struct prime_number_hash_policy {
    static size_t mod0(size_t) { return 0llu; }
    static size_t mod2(size_t hash) { return hash % 2llu; }
    static size_t mod3(size_t hash) { return hash % 3llu; }
    static size_t mod5(size_t hash) { return hash % 5llu; }
    static size_t mod7(size_t hash) { return hash % 7llu; }
    static size_t mod11(size_t hash) { return hash % 11llu; }
    static size_t mod13(size_t hash) { return hash % 13llu; }
    static size_t mod17(size_t hash) { return hash % 17llu; }
    static size_t mod23(size_t hash) { return hash % 23llu; }
    static size_t mod29(size_t hash) { return hash % 29llu; }
    static size_t mod37(size_t hash) { return hash % 37llu; }
    static size_t mod47(size_t hash) { return hash % 47llu; }
    static size_t mod59(size_t hash) { return hash % 59llu; }
    static size_t mod73(size_t hash) { return hash % 73llu; }
    static size_t mod97(size_t hash) { return hash % 97llu; }
    static size_t mod127(size_t hash) { return hash % 127llu; }
    static size_t mod151(size_t hash) { return hash % 151llu; }
    static size_t mod197(size_t hash) { return hash % 197llu; }
    static size_t mod251(size_t hash) { return hash % 251llu; }
    static size_t mod313(size_t hash) { return hash % 313llu; }
    static size_t mod397(size_t hash) { return hash % 397llu; }
    static size_t mod499(size_t hash) { return hash % 499llu; }
    static size_t mod631(size_t hash) { return hash % 631llu; }
    static size_t mod797(size_t hash) { return hash % 797llu; }
    static size_t mod1009(size_t hash) { return hash % 1009llu; }
    static size_t mod1259(size_t hash) { return hash % 1259llu; }
    static size_t mod1597(size_t hash) { return hash % 1597llu; }
    static size_t mod2011(size_t hash) { return hash % 2011llu; }
    static size_t mod2539(size_t hash) { return hash % 2539llu; }
    static size_t mod3203(size_t hash) { return hash % 3203llu; }
    static size_t mod4027(size_t hash) { return hash % 4027llu; }
    static size_t mod5087(size_t hash) { return hash % 5087llu; }
    static size_t mod6421(size_t hash) { return hash % 6421llu; }
    static size_t mod8089(size_t hash) { return hash % 8089llu; }
    static size_t mod10193(size_t hash) { return hash % 10193llu; }
    static size_t mod12853(size_t hash) { return hash % 12853llu; }
    static size_t mod16193(size_t hash) { return hash % 16193llu; }
    static size_t mod20399(size_t hash) { return hash % 20399llu; }
    static size_t mod25717(size_t hash) { return hash % 25717llu; }
    static size_t mod32401(size_t hash) { return hash % 32401llu; }
    static size_t mod40823(size_t hash) { return hash % 40823llu; }
    static size_t mod51437(size_t hash) { return hash % 51437llu; }
    static size_t mod64811(size_t hash) { return hash % 64811llu; }
    static size_t mod81649(size_t hash) { return hash % 81649llu; }
    static size_t mod102877(size_t hash) { return hash % 102877llu; }
    static size_t mod129607(size_t hash) { return hash % 129607llu; }
    static size_t mod163307(size_t hash) { return hash % 163307llu; }
    static size_t mod205759(size_t hash) { return hash % 205759llu; }
    static size_t mod259229(size_t hash) { return hash % 259229llu; }
    static size_t mod326617(size_t hash) { return hash % 326617llu; }
    static size_t mod411527(size_t hash) { return hash % 411527llu; }
    static size_t mod518509(size_t hash) { return hash % 518509llu; }
    static size_t mod653267(size_t hash) { return hash % 653267llu; }
    static size_t mod823117(size_t hash) { return hash % 823117llu; }
    static size_t mod1037059(size_t hash) { return hash % 1037059llu; }
    static size_t mod1306601(size_t hash) { return hash % 1306601llu; }
    static size_t mod1646237(size_t hash) { return hash % 1646237llu; }
    static size_t mod2074129(size_t hash) { return hash % 2074129llu; }
    static size_t mod2613229(size_t hash) { return hash % 2613229llu; }
    static size_t mod3292489(size_t hash) { return hash % 3292489llu; }
    static size_t mod4148279(size_t hash) { return hash % 4148279llu; }
    static size_t mod5226491(size_t hash) { return hash % 5226491llu; }
    static size_t mod6584983(size_t hash) { return hash % 6584983llu; }
    static size_t mod8296553(size_t hash) { return hash % 8296553llu; }
    static size_t mod10453007(size_t hash) { return hash % 10453007llu; }
    static size_t mod13169977(size_t hash) { return hash % 13169977llu; }
    static size_t mod16593127(size_t hash) { return hash % 16593127llu; }
    static size_t mod20906033(size_t hash) { return hash % 20906033llu; }
    static size_t mod26339969(size_t hash) { return hash % 26339969llu; }
    static size_t mod33186281(size_t hash) { return hash % 33186281llu; }
    static size_t mod41812097(size_t hash) { return hash % 41812097llu; }
    static size_t mod52679969(size_t hash) { return hash % 52679969llu; }
    static size_t mod66372617(size_t hash) { return hash % 66372617llu; }
    static size_t mod83624237(size_t hash) { return hash % 83624237llu; }
    static size_t mod105359939(size_t hash) { return hash % 105359939llu; }
    static size_t mod132745199(size_t hash) { return hash % 132745199llu; }
    static size_t mod167248483(size_t hash) { return hash % 167248483llu; }
    static size_t mod210719881(size_t hash) { return hash % 210719881llu; }
    static size_t mod265490441(size_t hash) { return hash % 265490441llu; }
    static size_t mod334496971(size_t hash) { return hash % 334496971llu; }
    static size_t mod421439783(size_t hash) { return hash % 421439783llu; }
    static size_t mod530980861(size_t hash) { return hash % 530980861llu; }
    static size_t mod668993977(size_t hash) { return hash % 668993977llu; }
    static size_t mod842879579(size_t hash) { return hash % 842879579llu; }
    static size_t mod1061961721(size_t hash) { return hash % 1061961721llu; }
    static size_t mod1337987929(size_t hash) { return hash % 1337987929llu; }
    static size_t mod1685759167(size_t hash) { return hash % 1685759167llu; }
    static size_t mod2123923447(size_t hash) { return hash % 2123923447llu; }
    static size_t mod2675975881(size_t hash) { return hash % 2675975881llu; }
    static size_t mod3371518343(size_t hash) { return hash % 3371518343llu; }
    static size_t mod4247846927(size_t hash) { return hash % 4247846927llu; }
    static size_t mod5351951779(size_t hash) { return hash % 5351951779llu; }
    static size_t mod6743036717(size_t hash) { return hash % 6743036717llu; }
    static size_t mod8495693897(size_t hash) { return hash % 8495693897llu; }
    static size_t mod10703903591(size_t hash) { return hash % 10703903591llu; }
    static size_t mod13486073473(size_t hash) { return hash % 13486073473llu; }
    static size_t mod16991387857(size_t hash) { return hash % 16991387857llu; }
    static size_t mod21407807219(size_t hash) { return hash % 21407807219llu; }
    static size_t mod26972146961(size_t hash) { return hash % 26972146961llu; }
    static size_t mod33982775741(size_t hash) { return hash % 33982775741llu; }
    static size_t mod42815614441(size_t hash) { return hash % 42815614441llu; }
    static size_t mod53944293929(size_t hash) { return hash % 53944293929llu; }
    static size_t mod67965551447(size_t hash) { return hash % 67965551447llu; }
    static size_t mod85631228929(size_t hash) { return hash % 85631228929llu; }
    static size_t mod107888587883(size_t hash) { return hash % 107888587883llu; }
    static size_t mod135931102921(size_t hash) { return hash % 135931102921llu; }
    static size_t mod171262457903(size_t hash) { return hash % 171262457903llu; }
    static size_t mod215777175787(size_t hash) { return hash % 215777175787llu; }
    static size_t mod271862205833(size_t hash) { return hash % 271862205833llu; }
    static size_t mod342524915839(size_t hash) { return hash % 342524915839llu; }
    static size_t mod431554351609(size_t hash) { return hash % 431554351609llu; }
    static size_t mod543724411781(size_t hash) { return hash % 543724411781llu; }
    static size_t mod685049831731(size_t hash) { return hash % 685049831731llu; }
    static size_t mod863108703229(size_t hash) { return hash % 863108703229llu; }
    static size_t mod1087448823553(size_t hash) { return hash % 1087448823553llu; }
    static size_t mod1370099663459(size_t hash) { return hash % 1370099663459llu; }
    static size_t mod1726217406467(size_t hash) { return hash % 1726217406467llu; }
    static size_t mod2174897647073(size_t hash) { return hash % 2174897647073llu; }
    static size_t mod2740199326961(size_t hash) { return hash % 2740199326961llu; }
    static size_t mod3452434812973(size_t hash) { return hash % 3452434812973llu; }
    static size_t mod4349795294267(size_t hash) { return hash % 4349795294267llu; }
    static size_t mod5480398654009(size_t hash) { return hash % 5480398654009llu; }
    static size_t mod6904869625999(size_t hash) { return hash % 6904869625999llu; }
    static size_t mod8699590588571(size_t hash) { return hash % 8699590588571llu; }
    static size_t mod10960797308051(size_t hash) { return hash % 10960797308051llu; }
    static size_t mod13809739252051(size_t hash) { return hash % 13809739252051llu; }
    static size_t mod17399181177241(size_t hash) { return hash % 17399181177241llu; }
    static size_t mod21921594616111(size_t hash) { return hash % 21921594616111llu; }
    static size_t mod27619478504183(size_t hash) { return hash % 27619478504183llu; }
    static size_t mod34798362354533(size_t hash) { return hash % 34798362354533llu; }
    static size_t mod43843189232363(size_t hash) { return hash % 43843189232363llu; }
    static size_t mod55238957008387(size_t hash) { return hash % 55238957008387llu; }
    static size_t mod69596724709081(size_t hash) { return hash % 69596724709081llu; }
    static size_t mod87686378464759(size_t hash) { return hash % 87686378464759llu; }
    static size_t mod110477914016779(size_t hash) { return hash % 110477914016779llu; }
    static size_t mod139193449418173(size_t hash) { return hash % 139193449418173llu; }
    static size_t mod175372756929481(size_t hash) { return hash % 175372756929481llu; }
    static size_t mod220955828033581(size_t hash) { return hash % 220955828033581llu; }
    static size_t mod278386898836457(size_t hash) { return hash % 278386898836457llu; }
    static size_t mod350745513859007(size_t hash) { return hash % 350745513859007llu; }
    static size_t mod441911656067171(size_t hash) { return hash % 441911656067171llu; }
    static size_t mod556773797672909(size_t hash) { return hash % 556773797672909llu; }
    static size_t mod701491027718027(size_t hash) { return hash % 701491027718027llu; }
    static size_t mod883823312134381(size_t hash) { return hash % 883823312134381llu; }
    static size_t mod1113547595345903(size_t hash) { return hash % 1113547595345903llu; }
    static size_t mod1402982055436147(size_t hash) { return hash % 1402982055436147llu; }
    static size_t mod1767646624268779(size_t hash) { return hash % 1767646624268779llu; }
    static size_t mod2227095190691797(size_t hash) { return hash % 2227095190691797llu; }
    static size_t mod2805964110872297(size_t hash) { return hash % 2805964110872297llu; }
    static size_t mod3535293248537579(size_t hash) { return hash % 3535293248537579llu; }
    static size_t mod4454190381383713(size_t hash) { return hash % 4454190381383713llu; }
    static size_t mod5611928221744609(size_t hash) { return hash % 5611928221744609llu; }
    static size_t mod7070586497075177(size_t hash) { return hash % 7070586497075177llu; }
    static size_t mod8908380762767489(size_t hash) { return hash % 8908380762767489llu; }
    static size_t mod11223856443489329(size_t hash) { return hash % 11223856443489329llu; }
    static size_t mod14141172994150357(size_t hash) { return hash % 14141172994150357llu; }
    static size_t mod17816761525534927(size_t hash) { return hash % 17816761525534927llu; }
    static size_t mod22447712886978529(size_t hash) { return hash % 22447712886978529llu; }
    static size_t mod28282345988300791(size_t hash) { return hash % 28282345988300791llu; }
    static size_t mod35633523051069991(size_t hash) { return hash % 35633523051069991llu; }
    static size_t mod44895425773957261(size_t hash) { return hash % 44895425773957261llu; }
    static size_t mod56564691976601587(size_t hash) { return hash % 56564691976601587llu; }
    static size_t mod71267046102139967(size_t hash) { return hash % 71267046102139967llu; }
    static size_t mod89790851547914507(size_t hash) { return hash % 89790851547914507llu; }
    static size_t mod113129383953203213(size_t hash) { return hash % 113129383953203213llu; }
    static size_t mod142534092204280003(size_t hash) { return hash % 142534092204280003llu; }
    static size_t mod179581703095829107(size_t hash) { return hash % 179581703095829107llu; }
    static size_t mod226258767906406483(size_t hash) { return hash % 226258767906406483llu; }
    static size_t mod285068184408560057(size_t hash) { return hash % 285068184408560057llu; }
    static size_t mod359163406191658253(size_t hash) { return hash % 359163406191658253llu; }
    static size_t mod452517535812813007(size_t hash) { return hash % 452517535812813007llu; }
    static size_t mod570136368817120201(size_t hash) { return hash % 570136368817120201llu; }
    static size_t mod718326812383316683(size_t hash) { return hash % 718326812383316683llu; }
    static size_t mod905035071625626043(size_t hash) { return hash % 905035071625626043llu; }
    static size_t mod1140272737634240411(size_t hash) { return hash % 1140272737634240411llu; }
    static size_t mod1436653624766633509(size_t hash) { return hash % 1436653624766633509llu; }
    static size_t mod1810070143251252131(size_t hash) { return hash % 1810070143251252131llu; }
    static size_t mod2280545475268481167(size_t hash) { return hash % 2280545475268481167llu; }
    static size_t mod2873307249533267101(size_t hash) { return hash % 2873307249533267101llu; }
    static size_t mod3620140286502504283(size_t hash) { return hash % 3620140286502504283llu; }
    static size_t mod4561090950536962147(size_t hash) { return hash % 4561090950536962147llu; }
    static size_t mod5746614499066534157(size_t hash) { return hash % 5746614499066534157llu; }
    static size_t mod7240280573005008577(size_t hash) { return hash % 7240280573005008577llu; }
    static size_t mod9122181901073924329(size_t hash) { return hash % 9122181901073924329llu; }
    static size_t mod11493228998133068689(size_t hash) { return hash % 11493228998133068689llu; }
    static size_t mod14480561146010017169(size_t hash) { return hash % 14480561146010017169llu; }
    static size_t mod18446744073709551557(size_t hash) { return hash % 18446744073709551557llu; }
    
    using mod_function = size_t (*)(size_t);

    mod_function next_size_over(size_t& size) const {
        // prime numbers generated by the following method:
        // 1. start with a prime p = 2
        // 2. go to wolfram alpha and get p = NextPrime(2 * p)
        // 3. repeat 2. until you overflow 64 bits
        // you now have large gaps which you would hit if somebody called reserve() with an unlucky number.
        // 4. to fill the gaps for every prime p go to wolfram alpha and get ClosestPrime(p * 2^(1/3)) and ClosestPrime(p * 2^(2/3)) and put those in the gaps
        // 5. get PrevPrime(2^64) and put it at the end

        static constexpr const size_t prime_list[] =
        {
            2llu, 3llu, 5llu, 7llu, 11llu, 13llu, 17llu, 23llu, 29llu, 37llu, 47llu,
            59llu, 73llu, 97llu, 127llu, 151llu, 197llu, 251llu, 313llu, 397llu,
            499llu, 631llu, 797llu, 1009llu, 1259llu, 1597llu, 2011llu, 2539llu,
            3203llu, 4027llu, 5087llu, 6421llu, 8089llu, 10193llu, 12853llu, 16193llu,
            20399llu, 25717llu, 32401llu, 40823llu, 51437llu, 64811llu, 81649llu,
            102877llu, 129607llu, 163307llu, 205759llu, 259229llu, 326617llu,
            411527llu, 518509llu, 653267llu, 823117llu, 1037059llu, 1306601llu,
            1646237llu, 2074129llu, 2613229llu, 3292489llu, 4148279llu, 5226491llu,
            6584983llu, 8296553llu, 10453007llu, 13169977llu, 16593127llu, 20906033llu,
            26339969llu, 33186281llu, 41812097llu, 52679969llu, 66372617llu,
            83624237llu, 105359939llu, 132745199llu, 167248483llu, 210719881llu,
            265490441llu, 334496971llu, 421439783llu, 530980861llu, 668993977llu,
            842879579llu, 1061961721llu, 1337987929llu, 1685759167llu, 2123923447llu,
            2675975881llu, 3371518343llu, 4247846927llu, 5351951779llu, 6743036717llu,
            8495693897llu, 10703903591llu, 13486073473llu, 16991387857llu,
            21407807219llu, 26972146961llu, 33982775741llu, 42815614441llu,
            53944293929llu, 67965551447llu, 85631228929llu, 107888587883llu,
            135931102921llu, 171262457903llu, 215777175787llu, 271862205833llu,
            342524915839llu, 431554351609llu, 543724411781llu, 685049831731llu,
            863108703229llu, 1087448823553llu, 1370099663459llu, 1726217406467llu,
            2174897647073llu, 2740199326961llu, 3452434812973llu, 4349795294267llu,
            5480398654009llu, 6904869625999llu, 8699590588571llu, 10960797308051llu,
            13809739252051llu, 17399181177241llu, 21921594616111llu, 27619478504183llu,
            34798362354533llu, 43843189232363llu, 55238957008387llu, 69596724709081llu,
            87686378464759llu, 110477914016779llu, 139193449418173llu,
            175372756929481llu, 220955828033581llu, 278386898836457llu,
            350745513859007llu, 441911656067171llu, 556773797672909llu,
            701491027718027llu, 883823312134381llu, 1113547595345903llu,
            1402982055436147llu, 1767646624268779llu, 2227095190691797llu,
            2805964110872297llu, 3535293248537579llu, 4454190381383713llu,
            5611928221744609llu, 7070586497075177llu, 8908380762767489llu,
            11223856443489329llu, 14141172994150357llu, 17816761525534927llu,
            22447712886978529llu, 28282345988300791llu, 35633523051069991llu,
            44895425773957261llu, 56564691976601587llu, 71267046102139967llu,
            89790851547914507llu, 113129383953203213llu, 142534092204280003llu,
            179581703095829107llu, 226258767906406483llu, 285068184408560057llu,
            359163406191658253llu, 452517535812813007llu, 570136368817120201llu,
            718326812383316683llu, 905035071625626043llu, 1140272737634240411llu,
            1436653624766633509llu, 1810070143251252131llu, 2280545475268481167llu,
            2873307249533267101llu, 3620140286502504283llu, 4561090950536962147llu,
            5746614499066534157llu, 7240280573005008577llu, 9122181901073924329llu,
            11493228998133068689llu, 14480561146010017169llu, 18446744073709551557llu
        };
        static constexpr size_t (* const mod_functions[])(size_t) =
        {
            &mod0, &mod2, &mod3, &mod5, &mod7, &mod11, &mod13, &mod17, &mod23, &mod29, &mod37,
            &mod47, &mod59, &mod73, &mod97, &mod127, &mod151, &mod197, &mod251, &mod313, &mod397,
            &mod499, &mod631, &mod797, &mod1009, &mod1259, &mod1597, &mod2011, &mod2539, &mod3203,
            &mod4027, &mod5087, &mod6421, &mod8089, &mod10193, &mod12853, &mod16193, &mod20399,
            &mod25717, &mod32401, &mod40823, &mod51437, &mod64811, &mod81649, &mod102877,
            &mod129607, &mod163307, &mod205759, &mod259229, &mod326617, &mod411527, &mod518509,
            &mod653267, &mod823117, &mod1037059, &mod1306601, &mod1646237, &mod2074129,
            &mod2613229, &mod3292489, &mod4148279, &mod5226491, &mod6584983, &mod8296553,
            &mod10453007, &mod13169977, &mod16593127, &mod20906033, &mod26339969, &mod33186281,
            &mod41812097, &mod52679969, &mod66372617, &mod83624237, &mod105359939, &mod132745199,
            &mod167248483, &mod210719881, &mod265490441, &mod334496971, &mod421439783,
            &mod530980861, &mod668993977, &mod842879579, &mod1061961721, &mod1337987929,
            &mod1685759167, &mod2123923447, &mod2675975881, &mod3371518343, &mod4247846927,
            &mod5351951779, &mod6743036717, &mod8495693897, &mod10703903591, &mod13486073473,
            &mod16991387857, &mod21407807219, &mod26972146961, &mod33982775741, &mod42815614441,
            &mod53944293929, &mod67965551447, &mod85631228929, &mod107888587883, &mod135931102921,
            &mod171262457903, &mod215777175787, &mod271862205833, &mod342524915839,
            &mod431554351609, &mod543724411781, &mod685049831731, &mod863108703229,
            &mod1087448823553, &mod1370099663459, &mod1726217406467, &mod2174897647073,
            &mod2740199326961, &mod3452434812973, &mod4349795294267, &mod5480398654009,
            &mod6904869625999, &mod8699590588571, &mod10960797308051, &mod13809739252051,
            &mod17399181177241, &mod21921594616111, &mod27619478504183, &mod34798362354533,
            &mod43843189232363, &mod55238957008387, &mod69596724709081, &mod87686378464759,
            &mod110477914016779, &mod139193449418173, &mod175372756929481, &mod220955828033581,
            &mod278386898836457, &mod350745513859007, &mod441911656067171, &mod556773797672909,
            &mod701491027718027, &mod883823312134381, &mod1113547595345903, &mod1402982055436147,
            &mod1767646624268779, &mod2227095190691797, &mod2805964110872297, &mod3535293248537579,
            &mod4454190381383713, &mod5611928221744609, &mod7070586497075177, &mod8908380762767489,
            &mod11223856443489329, &mod14141172994150357, &mod17816761525534927,
            &mod22447712886978529, &mod28282345988300791, &mod35633523051069991,
            &mod44895425773957261, &mod56564691976601587, &mod71267046102139967,
            &mod89790851547914507, &mod113129383953203213, &mod142534092204280003,
            &mod179581703095829107, &mod226258767906406483, &mod285068184408560057,
            &mod359163406191658253, &mod452517535812813007, &mod570136368817120201,
            &mod718326812383316683, &mod905035071625626043, &mod1140272737634240411,
            &mod1436653624766633509, &mod1810070143251252131, &mod2280545475268481167,
            &mod2873307249533267101, &mod3620140286502504283, &mod4561090950536962147,
            &mod5746614499066534157, &mod7240280573005008577, &mod9122181901073924329,
            &mod11493228998133068689, &mod14480561146010017169, &mod18446744073709551557
        };

        const size_t* found = std::lower_bound(std::begin(prime_list), std::end(prime_list) - 1, size);
        size = *found;
        return mod_functions[1 + found - prime_list];
    }

    void commit(mod_function new_mod_function) {
        _current_mod_function = new_mod_function;
    }

    void reset() {
        _current_mod_function = &mod0;
    }

    size_t index_for_hash(size_t hash, size_t /*num_slots_minus_one*/) const {
        return _current_mod_function(hash);
    }

    size_t keep_in_range(size_t index, size_t num_slots_minus_one) const {
        return index > num_slots_minus_one ? _current_mod_function(index) : index;
    }
    
private:
    mod_function _current_mod_function = &mod0;
};

struct power_of_two_hash_policy {
    size_t index_for_hash(size_t hash, size_t num_slots_minus_one) const {
        return hash & num_slots_minus_one;
    }

    size_t keep_in_range(size_t index, size_t num_slots_minus_one) const {
        return index_for_hash(index, num_slots_minus_one);
    }

    int8_t next_size_over(size_t& size) const { // why use pass-by-ref?
        size = ddaof::next_power_of_two(size);
        return 0;
    }

    void commit(int8_t) {}
    void reset() {}
};

struct fibonacci_hash_policy {
    size_t index_for_hash(size_t hash, size_t num_slots_minus_one) const {
        return (11400714819323198485ull * hash) >> shift;
    }

    size_t keep_in_range(size_t index, size_t num_slots_minus_one) const {
        return index & num_slots_minus_one;
    }

    int8_t next_size_over(size_t& size) const {
        size = std::max(size_t(2), ddaof::next_power_of_two(size));
        return 64 - ddaof::log2(size);
    }

    void commit(int8_t shift) {
        this->shift = shift;
    }

    void reset() {
        shift = 63;
    }
private:
    int8_t shift = 63;
};

template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = std::allocator<std::pair<K, V>> >
// template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename A = ddaof::AmnesiaAllocator<std::pair<K, V>> >
class flat_hash_map : public ddaof::faster_hashtable<
            std::pair<K, V>, K,
            ddaof::key_or_value_hasher<K, std::pair<K, V>, H>, H,
            ddaof::key_or_value_equality<K, std::pair<K, V>, E>, E,
            typename std::allocator_traits<A>::template rebind_alloc<ddaof::faster_table_entry<std::pair<K, V>>>, A> {
using Table = ddaof::faster_hashtable<
            std::pair<K, V>, K,
            ddaof::key_or_value_hasher<K, std::pair<K, V>, H>, H,
            ddaof::key_or_value_equality<K, std::pair<K, V>, E>, E,
            typename std::allocator_traits<A>::template rebind_alloc<ddaof::faster_table_entry<std::pair<K, V>>>, A>;
public:
    using key_type = K;
    using value_type = V;

    using Table::Table;
    flat_hash_map() {}

    inline V& operator[](const K& key) {
        return emplace(key, convertible_to_value()).first->second;
    }

    inline V& operator[](K&& key) {
        return emplace(std::move(key), convertible_to_value()).first->second;
    }

    V& at(const K& key) {
        auto found = this->find(key); // this指针调用父类
        if (found == this->end()) {
            throw std::out_of_range("Argument passed to at() was not in the map.");
        }

        return  found->second;
    }

    const V& at(const K& key) const {
        auto found = this->find(key);
        if (found == this->end())
            throw std::out_of_range("Argument passed to at() was not in the map.");
        return found->second;
    }

    using Table::emplace;
    std::pair<typename Table::iterator, bool> emplace() { // ?
        return emplace(key_type(), convertible_to_value());
    }

    template<typename M>
    std::pair<typename Table::iterator, bool> insert_or_assign(const key_type& key, M&& m) {
        auto emplace_result = emplace(key, std::forward<M>(m));
        if (!emplace_result.second)
            emplace_result.first->second = std::forward<M>(m);
        return emplace_result;
    }

    template<typename M>
    std::pair<typename Table::iterator, bool> insert_or_assign(key_type&& key, M&& m) {
        auto emplace_result = emplace(std::move(key), std::forward<M>(m));
        if (!emplace_result.second)
            emplace_result.first->second = std::forward<M>(m);
        return emplace_result;
    }
    template<typename M>
    typename Table::iterator insert_or_assign(typename Table::const_iterator, const key_type& key, M&& m)
    {
        return insert_or_assign(key, std::forward<M>(m)).first;
    }

    template<typename M>
    typename Table::iterator insert_or_assign(typename Table::const_iterator, key_type&& key, M&& m) {
        return insert_or_assign(std::move(key), std::forward<M>(m)).first;
    }

    friend bool operator==(const flat_hash_map& lhs, const flat_hash_map& rhs) {
        if (lhs.size() != rhs.size())
            return false;
        for (const typename Table::value_type& value : lhs) {
            auto found = rhs.find(value.first);
            if (found == rhs.end())
                return false;
            else if (value.second != found->second)
                return false;
        }
        return true;
    }

    friend bool operator!=(const flat_hash_map& lhs, const flat_hash_map& rhs) {
        return !(lhs == rhs);
    }

private:
    struct convertible_to_value { // ?
        operator V() const {
            return V();
        }
    };
};

template<typename T, typename H = std::hash<T>, typename E = std::equal_to<T>, typename A = std::allocator<T> >
class flat_hash_set
        : public ddaof::faster_hashtable
        <
            T,
            T,
            H,
            ddaof::function_wrapper<size_t, H>,
            E,
            ddaof::function_wrapper<bool, E>,
            A,
            typename std::allocator_traits<A>::template rebind_alloc<ddaof::faster_table_entry<T>>
        >
{
    using Table = ddaof::faster_hashtable
    <
        T,
        T,
        H,
        ddaof::function_wrapper<size_t, H>,
        E,
        ddaof::function_wrapper<bool, E>,
        A,
        typename std::allocator_traits<A>::template rebind_alloc<ddaof::faster_table_entry<T>>
    >;
public:

    using key_type = T;

    using Table::Table;
    flat_hash_set() {}

    template<typename... Args>
    std::pair<typename Table::iterator, bool> emplace(Args&&... args) {
        return Table::emplace(T(std::forward<Args>(args)...));
    }

    std::pair<typename Table::iterator, bool> emplace(const key_type& arg) {
        return Table::emplace(arg);
    }

    std::pair<typename Table::iterator, bool> emplace(key_type& arg) {
        return Table::emplace(arg);
    }

    std::pair<typename Table::iterator, bool> emplace(const key_type&& arg) {
        return Table::emplace(std::move(arg));
    }

    std::pair<typename Table::iterator, bool> emplace(key_type&& arg) {
        return Table::emplace(std::move(arg));
    }

    friend bool operator==(const flat_hash_set& lhs, const flat_hash_set& rhs) {
        if (lhs.size() != rhs.size())
            return false;
        for (const T & value : lhs)
        {
            if (rhs.find(value) == rhs.end())
                return false;
        }
        return true;
    }

    friend bool operator!=(const flat_hash_set& lhs, const flat_hash_set& rhs) {
        return !(lhs == rhs);
    }
};

} // end namespace ddaof