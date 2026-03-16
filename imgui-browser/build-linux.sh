#!/usr/bin/env bash
# build-linux.sh -- Linux/Raspberry Pi build + package helper for imgui-browser
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${ROOT}/build-linux"
DIST_DIR="${ROOT}/dist"

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "[linux-build] ERROR: this script must run on Linux (including Raspberry Pi OS)."
    exit 1
fi

ARCH="$(uname -m)"

need_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "[linux-build] ERROR: missing command: $1"
        return 1
    fi
    return 0
}

for cmd in cmake pkg-config git; do
    need_cmd "$cmd" || {
        echo "[linux-build] Install prerequisites first."
        echo "[linux-build] Debian/Ubuntu/Raspberry Pi OS example:"
        echo "  sudo apt update"
        echo "  sudo apt install -y build-essential cmake pkg-config git libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libwebkit2gtk-4.1-dev"
        exit 1
    }
done

if ! pkg-config --exists webkit2gtk-4.1; then
    echo "[linux-build] ERROR: webkit2gtk-4.1 not found."
    echo "[linux-build] Install package: libwebkit2gtk-4.1-dev"
    exit 1
fi

if [[ ! -d "${ROOT}/vendor/imgui" || ! -d "${ROOT}/vendor/glfw" || ! -d "${ROOT}/vendor/httplib" ]]; then
    echo "[linux-build] fetching vendor dependencies..."
    bash "${ROOT}/fetch_deps.sh"
fi

cmake -S "${ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

JOBS="$(nproc 2>/dev/null || echo 4)"
cmake --build "${BUILD_DIR}" --config Release -j "${JOBS}"

BIN="${BUILD_DIR}/imgui_browser"
if [[ ! -x "${BIN}" ]]; then
    echo "[linux-build] ERROR: expected binary not found at ${BIN}"
    exit 1
fi

mkdir -p "${DIST_DIR}"
PKG_DIR="${DIST_DIR}/imgui-browser-linux-${ARCH}"
PKG_FILE="${DIST_DIR}/imgui-browser-linux-${ARCH}.tar.gz"
rm -rf "${PKG_DIR}"
mkdir -p "${PKG_DIR}"

cp "${BIN}" "${PKG_DIR}/"
cp "${ROOT}/settings.json" "${PKG_DIR}/"
cp "${ROOT}/README.md" "${PKG_DIR}/"

cat > "${PKG_DIR}/run.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
exec "${DIR}/imgui_browser" --ui-mode full --no-wasm "$@"
EOF
chmod +x "${PKG_DIR}/run.sh"

tar -C "${DIST_DIR}" -czf "${PKG_FILE}" "imgui-browser-linux-${ARCH}"

echo "[linux-build] build complete: ${BIN}"
echo "[linux-build] package ready: ${PKG_FILE}"
echo "[linux-build] transfer that tar.gz to your target Linux machine and run ./run.sh inside the extracted folder"
