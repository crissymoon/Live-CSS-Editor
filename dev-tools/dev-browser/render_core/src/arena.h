#pragma once
/*
 * arena.h  --  slab-based arena allocator
 *
 * All DOM nodes, CSS rules, and layout boxes are allocated from a single
 * arena per render context.  One free() call at context destruction reclaims
 * everything.  No individual object freeing is supported (not needed for
 * a parse-layout-paint-destroy cycle).
 *
 * Thread safety: none.  Each render context owns its own arena.
 */

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <new>

namespace xcm {

static constexpr std::size_t ARENA_SLAB_SIZE = 512 * 1024; // 512 KB per slab

struct ArenaSlab {
    ArenaSlab* next = nullptr;
    std::size_t used = 0;
    std::size_t cap  = 0;
    // data follows immediately after this header in memory
    char* data() { return reinterpret_cast<char*>(this + 1); }
};

class Arena {
public:
    Arena() = default;

    ~Arena() {
        ArenaSlab* s = head_;
        while (s) {
            ArenaSlab* next = s->next;
            std::free(s);
            s = next;
        }
    }

    // Non-copyable, movable.
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&& o) noexcept : head_(o.head_), current_(o.current_) {
        o.head_ = o.current_ = nullptr;
    }

    void* alloc(std::size_t n, std::size_t align = alignof(std::max_align_t)) {
        if (!current_ || aligned_remaining() < n) {
            grow(n);
        }
        // Align the bump pointer.
        std::size_t offset = current_->used;
        std::size_t extra  = (align - (offset % align)) % align;
        current_->used = offset + extra + n;
        assert(current_->used <= current_->cap);
        return current_->data() + offset + extra;
    }

    template<typename T, typename... Args>
    T* make(Args&&... args) {
        void* mem = alloc(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    // Allocate a null-terminated copy of a string.
    const char* strdup(const char* s, std::size_t len) {
        char* buf = static_cast<char*>(alloc(len + 1, 1));
        std::memcpy(buf, s, len);
        buf[len] = '\0';
        return buf;
    }

    const char* strdup(const char* s) {
        return strdup(s, std::strlen(s));
    }

    // Reset without freeing slabs (reuse memory for next parse).
    void reset() {
        ArenaSlab* s = head_;
        while (s) {
            s->used = 0;
            s = s->next;
        }
        current_ = head_;
    }

    std::size_t bytes_used() const {
        std::size_t total = 0;
        ArenaSlab* s = head_;
        while (s) { total += s->used; s = s->next; }
        return total;
    }

private:
    ArenaSlab* head_    = nullptr;
    ArenaSlab* current_ = nullptr;

    std::size_t aligned_remaining() const {
        if (!current_) return 0;
        return current_->cap - current_->used;
    }

    void grow(std::size_t min_size) {
        std::size_t cap = ARENA_SLAB_SIZE;
        if (min_size + alignof(std::max_align_t) > cap) {
            cap = min_size + alignof(std::max_align_t) + 64;
        }
        void* mem = std::malloc(sizeof(ArenaSlab) + cap);
        if (!mem) std::abort();  // OOM -- no heap recovery in WASM
        ArenaSlab* slab = new (mem) ArenaSlab();
        slab->cap  = cap;
        slab->used = 0;
        slab->next = nullptr;

        if (!head_) {
            head_ = current_ = slab;
        } else {
            // Find tail and append (also update current to new slab).
            ArenaSlab* tail = head_;
            while (tail->next) tail = tail->next;
            tail->next = slab;
            current_   = slab;
        }
    }
};

// Fixed-capacity small vector stored inside an Arena (no heap of its own).
// Used for DOM children lists, attribute lists, CSS declaration lists.
template<typename T>
class ArenaVec {
public:
    ArenaVec() = default;

    void push(Arena& arena, T val) {
        if (size_ == cap_) grow(arena);
        data_[size_++] = val;
    }

    T&       operator[](std::size_t i)       { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    T*       begin()       { return data_; }
    const T* begin() const { return data_; }
    T*       end()         { return data_ + size_; }
    const T* end()   const { return data_ + size_; }

    std::size_t size()  const { return size_; }
    bool        empty() const { return size_ == 0; }

private:
    T*          data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t cap_  = 0;

    static constexpr std::size_t INITIAL_CAP = 4;

    void grow(Arena& arena) {
        std::size_t new_cap = cap_ == 0 ? INITIAL_CAP : cap_ * 2;
        T* new_data = static_cast<T*>(arena.alloc(new_cap * sizeof(T), alignof(T)));
        if (data_) {
            for (std::size_t i = 0; i < size_; ++i) new_data[i] = data_[i];
        }
        data_ = new_data;
        cap_  = new_cap;
    }
};

} // namespace xcm
