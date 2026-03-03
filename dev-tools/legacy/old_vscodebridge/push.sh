#!/usr/bin/env bash
# vscode-bridge/push.sh
# Push projects/html-editor.html + css-editor.css + js-editor.js into the DB.
# Usage: bash vscode-bridge/push.sh [project-name]
#        project-name defaults to "crissys-style-tool"

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
NAME="${1:-crissys-style-tool}"

HTML="$SCRIPT_DIR/projects/html-editor.html"
CSS="$SCRIPT_DIR/projects/css-editor.css"
JS="$SCRIPT_DIR/projects/js-editor.js"

for f in "$HTML" "$CSS"; do
    [[ -f "$f" ]] || { echo "[push] ERROR: missing $f"; exit 1; }
done

PAYLOAD=$(php -r '
    $data = [
        "name"   => $argv[1],
        "html"   => file_get_contents($argv[2]),
        "css"    => file_get_contents($argv[3]),
        "js"     => file_exists($argv[4]) ? file_get_contents($argv[4]) : "",
        "source" => "vscode-bridge"
    ];
    echo json_encode($data);
' "$NAME" "$HTML" "$CSS" "$JS")

RESULT=$(echo "$PAYLOAD" | php "$SCRIPT_DIR/api/projects-cli.php" save-inline "")
echo "$RESULT"
