# Code Review Report — Code Smells

**Directory:** `/Users/mac/Desktop/api_keeper/xcm-builder`  
**Generated:** 2026-03-08 14:50:42  

---

```
Code Smells scan: /Users/mac/Desktop/api_keeper/xcm-builder
------------------------------------------------------------
[HIGH] bridge.py:120  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] bridge.py:122  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[MEDIUM] bridge.py:269  [LONG_METHOD]  Method body ~61 lines (max 60)
[LOW] bridge.py:88  [MAGIC_NUMBER]  Bare numeric literal 85
[LOW] bridge.py:273  [MAGIC_NUMBER]  Bare numeric literal 8765
[LOW] bridge.py:283  [MAGIC_NUMBER]  Bare numeric literal 82
[LOW] bridge.py:290  [MAGIC_NUMBER]  Bare numeric literal 8765
[LOW] bridge.py:359  [MAGIC_NUMBER]  Bare numeric literal 85
[HIGH] bridge.py:300  [EMPTY_CATCH]  Empty except block (swallowed exception)
------------------------------------------------------------
Total findings: 9
  HIGH: 3
  MEDIUM: 1
  LOW: 5
By type:
  MAGIC_NUMBER: 5
  DEEP_NESTING: 2
  LONG_METHOD: 1
  EMPTY_CATCH: 1
```
