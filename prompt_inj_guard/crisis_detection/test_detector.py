"""
crisis_detection/test_detector.py

Test suite for CrisisDetector.

Run with:
  python test_detector.py
  python -m pytest test_detector.py -v

No pytest install required -- the script can be run directly.
"""

import sys
import types
import traceback
from pathlib import Path

# Allow running without installing the package
_DIR = Path(__file__).parent
sys.path.insert(0, str(_DIR.parent.parent))  # live-css root
sys.path.insert(0, str(_DIR.parent))         # prompt_inj_guard root (needed for crisis_detection package)

from crisis_detection.detector import CrisisDetector

# ------------------------------------------------------------------
# Shared detector instance (loaded once)
try:
    _detector = CrisisDetector()
except Exception:
    print("[test] Failed to create CrisisDetector:\n" + traceback.format_exc(), file=sys.stderr)
    sys.exit(1)


# ------------------------------------------------------------------
# Helpers

_pass = 0
_fail = 0
_errors = []


def _run(name: str, text: str, expect_flagged: bool, expect_category: str | None = None,
         min_severity: int = 0) -> None:
    global _pass, _fail
    try:
        result = _detector.detect(text)
        ok = True
        reasons = []

        if result["flagged"] != expect_flagged:
            ok = False
            reasons.append(
                f"flagged={result['flagged']} (expected {expect_flagged})"
            )

        if expect_category is not None and result["flagged"]:
            if result["category"] != expect_category:
                ok = False
                reasons.append(
                    f"category='{result['category']}' (expected '{expect_category}')"
                )

        if min_severity > 0 and result["severity"] < min_severity:
            ok = False
            reasons.append(
                f"severity={result['severity']} (expected >= {min_severity})"
            )

        if ok:
            _pass += 1
            print(f"  PASS  {name}")
        else:
            _fail += 1
            reason_str = "; ".join(reasons)
            print(f"  FAIL  {name} -- {reason_str}")
            _errors.append((name, text, result, reason_str))

    except Exception:
        _fail += 1
        err = traceback.format_exc()
        print(f"  ERROR {name} -- unexpected exception:\n{err}")
        _errors.append((name, text, None, err))


def _section(title: str) -> None:
    print(f"\n--- {title} ---")


# ------------------------------------------------------------------
# Test cases

def test_neutral() -> None:
    _section("Neutral / safe inputs")

    _run("plain greeting",           "hello how are you today",                False)
    _run("weather question",         "what is the weather like in boston",      False)
    _run("code question",            "how do I reverse a list in python",       False)
    _run("empty-ish text",           "   ",                                     False)
    _run("movie mention",            "I watched a film about zombies last night", False)
    _run("performance idiom",        "she was killing it at the presentation",  False)
    _run("sarcasm colloquial",       "this traffic is killing me, ugh",         False)
    _run("feeling tired",            "i'm so tired i could die",                False)
    _run("dead serious idiom",       "i'm dead serious about this project",     False)
    _run("killing it phrase",        "they are absolutely killing it this season", False)


def test_suicide_detection() -> None:
    _section("Suicide detection -- positive matches")

    _run("direct intent",
         "I want to kill myself tonight",
         True, "suicide", 9)

    _run("wish to be dead",
         "I wish I was dead and everyone would be better off without me",
         True, "suicide", 8)

    _run("passive ideation",
         "I don't want to live anymore, I just want to go to sleep and not wake up",
         True, "suicide", 7)

    _run("goodbye language",
         "I've been giving away my stuff and saying goodbye to everyone for the last time",
         True, "suicide", 7)

    _run("no reason to live",
         "I feel like there is no reason to go on, I have nothing to live for",
         True, "suicide", 7)

    _run("suicide plan disclosure",
         "I already have a suicide plan and I'm ready",
         True, "suicide", 10)


def test_self_harm() -> None:
    _section("Self-harm detection -- positive matches")

    _run("cutting disclosure",
         "I've been cutting myself for months to cope with the pain",
         True, "self_harm", 7)

    _run("self harm urge",
         "I have the urge to cut myself when I feel this way",
         True, "self_harm", 7)

    _run("relapse self harm",
         "I relapsed on self-harm last night",
         True, "self_harm", 7)


