#include "xcm_input_capture.h"

#include <fstream>
#include <sstream>

namespace xcm {
namespace {

const char* kind_name(InputKind kind) {
    switch (kind) {
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

InputCapture::InputCapture(std::size_t max_events)
    : max_events_(max_events) {
    events_.reserve(max_events_);
}

void InputCapture::push(const InputEvent& ev) {
    if (max_events_ == 0) return;
    if (events_.size() >= max_events_) {
        events_.erase(events_.begin());
    }
    events_.push_back(ev);
}

void InputCapture::clear() {
    events_.clear();
}

std::size_t InputCapture::size() const {
    return events_.size();
}

std::vector<InputEvent> InputCapture::snapshot() const {
    return events_;
}

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

bool InputCapture::dump_to_file(const std::string& path) const {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;

    for (const auto& e : events_) {
        out << e.timestamp_ms
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

        if (!e.text.empty()) {
            out << " text=\"" << e.text << "\"";
        }
        out << "\n";
    }

    return static_cast<bool>(out);
}

bool InputCapture::dump_binary_to_file(const std::string& path) const {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;

    const uint32_t magic = 0x494E5054; // INPT
    const uint32_t version = 1;
    const uint64_t count = static_cast<uint64_t>(events_.size());
    out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    for (const auto& e : events_) {
        BinaryInputEvent b;
        b.kind = static_cast<uint8_t>(e.kind);
        b.hit_region = e.hit_region;
        b.modifiers = e.modifiers;
        b.text_len = static_cast<uint32_t>(e.text.size());
        b.timestamp_ms = e.timestamp_ms;
        b.x = e.x;
        b.y = e.y;
        b.dx = e.dx;
        b.dy = e.dy;
        b.button = e.button;
        b.wheel_x = e.wheel_x;
        b.wheel_y = e.wheel_y;
        b.keycode = e.keycode;

        out.write(reinterpret_cast<const char*>(&b), sizeof(b));
        if (!e.text.empty()) {
            out.write(e.text.data(), static_cast<std::streamsize>(e.text.size()));
        }
    }

    return static_cast<bool>(out);
}

} // namespace xcm
