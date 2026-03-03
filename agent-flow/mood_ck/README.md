# mood_ck

Mood analysis tool for AI prompt augmentation.
Uses the EMI (Emotion and Mood Intelligence) bio-inspired engine to score the emotional
state of any text, then returns a single-sentence addition you can append to any AI API prompt.

---

## How it works

1. Input text is split into overlapping word-windows (default 250 words, 30-word overlap)
   so arbitrarily long prompts are handled without memory pressure.
2. Each chunk is scored by the EMI engine using a bio-inspired adaptive sigmoid
   derived from 512-channel BCI neural recordings cross-referenced with a 370K-word
   emotional lexicon.
3. Per-chunk scores are aggregated using a weighted average where the weight is each
   chunk's lexicon match rate, so low-signal chunks do not distort the result.
4. The dominant emotion plus any significant runner-up produce a one-sentence descriptor.

---

## Files

```
mood_ck/
  mood_check.py     main analysis script
  emi_model/        symlink -> /Users/mac/Documents/deploy_emi_model
  README.md         this file
```

`emi_model/` contains:
- `engine.py`    EMIEngine class (imported directly by mood_check.py)
- `emi.py`       interactive TUI (not used by mood_check.py)
- `lexicon.json` 370K-word emotional lexicon with neural load data

---

## Usage

```bash
# full report with bars and scores
python mood_check.py "I am really excited about this project"

# one-sentence addon only -- paste this into your API prompt
python mood_check.py --addon "your user message here"

# read from a file
python mood_check.py --file /path/to/prompt.txt

# pipe from stdin
echo "I feel so overwhelmed today" | python mood_check.py --addon

# raw JSON output (no colour, suitable for scripting)
python mood_check.py --json "some text"

# tune chunking for very long prompts
python mood_check.py --chunk-words 150 --overlap 40 --addon "very long text..."

# use a custom lexicon
python mood_check.py --lexicon /path/to/lexicon.json "text"
```

---

## Output sentence examples

Each mood maps to a one-sentence prompt addition:

| Mood    | Sentence |
|---------|---------|
| joy     | The user is writing with enthusiasm and positivity. |
| love    | The user's message carries warmth and genuine affection. |
| care    | The user's tone is thoughtful and nurturing in this request. |
| anger   | The user's writing carries noticeable frustration or irritation. |
| sadness | The user appears to be writing from a place of sadness or low energy. |
| fear    | The user's message conveys anxiety or apprehension about the topic. |
| stress  | The user's writing suggests they may be under pressure or stress. |
| neutral | The user's message is calm and emotionally neutral. |

When a runner-up emotion reaches 20% or more of the signal, a secondary clause is added.
Example: "The user's message is calm and emotionally neutral with moments of optimism."

---

## Scripting into an API call

```python
import subprocess, json

result = subprocess.run(
    ["python", "mood_ck/mood_check.py", "--addon", user_text],
    capture_output=True, text=True
)
mood_sentence = result.stdout.strip()

messages = [
    {"role": "system", "content": f"Context: {mood_sentence}"},
    {"role": "user",   "content": user_text},
]
```

---

## Exit codes

| Code | Meaning |
|------|---------|
| 0    | Success |
| 1    | Engine load failure (missing lexicon or import error) |
| 2    | Bad arguments or no input supplied |

---

## Emotions detected

joy, love, care, anger, sadness, fear, stress, neutral
