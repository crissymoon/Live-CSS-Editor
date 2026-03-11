#include "xcm_rgba_raster.h"

#include <algorithm>
#include <cstring>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include <immintrin.h>
#define XCM_SIMD_X86 1
#if defined(__GNUC__) || defined(__clang__)
#define XCM_ATTR_TARGET_AVX2 __attribute__((target("avx2")))
#define XCM_ATTR_TARGET_SSE2 __attribute__((target("sse2")))
#else
#define XCM_ATTR_TARGET_AVX2
#define XCM_ATTR_TARGET_SSE2
#endif
#else
#define XCM_SIMD_X86 0
#define XCM_ATTR_TARGET_AVX2
#define XCM_ATTR_TARGET_SSE2
#endif

namespace xcm::simd {

namespace {

inline uint32_t pack_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return static_cast<uint32_t>(r)
         | (static_cast<uint32_t>(g) << 8)
         | (static_cast<uint32_t>(b) << 16)
         | (static_cast<uint32_t>(a) << 24);
}

void rgba_clear_scalar(uint8_t* pixels,
                       int width,
                       int height,
                       int stride_bytes,
                       uint32_t packed) {
    for (int y = 0; y < height; ++y) {
        auto* row = reinterpret_cast<uint32_t*>(pixels + y * stride_bytes);
        std::fill(row, row + width, packed);
    }
}

#if XCM_SIMD_X86
XCM_ATTR_TARGET_SSE2
void rgba_clear_sse2(uint8_t* pixels,
                     int width,
                     int height,
                     int stride_bytes,
                     uint32_t packed) {
    const __m128i v = _mm_set1_epi32(static_cast<int>(packed));
    for (int y = 0; y < height; ++y) {
        auto* row = reinterpret_cast<uint32_t*>(pixels + y * stride_bytes);
        int x = 0;
        for (; x + 3 < width; x += 4) {
            _mm_storeu_si128(reinterpret_cast<__m128i*>(row + x), v);
        }
        for (; x < width; ++x) row[x] = packed;
    }
}

XCM_ATTR_TARGET_AVX2
void rgba_clear_avx2(uint8_t* pixels,
                     int width,
                     int height,
                     int stride_bytes,
                     uint32_t packed) {
    const __m256i v = _mm256_set1_epi32(static_cast<int>(packed));
    for (int y = 0; y < height; ++y) {
        auto* row = reinterpret_cast<uint32_t*>(pixels + y * stride_bytes);
        int x = 0;
        for (; x + 7 < width; x += 8) {
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(row + x), v);
        }
        for (; x < width; ++x) row[x] = packed;
    }
}
#endif

#if XCM_SIMD_X86
XCM_ATTR_TARGET_AVX2
void rgba_fill_rect_opaque_avx2(uint8_t* pixels,
                                int stride_bytes,
                                int x0,
                                int y0,
                                int x1,
                                int y1,
                                uint32_t packed) {
    const __m256i v = _mm256_set1_epi32(static_cast<int>(packed));
    for (int py = y0; py < y1; ++py) {
        auto* row = reinterpret_cast<uint32_t*>(pixels + py * stride_bytes);
        int px = x0;
        for (; px + 7 < x1; px += 8) {
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(row + px), v);
        }
        for (; px < x1; ++px) row[px] = packed;
    }
}

XCM_ATTR_TARGET_SSE2
void rgba_fill_rect_opaque_sse2(uint8_t* pixels,
                                int stride_bytes,
                                int x0,
                                int y0,
                                int x1,
                                int y1,
                                uint32_t packed) {
    const __m128i v = _mm_set1_epi32(static_cast<int>(packed));
    for (int py = y0; py < y1; ++py) {
        auto* row = reinterpret_cast<uint32_t*>(pixels + py * stride_bytes);
        int px = x0;
        for (; px + 3 < x1; px += 4) {
            _mm_storeu_si128(reinterpret_cast<__m128i*>(row + px), v);
        }
        for (; px < x1; ++px) row[px] = packed;
    }
}

