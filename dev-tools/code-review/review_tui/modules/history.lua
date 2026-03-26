-- modules/history.lua
-- Time-travel snapshot history stored in a SQLite database.
--
-- Database location: %USERPROFILE%\Documents\dev_tools_history.db
-- Requires sqlite3.dll accessible via ffi.load("sqlite3").
-- When the DLL is not found the module silently disables itself.

local M = {}

local _db   = nil    -- sqlite3* handle
local _init = false  -- true after M._open() has been attempted
local _ok   = false  -- true when the DB is open and usable

local MAX_SNAPSHOTS   = 100   -- max snapshots kept per file
local AUTO_SNAP_INTERVAL = 120 -- seconds between periodic background snapshots

-- -----------------------------------------------------------------------
-- LuaJIT FFI binding
-- -----------------------------------------------------------------------
local ffi_mod = require("ffi")   -- LuaJIT FFI is always present in LÖVE2D

-- Declare types once (wrapped in pcall so re-loading doesn't error).
pcall(function()
    ffi_mod.cdef[[
        typedef long long   sq3_i64;
        typedef struct sq3  sq3;
        typedef struct sq3s sq3s;

        int  sqlite3_open       (const char *fname, sq3 **ppDb);
        int  sqlite3_close      (sq3 *db);
        int  sqlite3_exec       (sq3 *db, const char *sql,
                                 void *cb, void *cbarg, char **errmsg);
        void sqlite3_free       (void *ptr);

        int  sqlite3_prepare_v2 (sq3 *db, const char *zSql, int nByte,
                                 sq3s **ppStmt, const char **pzTail);
        int  sqlite3_step       (sq3s *pStmt);
        int  sqlite3_finalize   (sq3s *pStmt);
        int  sqlite3_reset      (sq3s *pStmt);

        int  sqlite3_bind_text  (sq3s *pStmt, int i,
                                 const char *zData, int nData, void *xDel);
        int  sqlite3_bind_int64 (sq3s *pStmt, int i, sq3_i64 iValue);

        const unsigned char *sqlite3_column_text  (sq3s *pStmt, int iCol);
        sq3_i64              sqlite3_column_int64 (sq3s *pStmt, int iCol);

        sq3_i64     sqlite3_last_insert_rowid (sq3 *db);
        const char *sqlite3_errmsg           (sq3 *db);
    ]]
end)

-- SQLITE_TRANSIENT = (void*)-1 : tells SQLite to copy the string immediately.
local SQLITE_TRANSIENT = ffi_mod.cast("void*", ffi_mod.cast("intptr_t", -1))
local SQLITE_ROW  = 100
local SQLITE_DONE = 101

local sq3 = nil  -- the loaded library handle

local function try_load_lib()
    -- 1. Try "sqlite3" via system PATH or executable directory.
    for _, name in ipairs({ "sqlite3", "sqlite3.dll" }) do
        local ok, lib = pcall(ffi_mod.load, name)
        if ok then return lib end
    end

    -- 2. Use the explicit DLL path exported by run.ps1 (set from the Python install).
    local explicit = os.getenv("CODE_REVIEW_SQLITE3")
    if explicit and explicit ~= "" then
        local ok, lib = pcall(ffi_mod.load, explicit)
        if ok then return lib end
    end

    -- 3. Derive from CODE_REVIEW_PYTHON: strip exe name, append DLLs\sqlite3.dll.
    local py_env = os.getenv("CODE_REVIEW_PYTHON") or ""
    local py_exe = py_env:match("^([^\"%s]+)")   -- first token, no quotes
    if py_exe then
        local py_dir = py_exe:match("^(.+)\\[^\\]+$")
        if py_dir then
            local candidate = py_dir .. "\\DLLs\\sqlite3.dll"
            local ok, lib = pcall(ffi_mod.load, candidate)
            if ok then return lib end
        end
    end

    return nil
end

-- -----------------------------------------------------------------------
-- Internal helpers
-- -----------------------------------------------------------------------
local function db_path()
    local up = os.getenv("USERPROFILE") or os.getenv("HOME") or "."
    return up .. "\\Documents\\dev_tools_history.db"
end

local function exec(sql)
    if not _db then return end
    sq3.sqlite3_exec(_db, sql, nil, nil, nil)
end

local function prepare(sql)
    if not _db or not sq3 then return nil end
    local s = ffi_mod.new("sq3s*[1]")
    if sq3.sqlite3_prepare_v2(_db, sql, -1, s, nil) ~= 0 then return nil end
    return s[0]
end

-- -----------------------------------------------------------------------
-- Public: init (called lazily)
-- -----------------------------------------------------------------------
function M._open()
    if _init then return end
    _init = true

    sq3 = try_load_lib()
    if not sq3 then return end  -- sqlite3.dll not available

    local db_ptr = ffi_mod.new("sq3*[1]")
    local path   = db_path()
    if sq3.sqlite3_open(path, db_ptr) ~= 0 then return end
    _db = db_ptr[0]
    _ok = true

    exec([[
        CREATE TABLE IF NOT EXISTS snapshots (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            filepath  TEXT    NOT NULL,
            content   TEXT    NOT NULL,
            saved_at  INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_snap
            ON snapshots (filepath, saved_at DESC);
    ]])
end

function M.enabled()
    M._open()
    return _ok
end

-- -----------------------------------------------------------------------
-- Public: save a snapshot
-- Returns the new snapshot id (integer), or nil on failure / duplicate.
-- -----------------------------------------------------------------------
function M.snapshot(filepath, lines)
    M._open()
    if not _ok then return nil end

    local content = table.concat(lines, "\n")

    -- Deduplicate: skip if content is identical to the most recent snapshot.
    local recent = M.list(filepath)
    if recent[1] then
        local prev = M.load(recent[1].id)
        if prev and table.concat(prev, "\n") == content then
            return recent[1].id
        end
    end

    local stmt = prepare(
        "INSERT INTO snapshots(filepath, content, saved_at) VALUES(?,?,?)")
    if not stmt then return nil end

    sq3.sqlite3_bind_text (stmt, 1, filepath, -1,       SQLITE_TRANSIENT)
    sq3.sqlite3_bind_text (stmt, 2, content,  -1,       SQLITE_TRANSIENT)
    sq3.sqlite3_bind_int64(stmt, 3, os.time())
    sq3.sqlite3_step(stmt)
    sq3.sqlite3_finalize(stmt)

    local new_id = tonumber(sq3.sqlite3_last_insert_rowid(_db))

    -- Keep only MAX_SNAPSHOTS per file.
    local prune = prepare(
        "DELETE FROM snapshots WHERE filepath=? AND id NOT IN "..
        "(SELECT id FROM snapshots WHERE filepath=? "..
        " ORDER BY saved_at DESC LIMIT ?)")
    if prune then
        sq3.sqlite3_bind_text (prune, 1, filepath, -1, SQLITE_TRANSIENT)
        sq3.sqlite3_bind_text (prune, 2, filepath, -1, SQLITE_TRANSIENT)
        sq3.sqlite3_bind_int64(prune, 3, MAX_SNAPSHOTS)
        sq3.sqlite3_step(prune)
        sq3.sqlite3_finalize(prune)
    end

    return new_id
end

-- -----------------------------------------------------------------------
-- Public: list snapshots for a file, newest first.
-- Returns: array of { id, saved_at }
-- -----------------------------------------------------------------------
function M.list(filepath)
    M._open()
    local result = {}
    if not _ok then return result end

    local stmt = prepare(
        "SELECT id, saved_at FROM snapshots "..
        "WHERE filepath=? ORDER BY saved_at DESC LIMIT 200")
    if not stmt then return result end

    sq3.sqlite3_bind_text(stmt, 1, filepath, -1, SQLITE_TRANSIENT)
    while sq3.sqlite3_step(stmt) == SQLITE_ROW do
        result[#result+1] = {
            id       = tonumber(sq3.sqlite3_column_int64(stmt, 0)),
            saved_at = tonumber(sq3.sqlite3_column_int64(stmt, 1)),
        }
    end
    sq3.sqlite3_finalize(stmt)
    return result
end

-- -----------------------------------------------------------------------
-- Public: load snapshot content by id.
-- Returns: array of lines, or nil.
-- -----------------------------------------------------------------------
function M.load(id)
    M._open()
    if not _ok then return nil end

    local stmt = prepare("SELECT content FROM snapshots WHERE id=?")
    if not stmt then return nil end

    sq3.sqlite3_bind_int64(stmt, 1, id)
    local lines = nil
    if sq3.sqlite3_step(stmt) == SQLITE_ROW then
        local ptr = sq3.sqlite3_column_text(stmt, 0)
        if ptr ~= nil then
            local text = ffi_mod.string(ptr)
            lines = {}
            for ln in (text .. "\n"):gmatch("([^\n]*)\n") do
                lines[#lines+1] = ln
            end
            if #lines > 1 and lines[#lines] == "" then
                lines[#lines] = nil
            end
            if #lines == 0 then lines = { "" } end
        end
    end
    sq3.sqlite3_finalize(stmt)
    return lines
end

return M
