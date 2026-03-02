#!/usr/bin/env python3
"""
smoke_project_mode.py
Smoke test for:
  1. multi_test accounting project (models, storage, ledger, reports)
  2. ProjectScanner -- scans multi_test/ and runs outliner
  3. AgentDB project_sessions table (create, update, get, list)
  4. ProjectSession.reject_all / approve_all flow via real Merger + DB

No external API calls are made. The test uses a temp SQLite DB.
Exit code 0 = all passed. Non-zero = failure.
"""

import json
import os
import sys
import tempfile
import traceback

# ---------------------------------------------------------------------------
# Path setup
# ---------------------------------------------------------------------------
HERE         = os.path.dirname(os.path.abspath(__file__))
MULTI_TEST   = os.path.join(HERE, "multi_test")
TUI_AGENT    = os.path.join(HERE, "..", "tui_agent")

sys.path.insert(0, MULTI_TEST)
sys.path.insert(0, TUI_AGENT)
sys.path.insert(0, os.path.join(HERE, ".."))   # c_tools/ for outliner

# ---------------------------------------------------------------------------
# Test helpers
# ---------------------------------------------------------------------------
_passed = 0
_failed = 0

def _ok(name: str):
    global _passed
    _passed += 1
    print(f"  PASS  {name}")

def _fail(name: str, reason: str = ""):
    global _failed
    _failed += 1
    msg = f"  FAIL  {name}"
    if reason:
        msg += f"  --  {reason}"
    print(msg, file=sys.stderr)


# ===========================================================================
# Suite 1: multi_test accounting project
# ===========================================================================

def test_accounting():
    print("\n[suite 1] multi_test accounting")
    from storage import Storage
    from ledger  import Ledger, LedgerError
    from models  import AccountType, EntryType
    import reports as rpt
    from datetime import date

    store = Storage(":memory:")
    if not store.open():
        _fail("storage.open", "returned False")
        return
    _ok("storage.open")

    ledger = Ledger(store)

    # create chart of accounts
    try:
        cash  = ledger.create_account("1000", "Cash",         AccountType.ASSET)
        rev   = ledger.create_account("4000", "Revenue",      AccountType.REVENUE)
        exp   = ledger.create_account("5000", "Expenses",     AccountType.EXPENSE)
        eq    = ledger.create_account("3000", "Owner Equity", AccountType.EQUITY)
        _ok("create_account x4")
    except Exception as exc:
        _fail("create_account", str(exc))
        store.close()
        return

    # post a balanced transaction: owner puts in 10000 cash
    try:
        txn = ledger.post(
            date(2024, 1, 1),
            "Initial capital",
            [("1000", EntryType.DEBIT, 10000.0, "cash injection"),
             ("3000", EntryType.CREDIT, 10000.0, "owner equity")],
        )
        assert txn.is_balanced, "txn is not balanced"
        _ok("post balanced txn")
    except Exception as exc:
        _fail("post balanced txn", str(exc))
        store.close()
        return

    # post revenue
    try:
        ledger.post(
            date(2024, 1, 15),
            "Service revenue",
            [("1000", EntryType.DEBIT,  3000.0, ""),
             ("4000", EntryType.CREDIT, 3000.0, "")],
        )
        _ok("post revenue txn")
    except Exception as exc:
        _fail("post revenue txn", str(exc))

    # post expense
    try:
        ledger.post(
            date(2024, 1, 20),
            "Operating expense",
            [("5000", EntryType.DEBIT,  1200.0, ""),
             ("1000", EntryType.CREDIT, 1200.0, "")],
        )
        _ok("post expense txn")
    except Exception as exc:
        _fail("post expense txn", str(exc))

    # check balances
    try:
        cash_acct = ledger.get_account_by_code("1000")
        assert abs(cash_acct.balance - 11800.0) < 0.005, f"cash balance={cash_acct.balance}"
        _ok("cash balance correct (11800.00)")
    except AssertionError as exc:
        _fail("cash balance", str(exc))

    # trial balance
    try:
        tb    = ledger.trial_balance()
        lines = rpt.trial_balance(ledger)
        assert len(lines) > 3, "trial balance output too short"
        balanced = "YES" in "\n".join(lines)
        if not balanced:
            _fail("trial balance balanced", "Balanced: NO in output")
        else:
            _ok("trial balance (balanced)")
    except Exception as exc:
        _fail("trial balance", str(exc))

    # reject unbalanced transaction
    try:
        ledger.post(
            date(2024, 1, 25),
            "Unbalanced",
            [("1000", EntryType.DEBIT, 500.0, "")],  # missing credit
        )
        _fail("unbalanced txn rejected", "should have raised LedgerError")
    except LedgerError:
        _ok("unbalanced txn correctly rejected")
    except Exception as exc:
        _fail("unbalanced txn rejected", str(exc))

    # list transactions
    try:
        txns = ledger.get_transactions()
        assert len(txns) == 3, f"expected 3 transactions, got {len(txns)}"
        _ok(f"list transactions ({len(txns)} found)")
    except Exception as exc:
        _fail("list transactions", str(exc))

    # balance sheet report
    try:
        bs_lines = rpt.balance_sheet(ledger)
        assert any("BALANCE SHEET" in l for l in bs_lines), "missing header"
        _ok("balance_sheet report generates")
    except Exception as exc:
        _fail("balance_sheet", str(exc))

    # income statement
    try:
        is_lines = rpt.income_statement(ledger)
        assert any("INCOME STATEMENT" in l for l in is_lines), "missing header"
        _ok("income_statement report generates")
    except Exception as exc:
        _fail("income_statement", str(exc))

    store.close()
    _ok("storage.close")


