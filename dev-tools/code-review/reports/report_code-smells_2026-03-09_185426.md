# Code Review Report — Code Smells

**Directory:** `/Users/mac/Documents/my_ruler/notepad`  
**Generated:** 2026-03-09 18:54:26  

---

```
Code Smells scan: /Users/mac/Documents/my_ruler/notepad
------------------------------------------------------------
[LOW] tests/test_navigation_history.py:12  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] tests/test_navigation_history.py:13  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] tests/test_navigation_history.py:16  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] tests/test_navigation_history.py:29  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] tests/test_navigation_history.py:32  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] tests/test_navigation_history.py:49  [MAGIC_NUMBER]  Bare numeric literal 999
[LOW] tests/test_navigation_history.py:56  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] tests/test_navigation_history.py:57  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] tests/test_recovery_retention.py:37  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] tests/test_recovery_retention.py:39  [MAGIC_NUMBER]  Bare numeric literal 365
[LOW] tests/test_settings.py:12  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] tests/test_settings.py:14  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] tests/test_settings.py:20  [MAGIC_NUMBER]  Bare numeric literal 99
[LOW] tests/test_settings.py:22  [MAGIC_NUMBER]  Bare numeric literal 99
[LOW] tests/test_settings.py:2  [DEAD_IMPORT]  'json' imported but never used
[LOW] tests/test_settings.py:3  [DEAD_IMPORT]  'tempfile' imported but never used
[LOW] src/simple_notepad/ui/main_window.py:20  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] src/simple_notepad/ui/main_window.py:117  [MAGIC_NUMBER]  Bare numeric literal 3000
[HIGH] src/simple_notepad/ui/tab_manager.py:230  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/ui/tab_manager.py:232  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/ui/tab_manager.py:233  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/ui/tab_manager.py:234  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/simple_notepad/ui/tab_manager.py:5  [DEAD_IMPORT]  'pyqtSignal' imported but never used
[LOW] src/simple_notepad/ui/restore_points_dialog.py:20  [MAGIC_NUMBER]  Bare numeric literal 520
[LOW] src/simple_notepad/ui/restore_points_dialog.py:30  [MAGIC_NUMBER]  Bare numeric literal 80
[LOW] src/simple_notepad/ui/restore_points_dialog.py:54  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] src/simple_notepad/persistence/paths.py:7  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] src/simple_notepad/persistence/sqlite_store.py:18  [PRIMITIVE_OBSESSION]  6 primitive-type parameters (max 4)
[LOW] src/simple_notepad/persistence/sqlite_store.py:40  [PRIMITIVE_OBSESSION]  6 primitive-type parameters (max 4)
[HIGH] src/simple_notepad/persistence/sqlite_store.py:56  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/persistence/sqlite_store.py:57  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/simple_notepad/persistence/sqlite_store.py:19  [MAGIC_NUMBER]  Bare numeric literal 1000
[LOW] src/simple_notepad/persistence/sqlite_store.py:41  [MAGIC_NUMBER]  Bare numeric literal 86400
[LOW] src/simple_notepad/persistence/sqlite_store.py:61  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] src/simple_notepad/persistence/recovery_service.py:14  [MAGIC_NUMBER]  Bare numeric literal 30000
[LOW] src/simple_notepad/persistence/recovery_service.py:36  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] src/simple_notepad/persistence/recovery_service.py:39  [MAGIC_NUMBER]  Bare numeric literal 256
[LOW] src/simple_notepad/persistence/settings.py:15  [MAGIC_NUMBER]  Bare numeric literal 30000
[LOW] src/simple_notepad/persistence/settings.py:16  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] src/simple_notepad/persistence/settings.py:18  [MAGIC_NUMBER]  Bare numeric literal 256
[LOW] src/simple_notepad/persistence/settings.py:22  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] src/simple_notepad/persistence/settings.py:46  [MAGIC_NUMBER]  Bare numeric literal 10
[HIGH] src/simple_notepad/syntax/syntax_loader.py:69  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/syntax/syntax_loader.py:75  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/syntax/syntax_loader.py:76  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/syntax/syntax_loader.py:77  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/syntax/syntax_loader.py:78  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/syntax/syntax_loader.py:79  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/syntax/syntax_loader.py:80  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/syntax/syntax_loader.py:82  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/syntax/syntax_loader.py:83  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/syntax/syntax_loader.py:84  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[MEDIUM] src/simple_notepad/editor/editor_tab.py:93  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 84–89
[HIGH] src/simple_notepad/editor/qcodeeditor_wrapper.py:93  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/editor/qcodeeditor_wrapper.py:94  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/editor/qcodeeditor_wrapper.py:95  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/editor/qcodeeditor_wrapper.py:96  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/editor/qcodeeditor_wrapper.py:97  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] src/simple_notepad/editor/qcodeeditor_wrapper.py:98  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] src/simple_notepad/editor/qcodeeditor_wrapper.py:41  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] src/simple_notepad/editor/qcodeeditor_wrapper.py:5  [DEAD_IMPORT]  'QColor' imported but never used
[LOW] src/simple_notepad/editor/qcodeeditor_wrapper.py:5  [DEAD_IMPORT]  'QTextCursor' imported but never used
------------------------------------------------------------
Total findings: 62
  HIGH: 22
  MEDIUM: 1
  LOW: 39
By type:
  MAGIC_NUMBER: 32
  DEEP_NESTING: 22
  DEAD_IMPORT: 5
  PRIMITIVE_OBSESSION: 2
  DUPLICATE_CODE: 1
```
