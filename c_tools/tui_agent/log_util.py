"""
log_util.py
Python mirror of logger.h / logger.c
Provides a thread-safe in-memory + file logger with level filtering.
All modules use get_logger() to share one instance.
"""

import os
import sys
import time
import datetime
import threading
from enum import IntEnum


class Level(IntEnum):
    DEBUG    = 0
    INFO     = 1
    WARNING  = 2
    ERROR    = 3
    CRITICAL = 4


LEVEL_LABELS = {
    Level.DEBUG:    "DBG",
    Level.INFO:     "INF",
    Level.WARNING:  "WRN",
    Level.ERROR:    "ERR",
    Level.CRITICAL: "CRT",
}


class Logger:
    def __init__(self, log_path=None, min_level=Level.DEBUG, console=False):
        self.min_level     = min_level
        self.console       = console
        self.log_path      = log_path
        self.entries       = []          # list of (timestamp, level, module, msg)
        self._lock         = threading.Lock()
        self._file         = None
        self.entry_count   = 0

        if log_path:
            try:
                os.makedirs(os.path.dirname(os.path.abspath(log_path)), exist_ok=True)
                self._file = open(log_path, "a", encoding="utf-8")
            except Exception as exc:
                print(f"[log_util] could not open log file {log_path}: {exc}", file=sys.stderr)

    # ------------------------------------------------------------------
    def log(self, level: Level, module: str, msg: str):
        if level < self.min_level:
            return
        ts = datetime.datetime.now().strftime("%H:%M:%S")
        label = LEVEL_LABELS.get(level, "???")
        line  = f"[{ts}] {label} [{module}] {msg}"

        with self._lock:
            self.entries.append((ts, level, module, msg))
            self.entry_count += 1
            if len(self.entries) > 2000:
                self.entries = self.entries[-1500:]  # trim old entries

        if self.console:
            print(line, file=sys.stderr)

        if self._file:
            try:
                self._file.write(line + "\n")
                self._file.flush()
            except Exception:
                pass

    def debug(self,    module, msg): self.log(Level.DEBUG,    module, msg)
    def info(self,     module, msg): self.log(Level.INFO,     module, msg)
    def warning(self,  module, msg): self.log(Level.WARNING,  module, msg)
    def error(self,    module, msg): self.log(Level.ERROR,    module, msg)
    def critical(self, module, msg): self.log(Level.CRITICAL, module, msg)

    def get_entries(self, max_entries=200):
        """Return the last N log entries as formatted strings."""
        with self._lock:
            recent = self.entries[-max_entries:]
        lines = []
        for ts, level, module, msg in recent:
            lines.append(f"[{ts}] {LEVEL_LABELS[level]} [{module}] {msg}")
        return lines

    def destroy(self):
        if self._file:
            try:
                self._file.close()
            except Exception:
                pass
            self._file = None


# Module-level singleton
_logger = None

def get_logger(log_path=None):
    global _logger
    if _logger is None:
        _logger = Logger(
            log_path=log_path or os.path.join(os.path.dirname(__file__), "agent.log"),
            min_level=Level.DEBUG,
            console=False,
        )
    return _logger
