# Code Review Report — God Functions

**Directory:** `/Users/mac/Desktop/api_keeper`  
**Generated:** 2026-03-08 14:48:27  

---

```
============================================================
GOD FUNCTIONS REPORT
============================================================
Files scanned:      143
Functions analyzed: 1263
God functions found: 8
  CRITICAL: 3
  HIGH:     5
By language:
  JavaScript: 7
  PHP: 1
------------------------------------------------------------
DETECTED GOD FUNCTIONS
------------------------------------------------------------
!!! [CRITICAL] toJSON()
    File: lead-pulls/timetechpro/public_html/xcm-datasets/datasets/javascript_code.js
    Language: JavaScript
    Thresholds exceeded: 3/4
    Reasons:
      - 167 lines (>150)
      - complexity 53 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] generatePreviewHTML()
    File: lead-pulls/xcaliburmoon/landing_mgr/app.js
    Language: JavaScript
    Thresholds exceeded: 3/4
    Reasons:
      - 193 lines (>150)
      - complexity 29 (>20)
      - nesting 5 (>4)
!!! [CRITICAL] generatePreviewHTML()
    File: lead-pulls/xcaliburmoon/landing_mgr/app.js
    Language: JavaScript
    Thresholds exceeded: 3/4
    Reasons:
      - 193 lines (>150)
      - complexity 29 (>20)
      - nesting 5 (>4)
 !  [HIGH] search_datasets()
    File: lead-pulls/timetechpro/public_html/xcm-datasets/index.php
    Language: PHP
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 21 (>20)
      - nesting 6 (>4)
 !  [HIGH] appendMessageElement()
    File: ai-chat/public/assets/js/chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 5 (>4)
 !  [HIGH] sendFileContent()
    File: ai-chat/public/assets/js/chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 50 (>20)
      - nesting 6 (>4)
 !  [HIGH] appendMessageElement()
    File: ai-chat/public/assets/js/chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 5 (>4)
 !  [HIGH] sendFileContent()
    File: ai-chat/public/assets/js/chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 50 (>20)
      - nesting 6 (>4)
============================================================
```
