cmake_minimum_required(VERSION 3.24)
# Must be set BEFORE project() for CMake to pass it to the compiler.
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "macOS deployment target")
project(imgui_browser LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 17)

# ── Dependencies (cloned by fetch_deps.sh into vendor/) ────────────────
set(VENDOR "${CMAKE_SOURCE_DIR}/vendor")

# GLFW -- already has a CMakeLists
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory("${VENDOR}/glfw" glfw_build)

# ── Dear ImGui (compile as a static lib) ────────────────────────────
set(IMGUI_DIR "${VENDOR}/imgui")
add_library(imgui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)
target_include_directories(imgui PUBLIC
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)
target_link_libraries(imgui PUBLIC glfw)
target_compile_definitions(imgui PUBLIC
    GL_SILENCE_DEPRECATION
)

# ── cpp-httplib (header-only, included inline) ──────────────────────
# No build step needed.

# ── Main executable ───────────────────────────────────────────────────
add_executable(imgui_browser
    src/main.mm
    src/chrome.cpp
    src/native_chrome.mm
    src/webview.mm
    src/server_manager.cpp
    src/cmd_server.cpp
    src/cf_client.cpp
    src/fps_counter.cpp
    src/xcm_shell.mm
)

target_include_directories(imgui_browser PRIVATE
    src/
    "${VENDOR}/imgui"
    "${VENDOR}/imgui/backends"
    "${VENDOR}/httplib"
    "${VENDOR}/glfw/include"
)

target_compile_options(imgui_browser PRIVATE
    -Wall
    -Wno-deprecated-declarations   # WebKit/OpenGL deprecations on macOS
    -fobjc-arc                     # Automatic Reference Counting for .mm
)

target_compile_definitions(imgui_browser PRIVATE
    GL_SILENCE_DEPRECATION
)

# macOS frameworks
find_library(COCOA_FW    Cocoa)
find_library(WEBKIT_FW   WebKit)
find_library(OPENGL_FW   OpenGL)
find_library(IOKIT_FW    IOKit)
find_library(COREVID_FW  CoreVideo)
find_library(QUARTZCORE  QuartzCore)
find_library(NETWORK_FW  Network)
find_library(SYSCFG_FW   SystemConfiguration)
find_library(CFNETWORK_FW CFNetwork)
find_library(SECURITY_FW  Security)

# libcurl -- ships with macOS; used by cf_client.cpp to call cf_bridge
find_library(CURL_FW  curl)

target_link_libraries(imgui_browser PRIVATE
    imgui
    glfw
    ${COCOA_FW}
    ${WEBKIT_FW}
    ${OPENGL_FW}
    ${IOKIT_FW}
    ${COREVID_FW}
    ${QUARTZCORE}
    ${NETWORK_FW}
    ${SYSCFG_FW}
    ${CFNETWORK_FW}
    ${SECURITY_FW}
    ${CURL_FW}
)

# Set macOS deployment target and bundle properties.
# Building as a proper .app gives WKWebView a stable CFBundleIdentifier so
# cookies, localStorage and session data persist correctly across pages.
set_target_properties(imgui_browser PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/src/Info.plist"
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "12.0"
)