XCM_ATTR_TARGET_AVX2
void rgba_fill_rect_alpha_avx2(uint8_t* pixels,
                               int width,
                               int height,
                               int stride_bytes,
                               int x0,
                               int y0,
                               int x1,
                               int y1,
                               uint8_t r,
                               uint8_t g,
                               uint8_t b,
                               uint8_t a,
                               uint16_t sa,
                               uint16_t ia) {
    const __m256i zero = _mm256_setzero_si256();
    const __m256i sa16 = _mm256_set1_epi16(static_cast<short>(sa));
    const __m256i ia16 = _mm256_set1_epi16(static_cast<short>(ia));
    const __m256i src8 = _mm256_setr_epi8(
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a),
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a),
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a),
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a),
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a),
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a),
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a),
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a));

    for (int py = y0; py < y1; ++py) {
        uint8_t* row8 = pixels + py * stride_bytes + x0 * 4;
        int count = x1 - x0;
        int i = 0;
        for (; i + 7 < count; i += 8) {
            __m256i dst8 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(row8 + i * 4));
            __m256i src_lo = _mm256_unpacklo_epi8(src8, zero);
            __m256i src_hi = _mm256_unpackhi_epi8(src8, zero);
            __m256i dst_lo = _mm256_unpacklo_epi8(dst8, zero);
            __m256i dst_hi = _mm256_unpackhi_epi8(dst8, zero);

            __m256i mul_src_lo = _mm256_mullo_epi16(src_lo, sa16);
            __m256i mul_src_hi = _mm256_mullo_epi16(src_hi, sa16);
            __m256i mul_dst_lo = _mm256_mullo_epi16(dst_lo, ia16);
            __m256i mul_dst_hi = _mm256_mullo_epi16(dst_hi, ia16);

            __m256i sum_lo = _mm256_add_epi16(mul_src_lo, mul_dst_lo);
            __m256i sum_hi = _mm256_add_epi16(mul_src_hi, mul_dst_hi);

            __m256i out_lo = _mm256_srli_epi16(sum_lo, 8);
            __m256i out_hi = _mm256_srli_epi16(sum_hi, 8);
            __m256i out8 = _mm256_packus_epi16(out_lo, out_hi);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(row8 + i * 4), out8);
        }

        for (; i < count; ++i) {
            uint8_t* p = row8 + i * 4;
            p[0] = static_cast<uint8_t>((r * sa + p[0] * ia) >> 8);
            p[1] = static_cast<uint8_t>((g * sa + p[1] * ia) >> 8);
            p[2] = static_cast<uint8_t>((b * sa + p[2] * ia) >> 8);
            p[3] = static_cast<uint8_t>((a * sa + p[3] * ia) >> 8);
        }
    }
}

XCM_ATTR_TARGET_SSE2
void rgba_fill_rect_alpha_sse2(uint8_t* pixels,
                               int width,
                               int height,
                               int stride_bytes,
                               int x0,
                               int y0,
                               int x1,
                               int y1,
                               uint8_t r,
                               uint8_t g,
                               uint8_t b,
                               uint8_t a,
                               uint16_t sa,
                               uint16_t ia) {
    const __m128i zero = _mm_setzero_si128();
    const __m128i sa16 = _mm_set1_epi16(static_cast<short>(sa));
    const __m128i ia16 = _mm_set1_epi16(static_cast<short>(ia));
    const __m128i src8 = _mm_setr_epi8(
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a),
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a),
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a),
        static_cast<char>(r), static_cast<char>(g), static_cast<char>(b), static_cast<char>(a));

    for (int py = y0; py < y1; ++py) {
        uint8_t* row8 = pixels + py * stride_bytes + x0 * 4;
        int count = x1 - x0;
        int i = 0;
        for (; i + 3 < count; i += 4) {
            __m128i dst8 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(row8 + i * 4));
            __m128i src_lo = _mm_unpacklo_epi8(src8, zero);
            __m128i src_hi = _mm_unpackhi_epi8(src8, zero);
            __m128i dst_lo = _mm_unpacklo_epi8(dst8, zero);
            __m128i dst_hi = _mm_unpackhi_epi8(dst8, zero);

            __m128i mul_src_lo = _mm_mullo_epi16(src_lo, sa16);
            __m128i mul_src_hi = _mm_mullo_epi16(src_hi, sa16);
            __m128i mul_dst_lo = _mm_mullo_epi16(dst_lo, ia16);
            __m128i mul_dst_hi = _mm_mullo_epi16(dst_hi, ia16);

            __m128i sum_lo = _mm_add_epi16(mul_src_lo, mul_dst_lo);
            __m128i sum_hi = _mm_add_epi16(mul_src_hi, mul_dst_hi);

            __m128i out_lo = _mm_srli_epi16(sum_lo, 8);
            __m128i out_hi = _mm_srli_epi16(sum_hi, 8);
            __m128i out8 = _mm_packus_epi16(out_lo, out_hi);
            _mm_storeu_si128(reinterpret_cast<__m128i*>(row8 + i * 4), out8);
        }
        for (; i < count; ++i) {
            uint8_t* p = row8 + i * 4;
            p[0] = static_cast<uint8_t>((r * sa + p[0] * ia) >> 8);
            p[1] = static_cast<uint8_t>((g * sa + p[1] * ia) >> 8);
            p[2] = static_cast<uint8_t>((b * sa + p[2] * ia) >> 8);
            p[3] = static_cast<uint8_t>((a * sa + p[3] * ia) >> 8);
        }
    }
}
#endif

} // namespace

