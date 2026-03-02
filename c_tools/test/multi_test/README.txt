multi_test -- Simple Double-Entry Accounting Project
======================================================

A small Python accounting application used to demonstrate and test
the TUI agent project workflow mode.

FILES
-----
models.py    Data models: Account, JournalEntry, EntryLine, Transaction (dataclasses)
storage.py   SQLite persistence layer (plain sqlite3, no ORM)
ledger.py    Core accounting logic: post transactions, update balances
reports.py   Financial statements: balance sheet, income statement, trial balance
cli.py       argparse command-line interface
README.txt   This file

USAGE
-----

1. Initialize the database and create default chart of accounts:

     python cli.py init

2. Add a custom account:

     python cli.py account add 4100 "Service Revenue" revenue

3. Post a balanced journal entry:

     python cli.py post 2024-01-15 "Initial capital injection" \
         1000:debit:10000 3000:credit:10000

     python cli.py post 2024-01-20 "Paid rent" \
         5100:debit:1200 1000:credit:1200

4. Generate reports:

     python cli.py report trial-balance
     python cli.py report balance-sheet
     python cli.py report income-statement

5. List accounts and transactions:

     python cli.py list accounts
     python cli.py list transactions

DESIGN NOTES
------------
- Double-entry: every posting must have equal debits and credits or it is rejected.
- Balances are maintained as running totals on the accounts table for fast reads.
- Storage is a single SQLite file (accounting.db by default).
- No dependencies beyond the Python standard library.
