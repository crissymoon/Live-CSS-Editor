#include "xcm_input_capture.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace {

struct BinaryInputEvent {
    uint8_t kind = 0;
    uint8_t hit_region = 0;
    uint16_t modifiers = 0;
    uint32_t text_len = 0;
    uint64_t timestamp_ms = 0;
    int32_t x = 0;
    int32_t y = 0;
    int32_t dx = 0;
    int32_t dy = 0;
    int32_t button = 0;
    int32_t wheel_x = 0;
    int32_t wheel_y = 0;
    int32_t keycode = 0;
};

const char* kind_name(uint8_t kind) {
    using xcm::InputKind;
    switch (static_cast<InputKind>(kind)) {
        case InputKind::MouseMove: return "mouse_move";
        case InputKind::MouseDown: return "mouse_down";
        case InputKind::MouseUp: return "mouse_up";
        case InputKind::MouseWheel: return "mouse_wheel";
        case InputKind::KeyDown: return "key_down";
        case InputKind::KeyUp: return "key_up";
        case InputKind::TextInput: return "text_input";
        case InputKind::WindowResize: return "window_resize";
        case InputKind::Quit: return "quit";
    }
    return "unknown";
}

const char* hit_name(uint8_t hit) {
    switch (hit) {
        case 1: return "content";
        case 2: return "panel";
        case 3: return "scrollbar";
        default: return "unknown";
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: xcm_input_dump <input.bin>\n";
        return 2;
    }

    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open: " << argv[1] << "\n";
        return 1;
    }

    uint32_t magic = 0;
    uint32_t version = 0;
    uint64_t count = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    in.read(reinterpret_cast<char*>(&count), sizeof(count));

    if (!in || magic != 0x494E5054 || version != 1) {
        std::cerr << "Invalid input capture binary format\n";
        return 1;
    }

    for (uint64_t i = 0; i < count; ++i) {
        BinaryInputEvent e;
        in.read(reinterpret_cast<char*>(&e), sizeof(e));
        if (!in) {
            std::cerr << "Corrupt record at event " << i << "\n";
            return 1;
        }

        std::string text;
        if (e.text_len > 0) {
            text.resize(e.text_len);
            in.read(&text[0], static_cast<std::streamsize>(e.text_len));
            if (!in) {
                std::cerr << "Corrupt text payload at event " << i << "\n";
                return 1;
            }
        }

        std::cout << e.timestamp_ms
                  << " kind=" << kind_name(e.kind)
                  << " hit=" << hit_name(e.hit_region)
                  << " x=" << e.x
                  << " y=" << e.y
                  << " dx=" << e.dx
                  << " dy=" << e.dy
                  << " button=" << e.button
                  << " wheel_x=" << e.wheel_x
                  << " wheel_y=" << e.wheel_y
                  << " key=" << e.keycode
                  << " mods=" << e.modifiers;
        if (!text.empty()) {
            std::cout << " text=\"" << text << "\"";
        }
        std::cout << "\n";
    }

    return 0;
}
