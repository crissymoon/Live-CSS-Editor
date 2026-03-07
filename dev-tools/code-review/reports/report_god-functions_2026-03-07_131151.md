# Code Review Report — God Functions

**Directory:** `/Users/mac/Documents/live-css/my_project`  
**Generated:** 2026-03-07 13:11:51  

---

```
============================================================
GOD FUNCTIONS REPORT
============================================================
Files scanned:      85
Functions analyzed: 836
God functions found: 51
  CRITICAL: 2
  HIGH:     49
By language:
  JavaScript: 49
  PHP: 2
------------------------------------------------------------
DETECTED GOD FUNCTIONS
------------------------------------------------------------
!!! [CRITICAL] openSlider()
    File: js/size-slider.js
    Language: JavaScript
    Thresholds exceeded: 3/4
    Reasons:
      - 165 lines (>150)
      - complexity 49 (>20)
      - 7 params (>5)
!!! [CRITICAL] load()
    File: js/app.js
    Language: JavaScript
    Thresholds exceeded: 3/4
    Reasons:
      - 268 lines (>150)
      - complexity 61 (>20)
      - nesting 10 (>4)
 !  [HIGH] bridge_api_template()
    File: setup.php
    Language: PHP
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 6 (>4)
 !  [HIGH] scoreThemes()
    File: style-sheets/parser.php
    Language: PHP
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 29 (>20)
      - nesting 6 (>4)
 !  [HIGH] sendMessage()
    File: js/ai-chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 7 (>4)
 !  [HIGH] sendMessage()
    File: js/ai-chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 7 (>4)
 !  [HIGH] attach()
    File: js/indent-guide.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - nesting 8 (>4)
 !  [HIGH] attach()
    File: js/indent-guide.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - nesting 8 (>4)
 !  [HIGH] on()
    File: js/indent-guide.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 24 (>20)
      - nesting 7 (>4)
 !  [HIGH] attachFuzzy()
    File: js/fuzzy.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 40 (>20)
      - nesting 5 (>4)
 !  [HIGH] attachFuzzy()
    File: js/fuzzy.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 40 (>20)
      - nesting 5 (>4)
 !  [HIGH] attachSwatches()
    File: js/color-swatch.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 6 (>4)
 !  [HIGH] attachSwatches()
    File: js/color-swatch.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 6 (>4)
 !  [HIGH] attachSizeSliders()
    File: js/size-slider.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - nesting 5 (>4)
 !  [HIGH] openSlider()
    File: js/size-slider.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - 165 lines (>150)
      - complexity 49 (>20)
 !  [HIGH] attachSizeSliders()
    File: js/size-slider.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - nesting 5 (>4)
 !  [HIGH] init()
    File: js/gutter.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - nesting 6 (>4)
 !  [HIGH] init()
    File: js/gutter.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 31 (>20)
      - nesting 6 (>4)
 !  [HIGH] init()
    File: js/wireframe/init.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] init()
    File: js/wireframe/init.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 5 (>4)
 !  [HIGH] clampResizeToLockedGuides()
    File: js/wireframe/geometry.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 6 params (>5)
 !  [HIGH] clampResizeToLockedGuides()
    File: js/wireframe/geometry.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - nesting 5 (>4)
      - 6 params (>5)
 !  [HIGH] onMousemove()
    File: js/wireframe/input.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 5 (>4)
 !  [HIGH] onMousemove()
    File: js/wireframe/input.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 5 (>4)
 !  [HIGH] chatSend()
    File: js/agent/agent-chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 35 (>20)
      - nesting 11 (>4)
 !  [HIGH] pump()
    File: js/agent/agent-chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 9 (>4)
 !  [HIGH] showApplyBar()
    File: js/agent/agent-chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 32 (>20)
      - nesting 7 (>4)
 !  [HIGH] chatSend()
    File: js/agent/agent-chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 35 (>20)
      - nesting 11 (>4)
 !  [HIGH] then()
    File: js/agent/agent-chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 10 (>4)
 !  [HIGH] pump()
    File: js/agent/agent-chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 9 (>4)
 !  [HIGH] then()
    File: js/agent/agent-chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 27 (>20)
      - nesting 8 (>4)
 !  [HIGH] showApplyBar()
    File: js/agent/agent-chat.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 32 (>20)
      - nesting 7 (>4)
 !  [HIGH] streamRun()
    File: js/agent/agent-run.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 6 (>4)
 !  [HIGH] streamRunDirect()
    File: js/agent/agent-run.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 6 (>4)
 !  [HIGH] applyAIResult()
    File: js/agent/agent-run.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 6 (>4)
 !  [HIGH] streamRun()
    File: js/agent/agent-run.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 23 (>20)
      - nesting 6 (>4)
 !  [HIGH] streamRunDirect()
    File: js/agent/agent-run.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 26 (>20)
      - nesting 6 (>4)
 !  [HIGH] applyAIResult()
    File: js/agent/agent-run.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 28 (>20)
      - nesting 6 (>4)
 !  [HIGH] then()
    File: js/agent/agent-run.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 25 (>20)
      - nesting 5 (>4)
 !  [HIGH] requestAiPreview()
    File: js/theme_randomizer/ai-preview.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - nesting 5 (>4)
 !  [HIGH] requestAiPreview()
    File: js/theme_randomizer/ai-preview.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 30 (>20)
      - nesting 5 (>4)
 !  [HIGH] findSelectorInLine()
    File: js/editor/goto-css.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 39 (>20)
      - nesting 7 (>4)
 !  [HIGH] jumpToCssRule()
    File: js/editor/goto-css.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 6 (>4)
 !  [HIGH] findSelectorInLine()
    File: js/editor/goto-css.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 39 (>20)
      - nesting 7 (>4)
 !  [HIGH] jumpToCssRule()
    File: js/editor/goto-css.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 33 (>20)
      - nesting 6 (>4)
 !  [HIGH] buildContextMenuScript()
    File: js/editor/preview.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - nesting 7 (>4)
 !  [HIGH] ctxMenuInit()
    File: js/editor/preview.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - nesting 6 (>4)
 !  [HIGH] buildContextMenuScript()
    File: js/editor/preview.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - nesting 7 (>4)
 !  [HIGH] ctxMenuInit()
    File: js/editor/preview.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 37 (>20)
      - nesting 6 (>4)
 !  [HIGH] pollProjectSave()
    File: vscode-bridge/js/bridge-sync.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 6 (>4)
 !  [HIGH] pollProjectSave()
    File: vscode-bridge/js/bridge-sync.js
    Language: JavaScript
    Thresholds exceeded: 2/4
    Reasons:
      - complexity 22 (>20)
      - nesting 6 (>4)
============================================================
```
