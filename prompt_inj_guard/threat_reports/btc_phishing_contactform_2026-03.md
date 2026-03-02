# Threat Intelligence Report: BTC Phishing Contact Form Attack
**Report ID:** TI-2026-03-01-BTC  
**Classification:** Internal / Pattern Library  
**Date of Attack:** 2026-03-01  
**Report Date:** 2026-03-02  
**Analyst:** XCaliburMoon Web Development  
**Status:** Patterns extracted and deployed to prompt_inj_guard

---

## PII Notice

This report intentionally omits all personally identifiable information from the
original forensic evidence. No client names, no SMTP addresses, no victim email
addresses, and no production domain names appear in this document. Raw forensic
data is stored separately with the client under strict confidentiality. Only
attack behavior signatures suitable for a shared threat pattern library are
recorded here.

---

## 1. Executive Summary

On March 1, 2026 a PHP contact form endpoint sustained a coordinated automated
spam attack over a 11.7-hour window. The attacker submitted 157 fraudulent
entries using a fixed bot persona and a rotating list of harvested third-party
email addresses as the "from" field. Every message advertised a fake BTC reward
of exactly 1.749542 BTC and linked to a redirector or a custom telegra.ph
article.

The attack followed two distinct waves separated by a 96-minute pause during
which the attacker switched URL infrastructure, suggesting active monitoring and
adaptive behavior.

**Attack category:** Contact form spam / BTC cryptocurrency reward phishing  
**Scam type:** Fake mining reward / double-target SMTP abuse  
**Total submissions:** 157  
**Duration:** 11.7 hours (2026-03-01 11:14 UTC to 22:56 UTC)  
**Overall rate:** 13.4 submissions per hour

---

## 2. Attacker Behavioral Fingerprint

### 2.1 Identity Signals (all 157 submissions, no variation)

| Field | Value |
|---|---|
| Submitter name | Same bot-persona string used as both first and last name |
| BTC amount | Exactly 1.749542 BTC -- never changed across all 157 messages |
| Phone | Random 11-digit non-US number, new value every submission |

The double-name pattern (same string used as first and last name) is a strong
bot indicator and was independently observed in three earlier separate campaigns
targeting the same form over the preceding 7 weeks.

### 2.2 Message Structure Template

```
[URGENCY_WORD]! [ACTION_VERB] [1.749542 BTC] [INSTRUCTION_PHRASE] [URL]



[5 blank lines]

[TOKEN_LABEL]: [60-65 character random alphanumeric code]
```

- URGENCY_WORD: alternated between URGENT and IMPORTANT (approximately 50/50 split)
- TOKEN_LABEL: rotated through ~25 labels (HASH, CASE ID, AUTH CODE, REFERENCE ID, SESSION ID, etc.)
- The token string itself was a fresh random alphanumeric code on each submission

### 2.3 Timing Analysis

| Metric | Wave 1 | Wave 2 |
|---|---|---|
| Submissions | 112 | 45 |
| Duration | 317 min | 288 min |
| Rate | 21.2 per hour | 9.4 per hour |
| Avg interval | 171 seconds | 393 seconds |
| Std deviation | 18.4s (highly consistent) | 109.6s (more variable) |1

Wave 1 timing standard deviation of 18.4 seconds is consistent with a cron-loop
or fixed-interval scheduler. Wave 2 was slower and more variable, suggesting a
different execution environment or deliberate rate reduction to avoid detection.

---

## 3. Wave Analysis and Infrastructure Shift

### Wave 1 (11:14 - 16:31 UTC)

- **URL strategy:** Short URL redirect services (45+ distinct domains used in rotation)
- **Anti-detection approach:** New short-link domain on nearly every submission to
  avoid a single-domain block
- **Top redirect domains observed:** grdn.link, cutxly.com, tau.lu, alstr.in, b1.fr,
  linkypay.com, cutx.me, vlblink.com, 1.g9.yt, uzip.ir, nordwit.com, s.kode4u.tech
- **Email target TLDs:** predominantly .com, .co.uk, .it, .ca

### Wave 2 (18:07 - 22:56 UTC, after 96-minute pause)

- **URL strategy:** 100% telegra.ph hosted articles
- **Slug pattern:** `/You-Mined-{N}-BTC-Message-id-{6digit}-03-01`
- **Behavioral note:** The slug claimed a different BTC amount ("13426 BTC") than
  the message text ("1.749542 BTC") -- a template coordination error that reveals
  the two components were assembled independently
- **Email target TLDs:** shifted to .com, .ru, .za, .cz (more international)

**Interpretation of the pause:** The attacker likely detected that short URL
redirectors may have been flagged or throttled and paused to switch to a
single-platform (telegra.ph) strategy. This adaptive behaviour suggests a human
operator monitoring results rather than a fully autonomous script.

