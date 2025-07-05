#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <type_traits>

#define DEVELOPER 1

using f64 = double;
using f32 = float;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;
using b32 = int32_t;

struct AppSettings {
    const char *name;
    int         windowWidth;
    int         windowHeight;
    f32         screenWidth;
    f32         screenHeight;

    inline f32 GetResolutionScale() const {
        f32 w = (f32)GetScreenWidth() / screenWidth;
        f32 h = (f32)GetScreenHeight() / screenHeight;
        f32 result = std::min(w, h);

        return result;
    }

};

static constexpr int TARGET_FPS = 90;
static constexpr f32 TIME_STEP = 1.0f / static_cast<f32>(TARGET_FPS);

template <typename T, int N>
struct Buffer {
    T   data[N];
    u32 len = 0;

    static_assert(std::is_trivially_copyable<T>::value &&std::is_standard_layout<T>::value, "T must be a POD-like type");

    Buffer() {
        memset(data, 0, sizeof(data));
    }

    T &operator[](int index) {
        return data[index];
    }

    const T &operator[](int index) const {
        return data[index];
    }

    int Add(const T &item) {
        assert(len < N);
        data[len++] = item;

        return len - 1;
    }

    void Clear() {
        memset(data, 0, sizeof(data));
        len = 0;
    }
};


namespace globals {
	AppSettings appSettings;
}