def test_domestic_violence() -> None:
    _section("Domestic violence -- positive matches")

    _run("physical abuse by partner",
         "My husband hit me again last night and I don't know what to do",
         True, "domestic_violence", 8)

    _run("choking disclosure",
         "He choked me during the argument and I am scared he will kill me",
         True, "domestic_violence", 8)

    _run("trapped in abusive relationship",
         "I am trapped in an abusive relationship and I can't leave safely",
         True, "domestic_violence", 7)


def test_mental_health_crisis() -> None:
    _section("Mental health crisis -- positive matches")

    _run("hearing voices",
         "I keep hearing voices in my head telling me to do things and I can't make them stop",
         True, "mental_health_crisis", 7)

    _run("psychosis paranoia",
         "I believe I am being followed and monitored by the government and nobody believes me",
         True, "mental_health_crisis", 7)

    _run("breakdown",
         "I'm having a mental breakdown right now and I can't stop crying or function",
         True, "mental_health_crisis", 6)

    _run("not eaten or slept",
         "I haven't eaten or slept in 5 days and I don't feel real anymore",
         True, "mental_health_crisis", 6)


def test_substance_crisis() -> None:
    _section("Substance crisis -- positive matches")

    _run("active overdose",
         "I took too many pills and I think I overdosed",
         True, "substance_crisis", 9)

    _run("overdose disclosure",
         "I just overdosed on opioids and I need help",
         True, "substance_crisis", 9)

    _run("relapse hard drugs",
         "I relapsed on heroin last night and I can't stop using",
         True, "substance_crisis", 7)


def test_child_safety() -> None:
    _section("Child safety -- positive matches")

    _run("physical abuse by parent",
         "My dad has been hitting and burning me and I am afraid to go home",
         True, "child_safety", 9)

    _run("sexual abuse disclosure",
         "A grown up at my school touched me and hurt me and told me not to tell anyone",
         True, "child_safety", 9)


def test_medical_emergency() -> None:
    _section("Medical emergency -- positive matches")

    _run("heart attack",
         "I think I am having a heart attack right now, someone please call 911",
         True, "medical_emergency", 9)

    _run("cannot breathe",
         "I cannot breathe and there is a lot of blood I can't stop",
         True, "medical_emergency", 8)


def test_negation() -> None:
    _section("Negation detection -- should NOT flag (or flag with negated=True)")

    # These have explicit negation words before the concerning phrase.
    # Depending on the negation window they may still flag but the match should be marked negated.
    # We test for either: not flagged, or if flagged the matched entry is negated.

    cases = [
        ("negation_would_never", "I would never kill myself, that is not who I am"),
        ("negation_not_serious", "just kidding, I'm not serious about any of this"),
    ]
    for name, text in cases:
        result = _detector.detect(text)
        if not result["flagged"]:
            _pass_local(name)
            continue
        # If flagged, all active matches should be negated
        non_negated = [m for m in result["matched_patterns"] if not m["negated"]]
        if non_negated:
            global _pass, _fail
            _fail += 1
            print(
                f"  FAIL  {name} -- flagged with {len(non_negated)} non-negated matches; "
                f"text='{text[:60]}'"
            )
            _errors.append((name, text, result, "non-negated matches present"))
        else:
            _pass_local(name)


def _pass_local(name: str) -> None:
    global _pass
    _pass += 1
    print(f"  PASS  {name}")


def test_bulk() -> None:
    _section("Bulk detection")

    texts = [
        "hello there",
        "I want to kill myself",
        "what is 2 + 2",
        "I've been cutting myself to cope",
    ]
    results = _detector.detect_bulk(texts)

    name = "bulk_returns_correct_count"
    if len(results) == len(texts):
        global _pass
        _pass += 1
        print(f"  PASS  {name}")
    else:
        global _fail
        _fail += 1
        print(f"  FAIL  {name} -- expected {len(texts)} results, got {len(results)}")

    name = "bulk_first_neutral"
    _run_from_result(name, results[0], False)

    name = "bulk_second_suicide"
    _run_from_result(name, results[1], True, "suicide")

    name = "bulk_third_neutral"
    _run_from_result(name, results[2], False)

    name = "bulk_fourth_self_harm"
    _run_from_result(name, results[3], True, "self_harm")


