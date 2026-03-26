-- modules/winpipe.lua
-- Run subprocesses without showing a console window on Windows.
-- Uses LuaJIT FFI + CreateProcess(CREATE_NO_WINDOW).
-- On non-Windows, thin wrappers around io.popen are provided instead.
--
-- API:
--   WP.lines(cmd)       -> iterator over output lines (reads all then iterates; good for fast commands)
--   WP.lines_live(cmd)  -> streaming line iterator (reads as process writes; good for long scans)
--   WP.is_dir(path)     -> bool, true if path is an existing directory (no subprocess, Win32 direct)

local M = {}

-- Detect platform without requiring ffi so this module loads cleanly in Love2D
-- worker threads on macOS (including Apple Silicon builds where ffi may be
-- unavailable). package.config:sub(1,1) is "\\" on Windows, "/" elsewhere.
local IS_WIN = package.config:sub(1, 1) == "\\"

-- -------------------------------------------------------------------------
-- Non-Windows fallback (macOS / Linux)
-- -------------------------------------------------------------------------
if not IS_WIN then
    function M.lines(cmd)
        local p = io.popen(cmd .. " 2>&1", "r")
        if not p then return function() return nil end end
        local out = {}
        for ln in p:lines() do out[#out + 1] = ln end
        p:close()
        local i = 0
        return function() i = i + 1; return out[i] end
    end

    function M.lines_live(cmd)
        local p = io.popen(cmd .. " 2>&1", "r")
        if not p then return nil end
        return function()
            local ln = p:read("*l")
            if ln == nil then p:close() end
            return ln
        end
    end

    function M.is_dir(path)
        -- Use opendir() via FFI when available to avoid fork() overhead.
        local ok, ffi_mod = pcall(require, "ffi")
        if ok then
            pcall(ffi_mod.cdef, [[void* opendir(const char*); int closedir(void*);]])
            local d = ffi_mod.C.opendir(path)
            if d ~= nil then
                ffi_mod.C.closedir(d)
                return true
            end
            return false
        end
        -- Fallback when ffi is not available.
        local rc = os.execute("[ -d " .. string.format("%q", path) .. " ]")
        return rc == 0 or rc == true
    end

    return M
end

-- -------------------------------------------------------------------------
-- Windows implementation via LuaJIT FFI
-- -------------------------------------------------------------------------
local ffi = require "ffi"

-- -------------------------------------------------------------------------
-- Windows implementation via LuaJIT FFI
-- -------------------------------------------------------------------------

-- Guard against calling ffi.cdef more than once per Lua state (e.g. if the
-- module somehow gets re-evaluated; pcall swallows "already defined" errors).
pcall(ffi.cdef, [[
  typedef void*          WP_HANDLE;
  typedef unsigned long  WP_DWORD;
  typedef int            WP_BOOL;
  typedef unsigned short WP_WORD;
  typedef unsigned char  WP_BYTE;

  typedef struct {
    WP_DWORD  nLength;
    void*     lpSecurityDescriptor;
    WP_BOOL   bInheritHandle;
  } WP_SECURITY_ATTRIBUTES;

  typedef struct {
    WP_DWORD    cb;
    char*       lpReserved;
    char*       lpDesktop;
    char*       lpTitle;
    WP_DWORD    dwX, dwY, dwXSize, dwYSize;
    WP_DWORD    dwXCountChars, dwYCountChars;
    WP_DWORD    dwFillAttribute;
    WP_DWORD    dwFlags;
    WP_WORD     wShowWindow, cbReserved2;
    WP_BYTE*    lpReserved2;
    WP_HANDLE   hStdInput, hStdOutput, hStdError;
  } WP_STARTUPINFOA;

  typedef struct {
    WP_HANDLE hProcess, hThread;
    WP_DWORD  dwProcessId, dwThreadId;
  } WP_PROCESS_INFORMATION;

  WP_BOOL  __stdcall CreatePipe(WP_HANDLE*, WP_HANDLE*,
                                WP_SECURITY_ATTRIBUTES*, WP_DWORD);
  WP_BOOL  __stdcall SetHandleInformation(WP_HANDLE, WP_DWORD, WP_DWORD);
  WP_BOOL  __stdcall CreateProcessA(const char*, char*,
                                    WP_SECURITY_ATTRIBUTES*,
                                    WP_SECURITY_ATTRIBUTES*,
                                    WP_BOOL, WP_DWORD, void*, const char*,
                                    WP_STARTUPINFOA*, WP_PROCESS_INFORMATION*);
  WP_DWORD __stdcall WaitForSingleObject(WP_HANDLE, WP_DWORD);
  WP_BOOL  __stdcall ReadFile(WP_HANDLE, void*, WP_DWORD, WP_DWORD*, void*);
  WP_BOOL  __stdcall CloseHandle(WP_HANDLE);
  WP_DWORD __stdcall GetFileAttributesA(const char*);
]])

local K = ffi.load("kernel32")

local CREATE_NO_WINDOW     = 0x08000000
local STARTF_USESTDHANDLES = 0x00000100
local NULL_H               = ffi.cast("WP_HANDLE", 0)
local INVALID_FILE_ATTRS   = 0xFFFFFFFF
local FILE_ATTR_DIRECTORY  = 0x10

-- Spawn `cmd` as a hidden process with stdout+stderr piped back.
-- Returns (hRead, pi_cdata) on success, or (nil, nil) on failure.
local function spawn(cmd)
    local sa = ffi.new("WP_SECURITY_ATTRIBUTES")
    sa.nLength        = ffi.sizeof("WP_SECURITY_ATTRIBUTES")
    sa.bInheritHandle = 1

    local hR = ffi.new("WP_HANDLE[1]")
    local hW = ffi.new("WP_HANDLE[1]")
    if K.CreatePipe(hR, hW, sa, 0) == 0 then return nil, nil end

    -- Do NOT let the child inherit the read end of the pipe.
    K.SetHandleInformation(hR[0], 1, 0)

    local si = ffi.new("WP_STARTUPINFOA")
    si.cb         = ffi.sizeof("WP_STARTUPINFOA")
    si.dwFlags    = STARTF_USESTDHANDLES
    si.hStdInput  = NULL_H
    si.hStdOutput = hW[0]
    si.hStdError  = hW[0]   -- merge stderr into the same pipe

    local pi   = ffi.new("WP_PROCESS_INFORMATION")
    local cmdz = ffi.new("char[?]", #cmd + 1)
    ffi.copy(cmdz, cmd)

    local ok = K.CreateProcessA(nil, cmdz, nil, nil, 1,
                                CREATE_NO_WINDOW, nil, nil, si, pi)

    -- Parent must close its write end BEFORE reading, otherwise ReadFile
    -- will block forever waiting for someone to write (deadlock).
    K.CloseHandle(hW[0])

    if ok == 0 then
        K.CloseHandle(hR[0])
        return nil, nil
    end
    return hR[0], pi
end

-- Drain hRead pipe to a string, then close handles.
local function drain(hRead, pi)
    local parts = {}
    local buf   = ffi.new("char[8192]")
    local cnt   = ffi.new("WP_DWORD[1]")
    while K.ReadFile(hRead, buf, 8192, cnt, nil) == 1 and cnt[0] > 0 do
        parts[#parts + 1] = ffi.string(buf, cnt[0])
    end
    K.WaitForSingleObject(pi.hProcess, 0xFFFFFFFF)
    K.CloseHandle(pi.hProcess)
    K.CloseHandle(pi.hThread)
    K.CloseHandle(hRead)
    return table.concat(parts)
end

-- Run cmd, collect ALL output, return a lines iterator.
-- Simple and allocation-efficient for short-lived commands (dir listings etc.).
function M.lines(cmd)
    local hRead, pi = spawn(cmd)
    if not hRead then return function() return nil end end

    local raw   = drain(hRead, pi)
    local lines = {}
    for ln in (raw .. "\n"):gmatch("([^\r\n]*)\r?\n") do
        lines[#lines + 1] = ln
    end
    -- trim trailing blank lines that cmd.exe appends
    while #lines > 0 and lines[#lines] == "" do
        lines[#lines] = nil
    end
    local i = 0
    return function() i = i + 1; return lines[i] end
end

-- Run cmd, return a live streaming line iterator.
-- Yields lines as the child process writes them — good for long scans.
-- Returns nil if the process could not be started.
function M.lines_live(cmd)
    local hRead, pi = spawn(cmd)
    if not hRead then return nil end

    local buf     = ffi.new("char[8192]")
    local cnt     = ffi.new("WP_DWORD[1]")
    local pending = ""
    local eof     = false

    local function close_all()
        if not eof then
            eof = true
            K.WaitForSingleObject(pi.hProcess, 0xFFFFFFFF)
            K.CloseHandle(pi.hProcess)
            K.CloseHandle(pi.hThread)
            K.CloseHandle(hRead)
        end
    end

    return function()
        while true do
            -- Deliver the next complete line from whatever is buffered.
            local s = pending:find("\n")
            if s then
                local ln = pending:sub(1, s - 1):gsub("\r$", "")
                pending  = pending:sub(s + 1)
                return ln
            end

            if eof then
                -- Flush any trailing content without a final newline.
                if pending ~= "" then
                    local ln = pending:gsub("\r?$", "")
                    pending  = ""
                    if ln ~= "" then return ln end
                end
                return nil   -- signals end of iteration
            end

            -- Read the next chunk from the pipe.
            local r = K.ReadFile(hRead, buf, 8192, cnt, nil)
            if r == 0 or cnt[0] == 0 then
                close_all()
            else
                pending = pending .. ffi.string(buf, cnt[0])
            end
        end
    end
end

-- Return true if `path` is an existing directory (uses Win32 directly, no subprocess).
function M.is_dir(path)
    local attr = K.GetFileAttributesA(path)
    return attr ~= INVALID_FILE_ATTRS
       and bit.band(attr, FILE_ATTR_DIRECTORY) ~= 0
end

return M
