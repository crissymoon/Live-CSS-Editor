#!/usr/bin/env bash
# vscode-bridge/pull.sh
# Pull a project from the DB back into projects/ as html-editor.html etc.
# Usage: bash vscode-bridge/pull.sh [project-name]
#        project-name defaults to "crissys-style-tool"

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NAME="${1:-crissys-style-tool}"
OUT="$SCRIPT_DIR/projects"

RAW=$(php "$SCRIPT_DIR/api/projects-cli.php" get "$NAME" 2>/dev/null)

SUCCESS=$(echo "$RAW" | php -r 'echo json_decode(file_get_contents("php://stdin"), true)["success"] ? "1" : "0";' 2>/dev/null || echo "0")

if [[ "$SUCCESS" != "1" ]]; then
    echo "[pull] ERROR: project \"$NAME\" not found in DB"
    echo "$RAW"
    exit 1
fi

php -r '
    $row = json_decode(file_get_contents("php://stdin"), true)["project"];
    $out = $argv[1];
    file_put_contents("$out/html-editor.html", $row["html"] ?? "");
    file_put_contents("$out/css-editor.css",  $row["css"]  ?? "");
    file_put_contents("$out/js-editor.js",    $row["js"]   ?? "");
    echo "[pull] Wrote html-editor.html, css-editor.css, js-editor.js to $out\n";
    echo "[pull] Project: " . $row["name"] . " | updated: " . ($row["updated_at"] ?? "unknown") . "\n";
' "$OUT" <<< "$RAW"
