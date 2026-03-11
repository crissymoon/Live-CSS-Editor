# Render Core Native Backend Setup

This engine stays C/C++ first and keeps language/runtime overhead low.

## Added Native Paths

- SDL preview path: `xcm_sdl_preview`
- GLFW + OpenGL preview path: `xcm_glfw_preview`
- Backend hooks (compile-time): OpenGL, Vulkan, DirectX
- SIMD RGBA software raster path: SSE2 and AVX2 on x86 targets
- Simple HTTP fetcher path: Berkeley sockets HTTP/1.1 GET
- Optional Walnut include wiring for C++ editor-style integration

## CMake Options

- `XCM_ENABLE_SDL_PREVIEW=ON|OFF`
- `XCM_ENABLE_GLFW_PREVIEW=ON|OFF`
- `XCM_ENABLE_OPENGL_BACKEND=ON|OFF`
- `XCM_ENABLE_VULKAN_BACKEND=ON|OFF`
- `XCM_ENABLE_DIRECTX_BACKEND=ON|OFF`
- `XCM_ENABLE_WALNUT_BRIDGE=ON|OFF`
- `XCM_WALNUT_ROOT=/path/to/walnut`
- `XCM_ENABLE_SIMD_RASTER=ON|OFF`

## macOS Example

Install dependencies:

```bash
brew install sdl2 sdl2_ttf sdl2_image glfw
```

Configure and build:

```bash
cmake -S . -B build-livecss \
  -DXCM_ENABLE_SDL_PREVIEW=ON \
  -DXCM_ENABLE_GLFW_PREVIEW=ON \
  -DXCM_ENABLE_OPENGL_BACKEND=ON \
  -DXCM_ENABLE_VULKAN_BACKEND=ON \
  -DXCM_ENABLE_SIMD_RASTER=ON

cmake --build build-livecss --target xcm_sdl_preview -j4
cmake --build build-livecss --target xcm_glfw_preview -j4
cmake --build build-livecss --target xcm_http_fetch -j4
cmake --build build-livecss --target xcm_input_dump -j4
```

Run:

```bash
./build-livecss/xcm_sdl_preview path/to/page.html path/to/page.css path/to/page.js --watch
./build-livecss/xcm_glfw_preview path/to/page.html path/to/page.css path/to/page.js --watch --gpu=opengl
./build-livecss/xcm_http_fetch http://example.com/
./build-livecss/xcm_input_dump /tmp/xcm_sdl_input.bin
```

SDL preview browser behavior:

- By default, browser preview opens in an in-app SDL preview window (own window).
- Use `--external-browser` if you explicitly want to launch the system browser.

## Windows Notes

- DirectX hooks are enabled only on Windows builds (`XCM_HAS_DIRECTX`).
- Link and implementation of full D3D backend can be added in dedicated renderer files.

## Vulkan Notes

- If Vulkan SDK is detected, compile definition `XCM_HAS_VULKAN` is enabled.
- The current GLFW preview accepts `--gpu=vulkan` and reports fallback behavior.

## Walnut Notes

- Turn on `XCM_ENABLE_WALNUT_BRIDGE` and set `XCM_WALNUT_ROOT` to your Walnut checkout.
- This enables the Walnut-ready RGBA software host source (`xcm_walnut_software_raster.*`).

Example:

```bash
cmake -S . -B build-livecss \
  -DXCM_ENABLE_WALNUT_BRIDGE=ON \
  -DXCM_WALNUT_ROOT=/absolute/path/to/Walnut \
  -DXCM_ENABLE_SIMD_RASTER=ON
```

## SIMD Raster Notes

- The engine now routes `PaintEngine::clear` and opaque rectangle fills through SIMD-capable RGBA routines.
- The engine also routes semi-transparent rectangle fills through a SIMD alpha blend path.
- On x86 targets, SSE2 and AVX2 paths are compiled and selected at runtime by CPU capability checks.
- On non-x86 targets, the scalar RGBA path is used automatically.

## HTTP Fetcher Notes

- `xcm_http_fetch` is a simple HTTP client built on Berkeley sockets.
- Supports plain `http://` URLs with GET requests.
- Follows redirects (301, 302, 303, 307, 308) up to a small max depth.
- Decodes `Transfer-Encoding: chunked` bodies.
- HTTPS/TLS is intentionally not included in this low-level implementation.

## Low-Level Input Capture Notes

- `xcm_sdl_preview` now captures low-level user input events with hit regions.
- Captured event types include mouse move/down/up/wheel, key down/up, text input, resize, and quit.
- Hit regions are tagged as content, panel, or scrollbar.
- Use palette command `inputlog [path]` to dump events to a log file.
- Use palette command `inputbin [path]` to dump fixed-record binary events.
- Use palette command `inputclear` to clear the in-memory capture buffer.
- Use `xcm_input_dump` to decode binary captures back to readable lines.
