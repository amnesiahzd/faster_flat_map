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
    using Entry = sherwood_v3_entry<T>;
    // std::allocator_traits allows you to use allocator function 
    // even current now u dont know the allocate details
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
private:

};






} // end namespace ddaof


