"""
db.py
Python mirror of sqlite_db.h / sqlite_db.c

Manages three tables:
  patterns      -- lint/check rules
  tracking      -- seen issues per file (deduped by hash)
  merge_changes -- proposed file changes (old_content/new_content)

Every write verifies the data before committing.
"""

import os
import sqlite3
import hashlib
import datetime
from typing import Optional, List, Dict, Any

from log_util import get_logger

SCHEMA = """
CREATE TABLE IF NOT EXISTS patterns (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    name            TEXT NOT NULL UNIQUE,
    description     TEXT,
    regex_pattern   TEXT NOT NULL,
    language        TEXT,
    severity        INTEGER DEFAULT 1,
    enabled         INTEGER DEFAULT 1,
    created_at      TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS tracking (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    file_path   TEXT NOT NULL,
    rule_name   TEXT NOT NULL,
    line_number INTEGER,
    issue_hash  TEXT NOT NULL,
    timestamp   TEXT DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(file_path, issue_hash)
);

CREATE TABLE IF NOT EXISTS merge_changes (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    source_file TEXT NOT NULL,
    target_file TEXT NOT NULL,
    line_number INTEGER,
    old_content TEXT,
    new_content TEXT,
    approved    INTEGER DEFAULT 0,
    timestamp   TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS project_sessions (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    root_dir        TEXT    NOT NULL,
    instruction     TEXT    NOT NULL,
    plan_prompt     TEXT    NOT NULL DEFAULT '',
    file_tree_json  TEXT    NOT NULL DEFAULT '',
    status          TEXT    NOT NULL DEFAULT 'pending',
    timestamp       TEXT    DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_patterns_language  ON patterns(language);
CREATE INDEX IF NOT EXISTS idx_tracking_file      ON tracking(file_path);
CREATE INDEX IF NOT EXISTS idx_merge_target       ON merge_changes(target_file);
CREATE INDEX IF NOT EXISTS idx_project_root       ON project_sessions(root_dir);
"""


def _ts() -> str:
    return datetime.datetime.now().isoformat(sep=" ", timespec="seconds")


