#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace xcm {

enum class InputKind : uint8_t {
    MouseMove,
    MouseDown,
    MouseUp,
    MouseWheel,
    KeyDown,
    KeyUp,
    TextInput,
    WindowResize,
    Quit
};

// 0=unknown, 1=content, 2=panel, 3=scrollbar
struct InputEvent {
    InputKind kind = InputKind::MouseMove;
    uint64_t timestamp_ms = 0;

    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = 0;

    int button = 0;
    int wheel_x = 0;
    int wheel_y = 0;
    int keycode = 0;
    uint16_t modifiers = 0;

    uint8_t hit_region = 0;
    std::string text;
};

class InputCapture {
public:
    explicit InputCapture(std::size_t max_events = 4096);

    void push(const InputEvent& ev);
    void clear();
    std::size_t size() const;
    std::vector<InputEvent> snapshot() const;

    bool dump_to_file(const std::string& path) const;
    bool dump_binary_to_file(const std::string& path) const;

private:
    std::size_t max_events_;
    std::vector<InputEvent> events_;
};

} // namespace xcm
