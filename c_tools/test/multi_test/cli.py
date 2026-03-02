"""
cli.py
Command-line interface for the accounting project.
Uses argparse subcommands so each operation is a distinct sub-command.

Usage examples:
  python cli.py init
  python cli.py account add 1000 Cash asset
  python cli.py post 2024-01-01 "Initial capital" 1000:debit:5000 3000:credit:5000
  python cli.py report balance-sheet
  python cli.py report income-statement
  python cli.py report trial-balance
  python cli.py list accounts
  python cli.py list transactions
"""

from __future__ import annotations

import argparse
import logging
import os
import sys
from datetime import date

from models import AccountType, EntryType
from storage import Storage
from ledger import Ledger, LedgerError
import reports as rpt

DEFAULT_DB = os.path.join(os.path.dirname(__file__), "accounting.db")

logging.basicConfig(
    level=logging.INFO,
    format="%(levelname)s  %(name)s  %(message)s",
    stream=sys.stderr,
)
log = logging.getLogger("cli")


def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="accounting",
        description="Simple double-entry accounting CLI",
    )
    p.add_argument("--db", default=DEFAULT_DB, help="Path to SQLite database file")

    sub = p.add_subparsers(dest="command")

    # init
    sub.add_parser("init", help="Initialize database and create default chart of accounts")

    # account add
    acct_p = sub.add_parser("account", help="Account management")
    acct_sub = acct_p.add_subparsers(dest="acct_command")
    add_p = acct_sub.add_parser("add", help="Add a new account")
    add_p.add_argument("code",         help="Account code, e.g. 1000")
    add_p.add_argument("name",         help="Account name, e.g. Cash")
    add_p.add_argument("account_type", choices=[t.value for t in AccountType],
                       help="Account type")
    add_p.add_argument("--desc", default="", help="Optional description")

    # post
    post_p = sub.add_parser("post", help="Post a journal entry")
    post_p.add_argument("date",        help="Transaction date YYYY-MM-DD")
    post_p.add_argument("description", help="Transaction description")
    post_p.add_argument("lines", nargs="+",
                        help="Lines as code:type:amount  e.g.  1000:debit:500")
    post_p.add_argument("--ref", default="", help="Optional reference")

    # report
    rep_p  = sub.add_parser("report", help="Generate financial reports")
    rep_p.add_argument("type",
                       choices=["balance-sheet", "income-statement", "trial-balance"])

    # list
    list_p = sub.add_parser("list", help="List data")
    list_p.add_argument("what", choices=["accounts", "transactions"])

    return p


def _open(db_path: str) -> tuple[Storage, Ledger]:
    store = Storage(db_path)
    if not store.open():
        print(f"ERROR: could not open database: {db_path}", file=sys.stderr)
        sys.exit(1)
    return store, Ledger(store)


def cmd_init(args) -> int:
    store, ledger = _open(args.db)
    defaults = [
        ("1000", "Cash",             AccountType.ASSET),
        ("1100", "Accounts Recv",    AccountType.ASSET),
        ("1500", "Equipment",        AccountType.ASSET),
        ("2000", "Accounts Pay",     AccountType.LIABILITY),
        ("2500", "Notes Payable",    AccountType.LIABILITY),
        ("3000", "Owner Equity",     AccountType.EQUITY),
        ("3100", "Retained Earnings",AccountType.EQUITY),
        ("4000", "Sales Revenue",    AccountType.REVENUE),
        ("5000", "Cost of Goods",    AccountType.EXPENSE),
        ("5100", "Rent Expense",     AccountType.EXPENSE),
        ("5200", "Payroll Expense",  AccountType.EXPENSE),
        ("5300", "Utilities",        AccountType.EXPENSE),
    ]
    created = 0
    for code, name, atype in defaults:
        existing = ledger.get_account_by_code(code)
        if existing is None:
            ledger.create_account(code, name, atype)
            created += 1
    print(f"init: {created} accounts created in {args.db}")
    store.close()
    return 0


def cmd_account_add(args) -> int:
    store, ledger = _open(args.db)
    try:
        acct = ledger.create_account(
            args.code,
            args.name,
            AccountType(args.account_type),
            description=args.desc,
        )
        print(f"created account id={acct.id}  {acct.code}  {acct.name}  ({acct.account_type.value})")
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        store.close()
        return 1
    store.close()
    return 0


def cmd_post(args) -> int:
    store, ledger = _open(args.db)
    lines = []
    for raw in args.lines:
        parts = raw.split(":")
        if len(parts) != 3:
            print(f"ERROR: bad line format '{raw}' -- expected code:debit|credit:amount",
                  file=sys.stderr)
            store.close()
            return 1
        code, etype, amount_str = parts
        try:
            amount = float(amount_str)
        except ValueError:
            print(f"ERROR: bad amount '{amount_str}'", file=sys.stderr)
            store.close()
            return 1
        try:
            et = EntryType(etype.lower())
        except ValueError:
            print(f"ERROR: bad entry type '{etype}' -- use debit or credit", file=sys.stderr)
            store.close()
            return 1
        lines.append((code, et, amount, ""))

    try:
        txn_date = date.fromisoformat(args.date)
    except ValueError:
        print(f"ERROR: bad date '{args.date}' -- use YYYY-MM-DD", file=sys.stderr)
        store.close()
        return 1

    try:
        txn = ledger.post(txn_date, args.description, lines, reference=args.ref)
        print(f"posted entry id={txn.entry.id}  date={txn.entry.date}  "
              f"dr={txn.total_debits:.2f}  cr={txn.total_credits:.2f}")
    except LedgerError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        store.close()
        return 1

    store.close()
    return 0


def cmd_report(args) -> int:
    store, ledger = _open(args.db)
    rtype = args.type
    if rtype == "balance-sheet":
        lines = rpt.balance_sheet(ledger)
    elif rtype == "income-statement":
        lines = rpt.income_statement(ledger)
    else:
        lines = rpt.trial_balance(ledger)
    print("\n".join(lines))
    store.close()
    return 0


def cmd_list(args) -> int:
    store, ledger = _open(args.db)
    if args.what == "accounts":
        for acct in ledger.list_accounts():
            print(f"  {acct.id:>3}  {acct.code}  {acct.name:<25}  "
                  f"{acct.account_type.value:<12}  balance={acct.balance:.2f}")
    else:
        for txn in ledger.get_transactions():
            print(f"  {txn.entry.id:>3}  {txn.entry.date}  {txn.entry.description}")
            for line in txn.lines:
                acct = ledger.get_account(line.account_id)
                name = acct.name if acct else f"acct#{line.account_id}"
                print(f"       {line.entry_type.value:<7}  {line.amount:>10.2f}  {name}  {line.memo}")
    store.close()
    return 0


def main():
    parser = _build_parser()
    args   = parser.parse_args()

    if args.command is None:
        parser.print_help()
        return 0

    if args.command == "init":
        return cmd_init(args)
    if args.command == "account":
        if args.acct_command == "add":
            return cmd_account_add(args)
        print("account: unknown sub-command", file=sys.stderr)
        return 1
    if args.command == "post":
        return cmd_post(args)
    if args.command == "report":
        return cmd_report(args)
    if args.command == "list":
        return cmd_list(args)

    print(f"unknown command: {args.command}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
