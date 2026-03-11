#pragma once

#include <string>

namespace xcm {

enum class GpuBackend {
    Software,
    OpenGL,
    Vulkan,
    DirectX
};

inline const char* backend_name(GpuBackend b) {
    switch (b) {
        case GpuBackend::Software: return "software";
        case GpuBackend::OpenGL: return "opengl";
        case GpuBackend::Vulkan: return "vulkan";
        case GpuBackend::DirectX: return "directx";
    }
    return "software";
}

GpuBackend parse_backend_name(const std::string& raw);

} // namespace xcm
