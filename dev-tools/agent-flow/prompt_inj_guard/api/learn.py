#!/usr/bin/env python3
"""
prompt_inj_guard/api/learn.py

Offline learning script. Reads feedback_log.jsonl (written by the /feedback
endpoint) and suggests new patterns to add to pattern_db.json.

How it works
------------
1. Reads feedback_log.jsonl and collects correction records
   (records where is_correction == true).
2. For confirmed injection / spam examples (predicted clean, correct != clean):
   - Extracts candidate n-gram phrases (2-5 words) from the text.
   - Groups by frequency across corrections.
   - Suggests new regex patterns for phrases seen >= --min-count times.
3. For confirmed clean examples (predicted non-clean, correct == clean):
   - Suggests new allowlist entries anchored to the start of the text.
4. Prints a diff of what would be added to pattern_db.json.
5. With --auto-apply: writes the changes to pattern_db.json directly.

Usage
-----
    python learn.py [--feedback feedback_log.jsonl] [--db pattern_db.json]
                    [--min-count N] [--auto-apply] [--dry-run]

Fallback error handling: all file I/O and pattern operations are wrapped in
try/except and print debug context to stderr so failures are visible.
"""

import os
import re
import sys
import json
import argparse
import traceback
from collections import Counter, defaultdict
from datetime import datetime, timezone

_HERE = os.path.dirname(os.path.abspath(__file__))
_DEFAULT_FEEDBACK = os.path.join(_HERE, 'feedback_log.jsonl')
_DEFAULT_DB       = os.path.join(_HERE, 'pattern_db.json')

VALID_LABELS = {'clean', 'spam', 'prompt_injection'}

# ---------------------------------------------------------------------------
# Feedback reader
# ---------------------------------------------------------------------------

def load_feedback(path):
    """Return list of dicts from feedback_log.jsonl, skip malformed lines."""
    records = []
    if not os.path.exists(path):
        print(f'INFO: feedback log not found: {path}', file=sys.stderr)
        return records
    try:
        with open(path, encoding='utf-8') as f:
            for i, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue
                try:
                    obj = json.loads(line)
                    records.append(obj)
                except json.JSONDecodeError as e:
                    print(f'WARN: skipping malformed line {i} in feedback log: {e}', file=sys.stderr)
    except Exception as e:
        print(f'ERROR loading feedback log: {e}', file=sys.stderr)
        traceback.print_exc()
    return records


# ---------------------------------------------------------------------------
# Pattern database
# ---------------------------------------------------------------------------

def load_db(path):
    """Return the pattern_db dict, or a skeleton if missing."""
    if not os.path.exists(path):
        return {'injection_patterns': [], 'spam_patterns': [], 'clean_allowlist': []}
    try:
        with open(path, encoding='utf-8') as f:
            return json.load(f)
    except Exception as e:
        print(f'ERROR loading pattern_db: {e}', file=sys.stderr)
        traceback.print_exc()
        return {'injection_patterns': [], 'spam_patterns': [], 'clean_allowlist': []}


def save_db(path, db):
    try:
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(db, f, indent=2, ensure_ascii=False)
            f.write('\n')
        print(f'pattern_db.json written: {path}')
    except Exception as e:
        print(f'ERROR saving pattern_db: {e}', file=sys.stderr)
        traceback.print_exc()


def existing_ids(db):
    """Return set of all pattern IDs already in the database."""
    ids = set()
    for section in ('injection_patterns', 'spam_patterns', 'clean_allowlist'):
        for entry in db.get(section, []):
            if 'id' in entry:
                ids.add(entry['id'])
    return ids


def next_id(db, section_prefix):
    """Generate the next sequential ID for a section."""
    ids = existing_ids(db)
    prefix_ids = [i for i in ids if i.startswith(section_prefix)]
    nums = []
    for pid in prefix_ids:
        try:
            nums.append(int(pid.replace(section_prefix, '')))
        except ValueError:
            pass
    n = max(nums, default=0) + 1
    return f'{section_prefix}{n:03d}'


# ---------------------------------------------------------------------------
# N-gram extraction
# ---------------------------------------------------------------------------

_WORD_RE = re.compile(r'\b[a-zA-Z0-9_\-]{2,}\b')

def _tokenize(text):
    return _WORD_RE.findall(text.lower())


def extract_ngrams(text, min_n=2, max_n=5):
    """Return Counter of n-gram tuples from text."""
    tokens = _tokenize(text)
    result = Counter()
    for n in range(min_n, max_n + 1):
        for i in range(len(tokens) - n + 1):
            gram = tuple(tokens[i:i + n])
            result[gram] += 1
    return result


def ngram_to_pattern(gram):
    """Convert an n-gram tuple to a case-insensitive regex string."""
    escaped = [re.escape(w) for w in gram]
    # Allow arbitrary whitespace between words to handle line-wrapped inputs.
    return r'(?i)\b' + r'\s+'.join(escaped) + r'\b'


