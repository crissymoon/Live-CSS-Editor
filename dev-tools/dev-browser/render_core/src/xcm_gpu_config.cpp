#include "xcm_gpu_config.h"

#include <algorithm>
#include <cctype>

namespace xcm {

GpuBackend parse_backend_name(const std::string& raw) {
    std::string s = raw;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (s == "opengl" || s == "gl") return GpuBackend::OpenGL;
    if (s == "vulkan" || s == "vk") return GpuBackend::Vulkan;
    if (s == "directx" || s == "d3d" || s == "d3d11") return GpuBackend::DirectX;
    return GpuBackend::Software;
}

} // namespace xcm