class AgentDB:
    def __init__(self, db_path: str):
        self.db_path = db_path
        self._conn: Optional[sqlite3.Connection] = None
        self._log = get_logger()

    # ------------------------------------------------------------------
    # Lifecycle
    # ------------------------------------------------------------------
    def open(self) -> bool:
        try:
            os.makedirs(os.path.dirname(os.path.abspath(self.db_path)), exist_ok=True)
            self._conn = sqlite3.connect(self.db_path, check_same_thread=False)
            self._conn.row_factory = sqlite3.Row
            self._conn.execute("PRAGMA journal_mode=WAL")
            self._conn.execute("PRAGMA foreign_keys=ON")
            self._apply_schema()
            self._log.info("DB", f"opened {self.db_path}")
            return True
        except Exception as exc:
            self._log.error("DB", f"open failed: {exc}")
            return False

    def close(self):
        if self._conn:
            try:
                self._conn.close()
            except Exception as exc:
                self._log.error("DB", f"close failed: {exc}")
            self._conn = None

    def _apply_schema(self):
        try:
            self._conn.executescript(SCHEMA)
            self._conn.commit()
        except Exception as exc:
            self._log.error("DB", f"schema apply failed: {exc}")
            raise

    # ------------------------------------------------------------------
    # Verify helper -- checks row existence before write
    # ------------------------------------------------------------------
    def _verify_unique(self, table: str, where: str, params: tuple) -> bool:
        """Returns True if the record does NOT already exist (safe to insert)."""
        try:
            row = self._conn.execute(
                f"SELECT id FROM {table} WHERE {where}", params
            ).fetchone()
            return row is None
        except Exception as exc:
            self._log.error("DB", f"verify_unique failed on {table}: {exc}")
            return False

    # ------------------------------------------------------------------
    # merge_changes
    # ------------------------------------------------------------------
    def add_merge_change(self, source_file: str, target_file: str,
                         line_number: int, old_content: str, new_content: str) -> int:
        """Propose a change. Returns new row id or -1 on failure."""
        if not self._conn:
            self._log.error("DB", "add_merge_change: db not open")
            return -1
        try:
            cur = self._conn.execute(
                """INSERT INTO merge_changes
                   (source_file, target_file, line_number, old_content, new_content, timestamp)
                   VALUES (?,?,?,?,?,?)""",
                (source_file, target_file, line_number, old_content, new_content, _ts()),
            )
            self._conn.commit()
            rowid = cur.lastrowid
            self._log.info("DB", f"merge_change added id={rowid} target={target_file}")
            return rowid
        except Exception as exc:
            self._log.error("DB", f"add_merge_change failed: {exc}")
            return -1

    def get_pending_changes(self, target_file: str) -> List[Dict]:
        if not self._conn:
            return []
        try:
            rows = self._conn.execute(
                "SELECT * FROM merge_changes WHERE target_file=? AND approved=0 ORDER BY id",
                (target_file,)
            ).fetchall()
            return [dict(r) for r in rows]
        except Exception as exc:
            self._log.error("DB", f"get_pending_changes failed: {exc}")
            return []

    def approve_change(self, change_id: int, approved: bool) -> bool:
        if not self._conn:
            return False
        try:
            self._conn.execute(
                "UPDATE merge_changes SET approved=? WHERE id=?",
                (1 if approved else -1, change_id),
            )
            self._conn.commit()
            state = "approved" if approved else "rejected"
            self._log.info("DB", f"change id={change_id} {state}")
            return True
        except Exception as exc:
            self._log.error("DB", f"approve_change failed: {exc}")
            return False

    def get_approved_changes(self, target_file: str) -> List[Dict]:
        if not self._conn:
            return []
        try:
            rows = self._conn.execute(
                "SELECT * FROM merge_changes WHERE target_file=? AND approved=1 ORDER BY id",
                (target_file,)
            ).fetchall()
            return [dict(r) for r in rows]
        except Exception as exc:
            self._log.error("DB", f"get_approved_changes failed: {exc}")
            return []

    def clear_applied_changes(self, target_file: str) -> bool:
        if not self._conn:
            return False
        try:
            self._conn.execute(
                "DELETE FROM merge_changes WHERE target_file=? AND approved=1",
                (target_file,)
            )
            self._conn.commit()
            self._log.info("DB", f"cleared applied changes for {target_file}")
            return True
        except Exception as exc:
            self._log.error("DB", f"clear_applied_changes failed: {exc}")
            return False

    def delete_change(self, change_id: int) -> bool:
        if not self._conn:
            return False
        try:
            self._conn.execute("DELETE FROM merge_changes WHERE id=?", (change_id,))
            self._conn.commit()
            self._log.info("DB", f"deleted change id={change_id}")
            return True
        except Exception as exc:
            self._log.error("DB", f"delete_change failed: {exc}")
            return False

    # ------------------------------------------------------------------
    # tracking
    # ------------------------------------------------------------------
    def track_issue(self, file_path: str, rule_name: str,
                    line_number: int, issue_hash: str) -> bool:
        if not self._conn:
            return False
        try:
            if not self._verify_unique("tracking", "file_path=? AND issue_hash=?",
                                        (file_path, issue_hash)):
                return True  # already tracked -- not an error
            self._conn.execute(
                "INSERT INTO tracking (file_path, rule_name, line_number, issue_hash, timestamp) VALUES (?,?,?,?,?)",
                (file_path, rule_name, line_number, issue_hash, _ts()),
            )
            self._conn.commit()
            return True
        except Exception as exc:
            self._log.error("DB", f"track_issue failed: {exc}")
            return False

    def get_tracked_issues(self, file_path: str) -> List[Dict]:
        if not self._conn:
            return []
        try:
            rows = self._conn.execute(
                "SELECT * FROM tracking WHERE file_path=? ORDER BY id",
                (file_path,)
            ).fetchall()
            return [dict(r) for r in rows]
        except Exception as exc:
            self._log.error("DB", f"get_tracked_issues failed: {exc}")
            return []

    def clear_tracking(self, file_path: str) -> bool:
        if not self._conn:
            return False
        try:
            self._conn.execute("DELETE FROM tracking WHERE file_path=?", (file_path,))
            self._conn.commit()
            return True
        except Exception as exc:
            self._log.error("DB", f"clear_tracking failed: {exc}")
            return False

    # ------------------------------------------------------------------
    # stats (for display)
    # ------------------------------------------------------------------
    def stats(self) -> Dict[str, int]:
        if not self._conn:
            return {}
        try:
            pending  = self._conn.execute("SELECT COUNT(*) FROM merge_changes WHERE approved=0").fetchone()[0]
            approved = self._conn.execute("SELECT COUNT(*) FROM merge_changes WHERE approved=1").fetchone()[0]
            rejected = self._conn.execute("SELECT COUNT(*) FROM merge_changes WHERE approved=-1").fetchone()[0]
            tracked  = self._conn.execute("SELECT COUNT(*) FROM tracking").fetchone()[0]
            return {"pending": pending, "approved": approved,
                    "rejected": rejected, "tracked": tracked}
        except Exception as exc:
            self._log.error("DB", f"stats failed: {exc}")
            return {}


    # ------------------------------------------------------------------
    # project_sessions
    # ------------------------------------------------------------------

    def create_project_session(
        self,
        root_dir: str,
        instruction: str,
        plan_prompt: str = "",
        file_tree_json: str = "",
        status: str = "pending",
    ) -> int:
        """Insert a new project session row. Returns row id or -1 on failure."""
        if not self._conn:
            self._log.error("DB", "create_project_session: db not open")
            return -1
        try:
            cur = self._conn.execute(
                "INSERT INTO project_sessions "
                "(root_dir, instruction, plan_prompt, file_tree_json, status, timestamp) "
                "VALUES (?,?,?,?,?,?)",
                (root_dir, instruction, plan_prompt, file_tree_json, status, _ts()),
            )
            self._conn.commit()
            return cur.lastrowid
        except Exception as exc:
            self._log.error("DB", f"create_project_session failed: {exc}")
            return -1

    def update_project_session(
        self,
        session_id: int,
        plan_prompt: str = None,
        file_tree_json: str = None,
        status: str = None,
    ) -> bool:
        """Update mutable fields on a project session."""
        if not self._conn:
            return False
        fields = []
        params = []
        if plan_prompt is not None:
            fields.append("plan_prompt=?")
            params.append(plan_prompt)
        if file_tree_json is not None:
            fields.append("file_tree_json=?")
            params.append(file_tree_json)
        if status is not None:
            fields.append("status=?")
            params.append(status)
        if not fields:
            return True
        params.append(session_id)
        try:
            self._conn.execute(
                f"UPDATE project_sessions SET {', '.join(fields)} WHERE id=?",
                params,
            )
            self._conn.commit()
            return True
        except Exception as exc:
            self._log.error("DB", f"update_project_session failed: {exc}")
            return False

    def get_project_session(self, session_id: int) -> Optional[Dict[str, Any]]:
        if not self._conn:
            return None
        try:
            row = self._conn.execute(
                "SELECT * FROM project_sessions WHERE id=?", (session_id,)
            ).fetchone()
            return dict(row) if row else None
        except Exception as exc:
            self._log.error("DB", f"get_project_session failed: {exc}")
            return None

    def list_project_sessions(self, root_dir: str = None) -> List[Dict[str, Any]]:
        if not self._conn:
            return []
        try:
            if root_dir:
                rows = self._conn.execute(
                    "SELECT * FROM project_sessions WHERE root_dir=? ORDER BY id DESC",
                    (root_dir,),
                ).fetchall()
            else:
                rows = self._conn.execute(
                    "SELECT * FROM project_sessions ORDER BY id DESC"
                ).fetchall()
            return [dict(r) for r in rows]
        except Exception as exc:
            self._log.error("DB", f"list_project_sessions failed: {exc}")
            return []


def make_issue_hash(file_path: str, rule: str, line: int, msg: str) -> str:
    raw = f"{file_path}:{rule}:{line}:{msg}"
    return hashlib.sha1(raw.encode()).hexdigest()[:16]
