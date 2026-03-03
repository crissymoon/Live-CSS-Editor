#!/usr/bin/env bash
# vscode-bridge/bridge.sh
#
# Push or pull a project through the same API the browser uses.
#
# Usage:
#   bash vscode-bridge/bridge.sh push [name]   -- save projects/ files to the tool
#   bash vscode-bridge/bridge.sh pull [name]   -- write DB project back to projects/
#
# Default project name: crissys-style-tool
# The tool must be running at http://127.0.0.1:8080

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECTS_DIR="$SCRIPT_DIR/projects"
BASE_URL="http://127.0.0.1:8080"
API="$BASE_URL/vscode-bridge/api/projects.php"

CMD="${1:-}"
NAME="${2:-crissys-style-tool}"

usage() {
    echo "Usage: bash vscode-bridge/bridge.sh push [name]"
    echo "       bash vscode-bridge/bridge.sh pull [name]"
    exit 1
}

[ -z "$CMD" ] && usage

# ── push ────────────────────────────────────────────────────────────────────
if [ "$CMD" = "push" ]; then
    HTML_FILE="$PROJECTS_DIR/html-editor.html"
    CSS_FILE="$PROJECTS_DIR/css-editor.css"
    JS_FILE="$PROJECTS_DIR/js-editor.js"

    for f in "$HTML_FILE" "$CSS_FILE"; do
        [ -f "$f" ] || { echo "[bridge] ERROR: missing $f"; exit 1; }
    done

    echo "[bridge] pushing \"$NAME\" to $API"

    PAYLOAD=$(php -r '
        echo json_encode([
            "name"   => $argv[1],
            "html"   => file_get_contents($argv[2]),
            "css"    => file_get_contents($argv[3]),
            "js"     => file_exists($argv[4]) ? file_get_contents($argv[4]) : "",
            "source" => "vscode-bridge",
        ]);
    ' "$NAME" "$HTML_FILE" "$CSS_FILE" "$JS_FILE")

    RESULT=$(curl -s -X POST \
        -H "Content-Type: application/json" \
        -d "$PAYLOAD" \
        "$API?action=save")

    echo "$RESULT" | php -r '
        $d = json_decode(file_get_contents("php://stdin"), true);
        if ($d["success"] ?? false) {
            echo "[bridge] saved \"" . $d["name"] . "\" -- updated: " . $d["updatedAt"] . "\n";
            echo "[bridge] open Load in the tool and select: " . $d["name"] . "\n";
        } else {
            echo "[bridge] ERROR: " . ($d["error"] ?? json_encode($d)) . "\n";
            exit(1);
        }
    '
fi

# ── pull ────────────────────────────────────────────────────────────────────
if [ "$CMD" = "pull" ]; then
    echo "[bridge] pulling \"$NAME\" from $API"

    RESULT=$(curl -s "$API?action=get&name=$(php -r 'echo urlencode($argv[1]);' "$NAME")")

    echo "$RESULT" | php -r '
        $d   = json_decode(file_get_contents("php://stdin"), true);
        $out = $argv[1];
        if (!($d["success"] ?? false)) {
            echo "[bridge] ERROR: " . ($d["error"] ?? json_encode($d)) . "\n";
            exit(1);
        }
        $p = $d["project"];
        file_put_contents("$out/html-editor.html", $p["html"] ?? "");
        file_put_contents("$out/css-editor.css",   $p["css"]  ?? "");
        file_put_contents("$out/js-editor.js",     $p["js"]   ?? "");
        echo "[bridge] wrote html-editor.html  (" . strlen($p["html"] ?? "") . " bytes)\n";
        echo "[bridge] wrote css-editor.css    (" . strlen($p["css"]  ?? "") . " bytes)\n";
        echo "[bridge] wrote js-editor.js      (" . strlen($p["js"]   ?? "") . " bytes)\n";
        echo "[bridge] project: " . $p["name"] . " | updated: " . ($p["updated_at"] ?? "unknown") . "\n";
    ' "$PROJECTS_DIR"
fi
