-- db_bridge/schema/pages.sql
-- Schema for the page-builder pages database.
-- Run once to initialise: sqlite3 pages.db < pages.sql

PRAGMA journal_mode=WAL;
PRAGMA foreign_keys=ON;
PRAGMA user_version=1;

-- Main pages table
CREATE TABLE IF NOT EXISTS pages (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    slug           TEXT    NOT NULL UNIQUE,
    title          TEXT    NOT NULL DEFAULT '',
    status         TEXT    NOT NULL DEFAULT 'draft'
                           CHECK(status IN ('draft', 'published', 'archived')),
    css_overrides  TEXT    NOT NULL DEFAULT '{}',   -- JSON
    meta           TEXT    NOT NULL DEFAULT '{}',   -- JSON
    created_at     INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    updated_at     INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
);

-- History of every save (append-only)
CREATE TABLE IF NOT EXISTS page_history (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    slug           TEXT    NOT NULL REFERENCES pages(slug) ON DELETE CASCADE,
    css_overrides  TEXT    NOT NULL DEFAULT '{}',
    saved_at       INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    saved_by       TEXT    NOT NULL DEFAULT ''       -- username or 'stage'
);

-- Indexes
CREATE INDEX IF NOT EXISTS idx_pages_slug       ON pages(slug);
CREATE INDEX IF NOT EXISTS idx_pages_status     ON pages(status);
CREATE INDEX IF NOT EXISTS idx_pages_updated    ON pages(updated_at);
CREATE INDEX IF NOT EXISTS idx_history_slug     ON page_history(slug);
CREATE INDEX IF NOT EXISTS idx_history_saved_at ON page_history(saved_at);

-- Auto-history trigger: every UPDATE to pages captures the old css_overrides
CREATE TRIGGER IF NOT EXISTS trg_pages_history
    AFTER UPDATE OF css_overrides ON pages
    FOR EACH ROW
BEGIN
    INSERT INTO page_history (slug, css_overrides, saved_at, saved_by)
    VALUES (OLD.slug, OLD.css_overrides, strftime('%s', 'now'), 'system');
END;
