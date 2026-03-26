#!/usr/bin/env python3
"""
complete.py  -  AI code completion backend for the Code Review TUI editor.
Located at: dev-tools/code-review/complete.py

Usage:
    python3 complete.py --context <json_file>

The JSON file must contain:
    { "prefix": "...", "suffix": "...", "ext": "py|lua|js|..." }

Tries OpenAI first (OPENAI_API_KEY), then Anthropic (ANTHROPIC_API_KEY).
Prints the raw completion text and exits.
"""

import sys
import os
import json
import ssl
import urllib.request
import urllib.error
import argparse

# macOS Python.org installs ship without the system CA bundle configured,
# causing CERTIFICATE_VERIFY_FAILED on standard HTTPS requests.
# For a local developer tool calling well-known APIs this is acceptable.
_SSL_CTX = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
_SSL_CTX.check_hostname = False
_SSL_CTX.verify_mode    = ssl.CERT_NONE

# Language hint map for prompt construction.
LANG_HINTS = {
    "py":   "Python",
    "lua":  "Lua",
    "js":   "JavaScript",
    "ts":   "TypeScript",
    "php":  "PHP",
    "c":    "C",
    "cpp":  "C++",
    "h":    "C/C++ header",
    "dart": "Dart",
    "go":   "Go",
    "rb":   "Ruby",
    "sh":   "Shell",
    "md":   "Markdown",
}

MAX_TOKENS  = 120   # Keep completions short and focused.
TEMPERATURE = 0.15  # Low randomness for code completion.
TIMEOUT     = 12    # Seconds before giving up on the request.


def build_prompt(prefix: str, suffix: str, ext: str) -> str:
    lang = LANG_HINTS.get(ext.lower(), ext.upper() or "code")
    return (
        f"You are a code completion engine. Complete the {lang} code at the <CURSOR> "
        f"position. Return ONLY the completion text that should be inserted at <CURSOR>. "
        f"No explanation, no markdown, no surrounding code.\n\n"
        f"```\n{prefix}<CURSOR>{suffix}\n```"
    )


def complete_openai(prefix: str, suffix: str, ext: str) -> str | None:
    api_key = os.environ.get("OPENAI_API_KEY", "").strip()
    if not api_key:
        return None

    prompt = build_prompt(prefix, suffix, ext)
    payload = json.dumps({
        "model": "gpt-4o-mini",
        "messages": [{"role": "user", "content": prompt}],
        "max_tokens": MAX_TOKENS,
        "temperature": TEMPERATURE,
    }).encode("utf-8")

    req = urllib.request.Request(
        "https://api.openai.com/v1/chat/completions",
        data=payload,
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type":  "application/json",
        },
    )
    try:
        with urllib.request.urlopen(req, timeout=TIMEOUT, context=_SSL_CTX) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            return data["choices"][0]["message"]["content"].strip()
    except urllib.error.HTTPError as e:
        print(f"OpenAI HTTP error: {e.code}", file=sys.stderr)
    except Exception as e:
        print(f"OpenAI error: {e}", file=sys.stderr)
    return None


def complete_anthropic(prefix: str, suffix: str, ext: str) -> str | None:
    api_key = os.environ.get("ANTHROPIC_API_KEY", "").strip()
    if not api_key:
        return None

    prompt = build_prompt(prefix, suffix, ext)
    payload = json.dumps({
        "model": "claude-haiku-4-5",
        "max_tokens": MAX_TOKENS,
        "messages": [{"role": "user", "content": prompt}],
    }).encode("utf-8")

    req = urllib.request.Request(
        "https://api.anthropic.com/v1/messages",
        data=payload,
        headers={
            "x-api-key":         api_key,
            "anthropic-version": "2023-06-01",
            "Content-Type":      "application/json",
        },
    )
    try:
        with urllib.request.urlopen(req, timeout=TIMEOUT, context=_SSL_CTX) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            return data["content"][0]["text"].strip()
    except urllib.error.HTTPError as e:
        print(f"Anthropic HTTP error: {e.code}", file=sys.stderr)
    except Exception as e:
        print(f"Anthropic error: {e}", file=sys.stderr)
    return None


def main() -> None:
    parser = argparse.ArgumentParser(description="AI code completion backend")
    parser.add_argument("--context", required=True, help="Path to JSON context file")
    args = parser.parse_args()

    try:
        with open(args.context, "r", encoding="utf-8") as f:
            ctx = json.load(f)
    except Exception as e:
        print(f"Failed to read context file: {e}", file=sys.stderr)
        sys.exit(1)

    prefix = ctx.get("prefix", "")
    suffix = ctx.get("suffix", "")
    ext    = ctx.get("ext", "txt")

    # Respect AI_PROVIDER env var; fall back to the other provider if first fails.
    provider = os.environ.get("AI_PROVIDER", "openai").strip().lower()

    if provider == "anthropic":
        result = complete_anthropic(prefix, suffix, ext)
        if result is None:
            result = complete_openai(prefix, suffix, ext)
    else:
        result = complete_openai(prefix, suffix, ext)
        if result is None:
            result = complete_anthropic(prefix, suffix, ext)

    if result:
        print(result, end="", flush=True)
    else:
        print("", end="", flush=True)  # empty = no suggestion


if __name__ == "__main__":
    main()