---

## 4. Email Address Strategy

The "from" email field was populated with real email addresses harvested from
third parties -- legitimate individuals who had no connection to the attack.

| Metric | Value |
|---|---|
| Total submissions | 157 |
| Unique email addresses used | 156 (1 reused across waves) |
| Country origin mix | US, UK, Italy, Canada, Australia, Ireland, Greece, Russia, Czech Republic, South Africa, Norway |
| Top provider Wave 1 | Yahoo-family (~93% of Wave 1 uses) |
| Top provider Wave 2 | Gmail (~71% of Wave 2 uses) |

This is a double-target attack structure:
1. The form endpoint's SMTP pipeline was abused to generate 157 alert emails
2. The harvested victim addresses were used as counterfeit senders without consent

The shift from Yahoo-dominant emails in Wave 1 to Gmail-dominant in Wave 2 suggests
the attacker has segmented email lists by provider and deploys them in separate
batches, possibly to test deliverability across different provider filters.

---

## 5. Pre-existing Separate Spam Campaigns (Earlier Entries)

The forensic dump also revealed four earlier spam entries from separate campaigns
unrelated to the March 1 bot. These reveal distinct attack patterns worth adding
to detection:

| Date | Pattern Type |
|---|---|
| 2026-01-11 | SEO solicitation spam submitted via web form |
| 2026-02-11 | Random character string / junk submission |
| 2026-02-15 | "Feedback Form EU" spam-as-a-service (message: "Did you know it is possible to send requests lawfully?") |
| 2026-02-21 | Same "Feedback Form EU" service, slightly different template |

The "it is possible to send... lawfully" phrasing is a documented signature of
a commercial spam-as-a-service operation that sells mass contact form submissions
as "legal advertising." Two instances targeted this same endpoint over 6 days.

---

## 6. Patterns Added to prompt_inj_guard

Ten new `spam_016` through `spam_025` patterns were added to `pattern_db.json`
based on this attack. The RuleGuard hot-reload will apply them without a server
restart.

| Pattern ID | Trigger | Confidence Weight |
|---|---|---|
| spam_016 | Exact BTC amount `1.749542 BTC` | 1.00 |
| spam_017 | telegra.ph You-Mined BTC Message-id slug | 1.00 |
| spam_018 | URGENT! or IMPORTANT! + multi-decimal BTC amount | 0.95 |
| spam_019 | Known fake token label + 55+ char alphanumeric code | 0.90 |
| spam_020 | Double-name bot persona (same word as first and last name) | 0.75 |
| spam_021 | Feedback Form EU boilerplate ("possible to send... lawfully") | 0.90 |
| spam_022 | General "you have mined/received X.X BTC" reward phishing | 0.95 |
| spam_023 | Exact attacker persona string from this specific campaign | 1.00 |
| spam_024 | `no.reply.` or `noreply.` spoofed address appearing in message body | 0.70 |
| spam_025 | 11-digit phone (non-US) combined with crypto/financial keywords | 0.80 |

### Detection Coverage Estimate

Based on the 157 documented submissions, the new patterns would have flagged:

- spam_016: 157/157 (100%) -- exact BTC amount never varied
- spam_017: 45/157 (28.7%) -- Wave 2 only
- spam_018: 157/157 (100%) -- all had urgency prefix + BTC amount
- spam_019: ~154/157 (~98%) -- all had a 60+ char token, slight variation in label names
- spam_020: 157/157 (100%) -- all had double-name persona; also catches 3 of 4 earlier spam entries
- spam_021: 2/4 earlier spam (the Feedback Form EU entries)
- spam_022: 157/157 (100%) -- all claimed BTC was mined/earned
- spam_023: 157/157 (100%) -- exact persona match
- spam_025: ~157/157 (~100%) -- all had 11-digit phone alongside BTC keywords

**Any single pattern match triggers the spam label.** For the BrandonTofEH
campaign, patterns spam_016, spam_018, spam_022, or spam_023 alone would have
stopped every submission. The combination ensures robustness if the attacker
changes one element.

---

## 7. Lessons Learned

### 7.1 Exact Amount Locking

The attacker used exactly `1.749542 BTC` across all 157 messages without variation.
This is a signature failure: an exact match on the specific amount is a perfect
discriminator with zero false positive risk. Any legitimate message about Bitcoin
would use rounded or different amounts.

**Lesson:** When a scam campaign uses a precise numeric constant, create an exact
string match immediately. These are trivially precise and never generate false
positives.

### 7.2 Template Fragility Reveals Infrastructure

The slug discrepancy (slug said "13426 BTC", message said "1.749542 BTC") shows
the URL generator and the message generator were separate components that were
not synchronized. Inconsistencies like this indicate template-based infrastructure
and can be exploited for detection: pattern the slug format itself, not just the
message content.

