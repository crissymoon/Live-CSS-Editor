#!/usr/bin/env bash
# fetch_deps.sh -- clone/update vendor dependencies
# Run once before the first build.  Safe to re-run.
set -euo pipefail

VENDOR="$(cd "$(dirname "$0")" && pwd)/vendor"
mkdir -p "$VENDOR"

clone_or_update() {
    local name="$1" url="$2" tag="$3"
    local dest="$VENDOR/$name"
    if [[ -d "$dest/.git" ]]; then
        echo "[deps] $name exists, pulling..."
        git -C "$dest" fetch --tags --quiet
        git -C "$dest" checkout "$tag" --quiet 2>/dev/null || true
    else
        echo "[deps] cloning $name @ $tag ..."
        git clone --branch "$tag" --depth 1 "$url" "$dest"
    fi
}

# Dear ImGui v1.91.x -- docking branch has extra stability fixes
clone_or_update imgui   "https://github.com/ocornut/imgui.git"    "v1.91.5"

# GLFW 3.4
clone_or_update glfw    "https://github.com/glfw/glfw.git"         "3.4"

# cpp-httplib (header-only)
clone_or_update httplib "https://github.com/yhirose/cpp-httplib.git" "v0.18.1"

echo ""
echo "[deps] all dependencies ready in $VENDOR"
echo "[deps] run ./build.sh to compile"