def is_anchor_worth_it(text):
    """Return True if the text starts with an imperative verb (worth anchoring)."""
    first_word = _tokenize(text)
    if not first_word:
        return False
    # Common imperative openings that might be false-positive clean text.
    imperative_starters = {
        'translate', 'summarise', 'summarize', 'write', 'give', 'debug',
        'explain', 'list', 'show', 'create', 'make', 'find', 'check',
        'help', 'tell', 'what', 'how', 'why', 'when', 'who', 'where',
        'describe', 'generate', 'convert', 'format', 'analyse', 'analyze',
    }
    return first_word[0] in imperative_starters


def text_to_allowlist_pattern(text):
    """Produce an anchored regex from the first 3-5 significant words."""
    tokens = _tokenize(text)
    take = min(5, max(3, len(tokens)))
    significant = tokens[:take]
    if not significant:
        return None
    escaped = [re.escape(w) for w in significant]
    return r'(?i)^\s*' + r'\s+'.join(escaped[:3])


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------

def analyse(records, min_count):
    """
    Returns:
        injection_ngrams  Counter of (gram -> count) across injection FNs
        spam_ngrams       Counter of (gram -> count) across spam FNs
        clean_fp_texts    list of texts that were false-positived as non-clean
    """
    injection_grams = Counter()
    spam_grams      = Counter()
    clean_fp_texts  = []

    for rec in records:
        if not rec.get('is_correction'):
            continue

        predicted = rec.get('predicted_label', '')
        correct   = rec.get('correct_label', '')
        text      = rec.get('text', '')

        if not text or predicted not in VALID_LABELS or correct not in VALID_LABELS:
            continue

        # False negative: model said clean but should have been injection/spam.
        if predicted == 'clean' and correct == 'prompt_injection':
            injection_grams.update(extract_ngrams(text).keys())

        elif predicted == 'clean' and correct == 'spam':
            spam_grams.update(extract_ngrams(text).keys())

        # False positive: model flagged non-clean but text is actually clean.
        elif correct == 'clean' and predicted != 'clean':
            clean_fp_texts.append(text)

    # Filter by min_count.
    injection_ngrams = Counter({g: c for g, c in injection_grams.items() if c >= min_count})
    spam_ngrams      = Counter({g: c for g, c in spam_grams.items()      if c >= min_count})

    return injection_ngrams, spam_ngrams, clean_fp_texts


# ---------------------------------------------------------------------------
# Suggestion builders
# ---------------------------------------------------------------------------

def suggest_injection_patterns(ngrams, db, top_n=10):
    """Return list of new injection pattern dicts to add."""
    existing_pats = {e['pattern'] for e in db.get('injection_patterns', [])}
    suggestions = []
    for gram, count in ngrams.most_common(top_n):
        pat = ngram_to_pattern(gram)
        if pat in existing_pats:
            continue
        new_id = next_id(db, 'inj_learn_')
        suggestions.append({
            'id':          new_id,
            'pattern':     pat,
            'flags':       'i',
            'weight':      1,
            'description': f'auto-learned from {count} feedback correction(s): "{" ".join(gram)}"',
            'source':      'learned',
        })
        # Temporarily insert so next_id() doesn't duplicate.
        db.setdefault('injection_patterns', []).append({'id': new_id})
    return suggestions


def suggest_spam_patterns(ngrams, db, top_n=10):
    """Return list of new spam pattern dicts to add."""
    existing_pats = {e['pattern'] for e in db.get('spam_patterns', [])}
    suggestions = []
    for gram, count in ngrams.most_common(top_n):
        pat = ngram_to_pattern(gram)
        if pat in existing_pats:
            continue
        new_id = next_id(db, 'spam_learn_')
        suggestions.append({
            'id':          new_id,
            'pattern':     pat,
            'flags':       'i',
            'weight':      1,
            'description': f'auto-learned from {count} feedback correction(s): "{" ".join(gram)}"',
            'source':      'learned',
        })
        db.setdefault('spam_patterns', []).append({'id': new_id})
    return suggestions


def suggest_allowlist_patterns(fp_texts, db, top_n=20):
    """Return list of new allowlist (clean) pattern dicts to add."""
    existing_pats = {e['pattern'] for e in db.get('clean_allowlist', [])}
    seen_this_run = set()
    suggestions = []

    for text in fp_texts[:top_n]:
        if not is_anchor_worth_it(text):
            continue
        pat = text_to_allowlist_pattern(text)
        if pat is None or pat in existing_pats or pat in seen_this_run:
            continue
        seen_this_run.add(pat)
        new_id = next_id(db, 'clean_learn_')
        suggestions.append({
            'id':          new_id,
            'pattern':     pat,
            'flags':       'i',
            'weight':      1,
            'description': f'auto-learned allowlist from FP: "{text[:60]}"',
            'source':      'learned',
        })
        db.setdefault('clean_allowlist', []).append({'id': new_id})
    return suggestions


# ---------------------------------------------------------------------------
# Pretty printing
# ---------------------------------------------------------------------------

