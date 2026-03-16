# Workspace Report (2026-03-16_08-14-00_UTC)

- Root: `C:\Users\criss\Desktop\dev_tools`
- Commands run: 3
- Successful: 2
- Failed: 1

## Artifacts

- Markdown: `C:\Users\criss\Desktop\dev_tools\dev-tools\zyx_planning_and_visuals\reports\workspace_report_2026-03-16_08-14-00_UTC.md`
- JSON: `C:\Users\criss\Desktop\dev_tools\dev-tools\zyx_planning_and_visuals\reports\workspace_report_2026-03-16_08-14-00_UTC.json`

## Command Results

### security_scan [PASS]

- Return code: `0`
- Duration: `46.47s`
- Command: `C:\Users\criss\Desktop\dev_tools\.venv\Scripts\python.exe C:\Users\criss\Desktop\dev_tools\dev-tools\code-review\security_ck.py C:\Users\criss\Desktop\dev_tools --no-color`

#### stdout (tail)

```text
[security_ck] Scanning: C:\Users\criss\Desktop\dev_tools

======================================================================
 SECURITY SCAN FINDINGS
======================================================================

  FILE: C:\Users\criss\Desktop\dev_tools\db_bridge\connection.php
    Line    37  [HIGH]  Database URL / DSN  ->  sqlite******

  FILE: C:\Users\criss\Desktop\dev_tools\dev-tools\debug-tool\HOW_TO_SEND_ERRORS.md
    Line   271  [HIGH]  API key assignment  ->  my-sec*******
    Line   279  [HIGH]  API key assignment  ->  my-sec*******

  FILE: C:\Users\criss\Desktop\dev_tools\page-builder\xcm_auth\README.md
    Line    34  [HIGH]  Database URL / DSN  ->  ./xcm_***********

  FILE: C:\Users\criss\Desktop\dev_tools\page-builder\xcm_auth\db\sqlite.go
    Line   105  [HIGH]  Database URL / DSN  ->  ./xcm_*******

  FILE: C:\Users\criss\Desktop\dev_tools\dev-tools\db-browser\databases\example_usage.c
    Line    16  [MEDIUM]  Password assignment  ->  MySecu**************
    Line    50  [MEDIUM]  Password assignment  ->  Secure************
    Line    72  [MEDIUM]  Password assignment  ->  Remote********************
    Line   102  [MEDIUM]  Password assignment  ->  Workin*************
    Line   145  [MEDIUM]  Password assignment  ->  Backup***********

----------------------------------------------------------------------
 SUMMARY
----------------------------------------------------------------------
  Total findings : 10
  HIGH      : 5
  MEDIUM    : 5
======================================================================

RECOMMENDATIONS:
  - Move secrets to environment variables or a .env file.
  - Add .env to .gitignore so it is never committed.
  - Use a secrets manager (e.g. AWS Secrets Manager, Vault, Doppler).
  - Rotate any keys that may have already been exposed.
  - Review each finding manually; some may be intentional test values.
```

#### stderr (tail)

```text
[security_ck] Loaded 29 detection patterns.
[security_ck] ...scanned 50 files so far
[security_ck] ...scanned 100 files so far
[security_ck] ...scanned 150 files so far
[security_ck] ...scanned 200 files so far
[security_ck] ...scanned 250 files so far
[security_ck] ...scanned 300 files so far
[security_ck] ...scanned 350 files so far
[security_ck] ...scanned 400 files so far
[security_ck] ...scanned 450 files so far
[security_ck] ...scanned 500 files so far
[security_ck] ...scanned 550 files so far
[security_ck] ...scanned 600 files so far
[security_ck] ...scanned 650 files so far
[security_ck] ...scanned 700 files so far
[security_ck] ...scanned 750 files so far
[security_ck] ...scanned 800 files so far
[security_ck] ...scanned 850 files so far
[security_ck] Scan complete. Files scanned: 873, errors: 0
```

### lines_count [PASS]

- Return code: `0`
- Duration: `1.22s`
- Command: `C:\Users\criss\Desktop\dev_tools\.venv\Scripts\python.exe C:\Users\criss\Desktop\dev_tools\dev-tools\code-review\lines_count.py C:\Users\criss\Desktop\dev_tools 1200`

#### stdout (tail)

