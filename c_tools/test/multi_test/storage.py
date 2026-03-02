"""
storage.py
SQLite persistence layer for the accounting project.
No ORM -- plain sqlite3 with explicit SQL so the schema is transparent.
"""

from __future__ import annotations

import logging
import sqlite3
from datetime import date
from typing import List, Optional, Tuple

from models import Account, AccountType, EntryLine, EntryType, JournalEntry, Transaction

log = logging.getLogger(__name__)

_DDL = """
CREATE TABLE IF NOT EXISTS accounts (
    id             INTEGER PRIMARY KEY AUTOINCREMENT,
    code           TEXT    NOT NULL UNIQUE,
    name           TEXT    NOT NULL,
    account_type   TEXT    NOT NULL,
    normal_balance TEXT    NOT NULL,
    balance        REAL    NOT NULL DEFAULT 0.0,
    description    TEXT    NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS journal_entries (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    date        TEXT    NOT NULL,
    description TEXT    NOT NULL,
    reference   TEXT    NOT NULL DEFAULT '',
    posted      INTEGER NOT NULL DEFAULT 1
);

CREATE TABLE IF NOT EXISTS entry_lines (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    entry_id    INTEGER NOT NULL REFERENCES journal_entries(id),
    account_id  INTEGER NOT NULL REFERENCES accounts(id),
    entry_type  TEXT    NOT NULL,
    amount      REAL    NOT NULL,
    memo        TEXT    NOT NULL DEFAULT ''
);
"""


class Storage:
    def __init__(self, db_path: str = ":memory:"):
        self._path = db_path
        self._conn: Optional[sqlite3.Connection] = None

    def open(self) -> bool:
        try:
            self._conn = sqlite3.connect(self._path, check_same_thread=False)
            self._conn.row_factory = sqlite3.Row
            self._conn.execute("PRAGMA journal_mode=WAL;")
            self._conn.execute("PRAGMA foreign_keys=ON;")
            for stmt in _DDL.strip().split(";"):
                s = stmt.strip()
                if s:
                    self._conn.execute(s)
            self._conn.commit()
            log.info("storage opened: %s", self._path)
            return True
        except Exception as exc:
            log.error("storage open failed: %s", exc)
            return False

    def close(self):
        if self._conn:
            try:
                self._conn.close()
            except Exception as exc:
                log.warning("storage close: %s", exc)
            self._conn = None

    # ------------------------------------------------------------------
    # Accounts
    # ------------------------------------------------------------------

    def insert_account(self, acct: Account) -> int:
        c = self._conn.execute(
            "INSERT INTO accounts (code, name, account_type, normal_balance, balance, description) "
            "VALUES (?, ?, ?, ?, ?, ?)",
            (acct.code, acct.name, acct.account_type.value,
             acct.normal_balance.value, acct.balance, acct.description),
        )
        self._conn.commit()
        return c.lastrowid

    def get_account(self, account_id: int) -> Optional[Account]:
        row = self._conn.execute(
            "SELECT * FROM accounts WHERE id = ?", (account_id,)
        ).fetchone()
        return _row_to_account(row) if row else None

    def get_account_by_code(self, code: str) -> Optional[Account]:
        row = self._conn.execute(
            "SELECT * FROM accounts WHERE code = ?", (code,)
        ).fetchone()
        return _row_to_account(row) if row else None

    def list_accounts(self, account_type: Optional[AccountType] = None) -> List[Account]:
        if account_type:
            rows = self._conn.execute(
                "SELECT * FROM accounts WHERE account_type = ? ORDER BY code",
                (account_type.value,),
            ).fetchall()
        else:
            rows = self._conn.execute(
                "SELECT * FROM accounts ORDER BY code"
            ).fetchall()
        return [_row_to_account(r) for r in rows]

    def update_balance(self, account_id: int, delta: float):
        self._conn.execute(
            "UPDATE accounts SET balance = balance + ? WHERE id = ?",
            (delta, account_id),
        )
        self._conn.commit()

    # ------------------------------------------------------------------
    # Journal entries + lines
    # ------------------------------------------------------------------

    def insert_journal_entry(
        self, entry: JournalEntry, lines: List[EntryLine]
    ) -> int:
        c = self._conn.execute(
            "INSERT INTO journal_entries (date, description, reference, posted) "
            "VALUES (?, ?, ?, ?)",
            (entry.date.isoformat(), entry.description, entry.reference, int(entry.posted)),
        )
        entry_id = c.lastrowid
        for line in lines:
            self._conn.execute(
                "INSERT INTO entry_lines (entry_id, account_id, entry_type, amount, memo) "
                "VALUES (?, ?, ?, ?, ?)",
                (entry_id, line.account_id, line.entry_type.value, line.amount, line.memo),
            )
        self._conn.commit()
        return entry_id

    def list_transactions(
        self,
        start: Optional[date] = None,
        end: Optional[date] = None,
        account_code: Optional[str] = None,
    ) -> List[Transaction]:
        params: list = []
        where: list  = []

        if start:
            where.append("je.date >= ?")
            params.append(start.isoformat())
        if end:
            where.append("je.date <= ?")
            params.append(end.isoformat())

        if account_code:
            acct_row = self._conn.execute(
                "SELECT id FROM accounts WHERE code = ?", (account_code,)
            ).fetchone()
            if acct_row:
                where.append(
                    "je.id IN (SELECT entry_id FROM entry_lines WHERE account_id = ?)"
                )
                params.append(acct_row["id"])

        sql = "SELECT * FROM journal_entries je"
        if where:
            sql += " WHERE " + " AND ".join(where)
        sql += " ORDER BY je.date, je.id"

        entries = self._conn.execute(sql, params).fetchall()
        result  = []
        for entry_row in entries:
            je = JournalEntry(
                id=entry_row["id"],
                date=date.fromisoformat(entry_row["date"]),
                description=entry_row["description"],
                reference=entry_row["reference"],
                posted=bool(entry_row["posted"]),
            )
            line_rows = self._conn.execute(
                "SELECT * FROM entry_lines WHERE entry_id = ?", (je.id,)
            ).fetchall()
            lines = [
                EntryLine(
                    id=r["id"],
                    entry_id=r["entry_id"],
                    account_id=r["account_id"],
                    entry_type=EntryType(r["entry_type"]),
                    amount=r["amount"],
                    memo=r["memo"],
                )
                for r in line_rows
            ]
            result.append(Transaction(entry=je, lines=lines))
        return result


def _row_to_account(row: sqlite3.Row) -> Account:
    return Account(
        id=row["id"],
        code=row["code"],
        name=row["name"],
        account_type=AccountType(row["account_type"]),
        normal_balance=EntryType(row["normal_balance"]),
        balance=row["balance"],
        description=row["description"],
    )
