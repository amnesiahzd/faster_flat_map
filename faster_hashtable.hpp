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

#include <initializer_list>
#include <math.h>
#include <memory>
#include <stdint.h>

#pragma once

namespace ddaof {

template <typename T, typename FindKey, 
          typename ArgumentHash, typename Hasher,
          typename ArgumentEqual, typename Equal,
          typename ArgumentAlloc, typename EntryAlloc>
// 1.its a has-a relationship, 
// 2.all the member function and member factor from father class would be hide at this class 
class faster_hashtable : private EntryAlloc, private Hasher, private Equal { 
    using Entry = sherwood_v3_entry<T>; // TODO: not done
    // std::allocator_traits allows you to use allocator function 
    // even current now u dont know the allocate details
    using AllocatorTraits = std::allocator_traits<EntryAlloc>; 
    using EntryPointer = typename AllocatorTraits::pointer;
    struct convertible_to_iterator; // TODO: not done

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

    // all constructor

    faster_hashtable() {}
    
    explicit faster_hashtable(size_type bucket_count, 
                              const ArgumentHash& hash = ArgumentHash(), 
                              const ArgumentEqual& equal = ArgumentEqual(), 
                              const ArgumentAlloc& alloc = ArgumentAlloc())
            : EntryAlloc(alloc), Hasher(hash), Equal(equal) {
        // use alloc/hash/equal to init base Class EntryAlloc/Hasher/Equal
        rehash(bucket_count); // TODO: not done
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
        insert(first, last); // TODO: not done/consider emplace?
    }

    template<typename It>
    faster_hashtable(It first, It last, size_type bucket_count,
                     const ArgumentAlloc& alloc = ArgumentAlloc())
            : faster_hashtable(bucket_count, ArgumentHash(), ArgumentEqual(), alloc) {
        insert(first, last); // TODO: can init caller user defualt params?
    }

    template<typename It>
    faster_hashtable(It first, It last, size_type bucket_count,
                     const ArgumentHash& hash = ArgumentHash(), 
                     const ArgumentAlloc& alloc = ArgumentAlloc())
            : faster_hashtable(bucket_count, hash, ArgumentEqual(), alloc) {
        insert(first, last);
    }

    faster_hashtable(std::initializer_list<T>& initializer_list, // consider dangling reference
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

    // copy constructor

    faster_hashtable(const faster_hashtable& other, const ArgumentAlloc& alloc)
            // Hasher is the base class of this 
            : EntryAlloc(alloc), Hasher(other), Equal(other), _max_load_factor(other._max_load_factor) {
        rehash_for_other_container(other);
        try {
            insert(other.begin(), other.end()); // TODO
        } catch(...) {
            clear();
            deallocate_data(_entries, _num_slots_minus_one, _max_lookups); // TODO
            throw;
        }
    }

    faster_hashtable(const faster_hashtable& other)
            : faster_hashtable(other, AllocatorTraits::select_on_container_copy_construction(other.get_allocator())) {}

    void rehash(size_t num_buckets) {
        num_buckets = std::max(num_buckets, static_cast<size_t>(std::ceil(_num_elements / static_cast<double>(_max_load_factor)))); //TODO: to accomplish
        if (num_buckets == 0) {
            reset_to_empty_state();
            return;
        }
        if (num_buckets == bucket_count()) { // TODO: not done
            return;
        }

        auto new_prime_index = hash_policy.next_size_over(num_buckets);
        // line 635
    }

    void reset_to_empty_state() {
        deallocate_data(_entries, _num_slots_minus_one, _max_lookups);
        _entries = Entry::empty_default_table();
        _num_slots_minus_one = 0;
        _hash_policy.reset();
        _max_lookups = detailv3::min_lookups - 1;
        return;
    }
private:
    EntryPointer _entries = Entry::empty_default_table();
    size_t _num_slots_minus_one = 0;
    typename HashPolicySelector<ArgumentHash>::type _hash_policy;
    int8_t _max_lookups = detailv3::min_lookups - 1; // TODO: not done
    float _max_load_factor = 0.5f;
    size_t _num_elements = 0;

};






} // end namespace ddaof


