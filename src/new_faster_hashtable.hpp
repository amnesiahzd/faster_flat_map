#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace ddaof {
static constexpr int8_t min_lookups = 4;

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

template<typename T>
struct faster_table_entry {
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
    union {T _value;}
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
    using EntryPointer = AllocatorTraits::pointer;
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
                     const ArgumentEqual& equal = Equal(), 
                     const ArgumentAlloc& alloc = ArgumentAlloc()) : Alloc(alloc), Hasher(hash), Equal(equal) {
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
        rehash_for_other(other)
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
        if (other == *this) {
            return *this;
        }
        // 2.清除自身状态
        clear();
        if constexpr (AllocatorTraits::propagate_on_container_copy_assignment::value) {
            if (static_cast<EntryAlloc>(*this) != static_cast<const EntryAlloc&>(other)) {
                reset_to_empty_state();
            }
            AssignIfTrue<EntryAlloc, AllocatorTraits::propagate_on_container_copy_assignment::value>()(*this, other);
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
            AssignIfTrue<EntryAlloc, AllocatorTraits::propagate_on_container_move_assignment::value>()(*this, std::move(other));
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
            return std::addressof(_current->value);
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

private:
    EntryPointer _entries = Entry::empty_default_table();
    size_t _num_slots_minus_one = 0;
    typename HashPolicySelector<ArgumentHash>::type _hash_policy;
    int8_t _max_lookups = ddaof::min_lookups - 1;
    float _max_load_factor = 0.5f;
    size_t _num_elements = 0;

};

} // end namespace ddaof