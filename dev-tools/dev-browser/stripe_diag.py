#!/usr/bin/env python3
"""
stripe_diag.py -- Stripe network + API diagnostic for the imgui browser.

Does NOT require a Stripe API key for the connectivity tests.
If you set STRIPE_API_KEY env var, it also runs an authenticated
StripeClient probe to verify the key and account reachability.

Run: .venv/bin/python stripe_diag.py
"""
import os
import sys
import time
import json
import socket
import urllib.request
import urllib.error

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)

# ── Colour helpers ────────────────────────────────────────────────────────────
OK   = "\033[32m[OK]\033[0m"
FAIL = "\033[31m[FAIL]\033[0m"
WARN = "\033[33m[WARN]\033[0m"
INFO = "\033[36m[INFO]\033[0m"

def check(label, ok, detail=""):
    tag = OK if ok else FAIL
    print(f"  {tag}  {label}" + (f"  -- {detail}" if detail else ""))
    return ok

# ── DNS resolution ────────────────────────────────────────────────────────────
def dns_check(host):
    try:
        ip = socket.gethostbyname(host)
        return check(f"DNS {host}", True, ip)
    except Exception as e:
        return check(f"DNS {host}", False, str(e))

# ── TCP reachability ──────────────────────────────────────────────────────────
def tcp_check(host, port=443, timeout=3):
    try:
        s = socket.create_connection((host, port), timeout=timeout)
        s.close()
        return check(f"TCP {host}:{port}", True)
    except Exception as e:
        return check(f"TCP {host}:{port}", False, str(e))

# ── HTTP GET (no auth) ────────────────────────────────────────────────────────
def http_get(url, label=None, expect=(200,), timeout=5):
    label = label or url
    try:
        req = urllib.request.Request(url, headers={
            "User-Agent": "xcm-imgui-stripe-diag/1.0",
            "Origin": "https://platform.claude.com",
        })
        with urllib.request.urlopen(req, timeout=timeout) as r:
            code = r.status
            body = r.read(256)
            return check(label, code in expect, f"HTTP {code}")
    except urllib.error.HTTPError as e:
        return check(label, e.code in expect, f"HTTP {e.code}")
    except Exception as e:
        return check(label, False, str(e))

# ── Stripe SDK probe (requires key) ──────────────────────────────────────────
def stripe_sdk_probe(api_key):
    try:
        import stripe
        client = stripe.StripeClient(api_key)
        # Lightweight: list 1 customer -- just verifies auth + network
        result = client.customers.list({"limit": 1})
        return check("StripeClient.customers.list", True,
                     f"returned {len(result.data)} row(s)")
    except Exception as e:
        return check("StripeClient.customers.list", False, str(e))

# ── Main ──────────────────────────────────────────────────────────────────────
def main():
    print(f"\n{INFO} Stripe network diagnostic -- {time.strftime('%Y-%m-%d %H:%M:%S')}\n")

    print("[ DNS resolution ]")
    for h in ["api.stripe.com", "js.stripe.com", "r.stripe.com",
              "m.stripe.network", "b.stripecdn.com", "platform.claude.com"]:
        dns_check(h)

    print("\n[ TCP connectivity (port 443) ]")
    for h in ["api.stripe.com", "js.stripe.com", "r.stripe.com",
              "m.stripe.network", "platform.claude.com"]:
        tcp_check(h)

    print("\n[ HTTP endpoints ]")
    # Stripe.js CDN -- should return 200
    http_get("https://js.stripe.com/v3/", "Stripe.js CDN")
    # Stripe API -- returns 401 without auth, which means the network path works
    http_get("https://api.stripe.com/v1", "Stripe API (401=network ok)", expect=(401,))
    # Stripe radar beacon -- returns 4xx without data, network path test
    http_get("https://r.stripe.com/b", "Stripe Radar beacon (4xx=network ok)", expect=(400,403,405,))
    # Claude billing page
    http_get("https://platform.claude.com/", "Claude platform (200 or 3xx)", expect=(200,301,302,307,308))

    print("\n[ Stripe SDK probe ]")
    api_key = os.environ.get("STRIPE_API_KEY", "")
    if api_key:
        stripe_sdk_probe(api_key)
    else:
        print(f"  {WARN}  STRIPE_API_KEY not set -- skipping authenticated probe")
        print(f"         Set it to run: STRIPE_API_KEY=sk_test_... python stripe_diag.py")

    print(f"\n{INFO} Done.\n")

if __name__ == "__main__":
    main()
