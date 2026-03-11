cmake_minimum_required(VERSION 3.20)
project(xcm_render_core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/html_tokenizer.cpp
    src/css_parser.cpp
    src/style_resolver.cpp
    src/layout.cpp
    src/paint.cpp
    src/xcm_rgba_raster.cpp
    src/xcm_input_capture.cpp
    src/exports.cpp
    src/xcm_tui.cpp
)

# TUI-only sources (no exports.cpp -- standalone binary only)
set(TUI_SOURCES
    src/html_tokenizer.cpp
    src/css_parser.cpp
    src/style_resolver.cpp
    src/layout.cpp
    src/paint.cpp
    src/xcm_rgba_raster.cpp
    src/xcm_input_capture.cpp
    src/xcm_tui.cpp
    src/xcm_tuiapp.cpp
)

set(TUI_SHELL_SOURCES
    src/html_tokenizer.cpp
    src/css_parser.cpp
    src/style_resolver.cpp
    src/layout.cpp
    src/paint.cpp
    src/xcm_rgba_raster.cpp
    src/xcm_input_capture.cpp
    src/xcm_tui.cpp
    src/xcm_tui_shell.cpp
)

set(SDL_PREVIEW_SOURCES
    src/html_tokenizer.cpp
    src/css_parser.cpp
    src/style_resolver.cpp
    src/layout.cpp
    src/paint.cpp
    src/xcm_rgba_raster.cpp
    src/xcm_input_capture.cpp
    src/xcm_http_fetch.cpp
    src/xcm_sdl_preview.cpp
)

set(GLFW_PREVIEW_SOURCES
    src/html_tokenizer.cpp
    src/css_parser.cpp
    src/style_resolver.cpp
    src/layout.cpp
    src/paint.cpp
    src/xcm_rgba_raster.cpp
    src/xcm_input_capture.cpp
    src/xcm_gpu_config.cpp
    src/xcm_glfw_preview.cpp
)

# -------------------------------------------------------------------------
# Detect if we are building for WASM via Emscripten
# -------------------------------------------------------------------------
if(EMSCRIPTEN)
    message(STATUS "Building WASM target with Emscripten")

    add_executable(render_core ${SOURCES})

    set(EXPORTED_FUNCTIONS
        "_xcm_create"
        "_xcm_destroy"
        "_xcm_render"
        "_xcm_pixels"
        "_xcm_width"
        "_xcm_height"
        "_xcm_doc_height"
        "_xcm_metrics_json"
        "_xcm_ansi_frame"
        "_xcm_render_ansi"
        "_xcm_alloc"
        "_xcm_free"
        "_xcm_tui_create"
        "_xcm_tui_destroy"
        "_xcm_tui_render"
        "_xcm_tui_frame"
    )
    string(JOIN "," EXP_FUNCS_STR ${EXPORTED_FUNCTIONS})

    set_target_properties(render_core PROPERTIES
        OUTPUT_NAME "render_core"
        SUFFIX ".js"
    )

    # -----------------------------------------------------------------------
    # WASM SIMD128 (-msimd128)
    #   Unlocks 128-bit vector operations in the WASM binary.  The Emscripten
    #   back-end emits SIMD load/store for std::fill and tight pixel loops
    #   automatically when this flag is present.  Supported by Chrome 91+,
    #   Firefox 89+, Safari 16.4+, Node 16.4+.
    #
    # Memory64 (-sMEMORY64=1) -- NOT enabled.
    #   Memory64 extends the heap address space to 64 bits (>4 GB).  It does
    #   not improve throughput or reduce per-pixel cost; it only matters when
    #   a single allocation exceeds ~3.5 GB.  Render buffers here are at most
    #   a few MB.  Additionally Memory64 is behind a flag in Safari as of 2026
    #   and changes the type of every heap pointer in the generated JS glue,
    #   which breaks the cwrap/ccall bridge used by wasm-renderer.js.  Leave
    #   it off until all target browsers ship with unconditional support.
    # -----------------------------------------------------------------------
    target_compile_options(render_core PRIVATE
        -O3
        -fno-exceptions
        -msimd128
    )

    target_link_options(render_core PRIVATE
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=16777216   # 16 MB
        -sSTACK_SIZE=2097152        # 2 MB
        -sEXPORTED_FUNCTIONS=${EXP_FUNCS_STR}
        "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,stringToUTF8,HEAPU8"
        -sENVIRONMENT=web,worker
        -sMODULARIZE=1
        -sEXPORT_NAME=XcmRenderCore
        -sSINGLE_FILE=0
        -O3
        -msimd128
    )