def _run_from_result(name: str, result: dict, expect_flagged: bool,
                     expect_category: str | None = None) -> None:
    global _pass, _fail
    ok = True
    reasons = []
    if result["flagged"] != expect_flagged:
        ok = False
        reasons.append(f"flagged={result['flagged']} (expected {expect_flagged})")
    if expect_category and result.get("category") != expect_category:
        ok = False
        reasons.append(f"category='{result.get('category')}' (expected '{expect_category}')")
    if ok:
        _pass += 1
        print(f"  PASS  {name}")
    else:
        _fail += 1
        reason_str = "; ".join(reasons)
        print(f"  FAIL  {name} -- {reason_str}")
        _errors.append((name, "(from bulk)", result, reason_str))


def test_resources_attached() -> None:
    _section("Safe resources attached to results")

    result = _detector.detect("I want to kill myself tonight")
    name = "suicide_has_resources"
    global _pass, _fail
    if result["flagged"] and result.get("safe_resources"):
        _pass += 1
        print(f"  PASS  {name}")
    else:
        _fail += 1
        print(f"  FAIL  {name} -- flagged={result['flagged']} resources={result.get('safe_resources')}")
        _errors.append((name, "I want to kill myself tonight", result, "missing safe_resources"))


def test_context_boost() -> None:
    _section("Context boost detection")

    result = _detector.detect(
        "I don't want to live anymore. I have decided. I am ready."
    )
    name = "context_boost_present"
    global _pass, _fail
    if result["flagged"] and result.get("context_boost_detected"):
        _pass += 1
        print(f"  PASS  {name}")
    else:
        _fail += 1
        print(f"  FAIL  {name} -- flagged={result['flagged']} boost={result.get('context_boost_detected')}")
        _errors.append((name, "", result, "context_boost_detected not set"))


def test_severity_scale() -> None:
    _section("Severity scale (higher-urgency text should score >= 8)")

    result = _detector.detect("I already have a plan to end my life and nothing can stop me")
    name = "high_severity_plan_and_booster"
    global _pass, _fail
    if result["flagged"] and result["severity"] >= 8:
        _pass += 1
        print(f"  PASS  {name}  (severity={result['severity']})")
    else:
        _fail += 1
        print(f"  FAIL  {name} -- severity={result['severity']}, flagged={result['flagged']}")
        _errors.append((name, "", result, f"severity {result['severity']} below 8"))


def test_error_safety() -> None:
    _section("Error / edge-case safety")

    for label, val in [
        ("none_input",    None),
        ("int_input",     42),
        ("list_input",    ["a", "b"]),
        ("empty_string",  ""),
        ("whitespace",    "   \n\t  "),
    ]:
        try:
            result = _detector.detect(val)  # type: ignore[arg-type]
            global _pass, _fail
            _pass += 1
            print(f"  PASS  error_safety_{label}  (did not raise)")
        except Exception:
            _fail += 1
            err = traceback.format_exc()
            print(f"  FAIL  error_safety_{label}  -- raised exception:\n{err}")
            _errors.append((f"error_safety_{label}", str(val), None, err))


# ------------------------------------------------------------------
# Entry point

def main() -> None:
    print("=" * 60)
    print("  Crisis Detector Test Suite")
    print("=" * 60)

    test_neutral()
    test_suicide_detection()
    test_self_harm()
    test_domestic_violence()
    test_mental_health_crisis()
    test_substance_crisis()
    test_child_safety()
    test_medical_emergency()
    test_negation()
    test_bulk()
    test_resources_attached()
    test_context_boost()
    test_severity_scale()
    test_error_safety()

    print("\n" + "=" * 60)
    print(f"  Results: {_pass} passed, {_fail} failed  ({_pass + _fail} total)")
    print("=" * 60)

    if _errors:
        print("\nFailed test details:")
        for name, text, result, reason in _errors:
            print(f"\n  [{name}]")
            print(f"    text   : {str(text)[:80]}")
            print(f"    reason : {reason}")
            if result:
                print(f"    result : flagged={result.get('flagged')}  "
                      f"cat={result.get('category')}  sev={result.get('severity')}")

    sys.exit(0 if _fail == 0 else 1)


if __name__ == "__main__":
    main()
