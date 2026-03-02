#!/usr/bin/env python3
"""
main.py  --  c_tools TUI Agent
Curses-based terminal UI that orchestrates:
  scanner -> agent (Haiku) -> emoji_clean -> merger (DB stage) -> apply

Controls
--------
  Arrow keys     navigate file list or focused panel
  Tab            switch focus between LOG and DIFF panels
  p              enter prompt for selected file  (opens input bar)
  d              toggle right panel to DIFF view
  l              toggle right panel to LOG view
  a              approve + apply pending change for selected file
  r              reject pending change for selected file
  c              copy focused panel content to clipboard (pbcopy)
  v              view mode: disable mouse so terminal can natively select text
  q / Esc        quit

Right-click copy
  Right-click anywhere in the log or diff panel to copy that line to clipboard.
"""

import curses
import curses.panel
import os
import sys
import subprocess
import threading
import textwrap
from typing import List, Optional

# ---- ensure tui_agent package is importable when run directly ---------------
sys.path.insert(0, os.path.dirname(__file__))

from log_util   import get_logger
from db         import AgentDB
from scanner    import FolderScanner
from merger     import Merger
import agent as _agent

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
HERE      = os.path.dirname(__file__)
TEST_DIR  = os.path.normpath(os.path.join(HERE, "../test"))
DB_PATH   = os.path.join(HERE, "agent.db")

# ---------------------------------------------------------------------------
# Colours
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
    curses.init_pair(C_MUTED,   curses.COLOR_WHITE,   -1)  # dim via A_DIM
    curses.init_pair(C_WARN,    curses.COLOR_YELLOW,  -1)


# ---------------------------------------------------------------------------
# App state
# ---------------------------------------------------------------------------
class State:
    def __init__(self):
        self.files:          List[str] = []
        self.file_sel:       int       = 0
        self.log_lines:      List[str] = []
        self.diff_lines:     List[str] = []
        self.log_scroll:     int       = 0
        self.diff_scroll:    int       = 0
        self.right_focus:    str       = "log"   # "log" or "diff"
        self.focus:          str       = "files" # "files" | "log" | "diff"
        self.input_mode:     bool      = False
        self.input_buf:      str       = ""
        self.status:         str       = "ready"
        self.status_kind:    str       = ""      # "" | "ok" | "err" | "warn"
        self.view_mode:      bool      = False   # native copy mode
        self.pending_change: int       = -1      # change id
        self.busy:           bool      = False
        self._lock           = threading.Lock()

    def set_status(self, msg: str, kind: str = ""):
        with self._lock:
            self.status      = msg
            self.status_kind = kind

    def push_log(self, lines: List[str]):
        with self._lock:
            self.log_lines = lines

    def push_diff(self, diff: str):
        with self._lock:
            self.diff_lines  = diff.splitlines()
            self.diff_scroll = 0
            self.right_focus = "diff"


# ---------------------------------------------------------------------------
# Drawing helpers
# ---------------------------------------------------------------------------
def _attr(pair: int, bold: bool = False, dim: bool = False) -> int:
    a = curses.color_pair(pair)
    if bold: a |= curses.A_BOLD
    if dim:  a |= curses.A_DIM
    return a

def _draw_box_title(win, title: str, focused: bool = False):
    """Draw a window border with a title in the top-left."""
    try:
        win.box()
        h, w = win.getmaxyx()
        attr = _attr(C_HEADER, bold=focused)
        label = f" {title} "
        win.addstr(0, 2, label[:w - 4], attr)
    except curses.error:
        pass

def _draw_line(win, y: int, x: int, text: str, max_w: int, attr: int):
    try:
        win.addstr(y, x, text[:max_w], attr)
    except curses.error:
        pass

