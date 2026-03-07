#!/usr/bin/env bash
# scripts/apply-patches.sh
# ─────────────────────────────────────────────────────────────────────────────
# Apply any patches in patches/ to the PHP source tree.
# Patches must be in unified diff format (.patch files).
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

PATCHES_DIR="$(dirname "$0")/../patches"
PHP_SRC="$(dirname "$0")/../php-src"

if [[ ! -d "${PHP_SRC}" ]]; then
    echo "✗ php-src/ not found. Run scripts/fetch-php.sh first."
    exit 1
fi

shopt -s nullglob
PATCH_FILES=("${PATCHES_DIR}"/*.patch)

if [[ ${#PATCH_FILES[@]} -eq 0 ]]; then
    echo "▸ No patches to apply."
    exit 0
fi

for patch in "${PATCH_FILES[@]}"; do
    name="$(basename "${patch}")"
    echo "▸ Applying ${name}…"
    patch -d "${PHP_SRC}" -p1 --forward < "${patch}" && echo "  ✓ ${name}" \
        || echo "  ⚠  ${name} — already applied or skipped"
done

echo "✓ All patches processed."
