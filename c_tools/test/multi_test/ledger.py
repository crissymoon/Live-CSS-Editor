"""
ledger.py
Core double-entry ledger logic.
All accounting operations go through Ledger so the rest of the app
never touches the DB directly.
"""

from __future__ import annotations

import logging
from datetime import date
from typing import Dict, List, Optional, Tuple

from models import Account, AccountType, EntryLine, EntryType, JournalEntry, Transaction
from storage import Storage

log = logging.getLogger(__name__)


class LedgerError(Exception):
    """Raised when a ledger operation violates accounting rules."""


class Ledger:
    def __init__(self, storage: Storage):
        self._store = storage

    # ------------------------------------------------------------------
    # Accounts
    # ------------------------------------------------------------------

    def create_account(
        self,
        code: str,
        name: str,
        account_type: AccountType,
        description: str = "",
    ) -> Account:
        """
        Create a new account.
        Infers normal_balance from account_type:
          assets, expenses   -> debit
          liabilities, equity, revenue -> credit
        """
        if account_type in (AccountType.ASSET, AccountType.EXPENSE):
            normal = EntryType.DEBIT
        else:
            normal = EntryType.CREDIT

        acct = Account(
            id=0,
            code=code.strip(),
            name=name.strip(),
            account_type=account_type,
            normal_balance=normal,
            description=description,
        )
        acct_id = self._store.insert_account(acct)
        acct.id = acct_id
        log.info("created account %s %s (id=%d)", code, name, acct_id)
        return acct

    def get_account(self, account_id: int) -> Optional[Account]:
        return self._store.get_account(account_id)

    def get_account_by_code(self, code: str) -> Optional[Account]:
        return self._store.get_account_by_code(code)

    def list_accounts(self, account_type: Optional[AccountType] = None) -> List[Account]:
        return self._store.list_accounts(account_type)

    # ------------------------------------------------------------------
    # Journal entries
    # ------------------------------------------------------------------

    def post(
        self,
        txn_date: date,
        description: str,
        lines: List[Tuple[str, EntryType, float, str]],  # (account_code, type, amount, memo)
        reference: str = "",
    ) -> Transaction:
        """
        Post a balanced transaction.
        lines is a list of (account_code, debit|credit, amount, memo).
        Raises LedgerError if the entry does not balance.
        """
        if not lines:
            raise LedgerError("transaction must have at least one line")

        entry_lines: List[EntryLine] = []
        for code, entry_type, amount, memo in lines:
            acct = self._store.get_account_by_code(code)
            if acct is None:
                raise LedgerError(f"account not found: {code!r}")
            entry_lines.append(
                EntryLine(
                    id=0,
                    entry_id=0,
                    account_id=acct.id,
                    entry_type=entry_type,
                    amount=amount,
                    memo=memo,
                )
            )

        total_dr = sum(l.amount for l in entry_lines if l.entry_type == EntryType.DEBIT)
        total_cr = sum(l.amount for l in entry_lines if l.entry_type == EntryType.CREDIT)

        if abs(total_dr - total_cr) >= 0.005:
            raise LedgerError(
                f"unbalanced entry: debits={total_dr:.2f} credits={total_cr:.2f}"
            )

        journal_entry = JournalEntry(
            id=0,
            date=txn_date,
            description=description,
            reference=reference,
            posted=True,
        )

        entry_id = self._store.insert_journal_entry(journal_entry, entry_lines)
        journal_entry.id = entry_id
        for line in entry_lines:
            line.entry_id = entry_id

        # update running balances
        for line in entry_lines:
            acct    = self._store.get_account(line.account_id)
            if acct is None:
                log.error("could not reload account id=%d for balance update", line.account_id)
                continue
            delta = line.amount if line.entry_type == acct.normal_balance else -line.amount
            self._store.update_balance(acct.id, delta)

        log.info(
            "posted txn id=%d  date=%s  dr=%.2f  cr=%.2f  '%s'",
            entry_id, txn_date, total_dr, total_cr, description,
        )
        return Transaction(entry=journal_entry, lines=entry_lines)

    def get_transactions(
        self,
        start: Optional[date] = None,
        end: Optional[date] = None,
        account_code: Optional[str] = None,
    ) -> List[Transaction]:
        return self._store.list_transactions(start, end, account_code)

    # ------------------------------------------------------------------
    # Trial balance
    # ------------------------------------------------------------------

    def trial_balance(self) -> Dict[str, float]:
        """Return {account_code: balance} for all accounts."""
        accounts = self._store.list_accounts()
        return {a.code: a.balance for a in accounts}

    def is_balanced(self) -> bool:
        """
        Double-entry check: sum of asset+expense balances == sum of liability+equity+revenue.
        In practice: total debits == total credits.
        """
        accounts      = self._store.list_accounts()
        debit_total   = sum(
            a.balance for a in accounts
            if a.account_type in (AccountType.ASSET, AccountType.EXPENSE)
        )
        credit_total  = sum(
            a.balance for a in accounts
            if a.account_type in (AccountType.LIABILITY, AccountType.EQUITY, AccountType.REVENUE)
        )
        return abs(debit_total - credit_total) < 0.005
