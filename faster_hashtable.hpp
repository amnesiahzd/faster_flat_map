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

#include <algorithm>
#include <initializer_list>
#include <math.h>
#include <memory>
#include <utility>
#include <stdint.h>

// using faster_hashtable = sherwood_v3_table;

namespace ddaof {

static constexpr int8_t min_lookups = 4;

template<typename T>
struct sherwood_v3_entry {
    sherwood_v3_entry() {}
    sherwood_v3_entry(int8_t distance_from_desired)
            :_distance_from_desired(distance_from_desired) {}
    ~sherwood_v3_entry() {}

    static sherwood_v3_entry* empty_default_table() {
        static sherwood_v3_entry result[min_lookups] = { {}, {}, {}, {_special_end_value} }; // why?
        return result;
    }

    bool has_value() const {
        return _distance_from_desired >= 0;
    }

    bool is_empty() const {
        return _distance_from_desired < 0;
    }

    bool is_at_desired_position() const { 
        return _distance_from_desired <= 0;
    }

    template<typename... Args>
    void emplace(int8_t distance, Args&& ...args) { 
        new (std::addressof(value)) T(std::forward<Args>(args)...);
        _distance_from_desired = distance;
    }

    void destroy_value() {
        value.~T(); // TODO: how about to add a judge function to judge if it had a destroy function
        _distance_from_desired = -1;
    }

    int8_t _distance_from_desired = -1;
    static constexpr int8_t _special_end_value = 0;
    union { T value }; // why?
};


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
            insert(other.begin(), other.end());
        } catch(...) {
            clear();
            deallocate_data(_entries, _num_slots_minus_one, _max_lookups);
            throw;
        }
    }

    faster_hashtable(const faster_hashtable& other)
            : faster_hashtable(other, AllocatorTraits::select_on_container_copy_construction(other.get_allocator())) {} // TODO

    void rehash(size_t num_buckets) {
        // step1 caculate the new num of buckets
        num_buckets = std::max(num_buckets, static_cast<size_t>(std::ceil(_num_elements / static_cast<double>(_max_load_factor))));
        if (num_buckets == 0) {
            reset_to_empty_state();
            return;
        }
        if (num_buckets == bucket_count()) { // do not need do anything
            return;
        }

        // step2 caculate new prime number and new max lookups
        auto new_prime_index = _hash_policy.next_size_over(num_buckets);
        int8_t new_max_lookups = compute_max_lookups(num_buckets);

        // step3 apply new buckets
        EntryPointer new_buckets(AllocatorTraits::allocate(*this, num_buckets + new_max_lookups)); // Calculate the new maximum number of lookups, based on the new number of buckets
        EntryPointer special_end_item = new_buckets + static_cast<ptrdiff_t>(num_buckets + new_max_lookups - 1);
        
        // step4 swap new and old buctets
        for (EntryPointer it = new_buckets; it != special_end_item; ++it) {
            it->distance_from_desired = -1;
        }
        std::swap(_entries, new_buckets);
        std::swap(_num_slots_minus_one, num_buckets);
        --_num_slots_minus_one;
        _hash_policy.commit(new_prime_index); // TODO: how about using move

        int8_t old_max_lookups = _max_lookups;
        _max_lookups = new_max_lookups; // TODO: how about after deallocate
        _num_elements = 0;

        // step5 deallocate old buckets
        for (EntryPointer it = new_buckets, end = it + static_cast<ptrdiff_t>(num_buckets + old_max_lookups); it != end; ++it) {
            if (it->has_value()) {
                emplace(std::move(it->value));
                it->destroy_value();
            }
        }
        deallocate_data(new_buckets, num_buckets, old_max_lookups);
    }

    // get the max of the buckets to reserve
    size_t num_buckets_for_reserve(size_t num_elements) const {
        return static_cast<size_t>(std::ceil(num_elements / std::min(0.5, static_cast<double>(_max_load_factor))));
    }

    void rehash_for_other_container(const faster_hashtable& other) {
        rehash(num_buckets_for_reserve(other.size()), other.bucket_count());
    }

    template<typename Key, typename ...Args>
    std::pair<iterator, bool> emplace(Key&& key, Args&& ..args) {
        // step1: get the index of new key
        size_t index = _hash_policy.index_for_hash(hash_object(key), _num_slots_minus_one);
        
        // step2: check the key if it has already in the hashtable
        EntryPointer current_entry = _entries + ptrdiff_t(index);
        int8_t distance_from_desired = 0;
        for (; current_entry->distance_from_desired >= distance_from_desired; ++current_entry, ++distance_from_desired) {
            if (compares_equal(key, current_entry->value)) {
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

    // Ensures consistency with standard containers, 
    // but always inserts at the first position
    iterator insert(const_iterator, const value_type& value) {
        return emplace(value).first;
    }

    iterator insert(const_iterator, value_type && value) {
        return emplace(std::move(value)).first;
    }

    void reset_to_empty_state() {
        deallocate_data(_entries, _num_slots_minus_one, _max_lookups);
        _entries = Entry::empty_default_table();
        _num_slots_minus_one = 0;
        _hash_policy.reset();
        _max_lookups = detailv3::min_lookups - 1;
        return;
    }

    size_t bucket_count() const {
        return _num_slots_minus_one ? _num_slots_minus_one + 1 : 0;
    }

private:
    EntryPointer _entries = Entry::empty_default_table();
    size_t _num_slots_minus_one = 0;
    typename HashPolicySelector<ArgumentHash>::type _hash_policy;
    int8_t _max_lookups = detailv3::min_lookups - 1; // TODO: not done
    float _max_load_factor = 0.5f;
    size_t _num_elements = 0;

    static int8_t compute_max_lookups(size_t num_buckets) {
        int8_t desired = log2(num_buckets);
        return std::max(detailv3::min_lookups, desired);
    }

    void deallocate_data(EntryPointer begin, size_t num_slots_minus_one, int8_t max_lookups) {
        if (begin != Entry::empty_default_table()) {
            AllocatorTraits::deallocate(*this, begin, num_slots_minus_one + max_lookups + 1);
        }
    }

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




} // end namespace ddaof


