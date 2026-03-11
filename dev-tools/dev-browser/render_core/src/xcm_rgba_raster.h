#pragma once

#include <cstdint>

namespace xcm::simd {

struct CpuCaps {
    bool sse2 = false;
    bool avx2 = false;
};

CpuCaps detect_cpu_caps();

void rgba_clear(uint8_t* pixels,
                int width,
                int height,
                int stride_bytes,
                uint8_t r,
                uint8_t g,
                uint8_t b,
                uint8_t a);

void rgba_fill_rect_opaque(uint8_t* pixels,
                           int width,
                           int height,
                           int stride_bytes,
                           int x,
                           int y,
                           int w,
                           int h,
                           uint8_t r,
                           uint8_t g,
                           uint8_t b,
                           uint8_t a);

void rgba_fill_rect_alpha(uint8_t* pixels,
                          int width,
                          int height,
                          int stride_bytes,
                          int x,
                          int y,
                          int w,
                          int h,
                          uint8_t r,
                          uint8_t g,
                          uint8_t b,
                          uint8_t a);

} // namespace xcm::simd
