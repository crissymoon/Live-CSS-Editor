# imgui-browser

A C++17 native browser shell built with Dear ImGui chrome. Current platform:
macOS (WKWebView). Target: cross-platform packaged app with a per-platform
web engine backend.

## Vision

This is being built toward a cross-platform browser app packaging tool. The
goal is a single distributable binary per platform that wraps any web app
(PHP, static, WASM) into a native desktop app with a configurable chrome,
launch profiles, and a local server stack baked in. No Electron, no Python
runtime, no framework install required on the end-user machine.

Planned platform targets:

| Platform | Web engine | Status |
|----------|-----------|--------|
| macOS | WKWebView (WebKit) | Active |
| Windows | WebView2 (Chromium Edge) | Planned |
| Linux | WebKitGTK or CEF | Planned |

The PHP-WASM server (`php-wasm-project/`) is a first step toward baked-in
server capability -- run PHP apps with no PHP install on the host.

**Why Dear ImGui for chrome:**
The Python/PyQt6/WKWebView stack has inherent flash and jank from the
PyObjC bridge layer and PyQt6 event loop competing with WebKit rendering.
Dear ImGui renders the chrome (tabs, address bar, status bar) directly over
OpenGL at zero bridge cost. The web engine NSView is positionally managed
underneath the ImGui layer.

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

UI consistency note:
- The browser chrome is rendered by Dear ImGui on all platforms.
- Platform-specific native chrome paths are stubbed so tab/toolbar/status styling stays consistent across macOS, Windows, and Linux.
- GLFW window decorations are disabled so the app does not rely on OS title bars or grab bars.

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

Or use the cross-platform CLI (see below):

```bash
python xcm.py build
python xcm.py run explore
```

---

## xcm.py -- cross-platform CLI

`xcm.py` is a single-file Python 3 tool that handles building, launching, and
WASM-serving the app from named profiles.  No extra packages required -- only
the standard library.

### Profile dry runs

```bash
python xcm.py dry-run grab_bar      # see what would be launched
python xcm.py dry-run wasm          # inspect WASM args without starting
python xcm.py dry-run explore --url https://example.com
```

### Launch profiles

```bash
python xcm.py run grab_bar          # slim drag-strip only, no toolbar
python xcm.py run explore           # full toolbar, user can type any URL
python xcm.py run wasm              # full browser + PHP-WASM server (no PHP install)
python xcm.py run wasm_kiosk        # grab-bar chrome + WASM PHP
```

### Serve a PHP project without installing PHP

```bash
# Serve your project via PHP-WASM (requires Node.js, not PHP)
python xcm.py run wasm --project /path/to/your-php-project

# Or start the WASM server standalone and keep it running
python xcm.py serve-wasm --project /path/to/your-php-project --port 8082
python xcm.py stop-wasm
```

### Build

```bash
python xcm.py build           # configure + compile
python xcm.py build --clean   # clean build/ first
```

### List profiles

```bash
python xcm.py list
```

### Windows

Use `xcm.bat` instead of `python xcm.py` -- it locates Python 3 automatically:

```bat
xcm.bat list
xcm.bat build
xcm.bat run explore
xcm.bat dry-run wasm
```

Windows prerequisites:

- Visual Studio 2022 Build Tools (Desktop C++ workload)
- CMake on PATH
- Git on PATH (used to fetch `vendor/` dependencies)
- NuGet on PATH (recommended, for WebView2 SDK auto-install)

`xcm.py build` now auto-fetches missing native dependencies into `vendor/`.
If NuGet is available, it also installs the WebView2 SDK automatically.
If NuGet is not available, the project still builds with WebView stubs, but real browser rendering requires installing Microsoft WebView2 SDK.

WSL note:
- If WSL was just enabled, reboot Windows first.
- Then install a distro (example): `wsl --install -d Ubuntu`
- Build inside WSL using the Linux section below.

One-command Windows helper (build in WSL + open drop page):

```powershell
cd imgui-browser
powershell -ExecutionPolicy Bypass -File .\build-and-drop-wsl.ps1
```

Optional distro override:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-and-drop-wsl.ps1 -Distro kali-linux
```

### Linux / Raspberry Pi 5

Build directly on Linux (including Raspberry Pi OS) for best compatibility.

```bash
cd imgui-browser
bash build-linux.sh
```

What this script does:
- checks Linux prerequisites (`cmake`, `pkg-config`, `git`, `webkit2gtk-4.1`)
- fetches vendor deps if missing
- builds into `build-linux/`
- packages a transferable tarball in `dist/`

Output example:
- `dist/imgui-browser-linux-aarch64.tar.gz` (Pi 5 / ARM64)

You can quick-drop that tarball to your Pi or other Linux host, extract it, and run `./run.sh` from the extracted folder.

---

## settings.json -- launch profiles

`settings.json` defines named launch profiles.  Edit it to add your own or
override defaults.  All keys are optional; unset keys fall back to `defaults`.

```json
{
  "defaults": {
    "wasm_port": 8082,
    "php_port":  9879,
    "win_w":     1400,
    "win_h":     900
  },
  "profiles": {
    "grab_bar": {
      "ui_mode": "grab_bar_only",
      "show_toolbar": false,
      "wasm_enabled": false
    },
    "explore": {
      "ui_mode": "full",
      "startup_url": "about:blank"
    }
  }
}
```

Recognised `ui_mode` values: `full`, `no_tabs`, `grab_bar_only`.

---

## PHP-WASM server (no native PHP required)

The `php-wasm-project/` directory contains a WebAssembly build of PHP driven by
`server.js` (Node.js).  This lets you serve `.php` projects in the browser
without installing PHP on the host machine.

Requirements: **Node.js** (not PHP).

```bash
# Start the WASM server, sync a project into it, open in the app
python xcm.py run wasm --project ../   # serves the style tool root

# Or standalone
python xcm.py serve-wasm --project ../ --port 8082
```

The `--project <path>` flag copies the project into `php-wasm-project/public/`
before starting `node server.js`.  The server listens on `PORT` (default 8082).

---


## Command-line options

```
--url <url>          Initial URL (default: http://127.0.0.1:8080/pb_admin/login.php)
--ui-mode <mode>     UI chrome preset: full | no_tabs | grab_bar_only (overrides settings.json)
--apps-dir <path>    Path to dev-browser/apps/ (auto-detected)
--php-port <n>       PHP server port (default: 9879)
--cmd-port <n>       Command API port (default: 9878)
--width <n>          Window width (default: 1400)
--height <n>         Window height (default: 900)
--wasm               Enable PHP-WASM dev server on startup
--no-wasm            Disable PHP-WASM dev server
--wasm-port <n>      WASM dev server port (default: 8082)
--wasm-dir <path>    Path to php-wasm-project/ (auto-detected)
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
