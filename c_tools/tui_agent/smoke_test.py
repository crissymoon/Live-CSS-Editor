#!/usr/bin/env python3
"""smoke_test.py  --  end-to-end pipeline check (no TUI, no curses)"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))

from log_util    import get_logger
from db          import AgentDB
from scanner     import FolderScanner
from merger      import Merger
from fence_clean import strip_fences as _test_strip
import agent as _agent

TEST_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), "../test"))
DB_PATH  = "/tmp/smoke_tui_test.db"

def main():
    log = get_logger()
    log.info("SMOKE", "pipeline smoke test start")

    # 1. scanner
    scanner = FolderScanner(recursive=False)
    n = scanner.scan(TEST_DIR)
    files = scanner.get_files()
    print(f"[1] scanner: {n} files -> {[f.name for f in files]}")
    assert n >= 1, f"expected at least 1 file, got {n}"

    # 2. db
    db = AgentDB(DB_PATH)
    assert db.open(), "db.open() failed"
    print(f"[2] db opened: {DB_PATH}")

    # 3. merger
    merger = Merger(db)
    target = files[0].path
    print(f"[3] target: {target}")

    # 4. agent (real Haiku call)
    print("[4] calling claude-haiku-4-5-20251001 ...")
    with open(target, "r", encoding="utf-8") as f:
        content = f.read()
    new_content, err = _agent.call_haiku(
        target, content,
        "Add JSDoc comments to every function. Keep the logic identical."
    )
    if err:
        print(f"AGENT ERROR: {err}")
        sys.exit(1)
    print(f"    response {len(new_content)} chars")

    # 5. fence clean verification (agent already applies both cleaners internally)
    _, fence_passes = _test_strip(new_content)
    print(f"[5] fence_clean: {len(new_content)} chars, fence passes={fence_passes}")

    # 6. propose
    change_id = merger.propose(target, new_content, "haiku-smoke-test")
    print(f"[6] proposed change id={change_id}")
    assert change_id > 0, "expected new change id"

    # 7. diff preview
    diff = merger.preview(target)
    diff_excerpt = [l for l in diff.splitlines() if l.startswith(("+", "-", "@"))]
    print("[7] diff excerpt (first 8 lines):")
    for line in diff_excerpt[:8]:
        print("   ", line[:88])

    # 8. pending check
    pending = db.get_pending_changes(target)
    print(f"[8] pending in DB: {len(pending)}")
    assert len(pending) == 1

    # 9. reject (do not modify test file)
    ok = merger.reject(target, change_id)
    assert ok, "reject failed"
    print("[9] rejected (smoke test -- file not modified)")

    # 10. cleanup
    db.close()
    os.unlink(DB_PATH)
    print("[10] db cleaned up")

    print()
    print("pipeline smoke test PASSED")
    print()
    print("-- log entries --")
    for entry in log.get_entries():
        print(entry)

if __name__ == "__main__":
    main()