def _draw_scrollbar(win, scroll: int, total: int, visible: int):
    """Draw a simple right-side scroll indicator."""
    try:
        h, w = win.getmaxyx()
        inner_h = h - 2
        if total <= visible:
            return
        bar_h = max(1, inner_h * visible // total)
        bar_y = 1 + (inner_h - bar_h) * scroll // max(1, total - visible)
        for row in range(1, inner_h + 1):
            ch = curses.ACS_CKBOARD if bar_y <= row < bar_y + bar_h else curses.ACS_VLINE
            try:
                win.addch(row, w - 1, ch, _attr(C_MUTED, dim=True))
            except curses.error:
                pass
    except curses.error:
        pass


# ---------------------------------------------------------------------------
# Clipboard
# ---------------------------------------------------------------------------
def _to_clipboard(text: str, log):
    try:
        proc = subprocess.run(["pbcopy"], input=text.encode("utf-8"),
                              check=True, capture_output=True)
        log.info("CLIP", f"copied {len(text)} chars to clipboard")
        return True
    except Exception as exc:
        log.error("CLIP", f"pbcopy failed: {exc}")
        return False


# ---------------------------------------------------------------------------
# Main TUI class
# ---------------------------------------------------------------------------
class TUI:
    def __init__(self, stdscr):
        self.stdscr = stdscr
        self.state  = State()
        self.log    = get_logger()
        self.db     = AgentDB(DB_PATH)
        self.merger = Merger(self.db)

        # windows (created in _layout)
        self.win_files  = None
        self.win_log    = None
        self.win_diff   = None
        self.win_bar    = None

        # scanner
        self._scanner = FolderScanner(recursive=False)

    # ------------------------------------------------------------------
    # Setup
    # ------------------------------------------------------------------
    def setup(self):
        _init_colors()
        curses.curs_set(0)
        curses.noecho()
        curses.cbreak()
        self.stdscr.keypad(True)
        self.stdscr.nodelay(False)
        curses.mousemask(
            curses.ALL_MOUSE_EVENTS | curses.REPORT_MOUSE_POSITION
        )

        if not self.db.open():
            self.state.set_status("DB open failed -- check agent.log", "err")
            self.log.error("TUI", "db.open() failed")
        else:
            self._refresh_files()

        self._layout()
        self._full_redraw()

    # ------------------------------------------------------------------
    # Layout
    # ------------------------------------------------------------------
    def _layout(self):
        H, W = self.stdscr.getmaxyx()
        bar_h    = 3
        left_w   = max(20, W // 3)
        right_w  = W - left_w
        right_h  = H - bar_h
        log_h    = max(4, right_h * 6 // 10)
        diff_h   = right_h - log_h

        self.win_files = curses.newwin(right_h, left_w,        0,      0)
        self.win_log   = curses.newwin(log_h,   right_w,       0,      left_w)
        self.win_diff  = curses.newwin(diff_h,  right_w,       log_h,  left_w)
        self.win_bar   = curses.newwin(bar_h,   W,             right_h, 0)

        self.win_files.keypad(True)
        self.win_log.keypad(True)
        self.win_diff.keypad(True)

    def _dims(self, win):
        h, w = win.getmaxyx()
        return h - 2, w - 2   # inner (excluding border)

    # ------------------------------------------------------------------
    # Redraw
    # ------------------------------------------------------------------
    def _full_redraw(self):
        self.stdscr.noutrefresh()
        self._draw_files()
        self._draw_log()
        self._draw_diff()
        self._draw_bar()
        curses.doupdate()

    def _draw_files(self):
        win = self.win_files
        win.erase()
        focused = self.state.focus == "files"
        _draw_box_title(win, "FILES : test/", focused)
        ih, iw = self._dims(win)

        stats = self.db.stats()
        pending_total = stats.get("pending", 0)

        for i, fpath in enumerate(self.state.files):
            if i >= ih:
                break
            name   = os.path.basename(fpath)
            is_sel = (i == self.state.file_sel)

            # check pending changes
            pcount = len(self.db.get_pending_changes(fpath))
            suffix = f" [{pcount}P]" if pcount else ""
            label  = f"  {name}{suffix}"

            if is_sel:
                attr = _attr(C_SELECT, bold=True)
                win.addstr(i + 1, 1, " " * iw, attr)
                _draw_line(win, i + 1, 1, label, iw, attr)
            else:
                attr = _attr(C_NORMAL)
                _draw_line(win, i + 1, 1, label, iw, attr)

        # DB stats footer
        stat_str = f" db: {pending_total}P "
        _draw_line(win, ih + 1, 1, stat_str, iw, _attr(C_MUTED, dim=True))
        win.noutrefresh()

    def _draw_log(self):
        win    = self.win_log
        win.erase()
        focused = self.state.focus == "log"
        _draw_box_title(win, "LOG", focused)
        ih, iw = self._dims(win)

        # Refresh log lines from logger
        self.state.log_lines = self.log.get_entries(500)
        lines  = self.state.log_lines
        total  = len(lines)
        scroll = min(self.state.log_scroll, max(0, total - ih))
        self.state.log_scroll = scroll
        visible = lines[scroll: scroll + ih]

        for row, line in enumerate(visible):
            # Colour by level prefix
            if " ERR " in line or " CRT " in line:
                attr = _attr(C_REMOVE)
            elif " WRN " in line:
                attr = _attr(C_WARN)
            elif " INF " in line:
                attr = _attr(C_NORMAL)
            else:
                attr = _attr(C_MUTED, dim=True)
            _draw_line(win, row + 1, 1, line, iw, attr)

        _draw_scrollbar(win, scroll, total, ih)
        win.noutrefresh()

    def _draw_diff(self):
        win = self.win_diff
        win.erase()
        focused = self.state.focus == "diff"
        right_f = self.state.right_focus
        _draw_box_title(win, "DIFF" if right_f == "diff" else "DIFF (empty)", focused)
        ih, iw = self._dims(win)

        lines  = self.state.diff_lines
        total  = len(lines)
        scroll = min(self.state.diff_scroll, max(0, total - ih))
        self.state.diff_scroll = scroll
        visible = lines[scroll: scroll + ih]

        for row, line in enumerate(visible):
            if line.startswith("+") and not line.startswith("+++"):
                attr = _attr(C_ADD, bold=True)
            elif line.startswith("-") and not line.startswith("---"):
                attr = _attr(C_REMOVE, bold=True)
            elif line.startswith("@@"):
                attr = _attr(C_HEADER)
            elif line.startswith("+++") or line.startswith("---"):
                attr = _attr(C_WARN)
            else:
                attr = _attr(C_NORMAL)
            _draw_line(win, row + 1, 1, line, iw, attr)

        _draw_scrollbar(win, scroll, total, ih)
        win.noutrefresh()

    def _draw_bar(self):
        win = self.win_bar
        win.erase()
        H, W = win.getmaxyx()

        hints = " [p]rompt  [d]iff  [a]pprove  [r]eject  [c]opy  [v]iew  [q]uit"

        # status line
        sk = self.state.status_kind
        if sk == "ok":   s_attr = _attr(C_ADD,    bold=True)
        elif sk == "err": s_attr = _attr(C_REMOVE, bold=True)
        elif sk == "warn":s_attr = _attr(C_WARN,   bold=True)
        else:             s_attr = _attr(C_STATUS)

        status_text = f" {self.state.status} "
        if self.state.busy:
            status_text = " working... "

        try:
            win.addstr(0, 0, " " * (W - 1), _attr(C_STATUS))
            win.addstr(0, 0, hints[:W - len(status_text) - 1], _attr(C_STATUS))
            win.addstr(0, W - len(status_text) - 1, status_text, s_attr)
        except curses.error:
            pass

        # input line
        if self.state.input_mode:
            prompt_str = f" Prompt> {self.state.input_buf}"
            cursor_x   = min(len(prompt_str), W - 2)
            try:
                win.addstr(1, 0, " " * (W - 1), _attr(C_INPUT))
                win.addstr(1, 0, prompt_str[:W - 1], _attr(C_INPUT, bold=True))
                curses.curs_set(1)
                win.move(1, cursor_x)
            except curses.error:
                pass
        else:
            try:
                sel_name = ""
                if self.state.files and 0 <= self.state.file_sel < len(self.state.files):
                    sel_name = os.path.basename(self.state.files[self.state.file_sel])
                info = f" selected: {sel_name}  |  Tab=switch panel  |  arrows=scroll"
                win.addstr(1, 0, info[:W - 1], _attr(C_MUTED, dim=True))
                curses.curs_set(0)
            except curses.error:
                pass

        # view mode indicator
        if self.state.view_mode:
            try:
                vm_text = "[VIEW MODE - native copy active]"
                win.addstr(2, 0, vm_text[:W - 1], _attr(C_WARN, bold=True))
            except curses.error:
                pass

        win.noutrefresh()

    # ------------------------------------------------------------------
    # File list
    # ------------------------------------------------------------------
    def _refresh_files(self):
        if not os.path.isdir(TEST_DIR):
            self.state.set_status(f"test dir not found: {TEST_DIR}", "err")
            self.log.error("TUI", f"test dir missing: {TEST_DIR}")
            return
        self._scanner.scan(TEST_DIR)
        entries = self._scanner.get_files()
        self.state.files = [fe.path for fe in entries]
        self.log.info("SCANNER", f"found {len(self.state.files)} file(s) in test/")
        self.state.set_status(f"scanned test/ -- {len(self.state.files)} files", "ok")

    def _selected_file(self) -> Optional[str]:
        files = self.state.files
        if not files:
            return None
        idx = self.state.file_sel
        if 0 <= idx < len(files):
            return files[idx]
        return None

    # ------------------------------------------------------------------
    # Actions
    # ------------------------------------------------------------------
    def _enter_prompt(self):
        self.state.input_mode = True
        self.state.input_buf  = ""
        curses.curs_set(1)
        self._full_redraw()

    def _submit_prompt(self):
        instruction = self.state.input_buf.strip()
        self.state.input_mode = False
        self.state.input_buf  = ""
        curses.curs_set(0)

        if not instruction:
            self.state.set_status("prompt cancelled", "warn")
            return

        fpath = self._selected_file()
        if not fpath:
            self.state.set_status("no file selected", "err")
            return

        self.state.busy = True
        self.state.set_status(f"asking Haiku about {os.path.basename(fpath)}...", "warn")
        self._full_redraw()

        def _run():
            try:
                with open(fpath, "r", encoding="utf-8", errors="replace") as f:
                    content = f.read()

                new_content, err = _agent.call_haiku(fpath, content, instruction)

                if err:
                    self.state.set_status(f"agent error: {err[:60]}", "err")
                    self.log.error("TUI", f"agent error: {err}")
                    self.state.busy = False
                    return

                change_id = self.merger.propose(fpath, new_content,
                                                 source_label="haiku-4.5")
                if change_id < 0:
                    self.state.set_status("merger: failed to propose change", "err")
                    self.state.busy = False
                    return

                self.state.pending_change = change_id
                diff = self.merger.preview(fpath)
                self.state.push_diff(diff)

                if change_id == 0:
                    self.state.set_status("no changes -- file already matches", "ok")
                else:
                    self.state.set_status(
                        f"change id={change_id} staged -- [a]pprove or [r]eject", "warn"
                    )

            except Exception as exc:
                self.log.error("TUI", f"_run thread error: {exc}")
                self.state.set_status(f"error: {exc}", "err")
            finally:
                self.state.busy = False

        t = threading.Thread(target=_run, daemon=True)
        t.start()
        t.join()  # block so curses loop is simple
        self._refresh_files()

    def _approve(self):
        fpath = self._selected_file()
        if not fpath:
            self.state.set_status("no file selected", "err")
            return

        pending = self.db.get_pending_changes(fpath)
        if not pending:
            self.state.set_status("no pending changes to approve", "warn")
            return

        change_id = pending[-1]["id"]
        ok = self.merger.apply(fpath, change_id)
        if ok:
            self.state.set_status(f"applied change id={change_id}", "ok")
            self.state.diff_lines  = []
            self.state.diff_scroll = 0
            self.state.pending_change = -1
        else:
            self.state.set_status("apply failed -- see log", "err")

        self._refresh_files()

    def _reject(self):
        fpath = self._selected_file()
        if not fpath:
            self.state.set_status("no file selected", "err")
            return

        pending = self.db.get_pending_changes(fpath)
        if not pending:
            self.state.set_status("no pending changes to reject", "warn")
            return

        change_id = pending[-1]["id"]
        ok = self.merger.reject(fpath, change_id)
        if ok:
            self.state.set_status(f"rejected change id={change_id}", "ok")
            self.state.diff_lines  = []
            self.state.diff_scroll = 0
        else:
            self.state.set_status("reject failed -- see log", "err")

        self._refresh_files()

    def _show_diff(self):
        fpath = self._selected_file()
        if not fpath:
            self.state.set_status("no file selected", "err")
            return
        diff = self.merger.preview(fpath)
        self.state.push_diff(diff)
        self.state.focus = "diff"
        self.state.set_status(f"diff for {os.path.basename(fpath)}", "ok")

    def _copy_focused(self):
        if self.state.focus in ("log", "files"):
            lines = self.state.log_lines
            text  = "\n".join(lines[-50:]) if lines else "(empty log)"
        else:
            lines = self.state.diff_lines
            text  = "\n".join(lines) if lines else "(no diff)"

        ok = _to_clipboard(text, self.log)
        if ok:
            self.state.set_status("copied to clipboard", "ok")
        else:
            self.state.set_status("copy failed (pbcopy)", "err")

    def _copy_line_at_mouse(self, my: int, mx: int):
        """Copy the log or diff line at the given screen coordinates."""
        # Determine which panel was clicked
        win_log_y,  win_log_x  = self.win_log.getbegyx()
        win_log_h,  win_log_w  = self.win_log.getmaxyx()
        win_diff_y, win_diff_x = self.win_diff.getbegyx()
        win_diff_h, win_diff_w = self.win_diff.getmaxyx()

        if (win_log_y <= my < win_log_y + win_log_h and
                win_log_x <= mx < win_log_x + win_log_w):
            row    = my - win_log_y - 1
            lines  = self.state.log_lines
            scroll = self.state.log_scroll
        elif (win_diff_y <= my < win_diff_y + win_diff_h and
                win_diff_x <= mx < win_diff_x + win_diff_w):
            row    = my - win_diff_y - 1
            lines  = self.state.diff_lines
            scroll = self.state.diff_scroll
        else:
            return

        idx = scroll + row
        if 0 <= idx < len(lines):
            ok = _to_clipboard(lines[idx], self.log)
            if ok:
                self.state.set_status(f"copied line {idx+1}", "ok")

    def _toggle_view_mode(self):
        self.state.view_mode = not self.state.view_mode
        if self.state.view_mode:
            curses.mousemask(0)
            self.state.set_status("view mode ON -- terminal copy enabled", "warn")
        else:
            curses.mousemask(curses.ALL_MOUSE_EVENTS | curses.REPORT_MOUSE_POSITION)
            self.state.set_status("view mode OFF -- mouse restored", "ok")

    # ------------------------------------------------------------------
    # Input line editing
    # ------------------------------------------------------------------
    def _handle_input_key(self, key: int) -> bool:
        """Returns True to stay in input mode, False to exit."""
        if key in (curses.KEY_ENTER, ord("\n"), ord("\r")):
            self._submit_prompt()
            return False
        if key == 27:  # Esc
            self.state.input_mode = False
            self.state.input_buf  = ""
            curses.curs_set(0)
            self.state.set_status("prompt cancelled")
            return False
        if key in (curses.KEY_BACKSPACE, 127, 8):
            self.state.input_buf = self.state.input_buf[:-1]
        elif 32 <= key <= 126:
            self.state.input_buf += chr(key)
        return True

    # ------------------------------------------------------------------
    # Scroll helpers
    # ------------------------------------------------------------------
    def _scroll_log(self, delta: int):
        total   = len(self.state.log_lines)
        ih, _   = self._dims(self.win_log)
        self.state.log_scroll = max(0, min(self.state.log_scroll + delta,
                                           max(0, total - ih)))

    def _scroll_diff(self, delta: int):
        total   = len(self.state.diff_lines)
        ih, _   = self._dims(self.win_diff)
        self.state.diff_scroll = max(0, min(self.state.diff_scroll + delta,
                                            max(0, total - ih)))

    def _scroll_files(self, delta: int):
        count = len(self.state.files)
        if count == 0:
            return
        self.state.file_sel = (self.state.file_sel + delta) % count

    # ------------------------------------------------------------------
    # Main loop
    # ------------------------------------------------------------------
    def run(self):
        self.setup()
        while True:
            try:
                key = self.stdscr.getch()
            except KeyboardInterrupt:
                break
            except Exception as exc:
                self.log.error("TUI", f"getch error: {exc}")
                continue

            try:
                cont = self._handle_key(key)
                if not cont:
                    break
            except Exception as exc:
                self.log.error("TUI", f"handle_key error: {exc}")
                self.state.set_status(f"error: {exc}", "err")

            # Resize
            H, W = self.stdscr.getmaxyx()
            try:
                new_h, new_w = self.win_files.getmaxyx()
            except Exception:
                new_h, new_w = 0, 0
            if H != new_h or W != new_w:
                try:
                    curses.resizeterm(H, W)
                    self._layout()
                except Exception as exc:
                    self.log.warning("TUI", f"resize failed: {exc}")

            self._full_redraw()

    def _handle_key(self, key: int) -> bool:
        # --- input mode -----------------------------------------------------
        if self.state.input_mode:
            self._handle_input_key(key)
            return True

        # --- global ---------------------------------------------------------
        if key in (ord("q"), ord("Q"), 27):
            return False   # quit

        if key == curses.KEY_MOUSE:
            try:
                _, mx, my, _, bstate = curses.getmouse()
                # Right click (button 3)
                if bstate & curses.BUTTON3_CLICKED:
                    self._copy_line_at_mouse(my, mx)
            except Exception as exc:
                self.log.warning("TUI", f"mouse event error: {exc}")
            return True

        if key == ord("v"):
            self._toggle_view_mode()
            return True

        if key == ord("c"):
            self._copy_focused()
            return True

        if key == ord("p"):
            self._enter_prompt()
            return True

        if key == ord("a"):
            self._approve()
            return True

        if key == ord("r"):
            self._reject()
            return True

        if key == ord("d"):
            self._show_diff()
            return True

        if key == ord("l"):
            self.state.focus = "log"
            self.state.right_focus = "log"
            return True

        if key == ord("s"):
            self._refresh_files()
            return True

        if key == 9:  # Tab
            cycle = ["files", "log", "diff"]
            idx   = cycle.index(self.state.focus) if self.state.focus in cycle else 0
            self.state.focus = cycle[(idx + 1) % len(cycle)]
            return True

        # --- arrow keys (context-sensitive) ---------------------------------
        focus = self.state.focus
        if key == curses.KEY_UP:
            if focus == "files":  self._scroll_files(-1)
            elif focus == "log":  self._scroll_log(-1)
            else:                 self._scroll_diff(-1)
        elif key == curses.KEY_DOWN:
            if focus == "files":  self._scroll_files(1)
            elif focus == "log":  self._scroll_log(1)
            else:                 self._scroll_diff(1)
        elif key == curses.KEY_PPAGE:   # Page Up
            if focus == "log":   self._scroll_log(-10)
            else:                self._scroll_diff(-10)
        elif key == curses.KEY_NPAGE:   # Page Down
            if focus == "log":   self._scroll_log(10)
            else:                self._scroll_diff(10)
        elif key == curses.KEY_END:
            if focus == "log":
                total = len(self.state.log_lines)
                ih, _ = self._dims(self.win_log)
                self.state.log_scroll = max(0, total - ih)
            else:
                total = len(self.state.diff_lines)
                ih, _ = self._dims(self.win_diff)
                self.state.diff_scroll = max(0, total - ih)
        elif key == curses.KEY_HOME:
            if focus == "log":   self.state.log_scroll  = 0
            else:                self.state.diff_scroll = 0

        return True

    # ------------------------------------------------------------------
    # Cleanup
    # ------------------------------------------------------------------
    def teardown(self):
        self.db.close()
        self.log.destroy()
        try:
            curses.mousemask(0)
            curses.curs_set(1)
        except Exception:
            pass


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main(stdscr):
    tui = TUI(stdscr)
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