CpuCaps detect_cpu_caps() {
    CpuCaps caps{};
#if XCM_SIMD_X86
#if defined(__GNUC__) || defined(__clang__)
    caps.sse2 = __builtin_cpu_supports("sse2");
    caps.avx2 = __builtin_cpu_supports("avx2");
#else
    caps.sse2 = true;
    caps.avx2 = false;
#endif
#endif
    return caps;
}

void rgba_clear(uint8_t* pixels,
                int width,
                int height,
                int stride_bytes,
                uint8_t r,
                uint8_t g,
                uint8_t b,
                uint8_t a) {
    if (!pixels || width <= 0 || height <= 0 || stride_bytes < width * 4) return;

    const uint32_t packed = pack_rgba(r, g, b, a);
    const CpuCaps caps = detect_cpu_caps();

#if XCM_SIMD_X86
    if (caps.avx2) {
        rgba_clear_avx2(pixels, width, height, stride_bytes, packed);
        return;
    }
    if (caps.sse2) {
        rgba_clear_sse2(pixels, width, height, stride_bytes, packed);
        return;
    }
#endif

    rgba_clear_scalar(pixels, width, height, stride_bytes, packed);
}

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
                           uint8_t a) {
    if (!pixels || width <= 0 || height <= 0 || w <= 0 || h <= 0) return;

    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(width, x + w);
    int y1 = std::min(height, y + h);
    if (x0 >= x1 || y0 >= y1) return;

    const uint32_t packed = pack_rgba(r, g, b, a);
    const CpuCaps caps = detect_cpu_caps();

#if XCM_SIMD_X86
    if (caps.avx2) {
        rgba_fill_rect_opaque_avx2(pixels, stride_bytes, x0, y0, x1, y1, packed);
        return;
    }
    if (caps.sse2) {
        rgba_fill_rect_opaque_sse2(pixels, stride_bytes, x0, y0, x1, y1, packed);
        return;
    }
#endif

    for (int py = y0; py < y1; ++py) {
        auto* row = reinterpret_cast<uint32_t*>(pixels + py * stride_bytes);
        std::fill(row + x0, row + x1, packed);
    }
}

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
                          uint8_t a) {
    if (!pixels || width <= 0 || height <= 0 || w <= 0 || h <= 0 || a == 0) return;
    if (a == 255) {
        rgba_fill_rect_opaque(pixels, width, height, stride_bytes, x, y, w, h, r, g, b, a);
        return;
    }

    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(width, x + w);
    int y1 = std::min(height, y + h);
    if (x0 >= x1 || y0 >= y1) return;

    const CpuCaps caps = detect_cpu_caps();
    const uint16_t sa = static_cast<uint16_t>(a);
    const uint16_t ia = static_cast<uint16_t>(255 - a);

#if XCM_SIMD_X86
    if (caps.avx2) {
        rgba_fill_rect_alpha_avx2(pixels, width, height, stride_bytes, x0, y0, x1, y1, r, g, b, a, sa, ia);
        return;
    }

    if (caps.sse2) {
        rgba_fill_rect_alpha_sse2(pixels, width, height, stride_bytes, x0, y0, x1, y1, r, g, b, a, sa, ia);
        return;
    }
#endif

    for (int py = y0; py < y1; ++py) {
        uint8_t* row8 = pixels + py * stride_bytes + x0 * 4;
        for (int i = 0; i < (x1 - x0); ++i) {
            uint8_t* p = row8 + i * 4;
            p[0] = static_cast<uint8_t>((r * sa + p[0] * ia) >> 8);
            p[1] = static_cast<uint8_t>((g * sa + p[1] * ia) >> 8);
            p[2] = static_cast<uint8_t>((b * sa + p[2] * ia) >> 8);
            p[3] = static_cast<uint8_t>((a * sa + p[3] * ia) >> 8);
        }
    }
}

} // namespace xcm::simd
