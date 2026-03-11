#include "xcm_walnut_software_raster.h"

#include "xcm_rgba_raster.h"

#include <algorithm>

namespace xcm {

void WalnutSoftwareRasterHost::resize(int width, int height) {
    width_ = std::max(0, width);
    height_ = std::max(0, height);
    stride_ = width_ * 4;
    pixels_.assign(static_cast<std::size_t>(stride_) * static_cast<std::size_t>(height_), 0);
}

void WalnutSoftwareRasterHost::clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (pixels_.empty()) return;
    simd::rgba_clear(pixels_.data(), width_, height_, stride_, r, g, b, a);
}

void WalnutSoftwareRasterHost::fill_rect(int x, int y, int w, int h,
                                         uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (pixels_.empty()) return;
    simd::rgba_fill_rect_opaque(pixels_.data(), width_, height_, stride_, x, y, w, h, r, g, b, a);
}

} // namespace xcm
