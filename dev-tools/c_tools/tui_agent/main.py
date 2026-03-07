#!/usr/bin/env python3
"""
main.py  --  c_tools TUI Agent

Curses-based terminal UI:
  scanner -> agent (Haiku, streaming) -> emoji_clean -> merger (DB stage) -> apply

ANIMATION
  Spinner ticks in status bar while busy.
  Transfer bar [####    ] shows streaming chars received vs max_tokens.
  Last received snippet visible in status bar: " recv: 1,247c  ...snippet..."
  Typewriter reveal plays when diff arrives (3 lines per tick).
  Pulsing ">" marker on the selected file row.
  Log auto-scrolls to bottom while busy.
  getch timeout = 80 ms so the loop always ticks.

DIRECTORY NAVIGATION
  Starts in the directory passed as argv[1] (default: cwd).
  Browse forward into subdirectories: Enter / Right Arrow.
  Go up one level: Backspace / - / Left Arrow.
  Cannot go above the launch root.
  Agent only edits EXISTING files inside the current directory.

Controls
--------
  Arrow keys         navigate file list or scroll focused panel
  Tab                cycle focus: Files -> Log -> Diff
  Enter / Right      (files panel) descend into selected directory
  Backspace / -      (files panel) go up one level within root
  p                  enter prompt for selected file
  P                  enter project mode instruction (scans dir, plans, edits via Haiku)
  d                  show diff for selected file
  a                  approve + apply pending change
  r                  reject pending change
  c                  copy focused panel content to clipboard (pbcopy)
  v                  toggle view mode (disables mouse for terminal copy)
  s                  rescan current directory
  q / Esc            quit

Right-click any line in log or diff to copy it to clipboard.
"""

import curses
import os
import sys
import subprocess
import threading
from typing import Callable, List, Optional, Tuple

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from log_util    import get_logger
from db          import AgentDB
from scanner     import FolderScanner
from merger      import Merger
import agent as _agent

try:
    from convo import ConvoSession as _ConvoSession
except Exception as _convo_import_err:
    _ConvoSession = None  # type: ignore
    print(f"[main] convo import failed: {_convo_import_err}", file=sys.stderr)

try:
    from project_mode import ProjectSession as _ProjectSession
except Exception as _pm_import_err:
    _ProjectSession = None  # type: ignore
    print(f"[main] project_mode import failed: {_pm_import_err}", file=sys.stderr)

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
HERE     = os.path.dirname(os.path.abspath(__file__))
DB_PATH  = os.path.join(HERE, "agent.db")

# max_tokens sent to Haiku -- used for transfer bar scaling
HAIKU_MAX_TOKENS = 4096

# ---------------------------------------------------------------------------
# Animation frames
# ---------------------------------------------------------------------------
SPINNER      = ["|", "/", "-", "\\"]
BUSY_DOTS    = ["   ", ".  ", ".. ", "..."]
PULSE_MARKS  = [">", ">", " ", " "]

# ---------------------------------------------------------------------------
# Colour pair IDs
# ---------------------------------------------------------------------------
C_NORMAL   = 1
C_HEADER   = 2
C_SELECT   = 3
C_ADD      = 4
C_REMOVE   = 5
C_STATUS   = 6
C_INPUT    = 7
C_MUTED    = 8
C_WARN     = 9
C_BUSY     = 10
C_DIR      = 11
C_CHAT     = 12   # rolling conversation lines from convo.py
C_PROJECT  = 13   # project mode status lines

def _init_colors():
    curses.start_color()
    curses.use_default_colors()
    curses.init_pair(C_NORMAL,  curses.COLOR_WHITE,   -1)
    curses.init_pair(C_HEADER,  curses.COLOR_CYAN,    -1)
    curses.init_pair(C_SELECT,  curses.COLOR_BLACK,   curses.COLOR_CYAN)
    curses.init_pair(C_ADD,     curses.COLOR_GREEN,   -1)
    curses.init_pair(C_REMOVE,  curses.COLOR_RED,     -1)
    curses.init_pair(C_STATUS,  curses.COLOR_BLACK,   curses.COLOR_WHITE)
    curses.init_pair(C_INPUT,   curses.COLOR_YELLOW,  -1)
    curses.init_pair(C_MUTED,   curses.COLOR_WHITE,   -1)
    curses.init_pair(C_WARN,    curses.COLOR_YELLOW,  -1)
    curses.init_pair(C_BUSY,    curses.COLOR_MAGENTA, -1)
    curses.init_pair(C_DIR,     curses.COLOR_CYAN,    -1)
    curses.init_pair(C_CHAT,    curses.COLOR_GREEN,   -1)
    curses.init_pair(C_PROJECT, curses.COLOR_YELLOW,  curses.COLOR_BLACK)


# ---------------------------------------------------------------------------
# Drawing helpers
# ---------------------------------------------------------------------------
def _attr(pair: int, bold: bool = False, dim: bool = False) -> int:
    a = curses.color_pair(pair)
    if bold: a |= curses.A_BOLD
    if dim:  a |= curses.A_DIM
    return a

def _draw_line(win, y: int, x: int, text: str, max_w: int, attr: int):
    try:
        win.addstr(y, x, text[:max(0, max_w)], attr)
    except curses.error:
        pass

