#!/bin/bash
# push-project.sh
# Reads html-editor.html, css-editor.css, js-editor.js from the projects/ folder
# and saves them as a project into the SQLite database via projects-cli.php.
#
# Usage:
#   ./push-project.sh <project-name>
#   ./push-project.sh                   (defaults to "custom-design")
#
# The user then loads the project from the Live CSS tool's Load dialog.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECTS_DIR="$SCRIPT_DIR/projects"
CLI="$SCRIPT_DIR/api/projects-cli.php"

# Project name from arg or default
PROJECT_NAME="${1:-custom-design}"

# Verify files exist
HTML_FILE="$PROJECTS_DIR/html-editor.html"
CSS_FILE="$PROJECTS_DIR/css-editor.css"
JS_FILE="$PROJECTS_DIR/js-editor.js"

for f in "$HTML_FILE" "$CSS_FILE" "$JS_FILE"; do
  if [ ! -f "$f" ]; then
    echo "[push-project] ERROR: Missing file: $f" >&2
    exit 1
  fi
done

# Read file contents
HTML_CONTENT=$(cat "$HTML_FILE")
CSS_CONTENT=$(cat "$CSS_FILE")
JS_CONTENT=$(cat "$JS_FILE")

# Build JSON payload using php for safe encoding (handles quotes, newlines, etc.)
JSON_PAYLOAD=$(php -r '
  $data = [
    "name"   => $argv[1],
    "html"   => file_get_contents($argv[2]),
    "css"    => file_get_contents($argv[3]),
    "js"     => file_get_contents($argv[4]),
    "source" => "vscode-bridge"
  ];
  echo json_encode($data);
' "$PROJECT_NAME" "$HTML_FILE" "$CSS_FILE" "$JS_FILE")

if [ -z "$JSON_PAYLOAD" ]; then
  echo "[push-project] ERROR: Failed to build JSON payload" >&2
  exit 1
fi

echo "[push-project] Saving project: $PROJECT_NAME"
echo "[push-project] HTML: $(wc -c < "$HTML_FILE" | tr -d ' ') bytes"
echo "[push-project] CSS:  $(wc -c < "$CSS_FILE" | tr -d ' ') bytes"
echo "[push-project] JS:   $(wc -c < "$JS_FILE" | tr -d ' ') bytes"

# Push to database via CLI (pass empty arg so CLI reads JSON from stdin)
RESULT=$(echo "$JSON_PAYLOAD" | php "$CLI" save-inline "")
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
  echo "[push-project] ERROR: CLI returned exit code $EXIT_CODE" >&2
  echo "$RESULT" >&2
  exit 1
fi

# Check success field in response
SUCCESS=$(echo "$RESULT" | php -r 'echo json_decode(file_get_contents("php://stdin"), true)["success"] ? "true" : "false";' 2>/dev/null || echo "unknown")

if [ "$SUCCESS" = "true" ]; then
  echo "[push-project] OK - Project saved to database"
  echo "[push-project] Load it in Live CSS tool using: $PROJECT_NAME"
  echo "$RESULT"
else
  echo "[push-project] ERROR: Save failed" >&2
  echo "$RESULT" >&2
  exit 1
fi
