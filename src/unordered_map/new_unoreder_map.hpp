#pragma once

#include "new_faster_hashtable.hpp"

#include <type_traits>

namespace ddaof {
template<typename T, typename Allocator>
struct sherwood_v10_entry {
    sherwood_v10_entry() {}
    ~sherwood_v10_entry() {}

    using EntryPointer = typename std::allocator_traits<typename std::allocator_traits<Allocator>::template rebind_alloc<sherwood_v10_entry>>::pointer;

    EntryPointer next = nullptr;
    union { T value; };

    static EntryPointer* empty_pointer() {
        static EntryPointer result[3] = { EntryPointer(nullptr) + ptrdiff_t(1), nullptr, nullptr };
        return result + 1;
    }
};
    
using ddaof::function_wrapper;
using ddaof::key_or_value_hasher;
using ddaof::key_or_value_equality;
using ddaof::assign_if_true;
using ddaof::HashPolicySelector;

template<typename T, typename FindKey, 
         typename ArgumentHash, typename Hasher, 
         typename ArgumentEqual, typename Equal, 
         typename ArgumentAlloc, typename EntryAlloc, 
         typename BucketAllocator>
class sherwood_v10_table : private EntryAlloc, private Hasher, private Equal, private BucketAllocator {
    using Entry = detailv10::sherwood_v10_entry<T, ArgumentAlloc>;
    using AllocatorTraits = std::allocator_traits<EntryAlloc>;
    using BucketAllocatorTraits = std::allocator_traits<BucketAllocator>;
    using EntryPointer = typename AllocatorTraits::pointer;
    struct convertible_to_iterator;

public:
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;
    using hasher          = ArgumentHash;
    using key_equal       = ArgumentEqual;
    using allocator_type  = EntryAlloc;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;

    sherwood_v10_table() {}
    explicit sherwood_v10_table(size_type bucket_count, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
            : EntryAlloc(alloc), Hasher(hash), Equal(equal), BucketAllocator(alloc) {
        rehash(bucket_count);
    }

    sherwood_v10_table(size_type bucket_count, const ArgumentAlloc & alloc)
            : sherwood_v10_table(bucket_count, ArgumentHash(), ArgumentEqual(), alloc) {}

    sherwood_v10_table(size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
            : sherwood_v10_table(bucket_count, hash, ArgumentEqual(), alloc) {}

    explicit sherwood_v10_table(const ArgumentAlloc& alloc)
            : EntryAlloc(alloc), BucketAllocator(alloc) {}

    template<typename It>
    sherwood_v10_table(It first, It last, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
            : sherwood_v10_table(bucket_count, hash, equal, alloc) {
        insert(first, last);
    }

    template<typename It>
    sherwood_v10_table(It first, It last, size_type bucket_count, const ArgumentAlloc & alloc)
            : sherwood_v10_table(first, last, bucket_count, ArgumentHash(), ArgumentEqual(), alloc) {}

    sherwood_v10_table(std::initializer_list<T> il, size_type bucket_count = 0, const ArgumentHash & hash = ArgumentHash(), const ArgumentEqual & equal = ArgumentEqual(), const ArgumentAlloc & alloc = ArgumentAlloc())
            : sherwood_v10_table(bucket_count, hash, equal, alloc) {
        if (bucket_count == 0) {
            reserve(il.size());
        }
        insert(il.begin(), il.end());
    }

    sherwood_v10_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentAlloc & alloc)
            : sherwood_v10_table(il, bucket_count, ArgumentHash(), ArgumentEqual(), alloc) {}

    sherwood_v10_table(std::initializer_list<T> il, size_type bucket_count, const ArgumentHash & hash, const ArgumentAlloc & alloc)
            : sherwood_v10_table(il, bucket_count, hash, ArgumentEqual(), alloc) {}

    sherwood_v10_table(const sherwood_v10_table & other)
            : sherwood_v10_table(other, AllocatorTraits::select_on_container_copy_construction(other.get_allocator())) {}

    sherwood_v10_table(const sherwood_v10_table & other, const ArgumentAlloc & alloc)
            : EntryAlloc(alloc), Hasher(other), Equal(other), BucketAllocator(alloc), _max_load_factor(other._max_load_factor) {
        try {
            rehash_for_other_container(other);
            insert(other.begin(), other.end());
        } catch(...) {
            clear();
            deallocate_data();
            throw;
        }
    }

    sherwood_v10_table(sherwood_v10_table && other) noexcept
            : EntryAlloc(std::move(other)), Hasher(std::move(other)), Equal(std::move(other)), BucketAllocator(std::move(other)), _max_load_factor(other._max_load_factor) {
        swap_pointers(other);
    }
    
    sherwood_v10_table(sherwood_v10_table && other, const ArgumentAlloc & alloc) noexcept
            : EntryAlloc(alloc), Hasher(std::move(other)), Equal(std::move(other)), BucketAllocator(alloc), _max_load_factor(other._max_load_factor) {
        swap_pointers(other);
    }

    // line 155
};


} // end namespace ddaof