# ===========================================================================
# Suite 2: ProjectScanner (no API calls)
# ===========================================================================

def test_project_scanner():
    print("\n[suite 2] ProjectScanner")
    try:
        from project_mode import ProjectScanner
    except Exception as exc:
        _fail("import ProjectScanner", str(exc))
        traceback.print_exc()
        return

    logs = []
    scanner = ProjectScanner(on_log=logs.append)
    try:
        tree = scanner.scan(MULTI_TEST)
        assert tree["root"] == os.path.realpath(MULTI_TEST), "root mismatch"
        assert len(tree["files"]) >= 4, f"expected >= 4 files, got {len(tree['files'])}"
        _ok(f"scan found {len(tree['files'])} files in multi_test/")
    except Exception as exc:
        _fail("ProjectScanner.scan", str(exc))
        traceback.print_exc()
        return

    # each entry should have rel, ext, size, outline
    for f in tree["files"]:
        for key in ("path", "rel", "ext", "size", "outline"):
            if key not in f:
                _fail(f"file entry missing key '{key}'", f["rel"])
                return
    _ok("all file entries have required keys")

    # at least one .py file should have outline nodes
    py_files = [f for f in tree["files"] if f["ext"] == ".py"]
    if py_files:
        has_nodes = any(len(f["outline"]) > 0 for f in py_files)
        if has_nodes:
            _ok("at least one .py file has outline nodes")
        else:
            _fail("outline nodes", "no .py file returned outline nodes")
    else:
        _fail("py files", "no .py files found in multi_test/")


# ===========================================================================
# Suite 3: AgentDB project_sessions table
# ===========================================================================

def test_project_sessions_db():
    print("\n[suite 3] AgentDB project_sessions")
    try:
        from db import AgentDB
    except Exception as exc:
        _fail("import AgentDB", str(exc))
        return

    with tempfile.TemporaryDirectory() as tmpdir:
        db_path = os.path.join(tmpdir, "test.db")
        db = AgentDB(db_path)
        if not db.open():
            _fail("db.open", "returned False")
            return
        _ok("db.open")

        # create session
        sid = db.create_project_session(
            root_dir=MULTI_TEST,
            instruction="add docstrings to everything",
            status="pending",
        )
        if sid < 0:
            _fail("create_project_session", f"returned {sid}")
            db.close()
            return
        _ok(f"create_project_session id={sid}")

        # update
        ok = db.update_project_session(sid, plan_prompt="revised plan", status="planning")
        if not ok:
            _fail("update_project_session", "returned False")
        else:
            _ok("update_project_session")

        # get
        row = db.get_project_session(sid)
        if row is None:
            _fail("get_project_session", "returned None")
        else:
            assert row["status"] == "planning", f"status={row['status']}"
            assert row["plan_prompt"] == "revised plan", f"plan_prompt={row['plan_prompt']}"
            _ok("get_project_session (status + plan_prompt verified)")

        # list
        rows = db.list_project_sessions(root_dir=MULTI_TEST)
        if len(rows) != 1:
            _fail("list_project_sessions", f"expected 1 row, got {len(rows)}")
        else:
            _ok("list_project_sessions (1 row)")

        # stats still work
        stats = db.stats()
        assert isinstance(stats, dict), "stats must be dict"
        _ok("stats() still returns dict after schema migration")

        db.close()
        _ok("db.close")


