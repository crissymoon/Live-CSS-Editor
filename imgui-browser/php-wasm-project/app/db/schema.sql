-- app/db/schema.sql
-- Idempotent schema migrations. Safe to run on every boot.

CREATE TABLE IF NOT EXISTS stylesheets (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    name       TEXT    NOT NULL UNIQUE,
    css        TEXT    NOT NULL DEFAULT '',
    updated_at TEXT    NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS export_log (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT    NOT NULL,
    filename    TEXT    NOT NULL,
    exported_at TEXT    NOT NULL DEFAULT (datetime('now'))
);

CREATE INDEX IF NOT EXISTS idx_stylesheets_name       ON stylesheets (name);
CREATE INDEX IF NOT EXISTS idx_export_log_exported_at ON export_log  (exported_at);