def print_suggestions(inj_sugg, spam_sugg, allow_sugg):
    total = len(inj_sugg) + len(spam_sugg) + len(allow_sugg)
    print(f'\n{"="*60}')
    print(f'learn.py suggestions  ({total} total)')
    print(f'{"="*60}')

    if inj_sugg:
        print(f'\n[injection_patterns]  +{len(inj_sugg)} new')
        for s in inj_sugg:
            print(f'  {s["id"]}  {s["pattern"]}')
            print(f'        {s["description"]}')

    if spam_sugg:
        print(f'\n[spam_patterns]  +{len(spam_sugg)} new')
        for s in spam_sugg:
            print(f'  {s["id"]}  {s["pattern"]}')
            print(f'        {s["description"]}')

    if allow_sugg:
        print(f'\n[clean_allowlist]  +{len(allow_sugg)} new')
        for s in allow_sugg:
            print(f'  {s["id"]}  {s["pattern"]}')
            print(f'        {s["description"]}')

    if total == 0:
        print('\n  No new suggestions (not enough corrections yet).')
    print()


# ---------------------------------------------------------------------------
# Apply
# ---------------------------------------------------------------------------

def apply_suggestions(db, inj_sugg, spam_sugg, allow_sugg):
    """
    Remove the placeholder stub entries inserted during suggestion generation
    and append the real full entries.
    """
    # Clean out stubs (entries with only 'id', no 'pattern').
    for section in ('injection_patterns', 'spam_patterns', 'clean_allowlist'):
        db[section] = [e for e in db.get(section, []) if 'pattern' in e]

    db.setdefault('injection_patterns', []).extend(inj_sugg)
    db.setdefault('spam_patterns',      []).extend(spam_sugg)
    db.setdefault('clean_allowlist',    []).extend(allow_sugg)


# ---------------------------------------------------------------------------
# Statistics
# ---------------------------------------------------------------------------

def print_stats(records):
    corrections = [r for r in records if r.get('is_correction')]
    by_pair = Counter(
        (r.get('predicted_label', '?'), r.get('correct_label', '?'))
        for r in corrections
    )
    print(f'\nFeedback log: {len(records)} records, {len(corrections)} corrections')
    if corrections:
        print('Correction breakdown:')
        for (pred, corr), count in sorted(by_pair.items()):
            print(f'  predicted={pred}  correct={corr}  count={count}')


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='Extract patterns from feedback log and suggest updates to pattern_db.json',
    )
    parser.add_argument('--feedback',   default=_DEFAULT_FEEDBACK,
                        help='Path to feedback_log.jsonl (default: ./feedback_log.jsonl)')
    parser.add_argument('--db',         default=_DEFAULT_DB,
                        help='Path to pattern_db.json (default: ./pattern_db.json)')
    parser.add_argument('--min-count',  default=2, type=int,
                        help='Minimum correction count to suggest an n-gram pattern (default: 2)')
    parser.add_argument('--top-n',      default=10, type=int,
                        help='Max new patterns per category (default: 10)')
    parser.add_argument('--auto-apply', action='store_true',
                        help='Write suggestions directly to pattern_db.json')
    parser.add_argument('--dry-run',    action='store_true',
                        help='Print suggestions but do not write anything (default mode)')
    args = parser.parse_args()

    # Load data
    try:
        records = load_feedback(args.feedback)
        db      = load_db(args.db)
    except Exception as e:
        print(f'ERROR loading data: {e}', file=sys.stderr)
        traceback.print_exc()
        sys.exit(1)

    print_stats(records)

    # Analyse corrections
    try:
        inj_ngrams, spam_ngrams, fp_texts = analyse(records, args.min_count)
    except Exception as e:
        print(f'ERROR analysing feedback: {e}', file=sys.stderr)
        traceback.print_exc()
        sys.exit(1)

    # Generate suggestions
    # Note: suggest_* functions temporarily mutate db to track IDs during one
    # run. We work on a deep copy so --dry-run never changes the original obj.
    import copy
    db_work = copy.deepcopy(db)

    try:
        inj_sugg   = suggest_injection_patterns(inj_ngrams,    db_work, top_n=args.top_n)
        spam_sugg  = suggest_spam_patterns(spam_ngrams,        db_work, top_n=args.top_n)
        allow_sugg = suggest_allowlist_patterns(fp_texts,      db_work, top_n=args.top_n)
    except Exception as e:
        print(f'ERROR generating suggestions: {e}', file=sys.stderr)
        traceback.print_exc()
        sys.exit(1)

    print_suggestions(inj_sugg, spam_sugg, allow_sugg)

    total = len(inj_sugg) + len(spam_sugg) + len(allow_sugg)

    if args.auto_apply and not args.dry_run and total > 0:
        # Apply to the real db (not the work copy).
        apply_suggestions(db, inj_sugg, spam_sugg, allow_sugg)
        save_db(args.db, db)
        print(f'Applied {total} new pattern(s) to {args.db}')
        print('Restart the server (or trigger hot-reload) to activate changes.')
    elif args.auto_apply and total == 0:
        print('Nothing to apply.')
    else:
        if total > 0:
            print('Run with --auto-apply to write these to pattern_db.json.')


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print('\nInterrupted.', file=sys.stderr)
        sys.exit(0)
    except Exception as e:
        print(f'FATAL: {e}', file=sys.stderr)
        traceback.print_exc()
        sys.exit(1)
