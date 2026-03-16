# Workspace Report (2026-03-16_08-20-43_UTC)

- Root: `C:\Users\criss\Desktop`
- Commands run: 4
- Successful: 0
- Failed: 4

## Repository Context

This repo is an inspiration and rapid-start workspace for building apps and prototypes quickly.
It intentionally mixes starter flows, experiments, and reusable tooling in one growing codebase.

Some auth and database values in the workspace are for local dev preview and starter setups only, not production secrets.
Sensitive assets should remain outside the repo root and be linked in locally when needed.

## Artifacts

- Markdown: `C:\Users\criss\Desktop\dev_tools\dev-tools\zyx_planning_and_visuals\reports\workspace_report_2026-03-16_08-20-43_UTC.md`
- JSON: `C:\Users\criss\Desktop\dev_tools\dev-tools\zyx_planning_and_visuals\reports\workspace_report_2026-03-16_08-20-43_UTC.json`

## Command Results

### security_scan [FAIL]

- Return code: `2`
- Duration: `0.08s`
- Command: `C:\Users\criss\AppData\Local\Programs\Python\Python313\python.exe C:\Users\criss\Desktop\dev-tools\code-review\security_ck.py C:\Users\criss\Desktop --no-color`

#### stdout (tail)

```text
(no output)
```

#### stderr (tail)

```text
C:\Users\criss\AppData\Local\Programs\Python\Python313\python.exe: can't open file 'C:\\Users\\criss\\Desktop\\dev-tools\\code-review\\security_ck.py': [Errno 2] No such file or directory
```

### lines_count [FAIL]

- Return code: `2`
- Duration: `0.08s`
- Command: `C:\Users\criss\AppData\Local\Programs\Python\Python313\python.exe C:\Users\criss\Desktop\dev-tools\code-review\lines_count.py C:\Users\criss\Desktop 1200`

#### stdout (tail)

```text
(no output)
```

#### stderr (tail)

```text
C:\Users\criss\AppData\Local\Programs\Python\Python313\python.exe: can't open file 'C:\\Users\\criss\\Desktop\\dev-tools\\code-review\\lines_count.py': [Errno 2] No such file or directory
```

### python_audit [FAIL]

- Return code: `2`
- Duration: `0.07s`
- Command: `C:\Users\criss\AppData\Local\Programs\Python\Python313\python.exe C:\Users\criss\Desktop\dev-tools\code-review\py_audit.py --dir C:\Users\criss\Desktop`

#### stdout (tail)

```text
(no output)
```

#### stderr (tail)

```text
C:\Users\criss\AppData\Local\Programs\Python\Python313\python.exe: can't open file 'C:\\Users\\criss\\Desktop\\dev-tools\\code-review\\py_audit.py': [Errno 2] No such file or directory
```

### create_report [FAIL]

- Return code: `2`
- Duration: `0.07s`
- Command: `C:\Users\criss\AppData\Local\Programs\Python\Python313\python.exe C:\Users\criss\Desktop\dev-tools\zyx_planning_and_visuals\create_report.py --root C:\Users\criss\Desktop`

#### stdout (tail)

```text
(no output)
```

#### stderr (tail)

```text
C:\Users\criss\AppData\Local\Programs\Python\Python313\python.exe: can't open file 'C:\\Users\\criss\\Desktop\\dev-tools\\zyx_planning_and_visuals\\create_report.py': [Errno 2] No such file or directory
```