**Lesson:** Inspect URLs and their semantic relationship to message body content.
A link claiming one value while the body claims another is a high-confidence
spam signal.

### 7.3 Double-Name Bot Pattern is Cross-Campaign

The pattern of using the same string as both first and last name appeared in the
documented campaign (`BrandonTofEH BrandonTofEH`) AND in three earlier independent
campaigns from different dates and different operators. This is a widely shared
trait across different spam tool codebases.

**Lesson:** The double-name pattern is a generic bot indicator, not campaign-specific.
It should be part of any contact-form guard layer with a moderate confidence weight
(0.75 here -- reduced from 1.0 to leave room for the rare legitimate user with
an unusual name).

### 7.4 Adaptive Infrastructure Switching Within a Single Campaign

The 96-minute pause and full URL strategy replacement shows the attacker was
actively monitoring outcomes. A system that blocked Wave 1 URLs via domain
blacklist alone would not have stopped Wave 2 because it used a completely
different URL platform.

**Lesson:** Do not rely solely on URL domain blocking. Pattern the message
structure itself (urgency + BTC amount + blank lines + alphanumeric token).
Structure patterns survive infrastructure rotation.

### 7.5 Harvested Victim Email Lists Are Multi-Country and Pre-Segmented

The shift from Yahoo-dominant (Wave 1) to Gmail-dominant (Wave 2) email use,
combined with the international geographic spread, indicates the attacker
maintains a pre-built segmented email database. The "from" address cannot be
used as a reliable spam signal because it was a harvested innocent victim address.

**Lesson:** Never build a blocklist from "from" addresses in a bot attack without
verifying they are attacker-controlled. The emails used here were stolen third-party
addresses. Block on message content patterns, not on sender fields.

### 7.6 Token Label Diversity vs. Token Structure Consistency

The token label rotated through ~25 labels (HASH, CASE ID, AUTH CODE, etc.) but
the structure was always the same: `LABEL: [60-65 alphanumeric chars]`. The
rotation was designed to defeat simple keyword matching on the label alone, but
the structural pattern (a known label followed by a very long alphanumeric code)
remained constant across all 157 submissions.

**Lesson:** Match structure (label type + token length) rather than an exhaustive
label list. The regex `[A-Z ]+: [A-Za-z0-9]{55,}` catches all variants including
unknown future label names.

### 7.7 Rate and Timing Analysis Identifies Automated Origin

Wave 1 had a standard deviation of only 18.4 seconds on ~171 second intervals.
Human users cannot submit forms at that consistency. This signature is useful for
real-time rate-limit triggers.

**Lesson:** At IP/session level, submission rate consistency (very low variance
in interval timing) is a stronger bot signal than raw rate alone. A human
submitting quickly will have high variance; a scheduler will have low variance.

---

## 8. Recommended Follow-On Actions

1. **Add a rate-limit layer to the guard API**: if the same source submits more
   than 3 requests in 5 minutes, flag as high-risk regardless of content.
2. **Add telegra.ph domain to a URL reputation list**: any message containing a
   telegra.ph link combined with financial keywords should get elevated spam
   confidence.
3. **Extend spam_019 token pattern**: add the remaining ~20 token label variants
   (PROJECT ID, SURVEY ID, SERIAL NUMBER ID) observed in Wave 2 to the pattern.
4. **Consider a numeric-precision BTC pattern**: `\b\d+\.\d{6}\s*BTC\b` catches
   any 6-decimal BTC amount claim, not just the specific 1.749542 value -- useful
   for future campaigns that change the amount but keep the same 6-decimal format.
5. **Feed these submissions through learn.py**: the attack provided 157 confirmed
   spam examples. Running `learn.py --auto-apply --min-count 3` on feedback log
   entries from these submissions would auto-generate additional n-gram patterns.

---

## 9. Pattern Changelog

```
2026-03-02  Added spam_016  1.749542 BTC exact match
2026-03-02  Added spam_017  telegra.ph You-Mined slug
2026-03-02  Added spam_018  URGENT/IMPORTANT + precise BTC
2026-03-02  Added spam_019  token label + 55+ char alphanumeric
2026-03-02  Added spam_020  double-name bot persona (generic)
2026-03-02  Added spam_021  Feedback Form EU boilerplate
2026-03-02  Added spam_022  general BTC mined/received phishing
2026-03-02  Added spam_023  BrandonTofEH persona exact match
2026-03-02  Added spam_024  no.reply. spoofed address in body
2026-03-02  Added spam_025  11-digit phone + crypto keywords
```

---

*End of report. No personally identifiable information is stored in this file.*  
*Forensic source data is held separately under client confidentiality.*
