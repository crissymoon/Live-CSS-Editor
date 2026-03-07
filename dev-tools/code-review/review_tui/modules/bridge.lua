-- modules/bridge.lua
-- Async Python subprocess runner.
-- Launches bridge.py as a child process, reads stdout line by line.
-- Protocol lines: LINE:<text>  ERROR:<text>  REPORT:<path>  DONE

local M = {}

M.streaming   = false
M.last_report = nil   -- path of most recently written report

local _pipe   = nil
local _on_line   = nil   -- callback(text, kind)
local _on_done   = nil   -- callback(report_path_or_nil)

local BRIDGE_PY = nil   -- set by M.init()

function M.init(bridge_py_path)
    BRIDGE_PY = bridge_py_path
end

-- Start a scan.  args = list of strings e.g. {"security_scan", "/some/dir"}
-- on_line(text, kind)  called for each LINE: / ERROR: result
-- on_done(report)      called with report path (or nil) when DONE arrives
function M.start(args, on_line, on_done)
    if M.streaming then return end
    if not BRIDGE_PY then
        if on_line then on_line("bridge.py path not set", "error") end
        if on_done then on_done(nil) end
        return
    end

    local cmd = "python3 " .. shell_escape(BRIDGE_PY)
    for _, a in ipairs(args) do
        cmd = cmd .. " " .. shell_escape(a)
    end

    _pipe    = io.popen(cmd .. " 2>&1", "r")
    _on_line = on_line
    _on_done = on_done
    M.streaming   = true
    M.last_report = nil
end

-- Call from love.update every frame to drain the pipe.
function M.poll()
    if not _pipe then return end

    -- Read as many lines as are available without blocking.
    -- io.popen is blocking so we use file:read("*l") inside pcall;
    -- when the process ends read returns nil.
    local line = _pipe:read("*l")
    if line == nil then
        -- Process finished.
        pcall(function() _pipe:close() end)
        _pipe       = nil
        M.streaming = false
        if _on_done then
            local cb = _on_done
            _on_done = nil
            cb(M.last_report)
        end
        return
    end

    if line:sub(1, 5) == "LINE:" then
        if _on_line then _on_line(line:sub(6), "result") end
    elseif line:sub(1, 6) == "ERROR:" then
        if _on_line then _on_line(line:sub(7), "error") end
    elseif line:sub(1, 7) == "REPORT:" then
        M.last_report = line:sub(8)
        if _on_line then _on_line("Report saved: " .. M.last_report, "report") end
    elseif line == "DONE" then
        -- will be handled on next nil read; nothing to do here
    else
        -- pass-through raw lines (e.g. tracebacks)
        if _on_line then _on_line(line, "dim") end
    end
end

function M.abort()
    if _pipe then
        pcall(function() _pipe:close() end)
        _pipe       = nil
        M.streaming = false
        _on_done    = nil
    end
end

return M
