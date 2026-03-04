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
    src/exports.cpp
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
        "_xcm_alloc"
        "_xcm_free"
    )
    string(JOIN "," EXP_FUNCS_STR ${EXPORTED_FUNCTIONS})

    set_target_properties(render_core PROPERTIES
        OUTPUT_NAME "render_core"
        SUFFIX ".js"
    )

    target_compile_options(render_core PRIVATE
        -O3
        -fno-exceptions
    )

    target_link_options(render_core PRIVATE
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=33554432   # 32 MB
        -sSTACK_SIZE=2097152        # 2 MB
        -sEXPORTED_FUNCTIONS=${EXP_FUNCS_STR}
        "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,stringToUTF8,HEAPU8"
        -sENVIRONMENT=web,worker
        -sMODULARIZE=1
        -sEXPORT_NAME=XcmRenderCore
        -sSINGLE_FILE=0
        -O3
    )

else()
    message(STATUS "Building native shared library")

    add_library(render_core SHARED ${SOURCES})

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

    # Also build a CLI test tool.
    add_executable(xcm_render_test src/exports.cpp ${SOURCES})
    target_compile_definitions(xcm_render_test PRIVATE XCM_CLI_TEST=1)
    target_compile_options(xcm_render_test PRIVATE -O2 -g)
    target_include_directories(xcm_render_test PRIVATE src)
endif()

target_include_directories(render_core PRIVATE src)
