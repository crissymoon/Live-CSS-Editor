cmake_minimum_required(VERSION 3.24)

# macOS deployment target must be set BEFORE project() so CMake passes it
# to the compiler.  Ignored on other platforms.
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "macOS deployment target")

# Enable Objective-C/Objective-C++ only when targeting Apple -- the language
# IDs are not valid on other platforms and CMake will error if listed there.
if(APPLE)
    project(imgui_browser LANGUAGES C CXX OBJC OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
else()
    project(imgui_browser LANGUAGES C CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies (cloned by fetch_deps.sh into vendor/) ───────────────────
set(VENDOR "${CMAKE_SOURCE_DIR}/vendor")

# GLFW -- already has a CMakeLists
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory("${VENDOR}/glfw" glfw_build)

# ── Dear ImGui (compile as a static lib) ─────────────────────────────────
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

# ── cpp-httplib (header-only) ─────────────────────────────────────────────
# No build step needed.

# ── Source file lists ─────────────────────────────────────────────────────

# Shared: compiles on all platforms.
set(SOURCES_SHARED
    src/top-of-gui/chrome.cpp
    src/cmds-and-server/cmd_server.cpp
    src/fps_counter.cpp
)

# These main/ files are .mm for ObjC compatibility but contain pure C++ logic.
# On Apple they compile as ObjC++ automatically.  On Linux/Windows CMake is
# told to treat them as C++ via set_source_files_properties LANGUAGE below.
set(SOURCES_MAIN_SHARED
    src/main/main_args.mm
    src/main/main_layout.mm
    src/main/main_dispatch.mm
    src/main/main_callbacks.mm
    src/main/main_wvcbs.mm
    src/main/main_servers.mm
    src/main/main_render.mm
    src/main/main_globals.mm
)

# macOS: all existing Objective-C++ modules.
set(SOURCES_APPLE
    src/main.mm
    src/main/main_crash.mm
    src/main/main_menu.mm
    src/top-of-gui/native_chrome_stub.cpp
    src/webview/webview_state.mm
    src/webview/webview_delegates.mm
    src/webview/webview_popup.mm
    src/webview/webview_core.mm
    src/webview/webview_nav.mm
    src/webview/webview_cookies.mm
    src/xcm_shell.mm
    src/cmds-and-server/server_manager.cpp
    src/platform/platform_mac.mm
)

# Linux: WebKitGTK-backed webview + platform stubs.
set(SOURCES_LINUX
    src/main/main_entry_linux.cpp
    src/main/main_crash_cross.cpp
    src/webview/webview_linux.cpp
    src/top-of-gui/native_chrome_stub.cpp
    src/xcm_shell_stub.cpp
    src/cmds-and-server/server_manager.cpp
    src/platform/platform_linux.cpp
)

# Windows: WebView2-backed webview + platform stubs.
set(SOURCES_WIN
    src/main/main_entry_win.cpp
    src/main/main_crash_cross.cpp
    src/webview/webview_win.cpp
    src/cmds-and-server/server_manager_win.cpp
    src/top-of-gui/native_chrome_stub.cpp
    src/xcm_shell_stub.cpp
    src/platform/platform_win.cpp
)

if(APPLE)
    set(PLATFORM_SOURCES ${SOURCES_APPLE})
elseif(WIN32)
    set(PLATFORM_SOURCES ${SOURCES_WIN})
else()
    set(PLATFORM_SOURCES ${SOURCES_LINUX})
endif()

# ── Main executable ───────────────────────────────────────────────────────
add_executable(imgui_browser
    ${SOURCES_SHARED}
    ${SOURCES_MAIN_SHARED}
    ${PLATFORM_SOURCES}
)

# On non-Apple platforms the shared .mm files are treated as C++ translation
# units.  They contain no Objective-C syntax; the .mm extension is only there
# so Xcode/clang on macOS compiles them as ObjC++ for transitive ARC support.
if(NOT APPLE)
    set_source_files_properties(${SOURCES_MAIN_SHARED}
        PROPERTIES LANGUAGE CXX
    )
endif()

target_include_directories(imgui_browser PRIVATE
    src/
    src/main/
    src/cmds-and-server/
    src/top-of-gui/
    src/webview/
    src/platform/
    "${VENDOR}/imgui"
    "${VENDOR}/imgui/backends"
    "${VENDOR}/httplib"
    "${VENDOR}/glfw/include"
)

# ── Platform-specific compile options and libraries ───────────────────────

if(APPLE)
    target_compile_options(imgui_browser PRIVATE
        -Wall
        -Wno-deprecated-declarations
        -fobjc-arc
    )
    target_compile_definitions(imgui_browser PRIVATE
        GL_SILENCE_DEPRECATION
        XCM_PLATFORM_MACOS=1
    )

    find_library(COCOA_FW     Cocoa      REQUIRED)
    find_library(WEBKIT_FW    WebKit     REQUIRED)
    find_library(OPENGL_FW    OpenGL     REQUIRED)
    find_library(IOKIT_FW     IOKit      REQUIRED)
    find_library(COREVID_FW   CoreVideo  REQUIRED)
    find_library(QUARTZCORE   QuartzCore REQUIRED)
    find_library(NETWORK_FW   Network    REQUIRED)
    find_library(SYSCFG_FW    SystemConfiguration REQUIRED)
    find_library(CFNETWORK_FW CFNetwork  REQUIRED)
    find_library(SECURITY_FW  Security   REQUIRED)

    target_link_libraries(imgui_browser PRIVATE
        imgui glfw
        ${COCOA_FW} ${WEBKIT_FW} ${OPENGL_FW} ${IOKIT_FW}
        ${COREVID_FW} ${QUARTZCORE} ${NETWORK_FW} ${SYSCFG_FW}
        ${CFNETWORK_FW} ${SECURITY_FW}
    )

    # Build as a proper .app bundle so WKWebView gets a stable
    # CFBundleIdentifier for cookies and localStorage.
    set_target_properties(imgui_browser PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/src/Info.plist"
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "12.0"
    )

elseif(WIN32)
    target_compile_definitions(imgui_browser PRIVATE
        XCM_PLATFORM_WINDOWS=1
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    target_compile_options(imgui_browser PRIVATE /W3 /MP)

    # WebView2 SDK (Chromium Edge) -- supports either:
    # 1) vendor/webview2/include + vendor/webview2/lib/x64
    # 2) NuGet layout: Microsoft.Web.WebView2/build/native/{include,x64}
    set(WEBVIEW2_DIR "${VENDOR}/webview2" CACHE PATH "WebView2 SDK root")
    set(WEBVIEW2_INCLUDE_DIR "" CACHE PATH "Path containing WebView2.h")
    set(WEBVIEW2_LOADER_LIB "" CACHE FILEPATH "Path to WebView2LoaderStatic.lib")

    if(NOT WEBVIEW2_INCLUDE_DIR)
        if(EXISTS "${WEBVIEW2_DIR}/include/WebView2.h")
            set(WEBVIEW2_INCLUDE_DIR "${WEBVIEW2_DIR}/include")
        else()
            file(GLOB WEBVIEW2_NUGET_DIRS "${WEBVIEW2_DIR}/Microsoft.Web.WebView2*")
            foreach(_pkg IN LISTS WEBVIEW2_NUGET_DIRS)
                if(EXISTS "${_pkg}/build/native/include/WebView2.h")
                    set(WEBVIEW2_INCLUDE_DIR "${_pkg}/build/native/include")
                    break()
                endif()
            endforeach()
        endif()
    endif()

    if(NOT WEBVIEW2_LOADER_LIB)
        if(EXISTS "${WEBVIEW2_DIR}/lib/x64/WebView2LoaderStatic.lib")
            set(WEBVIEW2_LOADER_LIB "${WEBVIEW2_DIR}/lib/x64/WebView2LoaderStatic.lib")
        elseif(WEBVIEW2_INCLUDE_DIR)
            # If include path came from NuGet package, infer sibling x64 lib dir.
            get_filename_component(_wv2_native_dir "${WEBVIEW2_INCLUDE_DIR}" DIRECTORY)
            if(EXISTS "${_wv2_native_dir}/x64/WebView2LoaderStatic.lib")
                set(WEBVIEW2_LOADER_LIB "${_wv2_native_dir}/x64/WebView2LoaderStatic.lib")
            endif()
        endif()
    endif()

    if(WEBVIEW2_INCLUDE_DIR)
        target_include_directories(imgui_browser PRIVATE "${WEBVIEW2_INCLUDE_DIR}")
    endif()

    if(WEBVIEW2_INCLUDE_DIR AND WEBVIEW2_LOADER_LIB)
        target_link_libraries(imgui_browser PRIVATE
            imgui glfw
            "${WEBVIEW2_LOADER_LIB}"
            opengl32 gdi32 user32 shell32 ole32 oleaut32 shlwapi
        )
    else()
        message(WARNING
            "WebView2 SDK not fully resolved. include='${WEBVIEW2_INCLUDE_DIR}' lib='${WEBVIEW2_LOADER_LIB}'. "
            "Run: nuget install Microsoft.Web.WebView2 -OutputDirectory vendor/webview2"
        )
        target_link_libraries(imgui_browser PRIVATE
            imgui glfw
            opengl32 gdi32 user32 shell32 ole32 oleaut32 shlwapi
        )
    endif()

else()
    # Linux (X11 or Wayland -- GLFW handles the difference)
    target_compile_definitions(imgui_browser PRIVATE
        XCM_PLATFORM_LINUX=1
    )
    target_compile_options(imgui_browser PRIVATE -Wall -Wno-deprecated-declarations)

    # WebKitGTK 4.1 is the WebKit equivalent on Linux (same engine as WKWebView).
    # Install: sudo apt install libwebkit2gtk-4.1-dev
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)

    target_include_directories(imgui_browser PRIVATE ${WEBKIT2GTK_INCLUDE_DIRS})
    target_compile_options(imgui_browser    PRIVATE ${WEBKIT2GTK_CFLAGS_OTHER})
    target_link_libraries(imgui_browser PRIVATE
        imgui glfw
        ${WEBKIT2GTK_LIBRARIES}
        GL dl pthread
    )
endif()