def _draw_scrollbar(win, scroll: int, total: int, visible: int):
    try:
        h, w = win.getmaxyx()
        inner_h = h - 2
        if total <= visible or inner_h < 1:
            return
        bar_h = max(1, inner_h * visible // max(1, total))
        bar_y = 1 + (inner_h - bar_h) * scroll // max(1, total - visible)
        for row in range(1, inner_h + 1):
            ch = curses.ACS_CKBOARD if bar_y <= row < bar_y + bar_h else curses.ACS_VLINE
            try:
                win.addch(row, w - 1, ch, _attr(C_MUTED, dim=True))
            except curses.error:
                pass
    except curses.error:
        pass

def _to_clipboard(text: str, log) -> bool:
    try:
        subprocess.run(["pbcopy"], input=text.encode("utf-8"),
                       check=True, capture_output=True)
        log.info("CLIP", f"copied {len(text)} chars")
        return True
    except Exception as exc:
        log.error("CLIP", f"pbcopy failed: {exc}")
        return False


# ---------------------------------------------------------------------------
# Entry type for file browser
# ---------------------------------------------------------------------------
class Entry:
    __slots__ = ("path", "name", "is_dir", "size", "pending")
    def __init__(self, path, name, is_dir, size=0, pending=0):
        self.path    = path
        self.name    = name
        self.is_dir  = is_dir
        self.size    = size
        self.pending = pending


# ---------------------------------------------------------------------------
# App state
# ---------------------------------------------------------------------------
class State:
    def __init__(self, root_dir: str):
        # directory navigation
        self.root_dir:       str            = root_dir
        self.cwd:            str            = root_dir

        # file browser
        self.entries:        List[Entry]    = []
        self.file_sel:       int            = 0

        # log / diff panels
        self.log_lines:      List[str]      = []
        self.diff_lines:     List[str]      = []
        self.log_scroll:     int            = 0
        self.diff_scroll:    int            = 0
        self.diff_reveal:    int            = 0   # typewriter -- lines revealed so far
        self.focus:          str            = "files"

        # input bar
        self.input_mode:     bool           = False
        self.input_buf:      str            = ""

        # status
        self.status:         str            = "ready"
        self.status_kind:    str            = ""

        # animation
        self.tick:           int            = 0
        self.busy:           bool           = False
        self.busy_msg:       str            = ""
        self.stream_chars:   int            = 0   # chars received so far from streaming API
        self.stream_snippet: str            = ""  # last few chars of latest chunk

        # input bar kind: "prompt" = agent instruction, "newfile" = create file
        self.input_kind:     str            = "prompt"

        # view mode
        self.view_mode:      bool           = False
        self.pending_change: int            = -1

        self._lock = threading.Lock()

    def set_status(self, msg: str, kind: str = ""):
        with self._lock:
            self.status      = msg
            self.status_kind = kind

    def set_busy(self, busy: bool, msg: str = ""):
        with self._lock:
            self.busy     = busy
            self.busy_msg = msg
            if not busy:
                # reset stream counters when done
                self.stream_chars   = 0
                self.stream_snippet = ""

    def set_stream(self, chars: int, snippet: str):
        """Called from background thread on each streaming chunk."""
        with self._lock:
            self.stream_chars   = chars
            self.stream_snippet = snippet

    def push_diff(self, diff: str):
        with self._lock:
            self.diff_lines  = diff.splitlines()
            self.diff_scroll = 0
            self.diff_reveal = 0   # restart typewriter
            self.focus       = "diff"

    def selected_entry(self) -> Optional[Entry]:
        if not self.entries or not (0 <= self.file_sel < len(self.entries)):
            return None
        return self.entries[self.file_sel]

    def selected_file(self) -> Optional[str]:
        e = self.selected_entry()
        if e and not e.is_dir:
            return e.path
        return None


# ---------------------------------------------------------------------------
# Main TUI
# ---------------------------------------------------------------------------
class TUI:
    def __init__(self, stdscr, root_dir: str):
        self.stdscr   = stdscr
        self.state    = State(root_dir)
        self.log      = get_logger()
        self.db       = AgentDB(DB_PATH)
        self.merger   = Merger(self.db)
        self._scanner = FolderScanner(recursive=False)

        self.win_files = None
        self.win_log   = None
        self.win_diff  = None
        self.win_bar   = None

        # rolling conversation session (convo.py / GPT-4o mini)
        self._convo: Optional[object] = None

        # project mode session (project_mode.py)
        self._project_session: Optional[object] = None

    # ------------------------------------------------------------------
    # Setup
    # ------------------------------------------------------------------
    def setup(self):
        _init_colors()
        curses.curs_set(0)
        curses.noecho()
        curses.cbreak()
        self.stdscr.keypad(True)
        # 80 ms timeout: getch returns -1 when no key, loop ticks for animation
        self.stdscr.timeout(80)
        if not self.state.view_mode:
            try:
                curses.mousemask(curses.ALL_MOUSE_EVENTS | curses.REPORT_MOUSE_POSITION)
            except Exception as exc:
                self.log.warning("TUI", f"mousemask failed: {exc}")

        if not self.db.open():
            self.state.set_status("DB open failed -- check agent.log", "err")
            self.log.error("TUI", "db.open() failed")
        else:
            self._scan_cwd()

        self._layout()
        self._full_redraw()

    # ------------------------------------------------------------------
    # Layout
    # ------------------------------------------------------------------
    def _layout(self):
        H, W = self.stdscr.getmaxyx()
        bar_h   = 3
        left_w  = max(22, W // 3)
        right_w = W - left_w
        right_h = H - bar_h
        log_h   = max(4, right_h * 53 // 100)   # log ~53%, diff ~47%
        diff_h  = right_h - log_h

        self.win_files = curses.newwin(right_h, left_w,    0,       0)
        self.win_log   = curses.newwin(log_h,   right_w,   0,       left_w)
        self.win_diff  = curses.newwin(diff_h,  right_w,   log_h,   left_w)
        self.win_bar   = curses.newwin(bar_h,   W,         right_h, 0)
        for w in (self.win_files, self.win_log, self.win_diff):
            w.keypad(True)

    def _dims(self, win) -> Tuple[int, int]:
        h, w = win.getmaxyx()
        return max(0, h - 2), max(0, w - 2)

    # ------------------------------------------------------------------
    # Full redraw (called every ~80 ms)
    # ------------------------------------------------------------------
    def _full_redraw(self):
        s = self.state
        s.tick += 1

        # advance typewriter reveal
        if s.diff_reveal < len(s.diff_lines):
            s.diff_reveal = min(s.diff_reveal + 3, len(s.diff_lines))

        # auto-scroll log to bottom while busy so new entries stay visible
        if s.busy:
            total = len(s.log_lines)
            ih, _ = self._dims(self.win_log)
            s.log_scroll = max(0, total - ih)

        self.stdscr.noutrefresh()
        self._draw_files()
        self._draw_log()
        self._draw_diff()
        self._draw_bar()
        curses.doupdate()

    # ------------------------------------------------------------------
    # Draw: files panel
    # ------------------------------------------------------------------
    def _draw_files(self):
        win     = self.win_files
        s       = self.state
        focused = s.focus == "files"
        win.erase()
        try:
            win.box()
        except curses.error:
            pass

        ih, iw = self._dims(win)

        # breadcrumb header
        rel   = os.path.relpath(s.cwd, s.root_dir)
        rel   = "." if rel == "" else rel
        crumb = rel if len(rel) <= iw - 6 else "..." + rel[-(iw - 9):]
        title = f" {crumb}/ "
        title_attr = _attr(C_HEADER, bold=focused)
        if focused:
            title_attr |= curses.A_UNDERLINE
        try:
            win.addstr(0, 2, title[:iw], title_attr)
        except curses.error:
            pass

        for i, entry in enumerate(s.entries):
            if i >= ih:
                break
            is_sel = (i == s.file_sel)

            # animated pulse marker on selected row when focused
            if is_sel and focused:
                pulse = PULSE_MARKS[(s.tick // 4) % len(PULSE_MARKS)]
            elif is_sel:
                pulse = ">"
            else:
                pulse = " "

            if entry.is_dir:
                label = f" {pulse} {entry.name}/"
                label = label.ljust(iw)
                attr  = _attr(C_SELECT, bold=True) if is_sel else _attr(C_DIR)
            else:
                suffix = f" [{entry.pending}P]" if entry.pending else ""
                label  = f" {pulse} {entry.name}{suffix}"
                label  = label.ljust(iw)
                attr   = _attr(C_SELECT, bold=True) if is_sel else _attr(C_NORMAL)

            _draw_line(win, i + 1, 1, label, iw, attr)

        # footer
        try:
            stats    = self.db.stats()
            at_root  = os.path.realpath(s.cwd) == os.path.realpath(s.root_dir)
            boundary = " [root]" if at_root else " [-=up]"
            footer   = f" db:{stats.get('pending', 0)}P{boundary} "
            _draw_line(win, ih + 1, 1, footer, iw, _attr(C_MUTED, dim=True))
        except Exception:
            pass
        win.noutrefresh()

    # ------------------------------------------------------------------
    # Draw: log panel
    # ------------------------------------------------------------------
    def _draw_log(self):
        win     = self.win_log
        s       = self.state
        focused = s.focus == "log"
        win.erase()
        try:
            win.box()
        except curses.error:
            pass

        ih, iw = self._dims(win)

        # title: spinner + recv counter when busy
        if s.busy:
            spin   = SPINNER[s.tick % len(SPINNER)]
            bdots  = BUSY_DOTS[(s.tick // 2) % len(BUSY_DOTS)]
            recv   = f"  recv:{s.stream_chars:,}c" if s.stream_chars > 0 else ""
            title  = f" LOG {spin}{bdots}{recv} "
            t_attr = _attr(C_BUSY, bold=True)
        else:
            title  = " LOG "
            t_attr = _attr(C_HEADER, bold=focused)
        try:
            win.addstr(0, 2, title[:iw], t_attr)
        except curses.error:
            pass

        # refresh lines from logger singleton
        s.log_lines = self.log.get_entries(500)
        lines    = s.log_lines
        total    = len(lines)
        scroll   = min(s.log_scroll, max(0, total - ih))
        s.log_scroll = scroll
        visible  = lines[scroll: scroll + ih]

        for row, line in enumerate(visible):
            if " ERR " in line or " CRT " in line:
                attr = _attr(C_REMOVE, bold=True)
            elif " WRN " in line:
                attr = _attr(C_WARN)
            elif " [CHAT] " in line:
                # tech-slang commentary from convo.py -- stand out in green
                attr = _attr(C_CHAT, bold=True)
            elif " INF " in line:
                attr = _attr(C_NORMAL)
            else:
                attr = _attr(C_MUTED, dim=True)
            _draw_line(win, row + 1, 1, line, iw - 1, attr)

        _draw_scrollbar(win, scroll, total, ih)
        win.noutrefresh()

    # ------------------------------------------------------------------
    # Draw: diff panel
    # ------------------------------------------------------------------
    def _draw_diff(self):
        win     = self.win_diff
        s       = self.state
        focused = s.focus == "diff"
        win.erase()
        try:
            win.box()
        except curses.error:
            pass

        ih, iw = self._dims(win)

        revealed = s.diff_reveal
        total_d  = len(s.diff_lines)
        if total_d:
            adds = sum(
                1 for l in s.diff_lines
                if l.startswith("+") and not l.startswith("+"+"+")
            )
            rems = sum(
                1 for l in s.diff_lines
                if l.startswith("-") and not l.startswith("-"+"-")
            )
            change_str = f"+{adds}/-{rems}"
            if revealed < total_d:
                # still loading via typewriter
                label = f"DIFF {change_str}  {revealed}/{total_d}"
            else:
                # fully revealed -- show scroll window so position is obvious
                end_line = min(s.diff_scroll + ih, total_d)
                label = f"DIFF {change_str}  [{s.diff_scroll + 1}-{end_line}/{total_d}]"
        else:
            label = "DIFF"
        title    = f" {label} "
        t_attr   = _attr(C_ADD, bold=True) if (total_d and revealed < total_d) \
                   else _attr(C_HEADER, bold=focused)
        try:
            win.addstr(0, 2, title[:iw], t_attr)
        except curses.error:
            pass

        visible_pool = s.diff_lines[:revealed]
        total_v      = len(visible_pool)
        scroll       = min(s.diff_scroll, max(0, total_v - ih))
        s.diff_scroll = scroll
        visible      = visible_pool[scroll: scroll + ih]

        for row, line in enumerate(visible):
            if line.startswith("+") and not line.startswith("+++"):
                attr = _attr(C_ADD, bold=True)
            elif line.startswith("-") and not line.startswith("---"):
                attr = _attr(C_REMOVE, bold=True)
            elif line.startswith("@@"):
                attr = _attr(C_HEADER, bold=True)
            elif line.startswith("+++") or line.startswith("---"):
                attr = _attr(C_WARN)
            else:
                attr = _attr(C_NORMAL)
            _draw_line(win, row + 1, 1, line, iw - 1, attr)

        _draw_scrollbar(win, scroll, total_v, ih)
        win.noutrefresh()

    # ------------------------------------------------------------------
    # Draw: status bar (3 rows)
    # ------------------------------------------------------------------
    def _draw_bar(self):
        win = self.win_bar
        s   = self.state
        win.erase()
        H, W = win.getmaxyx()

        # row 0: hints left, status right
        hints = " [p]rompt [P]roject [n]ew [d]iff [a]pprove [r]eject [c]opy [v]iew [s]can [q]uit"

        sk = s.status_kind
        if s.busy:
            spin     = SPINNER[s.tick % len(SPINNER)]
            bdots    = BUSY_DOTS[(s.tick // 2) % len(BUSY_DOTS)]
            status_t = f" {spin} {s.busy_msg}{bdots} "
            s_attr   = _attr(C_BUSY, bold=True)
        elif sk == "ok":
            status_t = f" {s.status} "
            s_attr   = _attr(C_ADD, bold=True)
        elif sk == "err":
            status_t = f" {s.status} "
            s_attr   = _attr(C_REMOVE, bold=True)
        elif sk == "warn":
            status_t = f" {s.status} "
            s_attr   = _attr(C_WARN, bold=True)
        else:
            status_t = f" {s.status} "
            s_attr   = _attr(C_STATUS)

        try:
            win.addstr(0, 0, " " * (W - 1), _attr(C_STATUS))
            left_w    = max(0, W - len(status_t) - 1)
            win.addstr(0, 0, hints[:left_w], _attr(C_STATUS))
            win.addstr(0, max(0, W - len(status_t) - 1), status_t, s_attr)
        except curses.error:
            pass

        # row 1: transfer bar when busy, file info otherwise
        # store cursor column so we can reapply it after any row-2 writes
        _input_cursor_x: Optional[int] = None
        if s.input_mode:
            bar_label  = "New file> " if s.input_kind == "newfile" else "Project> " if s.input_kind == "project" else "Prompt> "
            prompt_str = f" {bar_label}{s.input_buf}"
            _input_cursor_x = min(len(prompt_str), W - 2)
            try:
                win.addstr(1, 0, " " * (W - 1), _attr(C_INPUT))
                win.addstr(1, 0, prompt_str[:W - 1], _attr(C_INPUT, bold=True))
                curses.curs_set(1)
                win.move(1, _input_cursor_x)
            except curses.error:
                pass
        elif s.busy and s.stream_chars > 0:
            # transfer bar: [#######   ] recv: 1,247c  "...snippet..."
            bar_w    = min(24, W // 4)
            filled   = min(bar_w, int(bar_w * s.stream_chars / max(1, HAIKU_MAX_TOKENS * 3)))
            bar_str  = "[" + "#" * filled + " " * (bar_w - filled) + "]"
            snippet  = s.stream_snippet.replace("\n", " ").replace("\r", "")[-20:].strip()
            snip_str = f'  "{snippet}"' if snippet else ""
            xfer     = f" {bar_str} recv:{s.stream_chars:,}c{snip_str}"
            try:
                win.addstr(1, 0, " " * (W - 1), _attr(C_BUSY))
                win.addstr(1, 0, xfer[:W - 1], _attr(C_BUSY, bold=True))
                curses.curs_set(0)
            except curses.error:
                pass
        elif s.busy:
            # waiting for connection (no chunks yet)
            spin2  = SPINNER[(s.tick + 2) % len(SPINNER)]
            wait   = f" {spin2} connecting to Haiku API..."
            try:
                win.addstr(1, 0, " " * (W - 1), _attr(C_BUSY))
                win.addstr(1, 0, wait[:W - 1], _attr(C_BUSY, bold=True))
                curses.curs_set(0)
            except curses.error:
                pass
        else:
            curses.curs_set(0)
            try:
                sel_e    = s.selected_entry()
                sel_name = sel_e.name if sel_e else "(none)"
                kind     = "/" if (sel_e and sel_e.is_dir) else ""
                info     = (
                    f" sel: {sel_name}{kind}"
                    f"  |  Tab=panel  Enter/Right=cd  -/Back=up  arrows=scroll"
                )
                win.addstr(1, 0, info[:W - 1], _attr(C_MUTED, dim=True))
            except curses.error:
                pass

        # row 2: view mode notice or blank
        if s.view_mode:
            try:
                msg = " [VIEW MODE -- mouse disabled for terminal copy] "
                win.addstr(2, 0, msg[:W - 1], _attr(C_WARN, bold=True))
            except curses.error:
                pass

        # reapply cursor to row 1 after any row-2 writes -- without this the
        # cursor drifts to row 2 when view_mode and input_mode are both active
        if _input_cursor_x is not None:
            try:
                curses.curs_set(1)
                win.move(1, _input_cursor_x)
            except curses.error:
                pass

        win.noutrefresh()

    # ------------------------------------------------------------------
    # Directory navigation
    # ------------------------------------------------------------------
    def _scan_cwd(self):
        cwd = self.state.cwd
        if not os.path.isdir(cwd):
            self.state.set_status(f"dir not found: {cwd}", "err")
            self.log.error("TUI", f"cwd missing: {cwd}")
            return

        try:
            raw = sorted(os.scandir(cwd), key=lambda e: (not e.is_dir(), e.name.lower()))
        except Exception as exc:
            self.state.set_status(f"scan error: {exc}", "err")
            self.log.error("TUI", f"scandir {cwd}: {exc}")
            return

        entries = []
        for de in raw:
            # skip hidden files and __dunder__ directories (e.g. __pycache__)
            if de.name.startswith(".") or (de.name.startswith("__") and de.name.endswith("__")):
                continue
            try:
                is_dir = de.is_dir(follow_symlinks=False)
                size   = 0 if is_dir else de.stat().st_size
            except Exception as exc:
                self.log.warning("SCANNER", f"stat failed {de.path}: {exc}")
                continue

            pending = 0
            if not is_dir:
                try:
                    pending = len(self.db.get_pending_changes(de.path))
                except Exception:
                    pass

            entries.append(Entry(de.path, de.name, is_dir, size, pending))

        self.state.entries  = entries
        self.state.file_sel = min(self.state.file_sel, max(0, len(entries) - 1))
        self.log.info("SCANNER", f"scanned {cwd}: {len(entries)} entries")
        rel = os.path.relpath(cwd, self.state.root_dir)
        self.state.set_status(f"in ./{rel}/  {len(entries)} entries", "ok")

    def _descend(self):
        e = self.state.selected_entry()
        if not e or not e.is_dir:
            return
        new_cwd = os.path.realpath(e.path)
        self.state.cwd      = new_cwd
        self.state.file_sel = 0
        self.log.info("NAV", f"descend -> {new_cwd}")
        self._scan_cwd()

    def _ascend(self):
        root = os.path.realpath(self.state.root_dir)
        cwd  = os.path.realpath(self.state.cwd)
        if cwd == root:
            self.state.set_status("already at root -- cannot go further back", "warn")
            self.log.warning("NAV", "tried to ascend above root")
            return
        parent = os.path.dirname(cwd)
        if not parent.startswith(root):
            parent = root
        self.state.cwd      = parent
        self.state.file_sel = 0
        self.log.info("NAV", f"ascend -> {parent}")
        self._scan_cwd()

    # ------------------------------------------------------------------
    # Agent actions
    # ------------------------------------------------------------------
    def _enter_prompt(self):
        if not self.state.selected_file():
            self.state.set_status("select a file first (not a directory)", "warn")
            return
        self.state.input_mode = True
        self.state.input_kind = "prompt"
        self.state.input_buf  = ""
        curses.curs_set(1)

    def _launch_project_mode(self):
        """Open input bar to get a project-wide instruction, then run ProjectSession."""
        if _ProjectSession is None:
            self.state.set_status("project_mode not loaded -- check agent.log", "err")
            self.log.error("TUI", "ProjectSession unavailable")
            return
        if self.state.busy:
            self.state.set_status("busy -- wait for current operation to finish", "warn")
            return
        self.state.input_mode = True
        self.state.input_kind = "project"
        self.state.input_buf  = ""
        curses.curs_set(1)
        self.state.set_status(
            f"project mode: type instruction for {os.path.basename(self.state.cwd)}/ then Enter",
            "warn",
        )

    def _enter_newfile(self):
        """Open the input bar so the user can type a new filename to create in cwd."""
        self.state.input_mode = True
        self.state.input_kind = "newfile"
        self.state.input_buf  = ""
        curses.curs_set(1)
        self.state.set_status("type filename then Enter  (Esc to cancel)", "warn")

    def _submit_newfile(self):
        """Create an empty file in the current working directory."""
        name = self.state.input_buf.strip()
        self.state.input_mode = False
        self.state.input_buf  = ""
        curses.curs_set(0)

        if not name:
            self.state.set_status("new file cancelled", "warn")
            return

        # reject path separators -- filename only, no directory traversal
        if os.sep in name or "/" in name or "\\" in name or ".." in name:
            self.state.set_status("invalid filename -- no path separators allowed", "err")
            self.log.error("TUI", f"newfile: rejected unsafe name: {name!r}")
            return

        fpath = os.path.join(self.state.cwd, name)

        # guard: result must still be inside root_dir
        root = os.path.realpath(self.state.root_dir)
        real = os.path.realpath(fpath)
        if not (real == root or real.startswith(root + os.sep)):
            self.state.set_status("path outside root -- blocked", "err")
            self.log.error("TUI", f"newfile outside root: {real}")
            return

        if os.path.exists(fpath):
            self.state.set_status(f"{name} already exists", "warn")
            self.log.warning("TUI", f"newfile: already exists: {fpath}")
            return

        try:
            with open(fpath, "w", encoding="utf-8") as _f:
                pass   # create empty file
            self.log.info("TUI", f"created {fpath}")
            self.state.set_status(f"created {name}", "ok")
        except Exception as exc:
            self.log.error("TUI", f"newfile create failed: {exc}")
            self.state.set_status(f"create failed: {exc}", "err")

        self._scan_cwd()

    def _submit_prompt(self):
        instruction = self.state.input_buf.strip()
        self.state.input_mode = False
        self.state.input_buf  = ""
        curses.curs_set(0)

        if not instruction:
            self.state.set_status("prompt cancelled", "warn")
            return

        fpath = self.state.selected_file()
        if not fpath:
            self.state.set_status("no file selected", "err")
            return

        # safety: file must be inside root_dir
        root = os.path.realpath(self.state.root_dir)
        real = os.path.realpath(fpath)
        if not (real == root or real.startswith(root + os.sep)):
            self.state.set_status("file is outside root -- blocked", "err")
            self.log.error("TUI", f"file outside root: {real}")
            return

        self.state.set_busy(True, "asking Haiku")
        self.state.set_status("", "warn")

        state_ref  = self.state
        log_ref    = self.log

        # --- start rolling commentary (convo.py / GPT-4o mini) -----------
        convo_session = None
        if _ConvoSession is not None:
            try:
                convo_session = _ConvoSession()
                convo_session.start(
                    file_path=fpath,
                    instruction=instruction,
                    on_message=lambda msg: log_ref.info("CHAT", msg),
                )
                self._convo = convo_session
            except Exception as _ce:
                log_ref.warning("TUI", f"convo session could not start: {_ce}")
                convo_session = None
        else:
            log_ref.warning("TUI", "convo module not loaded -- skipping commentary")
        # -----------------------------------------------------------------

        def _on_chunk(chars: int, snippet: str):
            state_ref.set_stream(chars, snippet)

        def _run():
            try:
                with open(fpath, "r", encoding="utf-8", errors="replace") as f:
                    content = f.read()

                new_content, err = _agent.call_haiku(
                    fpath, content, instruction, on_chunk=_on_chunk
                )

                if err:
                    state_ref.set_status(f"agent error: {err[:60]}", "err")
                    log_ref.error("TUI", f"agent error: {err}")
                    return

                change_id = self.merger.propose(fpath, new_content, source_label="haiku-4.5")
                if change_id < 0:
                    state_ref.set_status("merger: failed to propose change", "err")
                    return

                state_ref.pending_change = change_id
                diff = self.merger.preview(fpath)
                state_ref.push_diff(diff)

                if change_id == 0:
                    state_ref.set_status("no changes -- content already matches", "ok")
                else:
                    state_ref.set_status(
                        f"change id={change_id} staged -- [a]pprove or [r]eject", "warn"
                    )

            except Exception as exc:
                log_ref.error("TUI", f"_run thread: {exc}")
                import traceback
                log_ref.error("TUI", traceback.format_exc()[:300])
                state_ref.set_status(f"error: {exc}", "err")
            finally:
                # stop the commentary session when Haiku finishes
                if convo_session is not None:
                    try:
                        convo_session.stop()
                    except Exception as _se:
                        log_ref.debug("TUI", f"convo stop error: {_se}")
                state_ref.set_busy(False)
                self._scan_cwd()

        threading.Thread(target=_run, daemon=True).start()

    def _submit_project(self):
        """Submit the project instruction and launch ProjectSession on the current directory."""
        instruction = self.state.input_buf.strip()
        self.state.input_mode = False
        self.state.input_buf  = ""
        curses.curs_set(0)

        if not instruction:
            self.state.set_status("project instruction cancelled", "warn")
            return

        if _ProjectSession is None:
            self.state.set_status("project_mode not available -- see log", "err")
            self.log.error("TUI", "ProjectSession class is None")
            return

        root_dir   = self.state.cwd
        state_ref  = self.state
        log_ref    = self.log
        merger_ref = self.merger
        db_ref     = self.db

        state_ref.set_busy(True, "project scan + plan")
        state_ref.set_status("", "warn")

        session = _ProjectSession(db=db_ref, merger=merger_ref)
        self._project_session = session

        def _on_log(msg: str):
            log_ref.info("PROJECT", msg)

        def _on_diff(fpath: str, diff_text: str):
            state_ref.push_diff(diff_text)
            state_ref.set_status(
                f"diff staged for {os.path.basename(fpath)} -- [a]pprove or [r]eject",
                "warn",
            )

        def _on_chunk(chars: int, snippet: str):
            state_ref.set_stream(chars, snippet)

        def _on_done(success: bool):
            if success:
                state_ref.set_status("project complete -- review diffs, [a]pprove or [r]eject", "ok")
            else:
                state_ref.set_status("project run complete -- no changes staged", "warn")
            state_ref.set_busy(False)
            self._scan_cwd()

        try:
            session.run(
                root_dir=root_dir,
                instruction=instruction,
                on_log=_on_log,
                on_diff=_on_diff,
                on_done=_on_done,
                on_chunk=_on_chunk,
            )
        except Exception as exc:
            log_ref.error("TUI", f"_submit_project: session.run raised: {exc}")
            print(f"[main] _submit_project session.run error: {exc}", file=sys.stderr)
            state_ref.set_busy(False)
            state_ref.set_status(f"project error: {exc}", "err")

    def _approve(self):
        fpath = self.state.selected_file()
        if not fpath:
            self.state.set_status("no file selected", "err")
            return
        pending = self.db.get_pending_changes(fpath)
        if not pending:
            self.state.set_status("no pending changes", "warn")
            return
        change_id = pending[-1]["id"]
        ok = self.merger.apply(fpath, change_id)
        if ok:
            self.state.set_status(f"applied change id={change_id}", "ok")
            self.state.diff_lines  = []
            self.state.diff_scroll = 0
            self.state.diff_reveal = 0
            self.state.pending_change = -1
        else:
            self.state.set_status("apply failed -- see log", "err")
        self._scan_cwd()

    def _reject(self):
        fpath = self.state.selected_file()
        if not fpath:
            self.state.set_status("no file selected", "err")
            return
        pending = self.db.get_pending_changes(fpath)
        if not pending:
            self.state.set_status("no pending changes", "warn")
            return
        change_id = pending[-1]["id"]
        ok = self.merger.reject(fpath, change_id)
        if ok:
            self.state.set_status(f"rejected change id={change_id}", "ok")
            self.state.diff_lines  = []
            self.state.diff_scroll = 0
            self.state.diff_reveal = 0
        else:
            self.state.set_status("reject failed -- see log", "err")
        self._scan_cwd()

    def _show_diff(self):
        fpath = self.state.selected_file()
        if not fpath:
            self.state.set_status("no file selected", "err")
            return
        diff = self.merger.preview(fpath)
        self.state.push_diff(diff)
        self.state.focus = "diff"
        self.state.set_status(f"diff for {os.path.basename(fpath)}", "ok")

    # ------------------------------------------------------------------
    # Copy
    # ------------------------------------------------------------------
    def _copy_focused(self):
        focus = self.state.focus
        if focus == "log":
            text = "\n".join(self.state.log_lines[-80:]) or "(empty log)"
        elif focus == "diff":
            text = "\n".join(self.state.diff_lines) or "(no diff)"
        else:
            text = "\n".join(e.path for e in self.state.entries) or "(empty)"
        ok = _to_clipboard(text, self.log)
        self.state.set_status(
            "copied to clipboard" if ok else "copy failed (pbcopy)",
            "ok" if ok else "err",
        )

    def _copy_line_at_mouse(self, my: int, mx: int):
        for win, lines_attr, scroll_attr in [
            (self.win_log,  "log_lines",  "log_scroll"),
            (self.win_diff, "diff_lines", "diff_scroll"),
        ]:
            try:
                wy, wx = win.getbegyx()
                wh, ww = win.getmaxyx()
                if wy <= my < wy + wh and wx <= mx < wx + ww:
                    row    = my - wy - 1
                    lines  = getattr(self.state, lines_attr)
                    scroll = getattr(self.state, scroll_attr)
                    idx    = scroll + row
                    if 0 <= idx < len(lines):
                        ok = _to_clipboard(lines[idx], self.log)
                        if ok:
                            self.state.set_status(f"copied line {idx+1}", "ok")
                    return
            except Exception as exc:
                self.log.warning("TUI", f"mouse copy error: {exc}")

    def _toggle_view_mode(self):
        self.state.view_mode = not self.state.view_mode
        if self.state.view_mode:
            try:
                curses.mousemask(0)
            except Exception:
                pass
            self.state.set_status("view mode ON -- mouse disabled", "warn")
        else:
            try:
                curses.mousemask(curses.ALL_MOUSE_EVENTS | curses.REPORT_MOUSE_POSITION)
            except Exception:
                pass
            self.state.set_status("view mode OFF -- mouse restored", "ok")

    # ------------------------------------------------------------------
    # Input bar key handling
    # ------------------------------------------------------------------
    def _handle_input_key(self, key: int):
        if key in (curses.KEY_ENTER, ord("\n"), ord("\r")):
            if self.state.input_kind == "newfile":
                self._submit_newfile()
            elif self.state.input_kind == "project":
                self._submit_project()
            else:
                self._submit_prompt()
        elif key == 27:
            self.state.input_mode = False
            self.state.input_buf  = ""
            curses.curs_set(0)
            kind_labels = {"newfile": "newfile", "project": "project"}
            kind_label = kind_labels.get(self.state.input_kind, "prompt")
            self.state.set_status(f"{kind_label} cancelled")
        elif key in (curses.KEY_BACKSPACE, 127, 8):
            self.state.input_buf = self.state.input_buf[:-1]
        elif 32 <= key <= 126:
            self.state.input_buf += chr(key)

    # ------------------------------------------------------------------
    # Scroll helpers
    # ------------------------------------------------------------------
    def _scroll_log(self, delta: int):
        total = len(self.state.log_lines)
        ih, _ = self._dims(self.win_log)
        self.state.log_scroll = max(0, min(self.state.log_scroll + delta,
                                           max(0, total - ih)))

    def _scroll_diff(self, delta: int):
        revealed = self.state.diff_reveal
        ih, _    = self._dims(self.win_diff)
        self.state.diff_scroll = max(0, min(self.state.diff_scroll + delta,
                                            max(0, revealed - ih)))

    def _scroll_files(self, delta: int):
        count = len(self.state.entries)
        if count:
            self.state.file_sel = (self.state.file_sel + delta) % count

    # ------------------------------------------------------------------
    # Main loop
    # ------------------------------------------------------------------
    def run(self):
        self.setup()
        last_h, last_w = self.stdscr.getmaxyx()

        while True:
            try:
                key = self.stdscr.getch()
            except KeyboardInterrupt:
                break
            except Exception as exc:
                self.log.error("TUI", f"getch error: {exc}")
                key = -1

            try:
                cont = self._handle_key(key)
                if not cont:
                    break
            except Exception as exc:
                self.log.error("TUI", f"handle_key error: {exc}")
                self.state.set_status(f"error: {exc}", "err")

            try:
                H, W = self.stdscr.getmaxyx()
                if H != last_h or W != last_w:
                    last_h, last_w = H, W
                    try:
                        curses.resizeterm(H, W)
                    except Exception:
                        pass
                    self._layout()
            except Exception as exc:
                self.log.warning("TUI", f"resize check: {exc}")

            self._full_redraw()

    def _handle_key(self, key: int) -> bool:
        if key == -1:
            # timer tick -- just let _full_redraw fire the animation
            return True

        if self.state.input_mode:
            self._handle_input_key(key)
            return True

        if key in (ord("q"), ord("Q"), 27):
            return False

        if key == curses.KEY_MOUSE:
            try:
                _, mx, my, _, bstate = curses.getmouse()
                if bstate & curses.BUTTON3_CLICKED:
                    self._copy_line_at_mouse(my, mx)
            except Exception as exc:
                self.log.warning("TUI", f"mouse: {exc}")
            return True

        action_map = {
            ord("p"): self._enter_prompt,
            ord("P"): self._launch_project_mode,
            ord("n"): self._enter_newfile,
            ord("a"): self._approve,
            ord("r"): self._reject,
            ord("d"): self._show_diff,
            ord("c"): self._copy_focused,
            ord("v"): self._toggle_view_mode,
            ord("s"): self._scan_cwd,
        }
        if key in action_map:
            action_map[key]()
            return True

        if key == ord("l"):
            self.state.focus = "log"
            return True

        if key == 9:  # Tab
            cycle = ["files", "log", "diff"]
            idx = cycle.index(self.state.focus) if self.state.focus in cycle else 0
            self.state.focus = cycle[(idx + 1) % len(cycle)]
            return True

        focus = self.state.focus
        if focus == "files":
            if key in (curses.KEY_ENTER, ord("\n"), ord("\r"), curses.KEY_RIGHT):
                e = self.state.selected_entry()
                if e and e.is_dir:
                    self._descend()
                return True
            if key in (curses.KEY_BACKSPACE, 127, 8, ord("-"), curses.KEY_LEFT):
                self._ascend()
                return True

        if key == curses.KEY_UP:
            if focus == "files": self._scroll_files(-1)
            elif focus == "log": self._scroll_log(-1)
            else:                self._scroll_diff(-1)
        elif key == curses.KEY_DOWN:
            if focus == "files": self._scroll_files(1)
            elif focus == "log": self._scroll_log(1)
            else:                self._scroll_diff(1)
        elif key == curses.KEY_PPAGE:
            if focus == "log": self._scroll_log(-10)
            else:              self._scroll_diff(-10)
        elif key == curses.KEY_NPAGE:
            if focus == "log": self._scroll_log(10)
            else:              self._scroll_diff(10)
        elif key == curses.KEY_HOME:
            if focus == "log": self.state.log_scroll  = 0
            else:              self.state.diff_scroll = 0
        elif key == curses.KEY_END:
            if focus == "log":
                ih, _ = self._dims(self.win_log)
                self.state.log_scroll = max(0, len(self.state.log_lines) - ih)
            else:
                ih, _ = self._dims(self.win_diff)
                self.state.diff_scroll = max(0, self.state.diff_reveal - ih)

        return True

    # ------------------------------------------------------------------
    # Cleanup
    # ------------------------------------------------------------------
    def teardown(self):
        try:
            self.db.close()
        except Exception:
            pass
        try:
            self.log.destroy()
        except Exception:
            pass
        try:
            curses.mousemask(0)
            curses.curs_set(1)
        except Exception:
            pass


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main(stdscr):
    if len(sys.argv) > 1:
        root = os.path.realpath(sys.argv[1])
    else:
        # default: one level above tui_agent/ so the user sees
        # the c_tools/ folder (which contains test/ etc.)
        # rather than landing inside tui_agent itself.
        root = os.path.realpath(os.path.join(HERE, ".."))
        print(f"[tui_agent] no path given -- defaulting to {root}", file=sys.stderr)

    if not os.path.isdir(root):
        print(f"[tui_agent] not a directory: {root}", file=sys.stderr)
        sys.exit(1)

    tui = TUI(stdscr, root)
    try:
        tui.run()
    except Exception as exc:
        tui.log.critical("TUI", f"unhandled exception: {exc}")
        raise
    finally:
        tui.teardown()


if __name__ == "__main__":
    try:
        curses.wrapper(main)
    except KeyboardInterrupt:
        pass
    except Exception as exc:
        print(f"[tui_agent] fatal error: {exc}", file=sys.stderr)
        import traceback
        traceback.print_exc(file=sys.stderr)
        sys.exit(1)
