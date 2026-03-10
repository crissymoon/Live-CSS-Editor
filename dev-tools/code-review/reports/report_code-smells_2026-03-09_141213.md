# Code Review Report — Code Smells

**Directory:** `/Users/mac/Desktop/api_keeper/my-ruler`  
**Generated:** 2026-03-09 14:12:13  

---

```
Code Smells scan: /Users/mac/Desktop/api_keeper/my-ruler
------------------------------------------------------------
[LOW] panel_state.py:1  [DEAD_IMPORT]  'annotations' imported but never used
[LOW] window_watcher.py:15  [DEAD_IMPORT]  'annotations' imported but never used
[MEDIUM] widget_ruler.py:404  [LONG_METHOD]  Method body ~65 lines (max 60)
[LOW] widget_ruler.py:226  [MAGIC_NUMBER]  Bare numeric literal 55
[LOW] widget_ruler.py:227  [MAGIC_NUMBER]  Bare numeric literal 50
[LOW] widget_ruler.py:228  [MAGIC_NUMBER]  Bare numeric literal 38
[LOW] widget_ruler.py:229  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] widget_ruler.py:230  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] widget_ruler.py:232  [MAGIC_NUMBER]  Bare numeric literal 14
[LOW] widget_ruler.py:253  [MAGIC_NUMBER]  Bare numeric literal 90
[LOW] widget_ruler.py:289  [MAGIC_NUMBER]  Bare numeric literal 90
[LOW] widget_ruler.py:440  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] widget_ruler.py:443  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] widget_ruler.py:17  [DEAD_IMPORT]  'annotations' imported but never used
[LOW] registry.py:24  [DEAD_IMPORT]  'annotations' imported but never used
[LOW] widget_tray.py:40  [MAGIC_NUMBER]  Bare numeric literal 22
[LOW] widget_tray.py:46  [MAGIC_NUMBER]  Bare numeric literal 40
[LOW] widget_tray.py:47  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] widget_tray.py:49  [MAGIC_NUMBER]  Bare numeric literal 80
[LOW] widget_tray.py:51  [MAGIC_NUMBER]  Bare numeric literal 19
[LOW] widget_tray.py:53  [MAGIC_NUMBER]  Bare numeric literal 180
[LOW] widget_tray.py:54  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] widget_tray.py:55  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] widget_tray.py:57  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] widget_tray.py:58  [MAGIC_NUMBER]  Bare numeric literal 11
[LOW] widget_tray.py:18  [DEAD_IMPORT]  'annotations' imported but never used
[LOW] widget_guide.py:165  [MAGIC_NUMBER]  Bare numeric literal 210
[LOW] widget_guide.py:168  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] widget_guide.py:171  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] widget_guide.py:179  [MAGIC_NUMBER]  Bare numeric literal 210
[LOW] widget_guide.py:182  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] widget_guide.py:185  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] widget_guide.py:14  [DEAD_IMPORT]  'annotations' imported but never used
[LOW] settings.py:16  [MAGIC_NUMBER]  Bare numeric literal 46
[LOW] settings.py:17  [MAGIC_NUMBER]  Bare numeric literal 28
[LOW] settings.py:18  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] settings.py:19  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] settings.py:20  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] settings.py:40  [MAGIC_NUMBER]  Bare numeric literal 14
[LOW] settings.py:41  [MAGIC_NUMBER]  Bare numeric literal 14
[LOW] settings.py:50  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] settings.py:51  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] settings.py:52  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] settings.py:53  [MAGIC_NUMBER]  Bare numeric literal 60
[LOW] settings.py:54  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] settings.py:55  [MAGIC_NUMBER]  Bare numeric literal 220
[LOW] settings.py:56  [MAGIC_NUMBER]  Bare numeric literal 255
[LOW] settings.py:62  [MAGIC_NUMBER]  Bare numeric literal 340
[LOW] settings.py:63  [MAGIC_NUMBER]  Bare numeric literal 460
[LOW] settings.py:64  [MAGIC_NUMBER]  Bare numeric literal 34
[LOW] settings.py:90  [MAGIC_NUMBER]  Bare numeric literal 141828
[LOW] settings.py:107  [MAGIC_NUMBER]  Bare numeric literal 243060
[LOW] main.py:96  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] main.py:116  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] main.py:119  [MAGIC_NUMBER]  Bare numeric literal 12
[LOW] main.py:154  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] main.py:280  [MAGIC_NUMBER]  Bare numeric literal 20
[LOW] main.py:22  [DEAD_IMPORT]  'annotations' imported but never used
------------------------------------------------------------
Total findings: 58
  MEDIUM: 1
  LOW: 57
By type:
  MAGIC_NUMBER: 50
  DEAD_IMPORT: 7
  LONG_METHOD: 1
```
