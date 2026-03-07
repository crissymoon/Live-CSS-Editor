"""
reports.py
Financial statement generators.
Each function returns a list of formatted strings suitable for printing
or further processing.
"""

from __future__ import annotations

from typing import List

from ledger import Ledger
from models import AccountType


def _header(title: str, width: int = 50) -> List[str]:
    bar = "=" * width
    return [bar, title.center(width), bar]


def _section(label: str, accounts, width: int = 50) -> List[str]:
    lines = [f"  {label}"]
    total = 0.0
    for acct in accounts:
        label_part = f"    {acct.code}  {acct.name}"
        amount_str = f"{acct.balance:>10.2f}"
        pad = width - len(label_part) - len(amount_str)
        lines.append(label_part + " " * max(1, pad) + amount_str)
        total += acct.balance
    total_str  = f"{total:>10.2f}"
    total_line = f"  {'Total':.<{width - 12}}{total_str}"
    lines.append(total_line)
    lines.append("")
    return lines, total


def balance_sheet(ledger: Ledger, width: int = 50) -> List[str]:
    """Assets = Liabilities + Equity."""
    out = _header("BALANCE SHEET", width)

    assets      = ledger.list_accounts(AccountType.ASSET)
    liabilities = ledger.list_accounts(AccountType.LIABILITY)
    equity      = ledger.list_accounts(AccountType.EQUITY)

    asset_lines, asset_total         = _section("ASSETS", assets, width)
    liability_lines, liability_total = _section("LIABILITIES", liabilities, width)
    equity_lines, equity_total       = _section("EQUITY", equity, width)

    out += asset_lines
    out += liability_lines
    out += equity_lines

    diff_str = f"{asset_total - (liability_total + equity_total):.2f}"
    out.append(f"  Balance check (should be 0.00): {diff_str}")
    return out


def income_statement(ledger: Ledger, width: int = 50) -> List[str]:
    """Revenue - Expenses = Net Income."""
    out = _header("INCOME STATEMENT", width)

    revenues = ledger.list_accounts(AccountType.REVENUE)
    expenses = ledger.list_accounts(AccountType.EXPENSE)

    rev_lines, rev_total   = _section("REVENUE", revenues, width)
    exp_lines, exp_total   = _section("EXPENSES", expenses, width)

    out += rev_lines
    out += exp_lines

    net = rev_total - exp_total
    out.append(f"  {'Net Income':.<{width - 12}}{net:>10.2f}")
    return out


def trial_balance(ledger: Ledger, width: int = 50) -> List[str]:
    """All account balances -- debits and credits must match."""
    out = _header("TRIAL BALANCE", width)

    all_accounts = ledger.list_accounts()
    debit_total  = 0.0
    credit_total = 0.0

    from models import EntryType
    for acct in all_accounts:
        label_part = f"  {acct.code}  {acct.name}"
        amount_str = f"{acct.balance:>10.2f}"
        pad = width - len(label_part) - len(amount_str)
        out.append(label_part + " " * max(1, pad) + amount_str)
        if acct.normal_balance == EntryType.DEBIT:
            debit_total += acct.balance
        else:
            credit_total += acct.balance

    out.append("-" * width)
    dr_str = f"{debit_total:>10.2f}"
    cr_str = f"{credit_total:>10.2f}"
    dr_line = f"  {'Total Debits':.<{width - 12}}{dr_str}"
    cr_line = f"  {'Total Credits':.<{width - 12}}{cr_str}"
    out.append(dr_line)
    out.append(cr_line)
    ok = abs(debit_total - credit_total) < 0.005
    out.append(f"  Balanced: {'YES' if ok else 'NO -- recheck postings'}")
    return out
