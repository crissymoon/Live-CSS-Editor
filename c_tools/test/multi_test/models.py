"""
models.py
Core data models for the accounting project.
Uses dataclasses for simplicity and SQLite compatibility.
"""

from __future__ import annotations
from dataclasses import dataclass, field
from datetime import date
from enum import Enum
from typing import Optional


class AccountType(str, Enum):
    ASSET     = "asset"
    LIABILITY = "liability"
    EQUITY    = "equity"
    REVENUE   = "revenue"
    EXPENSE   = "expense"


class EntryType(str, Enum):
    DEBIT  = "debit"
    CREDIT = "credit"


@dataclass
class Account:
    """
    A ledger account.
    normal_balance: "debit" for assets/expenses, "credit" for liabilities/equity/revenue.
    """
    id:             int
    code:           str            # e.g. "1000"
    name:           str            # e.g. "Cash"
    account_type:   AccountType
    normal_balance: EntryType
    balance:        float = 0.0
    description:    str   = ""

    def __post_init__(self):
        if isinstance(self.account_type, str):
            self.account_type = AccountType(self.account_type)
        if isinstance(self.normal_balance, str):
            self.normal_balance = EntryType(self.normal_balance)

    @property
    def is_debit_normal(self) -> bool:
        return self.normal_balance == EntryType.DEBIT


@dataclass
class JournalEntry:
    """A double-entry journal entry header."""
    id:          int
    date:        date
    description: str
    reference:   str = ""
    posted:      bool = False

    def __post_init__(self):
        if isinstance(self.date, str):
            self.date = date.fromisoformat(self.date)


@dataclass
class EntryLine:
    """
    A single debit or credit line within a JournalEntry.
    Constraint: sum(debits) == sum(credits) per JournalEntry.
    """
    id:         int
    entry_id:   int
    account_id: int
    entry_type: EntryType
    amount:     float
    memo:       str = ""

    def __post_init__(self):
        if isinstance(self.entry_type, str):
            self.entry_type = EntryType(self.entry_type)

        if self.amount < 0:
            raise ValueError(f"EntryLine amount must be >= 0, got {self.amount}")


@dataclass
class Transaction:
    """
    Convenience wrapper: one JournalEntry + its EntryLines.
    """
    entry:  JournalEntry
    lines:  list[EntryLine] = field(default_factory=list)

    @property
    def total_debits(self) -> float:
        return sum(l.amount for l in self.lines if l.entry_type == EntryType.DEBIT)

    @property
    def total_credits(self) -> float:
        return sum(l.amount for l in self.lines if l.entry_type == EntryType.CREDIT)

    @property
    def is_balanced(self) -> bool:
        return abs(self.total_debits - self.total_credits) < 0.005