else()
    message(STATUS "Building native shared library")

    option(XCM_ENABLE_OPENGL_BACKEND "Enable OpenGL backend support" ON)
    option(XCM_ENABLE_VULKAN_BACKEND "Enable Vulkan backend support (detection/hooks)" ON)
    option(XCM_ENABLE_DIRECTX_BACKEND "Enable DirectX backend support (Windows hooks)" ON)
    option(XCM_ENABLE_WALNUT_BRIDGE "Enable Walnut integration hooks" OFF)
    option(XCM_ENABLE_GLFW_PREVIEW "Build GLFW/OpenGL preview app" ON)
    option(XCM_ENABLE_SIMD_RASTER "Enable SIMD accelerated RGBA software rasterizer" ON)

    add_library(render_core SHARED ${SOURCES})
    target_sources(render_core PRIVATE src/xcm_http_fetch.cpp)

    if(XCM_ENABLE_SIMD_RASTER)
        target_compile_definitions(render_core PRIVATE XCM_ENABLE_SIMD_RASTER=1)
    endif()

    target_compile_options(render_core PRIVATE
        -O3
        -fvisibility=hidden
    )

    # For macOS dylib
    if(APPLE)
        set_target_properties(render_core PROPERTIES
            MACOSX_RPATH ON
        )
    endif()

    # CLI render smoke-test binary.
    add_executable(xcm_render_test src/exports.cpp ${SOURCES})
    target_compile_definitions(xcm_render_test PRIVATE XCM_CLI_TEST=1)
    target_compile_options(xcm_render_test PRIVATE -O2 -g)
    target_include_directories(xcm_render_test PRIVATE src)
    if(XCM_ENABLE_SIMD_RASTER)
        target_compile_definitions(xcm_render_test PRIVATE XCM_ENABLE_SIMD_RASTER=1)
    endif()

    # xcm_tui  --  standalone TUI browser binary
    add_executable(xcm_tui ${TUI_SOURCES})
    target_compile_options(xcm_tui PRIVATE -O2 -g)
    target_include_directories(xcm_tui PRIVATE src)
    if(XCM_ENABLE_SIMD_RASTER)
        target_compile_definitions(xcm_tui PRIVATE XCM_ENABLE_SIMD_RASTER=1)
    endif()

    # xcm_http_fetch  --  simple Berkeley-socket HTTP GET client
    add_executable(xcm_http_fetch src/xcm_http_fetch.cpp src/xcm_http_fetch_cli.cpp)
    target_compile_options(xcm_http_fetch PRIVATE -O2 -g)
    target_include_directories(xcm_http_fetch PRIVATE src)
    if(WIN32)
        target_link_libraries(xcm_http_fetch PRIVATE ws2_32)
    endif()

    # xcm_input_dump  --  decode binary input capture logs
    add_executable(xcm_input_dump src/xcm_input_dump_cli.cpp)
    target_compile_options(xcm_input_dump PRIVATE -O2 -g)
    target_include_directories(xcm_input_dump PRIVATE src)

    # xcm_tui_shell  --  interactive REPL shell for TUI rendering tests
    add_executable(xcm_tui_shell ${TUI_SHELL_SOURCES})
    target_compile_options(xcm_tui_shell PRIVATE -O2 -g)
    target_include_directories(xcm_tui_shell PRIVATE src)
    if(XCM_ENABLE_SIMD_RASTER)
        target_compile_definitions(xcm_tui_shell PRIVATE XCM_ENABLE_SIMD_RASTER=1)
    endif()

    # xcm_sdl_preview  --  optional SDL desktop preview app
    option(XCM_ENABLE_SDL_PREVIEW "Build SDL2 preview app" ON)
    if(XCM_ENABLE_SDL_PREVIEW)
        set(XCM_SDL_READY OFF)

        find_package(SDL2 QUIET CONFIG)
        if(SDL2_FOUND)
            set(XCM_SDL_READY ON)
            add_executable(xcm_sdl_preview ${SDL_PREVIEW_SOURCES})
            target_compile_options(xcm_sdl_preview PRIVATE -O2 -g)
            target_include_directories(xcm_sdl_preview PRIVATE src)
            if(XCM_ENABLE_SIMD_RASTER)
                target_compile_definitions(xcm_sdl_preview PRIVATE XCM_ENABLE_SIMD_RASTER=1)
            endif()
            if(TARGET SDL2::SDL2)
                target_link_libraries(xcm_sdl_preview PRIVATE SDL2::SDL2)
            elseif(TARGET SDL2::SDL2-static)
                target_link_libraries(xcm_sdl_preview PRIVATE SDL2::SDL2-static)
            endif()

            # Optional text rendering for command UI labels.
            find_package(SDL2_ttf QUIET CONFIG)
            if(SDL2_ttf_FOUND)
                if(TARGET SDL2_ttf::SDL2_ttf)
                    target_link_libraries(xcm_sdl_preview PRIVATE SDL2_ttf::SDL2_ttf)
                    target_compile_definitions(xcm_sdl_preview PRIVATE XCM_HAS_SDL_TTF=1)
                endif()
            endif()

            # Optional image decoding for <img> and background-image overlays.
            find_package(SDL2_image QUIET CONFIG)
            if(SDL2_image_FOUND)
                if(TARGET SDL2_image::SDL2_image)
                    target_link_libraries(xcm_sdl_preview PRIVATE SDL2_image::SDL2_image)
                    target_compile_definitions(xcm_sdl_preview PRIVATE XCM_HAS_SDL_IMAGE=1)
                endif()
            else()
                find_package(PkgConfig QUIET)
                if(PkgConfig_FOUND)
                    pkg_check_modules(SDL2_IMAGE_PC QUIET SDL2_image)
                    if(SDL2_IMAGE_PC_FOUND)
                        target_include_directories(xcm_sdl_preview PRIVATE ${SDL2_IMAGE_PC_INCLUDE_DIRS})
                        target_link_directories(xcm_sdl_preview PRIVATE ${SDL2_IMAGE_PC_LIBRARY_DIRS})
                        target_link_libraries(xcm_sdl_preview PRIVATE ${SDL2_IMAGE_PC_LIBRARIES})
                        target_compile_definitions(xcm_sdl_preview PRIVATE XCM_HAS_SDL_IMAGE=1)
                    endif()
                endif()
            endif()
        else()
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(SDL2_PC QUIET sdl2)
                if(SDL2_PC_FOUND)
                    set(XCM_SDL_READY ON)
                    add_executable(xcm_sdl_preview ${SDL_PREVIEW_SOURCES})
                    target_compile_options(xcm_sdl_preview PRIVATE -O2 -g)
                    target_include_directories(xcm_sdl_preview PRIVATE src ${SDL2_PC_INCLUDE_DIRS})
                    target_link_directories(xcm_sdl_preview PRIVATE ${SDL2_PC_LIBRARY_DIRS})
                    target_link_libraries(xcm_sdl_preview PRIVATE ${SDL2_PC_LIBRARIES})
                    if(XCM_ENABLE_SIMD_RASTER)
                        target_compile_definitions(xcm_sdl_preview PRIVATE XCM_ENABLE_SIMD_RASTER=1)
                    endif()

                    pkg_check_modules(SDL2_TTF_PC QUIET SDL2_ttf)
                    if(SDL2_TTF_PC_FOUND)
                        target_include_directories(xcm_sdl_preview PRIVATE ${SDL2_TTF_PC_INCLUDE_DIRS})
                        target_link_directories(xcm_sdl_preview PRIVATE ${SDL2_TTF_PC_LIBRARY_DIRS})
                        target_link_libraries(xcm_sdl_preview PRIVATE ${SDL2_TTF_PC_LIBRARIES})
                        target_compile_definitions(xcm_sdl_preview PRIVATE XCM_HAS_SDL_TTF=1)
                    endif()

                    pkg_check_modules(SDL2_IMAGE_PC QUIET SDL2_image)
                    if(SDL2_IMAGE_PC_FOUND)
                        target_include_directories(xcm_sdl_preview PRIVATE ${SDL2_IMAGE_PC_INCLUDE_DIRS})
                        target_link_directories(xcm_sdl_preview PRIVATE ${SDL2_IMAGE_PC_LIBRARY_DIRS})
                        target_link_libraries(xcm_sdl_preview PRIVATE ${SDL2_IMAGE_PC_LIBRARIES})
                        target_compile_definitions(xcm_sdl_preview PRIVATE XCM_HAS_SDL_IMAGE=1)
                    endif()
                endif()
            endif()
        endif()

        if(NOT XCM_SDL_READY)
            message(WARNING "SDL2 not found. xcm_sdl_preview target is skipped. Install SDL2 and reconfigure.")
        endif()
    endif()

    # xcm_glfw_preview  --  optional GLFW + OpenGL desktop preview app
    if(XCM_ENABLE_GLFW_PREVIEW)
        set(XCM_GLFW_READY OFF)
        set(XCM_OPENGL_READY OFF)

        if(XCM_ENABLE_OPENGL_BACKEND)
            find_package(OpenGL QUIET)
            if(OpenGL_FOUND)
                set(XCM_OPENGL_READY ON)
            endif()
        endif()

        if(XCM_OPENGL_READY)
            find_package(glfw3 QUIET CONFIG)
            if(glfw3_FOUND OR TARGET glfw)
                set(XCM_GLFW_READY ON)
                add_executable(xcm_glfw_preview ${GLFW_PREVIEW_SOURCES})
                target_compile_options(xcm_glfw_preview PRIVATE -O2 -g)
                target_include_directories(xcm_glfw_preview PRIVATE src)
                if(XCM_ENABLE_SIMD_RASTER)
                    target_compile_definitions(xcm_glfw_preview PRIVATE XCM_ENABLE_SIMD_RASTER=1)
                endif()
                if(TARGET glfw)
                    target_link_libraries(xcm_glfw_preview PRIVATE glfw OpenGL::GL)
                elseif(TARGET glfw3::glfw)
                    target_link_libraries(xcm_glfw_preview PRIVATE glfw3::glfw OpenGL::GL)
                endif()
                target_compile_definitions(xcm_glfw_preview PRIVATE XCM_HAS_OPENGL=1)
            else()
                find_package(PkgConfig QUIET)
                if(PkgConfig_FOUND)
                    pkg_check_modules(GLFW_PC QUIET glfw3)
                    if(GLFW_PC_FOUND)
                        set(XCM_GLFW_READY ON)
                        add_executable(xcm_glfw_preview ${GLFW_PREVIEW_SOURCES})
                        target_compile_options(xcm_glfw_preview PRIVATE -O2 -g)
                        target_include_directories(xcm_glfw_preview PRIVATE src ${GLFW_PC_INCLUDE_DIRS})
                        target_link_directories(xcm_glfw_preview PRIVATE ${GLFW_PC_LIBRARY_DIRS})
                        target_link_libraries(xcm_glfw_preview PRIVATE ${GLFW_PC_LIBRARIES} OpenGL::GL)
                        if(XCM_ENABLE_SIMD_RASTER)
                            target_compile_definitions(xcm_glfw_preview PRIVATE XCM_ENABLE_SIMD_RASTER=1)
                        endif()
                        target_compile_definitions(xcm_glfw_preview PRIVATE XCM_HAS_OPENGL=1)
                    endif()
                endif()
            endif()
        endif()

        if(NOT XCM_OPENGL_READY)
            message(WARNING "OpenGL not found. xcm_glfw_preview target is skipped.")
        elseif(NOT XCM_GLFW_READY)
            message(WARNING "GLFW not found. xcm_glfw_preview target is skipped. Install glfw3 and reconfigure.")
        endif()

        if(NOT TARGET xcm_glfw_preview)
            add_custom_target(xcm_glfw_preview
                COMMAND ${CMAKE_COMMAND} -E echo "xcm_glfw_preview is disabled: install OpenGL + GLFW (glfw3) and reconfigure."
                VERBATIM
            )
        endif()
    endif()

    if(XCM_ENABLE_VULKAN_BACKEND)
        find_package(Vulkan QUIET)
        if(Vulkan_FOUND)
            target_compile_definitions(render_core PRIVATE XCM_HAS_VULKAN=1)
            target_compile_definitions(xcm_render_test PRIVATE XCM_HAS_VULKAN=1)
            if(TARGET xcm_glfw_preview)
                target_compile_definitions(xcm_glfw_preview PRIVATE XCM_HAS_VULKAN=1)
            endif()
            message(STATUS "Vulkan SDK detected; Vulkan backend hooks enabled.")
        else()
            message(STATUS "Vulkan SDK not found; Vulkan backend hooks disabled.")
        endif()
    endif()

    if(XCM_ENABLE_DIRECTX_BACKEND)
        if(WIN32)
            target_compile_definitions(render_core PRIVATE XCM_HAS_DIRECTX=1)
            target_compile_definitions(xcm_render_test PRIVATE XCM_HAS_DIRECTX=1)
            if(TARGET xcm_glfw_preview)
                target_compile_definitions(xcm_glfw_preview PRIVATE XCM_HAS_DIRECTX=1)
            endif()
            message(STATUS "DirectX hooks enabled for Windows builds.")
        else()
            message(STATUS "DirectX hooks requested but host is not Windows; leaving hooks disabled.")
        endif()
    endif()

    if(XCM_ENABLE_WALNUT_BRIDGE)
        set(XCM_WALNUT_ROOT "" CACHE PATH "Path to Walnut root for optional include wiring")
        if(EXISTS "${XCM_WALNUT_ROOT}/Walnut")
            target_sources(render_core PRIVATE src/xcm_walnut_software_raster.cpp)
            target_include_directories(render_core PRIVATE ${XCM_WALNUT_ROOT})
            target_compile_definitions(render_core PRIVATE XCM_HAS_WALNUT=1)
            if(TARGET xcm_glfw_preview)
                target_sources(xcm_glfw_preview PRIVATE src/xcm_walnut_software_raster.cpp)
                target_include_directories(xcm_glfw_preview PRIVATE ${XCM_WALNUT_ROOT})
                target_compile_definitions(xcm_glfw_preview PRIVATE XCM_HAS_WALNUT=1)
            endif()
            message(STATUS "Walnut include hooks enabled from: ${XCM_WALNUT_ROOT}")
        else()
            message(WARNING "XCM_ENABLE_WALNUT_BRIDGE is ON but XCM_WALNUT_ROOT is not valid; Walnut hooks were skipped.")
        endif()
    endif()
endif()

target_include_directories(render_core PRIVATE src)
