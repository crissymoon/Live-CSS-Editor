# crisis_detection

Pure-Python, regex-based crisis signal detection module.  
No ML models, no GPU, no external inference dependencies.

Part of the `prompt_inj_guard` safety stack.

---

## Overview

This module scans free-form text for signals that a person may be experiencing a
life-threatening or acute crisis situation and returns:

- Whether the text is flagged
- The dominant crisis category
- A severity score (0-10)
- Confidence (0.0-1.0)
- Which patterns matched (with negation tracking)
- Whether urgency context boosters were detected
- Relevant safe resources (hotlines, URLs) for the detected category

The detector hot-reloads `patterns.json` and `safe_resources.json` when their
on-disk modification time changes, so pattern updates take effect without
restarting the process.

---

## Categories

| Category | Label | Base Severity |
|---|---|---|
| Suicide / Suicidal Ideation | `suicide` | 10 |
| Non-Suicidal Self-Harm | `self_harm` | 8 |
| Domestic Violence / Abuse | `domestic_violence` | 9 |
| Acute Mental Health Crisis | `mental_health_crisis` | 7 |
| Substance / Overdose Crisis | `substance_crisis` | 9 |
| Child Safety / Abuse | `child_safety` | 10 |
| Acute Medical Emergency | `medical_emergency` | 9 |

---

## Files

```
crisis_detection/
  __init__.py          package init, exports CrisisDetector
  detector.py          CrisisDetector class (core logic)
  api.py               standalone Flask API server
  test_detector.py     test suite (no pytest required)
  patterns.json        crisis pattern database (regex + metadata)
  safe_resources.json  hotlines and URLs by category
```

---

## Quick Start

### Python

```python
from crisis_detection.detector import CrisisDetector

detector = CrisisDetector()
result = detector.detect("I want to kill myself tonight")

print(result["flagged"])        # True
print(result["category"])       # suicide
print(result["severity"])       # 10
print(result["confidence"])     # 0.9 (approx)

if result["safe_resources"]:
    for line in result["safe_resources"]["hotlines"]:
        print(line["name"], line.get("number"))
```

### Bulk detection

```python
results = detector.detect_bulk([
    "hello how are you",
    "I've been cutting myself to cope",
    "what's the weather today",
])
```

---

## Result Schema

```json
{
  "flagged": true,
  "category": "suicide",
  "category_display": "Suicide / Suicidal Ideation",
  "severity": 10,
  "confidence": 0.875,
  "matched_patterns": [
    {
      "id": "sui_001",
      "category": "suicide",
      "severity": 10,
      "description": "Direct statement of suicidal intent",
      "matched_text": "kill myself",
      "negated": false
    }
  ],
  "context_boost_detected": true,
  "safe_resources": {
    "label": "Suicide / Suicidal Ideation",
    "message": "You are not alone. Please reach out to a crisis line right now.",
    "hotlines": [
      {
        "name": "988 Suicide and Crisis Lifeline",
        "number": "988",
        "sms": "Text HOME to 741741",
        "url": "https://988lifeline.org",
        "availability": "24/7"
      }
    ]
  },
  "error": null
}
```

---

## API Server

Start the standalone server:

```bash
python crisis_detection/api.py
# or with options:
python crisis_detection/api.py --port 5200 --host 0.0.0.0
```

Environment variable configuration:

| Variable | Default | Description |
|---|---|---|
| `CRISIS_PATTERNS_PATH` | `patterns.json` (sibling) | Path to patterns.json |
| `CRISIS_RESOURCES_PATH` | `safe_resources.json` (sibling) | Path to resources file |
| `CRISIS_HOST` | `127.0.0.1` | Bind host |
| `CRISIS_PORT` | `5100` | Bind port |

### Endpoints

#### GET /health

```json
{
  "status": "ok",
  "module": "crisis_detection",
  "pattern_count": 14,
  "categories": ["suicide", "self_harm", "domestic_violence", ...]
}
```

#### POST /detect

Request:
```json
{"text": "I want to kill myself"}
```

Response: result schema above.

#### POST /detect/bulk

Request:
```json
{"texts": ["...", "...", "..."]}
```

Response:
```json
{
  "count": 3,
  "results": [...]
}
```

Maximum 200 texts per request.

---

## Running Tests

```bash
cd /path/to/live-css
python prompt_inj_guard/crisis_detection/test_detector.py
```

No pytest install required. Exit code 0 on all pass, 1 if any fail.

---

## Updating Patterns

Edit `patterns.json` directly. The running detector will pick up changes on the
next call without restart.

Each pattern entry:

```json
{
  "id": "sui_001",
  "category": "suicide",
  "severity": 10,
  "description": "Human-readable description",
  "weight": 1.0,
  "patterns": [
    "regex pattern one",
    "regex pattern two"
  ]
}
```

- `severity`: 1-10 (10 = most severe)
- `weight`: contribution to confidence score (0.5-1.0 range recommended)
- `patterns`: list of Python regex strings (compiled with `re.IGNORECASE`)

---

## Negation and Context

**Negation phrases** (in `patterns.json > negation_phrases`) are checked in the
80 characters before each match position. If a negation phrase is found, the
match is marked `"negated": true` and excluded from the severity/confidence
calculation.

**Context boosters** (in `patterns.json > context_boosters`) are phrases like
"right now", "tonight", "I have decided". When present they add +1 to severity
(capped at 10) and +0.10 to confidence.

---

## Integration with prompt_inj_guard

The crisis detector can be mounted alongside the existing guard API or run as a
separate service on its own port. It has no shared state with `rule_guard.py` or
`guard_classifier.py`.

Example combined check in a gateway layer:

```python
guard_result  = rule_guard.check(text)
crisis_result = detector.detect(text)

if crisis_result["flagged"] and crisis_result["severity"] >= 8:
    # surface safe resources before processing further
    ...
if guard_result["flagged"] and not guard_result["is_allowlisted"]:
    # block prompt injection
    ...
```

---

## Design Decisions

- No ML dependency -- keeps the module fast and deployable anywhere Python runs
- Hot-reload -- patterns can be tuned in production without downtime
- Negation window is character-based (80 chars look-behind), not sentence-based,
  to keep it simple and fast
- Safe resources are decoupled from patterns -- update hotlines without touching regex
- All errors are caught internally and logged to stderr; the public API never raises
