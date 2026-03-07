#!/usr/bin/env bash
# scripts/refresh-preview.sh
# Sends a refresh signal to the Crissy Style Tool browser window.
# The bridge-sync.js client polls this signal every 2 seconds and calls
# window.location.reload() automatically.
#
# Usage:
#   bash scripts/refresh-preview.sh
#   bash scripts/refresh-preview.sh --host http://127.0.0.1:7777
#
# The bridge toggle in the app header must be ON for the reload to fire.

set -euo pipefail

HOST="${1:-}"
if [[ "$HOST" == --host=* ]]; then
    HOST="${HOST#--host=}"
elif [[ "$HOST" == --host ]]; then
    HOST="${2:-http://127.0.0.1:7777}"
else
    HOST="http://127.0.0.1:7777"
fi

ENDPOINT="${HOST}/vscode-bridge/api/bridge.php?action=request_refresh"

echo "[refresh-preview] POST ${ENDPOINT}"

RESPONSE=$(curl -s -w "\n%{http_code}" -X POST \
    -H "Content-Type: application/json" \
    -d '{}' \
    "${ENDPOINT}" 2>&1) || {
    echo "[refresh-preview] ERROR: curl failed. Is the app running at ${HOST}?" >&2
    exit 1
}

HTTP_CODE=$(echo "${RESPONSE}" | tail -n1)
BODY=$(echo "${RESPONSE}" | head -n -1)

if [[ "${HTTP_CODE}" == "200" ]]; then
    echo "[refresh-preview] OK (HTTP 200): ${BODY}"
    echo "[refresh-preview] Browser will reload within 2 seconds if the bridge is ON."
else
    echo "[refresh-preview] ERROR: HTTP ${HTTP_CODE}" >&2
    echo "[refresh-preview] Response: ${BODY}" >&2
    exit 1
fi
