#pragma once

#include <cstdint>
#include <vector>

namespace xcm {

// Walnut-ready software RGBA frame host.
// Keeps raster state in plain C++ and can be bridged to Walnut image upload code.
class WalnutSoftwareRasterHost {
public:
    WalnutSoftwareRasterHost() = default;

    void resize(int width, int height);
    void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void fill_rect(int x, int y, int w, int h,
                   uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    uint8_t* data() { return pixels_.empty() ? nullptr : pixels_.data(); }
    const uint8_t* data() const { return pixels_.empty() ? nullptr : pixels_.data(); }

    int width() const { return width_; }
    int height() const { return height_; }
    int stride() const { return stride_; }

private:
    int width_ = 0;
    int height_ = 0;
    int stride_ = 0;
    std::vector<uint8_t> pixels_;
};

} // namespace xcm
