#pragma once

#include <climits>
#include <cstddef>

namespace ddaof {
template<typename T, size_t BlockSize = 4096>
class AmnesiaAllocator {
public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;

    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;

    template<typename U> struct rebind {
        using other = typename AmnesiaAllocator<U>;
    };

    AmnesiaAllocator() noexcept
        : _current_block(nullptr), 
          _current_slot(nullptr),
          _last_slot(nullptr),
          _free_slots(nullptr) {}

    AmnesiaAllocator(const AmnesiaAllocator& other) noexcept : AmnesiaAllocator() {}

    template<typename U>
    AmnesiaAllocator(const AmnesiaAllocator<U>& other) {
        AmnesiaAllocator();
    }

    AmnesiaAllocator& operator=(AmnesiaAllocator&& other) noexcept {
        if (other != this) {
            using std::swap;
            swap(_current_block, other._current_block);
            _current_slot = other._current_slot;
            _last_slot = other._last_slot;
            _free_slots = other._last_slot;
        }
        return *this;
    }

    ~AmnesiaAllocator() noexcept {
        block_pointer curr = _current_block;
        while (curr != nullptr) {
            block_pointer prev = curr->next;
            operator delete(reinterpret_cast<void*>(curr));
            curr = prev;
        }
    }

    inline pointer addressof(reference x) {
        return &x;
    }

    inline const_pointer addressof(const_reference x) {
        return &x;
    }

    inline pointer allocate(size_type n, const_pointer hint) {
        if (_free_slots != nullptr) {
            pointer result = reinterpret_cast<pointer>(_free_slots);
            _free_slots = _free_slots->next;
            return result;
        } else {
            if (_current_slot > _last_slot) {
                allocate_block();
                return reinterpret_cast<pointer>(_current_slot++);
            }
        }
    }

    inline void deallocate(pointer p, size_type n) {
        if (p != nullptr) {
            reinterpret_cast<block_pointer>(p)->next = _free_slots;
            _free_slots = reinterpret_cast<block_pointer>(p);
        }
    }

    inline size_type max_size() const noexcept {
        size_type max_blocks = -1 / BlockSize;
        return (BlockSize - sizeof(data_pointer)) / sizeof(block_type) * max_blocks;
    }

    template <typename U, class... Args>
    inline void construct(U* p, Args&&... args) {
        new (p) U(std::forward<Args>(args)...);
    }

    template <typename U>
    inline void destroy(U* p) {
        p->~U();
    }

    template <typename... Args>
    pointer newElement(Args&&... args) {
        pointer result = allocate();
        construct<value_type>(result, std::forward<Args>(args)...);
        return result;
    }

    inline void deleteElement(pointer p) {
        if (p != nullptr) {
            p->~value_type();
            deallocate(p);
        }
    }
private:
    union Block {
        value_type element;
        Block* next;
    };

    using data_pointer = char*;
    using block_type = Block;
    using block_pointer = Block*;

    block_pointer _current_block;
    block_pointer _current_slot;
    block_pointer _last_slot;
    block_pointer _free_slots;

    size_type pad_pointer(data_pointer p, size_type align) const noexcept {

    }

    void allocate_block() {
        // Allocate space for the new block and store a pointer to the previous one
        data_pointer new_block = reinterpret_cast<data_pointer>(operator new(BlockSize));
        reinterpret_cast<block_pointer>(new_block)->next = _current_block;
        _current_block = reinterpret_cast<block_pointer>(new_block);

        // Pad block body to staisfy the alignment requirements for elements
        data_pointer body = new_block + sizeof(block_pointer);
        size_type body_padding = pad_pointer(body, alignof(block_pointer));
        _current_slot = reinterpret_cast<block_pointer>(body + body_padding);
        _last_slot = reinterpret_cast<block_pointer>(new_block + BlockSize - sizeof(block_type) + 1);
    }

    static_assert(BlockSize >= 2 * sizeof(block_type), "Block too small");
};
} // end namespace ddaof