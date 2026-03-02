"""
merger.py
Python mirror of merger.h / merger.c

Proposes changes via the DB, shows diffs, applies approved changes.
All changes are staged in SQLite before touching the filesystem.
The verify-before-write principle mirrors the C implementation.
"""

import os
import difflib
from typing import List, Optional, Tuple, Dict

from log_util import get_logger
from db import AgentDB


class Merger:
    def __init__(self, db: AgentDB):
        self._db  = db
        self._log = get_logger()

    # ------------------------------------------------------------------
    # Propose
    # ------------------------------------------------------------------
    def propose(self, target_file: str, new_content: str,
                source_label: str = "agent") -> int:
        """
        Read the current file, compute a whole-file change, store in DB.
        Returns the change id or -1 on failure.
        """
        if not os.path.isfile(target_file):
            self._log.error("MERGER", f"target not found: {target_file}")
            return -1

        old_content = self._read_file(target_file)
        if old_content is None:
            return -1

        if old_content == new_content:
            self._log.info("MERGER", f"no changes detected for {target_file}")
            return 0  # 0 = nothing to do

        change_id = self._db.add_merge_change(
            source_file=source_label,
            target_file=target_file,
            line_number=0,
            old_content=old_content,
            new_content=new_content,
        )
        if change_id < 0:
            self._log.error("MERGER", "failed to store change in db")
            return -1

        self._log.info("MERGER", f"proposed change id={change_id} for {os.path.basename(target_file)}")
        return change_id

    # ------------------------------------------------------------------
    # Preview / diff
    # ------------------------------------------------------------------
    def preview(self, target_file: str) -> str:
        """Return a unified diff string for all pending changes on the file."""
        pending = self._db.get_pending_changes(target_file)
        if not pending:
            return "(no pending changes)"

        # Take the latest pending change
        change = pending[-1]
        old_lines = (change["old_content"] or "").splitlines(keepends=True)
        new_lines = (change["new_content"] or "").splitlines(keepends=True)
        fname = os.path.basename(target_file)

        diff = list(difflib.unified_diff(
            old_lines, new_lines,
            fromfile=f"a/{fname}", tofile=f"b/{fname}",
            lineterm="",
        ))
        if not diff:
            return "(no textual difference)"
        return "\n".join(diff)

    def pending_count(self, target_file: str) -> int:
        return len(self._db.get_pending_changes(target_file))

    # ------------------------------------------------------------------
    # Apply
    # ------------------------------------------------------------------
    def apply(self, target_file: str, change_id: int) -> bool:
        """
        Verify the change exists and is approved, then write to disk.
        Returns True on success.
        """
        # 1. Mark approved
        if not self._db.approve_change(change_id, True):
            self._log.error("MERGER", f"could not approve change id={change_id}")
            return False

        # 2. Fetch approved changes
        approved = self._db.get_approved_changes(target_file)
        if not approved:
            self._log.error("MERGER", f"no approved changes found for {target_file}")
            return False

        # 3. Verify target still exists before write
        if not os.path.isfile(target_file):
            self._log.error("MERGER", f"target disappeared before write: {target_file}")
            return False

        # 4. Use the latest approved change
        change = approved[-1]
        new_content = change["new_content"] or ""

        # 5. Write
        if not self._write_file(target_file, new_content):
            self._log.error("MERGER", f"write failed: {target_file}")
            return False

        # 6. Clean up applied records
        self._db.clear_applied_changes(target_file)
        self._log.info("MERGER", f"applied change id={change_id} to {os.path.basename(target_file)}")
        return True

    def reject(self, target_file: str, change_id: int) -> bool:
        ok = self._db.approve_change(change_id, False)
        if ok:
            self._log.info("MERGER", f"rejected change id={change_id}")
        return ok

    # ------------------------------------------------------------------
    # File I/O
    # ------------------------------------------------------------------
    def _read_file(self, path: str) -> Optional[str]:
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
            self._log.debug("MERGER", f"read {len(content)} chars from {os.path.basename(path)}")
            return content
        except Exception as exc:
            self._log.error("MERGER", f"read failed {path}: {exc}")
            return None

    def _write_file(self, path: str, content: str) -> bool:
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(content)
            self._log.info("MERGER", f"wrote {len(content)} chars to {os.path.basename(path)}")
            return True
        except Exception as exc:
            self._log.error("MERGER", f"write failed {path}: {exc}")
            return False
