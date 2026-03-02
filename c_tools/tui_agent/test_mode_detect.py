"""
Quick unit test for agent.get_mode() -- no API calls.
Run: python test_mode_detect.py
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
import agent

CASES = [
    ("main.py",                       "code"),
    ("styles.css",                    "code"),
    ("app.tsx",                       "code"),
    ("config.yaml",                   "code"),
    ("report.sql",                    "code"),
    ("data.csv",                      "csv"),
    ("export.tsv",                    "csv"),
    ("notes.txt",                     "txt"),
    ("unknown.xyz",                   "txt"),
    ("chat_log.txt",                  "conversation"),
    ("interview_transcript.txt",      "conversation"),
    ("dialog.md",                     "conversation"),
    ("report.pdf",                    "pdf"),
    ("readme.md",                     "doc"),
    ("specification.rst",             "doc"),
    ("letter.doc",                    "doc"),
    ("contract.docx",                 "doc"),
    ("email_template.html",           "email"),
    ("newsletter_campaign.html",      "email"),
    ("index.html",                    "code"),   # not email-like name
    ("base.htm",                      "code"),   # not email-like name
]

passed = failed = 0
for name, expected in CASES:
    got = agent.get_mode("/tmp/" + name)
    if got == expected:
        print(f"  PASS  {name!r:42s} -> {got!r}")
        passed += 1
    else:
        print(f"  FAIL  {name!r:42s}  expected {expected!r}  got {got!r}")
        failed += 1

print(f"\n{passed}/{passed+failed} passed", ("-- ALL OK" if failed == 0 else "-- FAILURES"))
sys.exit(0 if failed == 0 else 1)
