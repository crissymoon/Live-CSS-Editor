# docker/Dockerfile
# ─────────────────────────────────────────────────────────────────────────────
# Builds a self-contained image with:
#   - Emscripten SDK  (emcc, emconfigure, emmake)
#   - Build dependencies (autoconf, re2c, bison, etc.)
#
# Usage:
#   docker build -t php-wasm-builder -f docker/Dockerfile .
#   docker run --rm -v "$(pwd):/src" -w /src php-wasm-builder bash build.sh
#
# Or use: make docker-build  (which pulls the official emscripten/emsdk image)
# ─────────────────────────────────────────────────────────────────────────────

FROM emscripten/emsdk:3.1.56

LABEL maintainer="you@example.com"
LABEL description="PHP WASM build environment"

# ── System dependencies ────────────────────────────────────────────────────

RUN apt-get update && apt-get install -y --no-install-recommends \
        autoconf          \
        automake          \
        bison             \
        ca-certificates   \
        curl              \
        flex              \
        git               \
        libtool           \
        make              \
        patch             \
        pkg-config        \
        re2c              \
        unzip             \
        xz-utils          \
    && rm -rf /var/lib/apt/lists/*

# ── Verify Emscripten ──────────────────────────────────────────────────────

RUN emcc --version

# ── Working directory ──────────────────────────────────────────────────────

WORKDIR /src

# Default command: run the build script
CMD ["bash", "build.sh"]
