# imgui-browser

Native C++17 browser using Dear ImGui chrome + WKWebView content.

**Why this exists:**
The Python/PyQt6/WKWebView stack has inherent flash and jank from the
PyObjC bridge layer and PyQt6 event loop competing with WebKit rendering.
This build eliminates those layers: ImGui renders the chrome (tabs, address
bar, status bar) directly over OpenGL, and WKWebView is embedded as a raw
NSView inside the same GLFW window at zero bridge cost.

## Architecture

```
GLFW window (NSWindow)
  |
  +-- OpenGL 3.2 context  -->  Dear ImGui (chrome only)
  |     Tab bar, toolbar, URL input, status bar, FPS display
  |
  +-- NSView subviews  -->  WKWebView per tab
        Full macOS WebKit, no Python/PyObjC overhead
        Position managed by C++ (below ImGui chrome)
```

Servers (spawned as child processes):
- PHP built-in server on 127.0.0.1:9879 -> `../dev-browser/apps/`
- Node.js image-cache server on 127.0.0.1:7779
- HTTP command API on 127.0.0.1:9878 (compatible with push.sh)

## Quick start

```bash
cd imgui-browser

# 1. Fetch dependencies (imgui, glfw, cpp-httplib)
bash fetch_deps.sh

# 2. Build
bash build.sh

# 3. Run
bash run.sh
```

## Command-line options

```
--url <url>          Initial URL (default: http://127.0.0.1:8080/pb_admin/login.php)
--apps-dir <path>    Path to dev-browser/apps/ (auto-detected)
--php-port <n>       PHP server port (default: 9879)
--cmd-port <n>       Command API port (default: 9878)
--width <n>          Window width (default: 1400)
--height <n>         Window height (default: 900)
```

## Command API

Same endpoint contract as the Python dev-browser so tooling does not change.

```
POST /navigate  {"url": "https://example.com"}
POST /newtab    {"url": "https://example.com"}
POST /eval      {"js": "document.title"}
GET  /status    returns JSON with fps_dev, fps_eng, fps_host, url, title
GET  /ping      returns "pong"
```

## File layout

```
imgui-browser/
  CMakeLists.txt
  fetch_deps.sh
  build.sh
  run.sh
  src/
    main.mm             NSApplication + GLFW window + render loop
    app_state.h         Shared C++ state (tabs, FPS, cmd queue)
    chrome.h/cpp        Dear ImGui toolbar, tab bar, status bar
    webview.h/.mm       WKWebView lifecycle + Obj-C++ bridge
    server_manager.h/cpp  PHP/Node child process management
    cmd_server.h/cpp    Lightweight HTTP command API (cpp-httplib)
    fps_counter.cpp     Host-side FPS ring buffer tick
  vendor/               (populated by fetch_deps.sh)
    imgui/
    glfw/
    httplib/
  build/                (created by build.sh)
    imgui_browser       binary
```

## Differences from the Python version

| Feature               | Python (dev-browser/)         | ImGui (imgui-browser/) |
|-----------------------|-------------------------------|------------------------|
| Chrome rendering      | QWidget + PyQt6               | Dear ImGui / OpenGL3   |
| WKWebView binding     | PyObjC bridge                 | Direct Obj-C++ (ARC)   |
| Event loop            | Qt / Python GIL               | GLFW + NSRunLoop       |
| JS injection          | QWebEngineScript / pyobjc     | WKUserScript (C++)     |
| Command server        | Python HTTP thread            | cpp-httplib C++ thread |
| FPS probe             | QTimer -> runJavaScript       | NSTimer -> evaluateJS  |
| Startup time          | ~2-3 s (Python import)        | ~100 ms (native)       |
