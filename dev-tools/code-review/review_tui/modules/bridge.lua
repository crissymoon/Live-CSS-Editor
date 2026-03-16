-- modules/bridge.lua
-- Async Python subprocess runner.
-- Launches bridge.py as a child process, reads stdout line by line.
-- Protocol lines: LINE:<text>  ERROR:<text>  REPORT:<path>  DONE

local M = {}

M.streaming   = false
M.last_report = nil   -- path of most recently written report

local _thread    = nil
local _chan      = nil
local _job_id    = 0
local _on_line   = nil   -- callback(text, kind)
local _on_done   = nil   -- callback(report_path_or_nil)

local BRIDGE_PY = nil   -- set by M.init()

local WORKER_SRC = [[
local cmd, chan_name = ...
local chan = love.thread.getChannel(chan_name)

-- Use winpipe so the Python subprocess never spawns a visible console window.
local WP   = require "modules.winpipe"
local iter = WP.lines_live(cmd)
if not iter then
    chan:push("ERROR:Failed to start scanner process")
    chan:push("DONE")
    return
end

for line in iter do
    chan:push(line)
end

chan:push("DONE")
]]

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

    local cmd = PYTHON_CMD .. " " .. shell_escape(BRIDGE_PY)
    for _, a in ipairs(args) do
        cmd = cmd .. " " .. shell_escape(a)
    end

    _job_id = _job_id + 1
    local chan_name = "code_review_bridge_" .. tostring(_job_id)
    _chan = love.thread.getChannel(chan_name)
    _chan:clear()

    _thread = love.thread.newThread(WORKER_SRC)
    _thread:start(cmd, chan_name)

    _on_line = on_line
    _on_done = on_done
    M.streaming   = true
    M.last_report = nil
end

-- Call from love.update every frame to drain the pipe.
function M.poll()
    if not _chan then return end

    local processed = 0
    local max_per_frame = 200
    while processed < max_per_frame do
        local line = _chan:pop()
        if not line then break end
        processed = processed + 1

        if line:sub(1, 5) == "LINE:" then
            if _on_line then _on_line(line:sub(6), "result") end
        elseif line:sub(1, 6) == "ERROR:" then
            if _on_line then _on_line(line:sub(7), "error") end
        elseif line:sub(1, 7) == "REPORT:" then
            M.last_report = line:sub(8)
            if _on_line then _on_line("Report saved: " .. M.last_report, "report") end
        elseif line == "DONE" then
            _chan = nil
            _thread = nil
            M.streaming = false
            if _on_done then
                local cb = _on_done
                _on_done = nil
                cb(M.last_report)
            end
            return
        else
            -- pass-through raw lines (e.g. tracebacks)
            if _on_line then _on_line(line, "dim") end
        end
    end
end

function M.abort()
    _chan = nil
    _thread = nil
    M.streaming = false
    _on_done = nil
end

function M.queue_depth()
    if _chan then
        return _chan:getCount() or 0
    end
    return 0
end

return M