```text
Scanning : C:\Users\criss\Desktop\dev_tools
Threshold: 1200 lines
------------------------------------------------------------
Lines  File
-----  --------------------------------------------------
 2344  dev-tools\dev-browser\render_core\src\xcm_sdl_preview.cpp
 2045  dev-tools\db-browser\legacy\main.c
 1706  dev-tools\dev-browser\modules\_js_injections.py
 1503  page-builder\bindings\typescript\examples\wasm_web_embed.ts
 1469  page-builder\build.php
 1459  my_project\style-sheets\quill-cartesian.css
 1425  dev-tools\dev-browser\modules\wkwebview_widget.py
 1407  my_project\style-sheets\dark-neu.css
 1397  dev-tools\dev-browser\apps\pdf-sign\index.php
 1394  dev-tools\dev-browser\modules\console_panel.py
 1338  my_project\style-sheets\crystal-ui.css
 1327  page-builder\bindings\typescript\engine_showcase.html
 1291  my_project\css\agent.css
 1287  dev-tools\c_tools\tui_agent\main.py
 1280  my_project\style-sheets\neon-grid.css
 1252  my_project\style-sheets\keyboard-ui.css
 1250  my_project\style-sheets\retro90s.css
 1214  my_project\style-sheets\atom-age.css
 1212  my_project\style-sheets\neumorphism.css
------------------------------------------------------------
Total files over 1200 lines: 19
```

#### stderr (tail)

```text
(no output)
```

### python_audit [FAIL]

- Return code: `1`
- Duration: `1.34s`
- Command: `C:\Users\criss\Desktop\dev_tools\.venv\Scripts\python.exe C:\Users\criss\Desktop\dev_tools\dev-tools\code-review\py_audit.py --dir C:\Users\criss\Desktop\dev_tools`

#### stdout (tail)

```text
  ----------------------
  NOTE    L88    [A001] Public function 'cyan' has no type annotations (PEP 484). Adding annotations improves IDE support and catches type errors early.
  NOTE    L89    [A001] Public function 'green' has no type annotations (PEP 484). Adding annotations improves IDE support and catches type errors early.
  NOTE    L90    [A001] Public function 'yellow' has no type annotations (PEP 484). Adding annotations improves IDE support and catches type errors early.
  NOTE    L91    [A001] Public function 'red' has no type annotations (PEP 484). Adding annotations improves IDE support and catches type errors early.
  NOTE    L92    [A001] Public function 'bold' has no type annotations (PEP 484). Adding annotations improves IDE support and catches type errors early.
  NOTE    L93    [A001] Public function 'dim' has no type annotations (PEP 484). Adding annotations improves IDE support and catches type errors early.
  NOTE    L95    [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L96    [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L97    [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L98    [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L99    [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L405   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L408   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L419   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L420   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L421   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L427   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L428   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L429   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L430   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L431   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L447   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L450   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L518   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L528   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L535   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L538   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L544   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L552   [D003] print() in a library module -- consider using logging so callers can control verbosity.

  livecss_flutter\ios\Flutter\ephemeral\flutter_lldb_helper.py
  --------------------------------------------------------------
  NOTE    L21    [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L32    [D003] print() in a library module -- consider using logging so callers can control verbosity.

  make_readme.py
  ----------------
  NOTE    L162   [D003] print() in a library module -- consider using logging so callers can control verbosity.

  page-builder\xcm_auth\smoke\totp_bridge_server.py
  ---------------------------------------------------
  NOTE    L35    [L001] Line is 146 characters (limit 120). Long lines reduce readability.
           dbc = ((digest[offset] & 0x7F) << 24) | ((digest[offset + 1] & 0xFF) << 16) ...
  NOTE    L52    [L001] Line is 158 characters (limit 120). Long lines reduce readability.
           def __init__(self, auth_url: str, creds: Dict[str, Any], secret: str, issuer...
  NOTE    L187   [L001] Line is 133 characters (limit 120). Long lines reduce readability.
           self._json(401, {'ok': False, 'message': payload.get('message') ...
  NOTE    L192   [L001] Line is 160 characters (limit 120). Long lines reduce readability.
           self._json(400, {'ok': False, 'message': 'xcm_auth returned twof...
  NOTE    L223   [L001] Line is 134 characters (limit 120). Long lines reduce readability.
           self._json(200, {'ok': True, 'data': {'access_granted': True, 'messa...
  NOTE    L264   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L265   [D003] print() in a library module -- consider using logging so callers can control verbosity.
  NOTE    L266   [D003] print() in a library module -- consider using logging so callers can control verbosity.

====================================================================
  SUMMARY   1336 finding(s) in 90 file(s)
            errors=12  warnings=101  info=43  notes=1180
====================================================================
```

#### stderr (tail)

```text
(no output)
```
