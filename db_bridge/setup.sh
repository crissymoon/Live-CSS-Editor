#!/usr/bin/env bash
# db_bridge/setup.sh
# Initialises or migrations the pages database.
# Safe to run multiple times (uses CREATE TABLE IF NOT EXISTS).
# Usage: bash db_bridge/setup.sh [--force]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE="$(dirname "$SCRIPT_DIR")"
DB_DIR="$WORKSPACE/dev-tools/db-browser/databases"
DB_PATH="$DB_DIR/pages.db"
SCHEMA="$SCRIPT_DIR/schema/pages.sql"

mkdir -p "$DB_DIR"

if [[ "${1:-}" == "--force" && -f "$DB_PATH" ]]; then
    echo "Removing existing $DB_PATH..."
    rm -f "$DB_PATH" "$DB_PATH-wal" "$DB_PATH-shm"
fi

echo "Applying schema to $DB_PATH ..."
sqlite3 "$DB_PATH" < "$SCHEMA"

echo "Verification:"
sqlite3 "$DB_PATH" "PRAGMA journal_mode; SELECT name, type FROM sqlite_master WHERE type IN ('table','view','trigger','index') ORDER BY type, name;"
echo ""
echo "Done. Database: $DB_PATH"
