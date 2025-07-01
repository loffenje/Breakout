#pragma once

#include "common.h"

//NOTE: simple linear allocator, made to reduce the number of allocations for entities.

struct MemoryArena {
    size_t  capacity = 0;
    size_t  used = 0;
    u8 *    base = nullptr;

    inline void Init(size_t size, u8 *memory);

    inline void *PushSize(size_t size);

    template <typename T>
    inline T *Push();

    template <typename T, typename... Args>
    inline T *Push(Args &&... args);

    template <typename T, size_t N>
    inline T *PushArray();

    template <typename T>
    inline T *PushArray(size_t length);

    inline void Clear();
};


inline 
void MemoryArena::Init(size_t the_capacity, u8 *memory) {
    capacity = the_capacity;
    base = memory;
}

inline 
void *MemoryArena::PushSize(size_t size) {

    assert((used + size) <= capacity);
    void *result = base + used;
    used += size;

    return result;
}

template<typename T>
inline 
T *MemoryArena::Push() {
    void *ptr = PushSize(sizeof(T));

    return new(ptr) T();
}

template <typename T, typename... Args>
inline 
T *MemoryArena::Push(Args &&... args) {
    void *ptr = PushSize(sizeof(T));

    return new(ptr) T(std::forward<Args>(args)...);
}

template <typename T, size_t N>
inline 
T *MemoryArena::PushArray() {
    constexpr size_t length = N;
    static_assert(length != 0 && "Length of array should be at least 1.");

    T *ptr = (T *)PushSize(length * sizeof(T));
    for (size_t i = 0; i < length; i++) {
        new (ptr + i) T;
    }

    return ptr;
}

template<typename T>
inline
T *MemoryArena::PushArray(size_t length) {
    T *ptr = (T *)PushSize(length * sizeof(T));
    for (size_t i = 0; i < length; i++) {
        new (ptr + i) T;
    }

    return ptr;
}

inline 
void MemoryArena::Clear() {
    used = 0;
}