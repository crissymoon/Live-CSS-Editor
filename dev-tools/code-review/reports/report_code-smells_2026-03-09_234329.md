# Code Review Report — Code Smells

**Directory:** `/Users/mac/Documents/spreadsheet_tool/imgui-browser/src`  
**Generated:** 2026-03-09 23:43:29  

---

```
Code Smells scan: /Users/mac/Documents/spreadsheet_tool/imgui-browser/src
------------------------------------------------------------
[HIGH] browser_settings.h:158  [DEEP_NESTING]  Nesting depth ~9 (max 4)
[HIGH] browser_settings.h:180  [DEEP_NESTING]  Nesting depth ~9 (max 4)
[HIGH] browser_settings.h:193  [DEEP_NESTING]  Nesting depth ~9 (max 4)
[LOW] browser_settings.h:87  [MAGIC_NUMBER]  Bare numeric literal 8082
[LOW] browser_settings.h:122  [MAGIC_NUMBER]  Bare numeric literal 32
[LOW] browser_settings.h:123  [MAGIC_NUMBER]  Bare numeric literal 74
[LOW] browser_settings.h:124  [MAGIC_NUMBER]  Bare numeric literal 70
[LOW] browser_settings.h:125  [MAGIC_NUMBER]  Bare numeric literal 114
[LOW] browser_settings.h:197  [MAGIC_NUMBER]  Bare numeric literal 10
[MEDIUM] browser_settings.h:188  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 175–180
[HIGH] xcm_shell.h:38  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] xcm_shell.h:39  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] webview.h:71  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] webview.h:72  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[LOW] app_state.h:17  [MAGIC_NUMBER]  Bare numeric literal 44
[LOW] app_state.h:19  [MAGIC_NUMBER]  Bare numeric literal 70
[LOW] app_state.h:21  [MAGIC_NUMBER]  Bare numeric literal 82
[LOW] app_state.h:22  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] app_state.h:23  [MAGIC_NUMBER]  Bare numeric literal 2048
[LOW] app_state.h:24  [MAGIC_NUMBER]  Bare numeric literal 9878
[LOW] app_state.h:25  [MAGIC_NUMBER]  Bare numeric literal 9879
[LOW] app_state.h:57  [MAGIC_NUMBER]  Bare numeric literal 120
[LOW] app_state.h:96  [MAGIC_NUMBER]  Bare numeric literal 1400
[LOW] app_state.h:97  [MAGIC_NUMBER]  Bare numeric literal 900
[LOW] app_state.h:133  [MAGIC_NUMBER]  Bare numeric literal 128
[HIGH] persistence.h:77  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] persistence.h:78  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] persistence.h:79  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] persistence.h:80  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] persistence.h:81  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] persistence.h:82  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] persistence.h:83  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] persistence.h:84  [DEEP_NESTING]  Nesting depth ~7 (max 4)
[HIGH] persistence.h:85  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] persistence.h:86  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] persistence.h:87  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] persistence.h:88  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] persistence.h:164  [DEEP_NESTING]  Nesting depth ~8 (max 4)
[LOW] persistence.h:36  [MAGIC_NUMBER]  Bare numeric literal 700
[LOW] persistence.h:94  [MAGIC_NUMBER]  Bare numeric literal 700
[LOW] persistence.h:164  [MAGIC_NUMBER]  Bare numeric literal 800
[LOW] cmds-and-server/server_manager.h:16  [MAGIC_NUMBER]  Bare numeric literal 9879
[LOW] cmds-and-server/server_manager.h:25  [MAGIC_NUMBER]  Bare numeric literal 8082
[LOW] cmds-and-server/server_manager.h:32  [MAGIC_NUMBER]  Bare numeric literal 9879
[LOW] cmds-and-server/cmd_server.h:12  [MAGIC_NUMBER]  Bare numeric literal 9878
[LOW] webview/webview_js.h:22  [MAGIC_NUMBER]  Bare numeric literal 127
[LOW] webview/webview_js.h:37  [MAGIC_NUMBER]  Bare numeric literal 13
[LOW] webview/webview_js.h:38  [MAGIC_NUMBER]  Bare numeric literal 17
[LOW] webview/webview_js.h:52  [MAGIC_NUMBER]  Bare numeric literal 16
[LOW] webview/webview_js.h:55  [MAGIC_NUMBER]  Bare numeric literal 256
[LOW] webview/webview_js.h:118  [MAGIC_NUMBER]  Bare numeric literal 1280
[LOW] webview/webview_js.h:124  [MAGIC_NUMBER]  Bare numeric literal 800
[MEDIUM] webview/webview_js.h:110  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 101–106
[LOW] top-of-gui/chrome.js:1  [MAGIC_NUMBER]  Bare numeric literal 71
[LOW] top-of-gui/chrome.js:71  [MAGIC_NUMBER]  Bare numeric literal 14
[LOW] top-of-gui/chrome.js:72  [MAGIC_NUMBER]  Bare numeric literal 14
[LOW] top-of-gui/chrome.js:89  [MAGIC_NUMBER]  Bare numeric literal 24
[LOW] top-of-gui/chrome.js:111  [MAGIC_NUMBER]  Bare numeric literal 10
[LOW] top-of-gui/chrome.js:190  [MAGIC_NUMBER]  Bare numeric literal 9879
[LOW] top-of-gui/chrome.js:448  [MAGIC_NUMBER]  Bare numeric literal 220
[LOW] top-of-gui/chrome.js:464  [MAGIC_NUMBER]  Bare numeric literal 80
[LOW] top-of-gui/chrome.js:511  [MAGIC_NUMBER]  Bare numeric literal 40
[LOW] top-of-gui/chrome.js:514  [MAGIC_NUMBER]  Bare numeric literal 127
[LOW] top-of-gui/chrome.js:516  [MAGIC_NUMBER]  Bare numeric literal 40
[LOW] top-of-gui/chrome.js:534  [MAGIC_NUMBER]  Bare numeric literal 80
[LOW] top-of-gui/chrome.js:558  [MAGIC_NUMBER]  Bare numeric literal 80
[LOW] top-of-gui/chrome.js:781  [MAGIC_NUMBER]  Bare numeric literal 180
[HIGH] top-of-gui/chrome.js:12  [EMPTY_CATCH]  Empty catch block (swallowed exception)
[HIGH] top-of-gui/chrome.js:579  [EMPTY_CATCH]  Empty catch block (swallowed exception)
[HIGH] top-of-gui/chrome.js:658  [EMPTY_CATCH]  Empty catch block (swallowed exception)
[MEDIUM] top-of-gui/chrome.js:549  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 525–530
[MEDIUM] top-of-gui/chrome.js:550  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 526–531
[MEDIUM] top-of-gui/chrome.js:551  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 527–532
[MEDIUM] top-of-gui/chrome.js:552  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 528–533
[MEDIUM] top-of-gui/chrome.js:553  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 529–534
[MEDIUM] top-of-gui/chrome.js:554  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 530–535
[MEDIUM] top-of-gui/chrome.js:555  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 531–536
[MEDIUM] top-of-gui/chrome.js:556  [DUPLICATE_CODE]  Block of 6 lines duplicates lines 532–537
[HIGH] top-of-gui/chrome.h:20  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] top-of-gui/chrome.h:21  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] top-of-gui/chrome.h:22  [DEEP_NESTING]  Nesting depth ~5 (max 4)
[HIGH] top-of-gui/chrome.h:35  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[HIGH] top-of-gui/chrome.h:36  [DEEP_NESTING]  Nesting depth ~6 (max 4)
[LOW] main/main_crash.h:4  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] main/main_crash.h:5  [MAGIC_NUMBER]  Bare numeric literal 1024
[LOW] main/main_args.h:6  [MAGIC_NUMBER]  Bare numeric literal 8443
[LOW] main/main_args.h:9  [MAGIC_NUMBER]  Bare numeric literal 9879
[LOW] main/main_args.h:10  [MAGIC_NUMBER]  Bare numeric literal 9878
[LOW] main/main_args.h:11  [MAGIC_NUMBER]  Bare numeric literal 1400
[LOW] main/main_args.h:12  [MAGIC_NUMBER]  Bare numeric literal 900
------------------------------------------------------------
Total findings: 90
  HIGH: 28
  MEDIUM: 10
  LOW: 52
By type:
  MAGIC_NUMBER: 52
  DEEP_NESTING: 25
  DUPLICATE_CODE: 10
  EMPTY_CATCH: 3
```