# ===========================================================================
# Suite 4: ProjectSession approve/reject flow (no API, mock changes)
# ===========================================================================

def test_project_session_flow():
    print("\n[suite 4] ProjectSession approve/reject flow")
    try:
        from db           import AgentDB
        from merger       import Merger
        from project_mode import ProjectSession
    except Exception as exc:
        _fail("imports for session flow", str(exc))
        traceback.print_exc()
        return

    with tempfile.TemporaryDirectory() as tmpdir:
        db_path   = os.path.join(tmpdir, "flow.db")
        test_file = os.path.join(tmpdir, "sample.py")

        # create a real source file
        with open(test_file, "w") as f:
            f.write("x = 1\n")

        db = AgentDB(db_path)
        if not db.open():
            _fail("db.open (flow)", "False")
            return

        merger = Merger(db)

        # manually propose a change so we can test reject_all
        change_id = merger.propose(test_file, "x = 2\n", source_label="test")
        if change_id <= 0:
            _fail("merger.propose (flow)", f"change_id={change_id}")
            db.close()
            return
        _ok(f"merger.propose id={change_id}")

        # verify pending
        pending = db.get_pending_changes(test_file)
        assert len(pending) == 1, f"expected 1 pending, got {len(pending)}"
        _ok("pending change visible in DB")

        # preview diff
        diff_text = merger.preview(test_file)
        assert "+x = 2" in diff_text or "x = 2" in diff_text, f"diff missing: {diff_text[:100]}"
        _ok("merger.preview returns diff")

        # inject the tree into the session so reject_all can work
        session = ProjectSession(db=db, merger=merger)
        session._tree = {
            "root": tmpdir,
            "files": [{"path": test_file, "rel": "sample.py", "ext": ".py",
                        "size": 6, "outline": []}]
        }

        logs_r = []
        rejected = session.reject_all(tmpdir, on_log=logs_r.append)
        assert rejected == 1, f"expected 1 rejected, got {rejected}"
        _ok("reject_all rejected 1 change")

        # pending should now be empty (rejected changes have approved=-1)
        pending_after = db.get_pending_changes(test_file)
        assert len(pending_after) == 0, f"still {len(pending_after)} pending after reject"
        _ok("no pending changes after reject_all")

        # test approve_all: propose again then approve
        change_id2 = merger.propose(test_file, "x = 99\n", source_label="test")
        assert change_id2 > 0, f"change_id2={change_id2}"
        logs_a = []
        applied = session.approve_all(tmpdir, on_log=logs_a.append)
        assert applied == 1, f"expected 1 applied, got {applied}"
        _ok("approve_all applied 1 change")

        # verify file was updated
        with open(test_file) as f:
            content = f.read()
        assert content.strip() == "x = 99", f"file content: {content!r}"
        _ok("file content updated on disk after approve_all")

        db.close()


# ===========================================================================
# Main
# ===========================================================================

def main():
    print("=" * 60)
    print("smoke_project_mode.py")
    print("=" * 60)

    try:
        test_accounting()
    except Exception as exc:
        _fail("test_accounting (unhandled)", str(exc))
        traceback.print_exc()

    try:
        test_project_scanner()
    except Exception as exc:
        _fail("test_project_scanner (unhandled)", str(exc))
        traceback.print_exc()

    try:
        test_project_sessions_db()
    except Exception as exc:
        _fail("test_project_sessions_db (unhandled)", str(exc))
        traceback.print_exc()

    try:
        test_project_session_flow()
    except Exception as exc:
        _fail("test_project_session_flow (unhandled)", str(exc))
        traceback.print_exc()

    print()
    print("=" * 60)
    print(f"  Results: {_passed} passed, {_failed} failed")
    print("=" * 60)
    return 0 if _failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
