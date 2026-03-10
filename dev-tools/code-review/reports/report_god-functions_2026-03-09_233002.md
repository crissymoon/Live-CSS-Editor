# Code Review Report — God Functions

**Directory:** `/Users/mac/Documents/spreadsheet_tool/spreadsheet_tool`  
**Generated:** 2026-03-09 23:30:02  

---

```
============================================================
GOD FUNCTIONS REPORT
============================================================
Files scanned:      14
Functions analyzed: 209
God functions found: 8
  CRITICAL: 3
  HIGH:     5
By language:
  JavaScript: 8
------------------------------------------------------------
DETECTED GOD FUNCTIONS
------------------------------------------------------------
!!! [CRITICAL] Grid()
    File: js/grid.js
    Language: JavaScript
    Thresholds exceeded: 3/4
    Reasons:
      - 542 lines (>150)
      - complexity 153 (>20)
      - nesting 9 (>4)
!!! [CRITICAL] Formulas()
    File: js/formulas.js
    Language: JavaScript
    Thresholds exceeded: 3/4
    Reasons:
      - 388 lines (>150)
      - complexity 214 (>20)
      - nesting 7 (>4)
!!! [CRITICAL] AppModal()
    File: js/app_modal.js
    Language: JavaScript
    Thresholds exceeded: 3/4
    Reasons:
      - 157 lines (>150)
      - complexity 30 (>20)
      - nesting 5 (>4)
 !  [HIGH] GridPointer()
    File: js/grid_pointer.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - 153 lines (>150)
      - complexity 26 (>20)
 !  [HIGH] callFunction()
    File: js/formulas.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 102 (>20)
      - nesting 5 (>4)
 !  [HIGH] callFunction()
    File: js/formulas.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 102 (>20)
      - nesting 5 (>4)
 !  [HIGH] AppCore()
    File: js/app_core.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - 298 lines (>150)
      - complexity 56 (>20)
 !  [HIGH] GridEditor()
    File: js/grid_editor.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - nesting 5 (>4)
============================================================
```
