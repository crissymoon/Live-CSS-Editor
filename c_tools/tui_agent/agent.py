"""
agent.py
Calls Claude claude-haiku-4-5-20251001 via the Anthropic SDK.
Reads the API key from /Users/mac/Documents/live-css/ai/config.json
which points key files at ~/Desktop/my_keys/.

MODE-BASED PROMPTING
  The system prompt is chosen automatically from the file extension / name.
  It can also be overridden explicitly via the `mode` argument to call_haiku().

  Modes:
    code          .py .js .ts .jsx .tsx .c .cpp .h .css .php .rb .go .rs
                  .java .sh .bash .zsh .json .yaml .yml .toml .xml .sql
    csv           .csv .tsv
    txt           .txt (and unknown extensions)
    pdf           .pdf (treat file content as raw extracted text or source)
    doc           .doc .docx .odt .rtf .md .rst
    conversation  .txt / .md whose name contains "chat", "convo", "dialog",
                  "qa", "transcript", or "conversation"
    email         .html .htm whose name contains "email", "mail", "template",
                  "newsletter", or "campaign" -- also explicit mode="email"

Streaming is used so callers can receive live token counts via on_chunk(chars_so_far).
"""

import os
import json
import re
import sys
from typing import Callable, Optional, Tuple

from log_util import get_logger
from emoji_clean import clean as _emoji_clean
from fence_clean import clean as _fence_clean

HAIKU_MODEL = "claude-haiku-4-5-20251001"
CONFIG_PATH = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "../../ai/config.json")
)

# ---------------------------------------------------------------------------
# System prompts -- one per mode
# ---------------------------------------------------------------------------

_PROMPT_CODE = """\
You are a precise code editing agent operating inside a sandboxed tool.
Rules you must follow without exception:
1. You receive the FULL content of ONE existing source-code file plus a user instruction.
2. You return ONLY the complete updated file content with the instruction applied.
3. Do NOT create, reference, or produce content for any other file path.
4. Do NOT include markdown code fences, explanations, or commentary of any kind.
5. Do NOT prepend or append any text outside the file content itself.
6. Preserve indentation style, encoding, and line endings exactly as in the original.
7. Preserve all comments unless the instruction explicitly asks you to change them.
8. If no changes are needed, return the original content exactly as given.
9. Never add new file headers that suggest a different filename.
The output will be written verbatim back to the same source file.
""".strip()

_PROMPT_CSV = """\
You are a precise data-file editing agent operating inside a sandboxed tool.
Rules you must follow without exception:
1. You receive the FULL content of ONE CSV or TSV file plus a user instruction.
2. You return ONLY the complete updated file content with the instruction applied.
3. Preserve the exact delimiter character (comma for CSV, tab for TSV) throughout.
4. Preserve all column headers exactly unless the instruction explicitly asks you to rename them.
5. Do NOT add explanations, notes, or commentary lines anywhere in the output.
6. Do NOT wrap the output in markdown fences or any other markup.
7. Do NOT add or remove columns unless explicitly instructed.
8. Keep numeric formatting consistent with the original (decimal separator, quoting rules).
9. If no changes are needed, return the original content exactly as given.
The output will be written verbatim back to the same data file.
""".strip()

_PROMPT_TXT = """\
You are a precise plain-text editing agent operating inside a sandboxed tool.
Rules you must follow without exception:
1. You receive the FULL content of ONE plain-text file plus a user instruction.
2. You return ONLY the complete updated file content with the instruction applied.
3. Do NOT add markdown formatting, bullet symbols, headers, or any markup.
4. Do NOT add explanations, preamble, or closing remarks.
5. Preserve line breaks and paragraph spacing as in the original unless the instruction
   explicitly asks you to reformat them.
6. Preserve any existing structure such as numbered lists or indented sections.
7. If no changes are needed, return the original content exactly as given.
The output will be written verbatim back to the same plain-text file.
""".strip()

_PROMPT_PDF = """\
You are a precise document content editing agent operating inside a sandboxed tool.
The file you receive contains plain text that was extracted from a PDF or will be
compiled into a PDF. It may contain layout artifacts such as page numbers, headers,
footers, or hyphenated line breaks from the extraction process.

Rules you must follow without exception:
1. You receive the FULL extracted text of ONE document plus a user instruction.
2. You return ONLY the updated text content with the instruction applied.
3. Do NOT add markdown, HTML, or any other markup tags.
4. Do NOT add explanations or commentary outside the document text.
5. Do NOT invent content that was not in the original unless the instruction asks you to.
6. If the instruction asks you to fix extraction artifacts (broken hyphenation, duplicate
   spaces, garbled characters), apply only the minimal fix needed.
7. Preserve section titles, paragraph order, and heading hierarchy unless explicitly
   instructed to reorganize.
8. If no changes are needed, return the original content exactly as given.
The output will be written verbatim back to the same document file.
""".strip()

_PROMPT_DOC = """\
You are a precise long-form document editing agent operating inside a sandboxed tool.
Rules you must follow without exception:
1. You receive the FULL content of ONE document file (Markdown, RST, RTF, or plain text
   representation of Word/ODT) plus a user instruction.
2. You return ONLY the complete updated document content with the instruction applied.
3. Preserve all headings, sections, numbered lists, and existing structure unless the
   instruction explicitly asks you to change them.
4. Do NOT add commentary, author notes, or change-log entries to the document.
5. Do NOT add markdown fences or any outer wrapper around the output.
6. Maintain consistent heading depth and list indentation with the original.
7. If the file is Markdown or RST, preserve all existing markup characters (*, #, >, etc.)
   that are not directly related to the requested change.
8. If no changes are needed, return the original content exactly as given.
The output will be written verbatim back to the same document file.
""".strip()

_PROMPT_CONVERSATION = """\
You are a precise conversation / dialogue editing agent operating inside a sandboxed tool.
The file you receive contains a transcript, chat log, Q+A session, or scripted dialogue.

Rules you must follow without exception:
1. You receive the FULL content of ONE conversation file plus a user instruction.
2. You return ONLY the complete updated conversation with the instruction applied.
3. Preserve the speaker label format exactly (e.g. "User:", "Assistant:", "Alice:").
   Do NOT rename speakers unless the instruction explicitly asks you to.
4. Preserve the chronological order of turns unless the instruction asks you to reorder.
5. Do NOT add narration, stage directions, or meta-commentary unless explicitly asked.
6. Do NOT add markdown code fences, explanations, or content outside the conversation.
7. Keep each speaker turn on its own line or paragraph as in the original.
8. If no changes are needed, return the original content exactly as given.
The output will be written verbatim back to the same conversation file.
""".strip()

_PROMPT_EMAIL = """\
You are a precise email template editing agent operating inside a sandboxed tool.
The file you receive is an HTML or plain-text email template which may contain
template variables using common placeholder formats such as {{name}}, {name},
%name%, or [NAME].

Rules you must follow without exception:
1. You receive the FULL content of ONE email template file plus a user instruction.
2. You return ONLY the complete updated template with the instruction applied.
3. Preserve ALL existing template variables exactly -- do not expand, rename, or remove
   any placeholder tokens unless the instruction explicitly asks you to.
4. Preserve the DOCTYPE, <html>, <head>, <body> structure if the file is HTML.
5. Do NOT add tracking pixels, external link rewrites, or meta tags unless instructed.
6. Do NOT add explanations or commentary outside the template markup.
7. Keep inline CSS intact and do not convert it to external stylesheets unless asked.
8. If no changes are needed, return the original content exactly as given.
The output will be written verbatim back to the same template file.
""".strip()

# Map mode name -> system prompt
PROMPTS: dict[str, str] = {
    "code":         _PROMPT_CODE,
    "csv":          _PROMPT_CSV,
    "txt":          _PROMPT_TXT,
    "pdf":          _PROMPT_PDF,
    "doc":          _PROMPT_DOC,
    "conversation": _PROMPT_CONVERSATION,
    "email":        _PROMPT_EMAIL,
}

# ---------------------------------------------------------------------------
# Extension / name -> mode detection
# ---------------------------------------------------------------------------

_EXT_MAP: dict[str, str] = {
    # code
    ".py": "code", ".js": "code", ".ts": "code", ".jsx": "code", ".tsx": "code",
    ".c": "code", ".cpp": "code", ".cc": "code", ".cxx": "code",
    ".h": "code", ".hpp": "code",
    ".css": "code", ".scss": "code", ".less": "code",
    ".php": "code", ".rb": "code", ".go": "code", ".rs": "code",
    ".java": "code", ".kt": "code", ".swift": "code",
    ".sh": "code", ".bash": "code", ".zsh": "code", ".fish": "code",
    ".json": "code", ".yaml": "code", ".yml": "code", ".toml": "code",
    ".xml": "code", ".sql": "code", ".lua": "code", ".r": "code",
    # csv / data
    ".csv": "csv", ".tsv": "csv",
    # document
    ".md": "doc", ".rst": "doc", ".rtf": "doc",
    ".doc": "doc", ".docx": "doc", ".odt": "doc",
    # pdf source / extract
    ".pdf": "pdf",
    # plain text (default)
    ".txt": "txt",
    # html/htm -- resolved further by name pattern below
    ".html": "email", ".htm": "email",
}

# Name substrings that trigger the conversation mode for .txt / .md files
_CONVO_PATTERNS = re.compile(
    r"(chat|convo|dialog|dialogue|transcript|conversation|qa|q_a)",
    re.IGNORECASE,
)

# Name substrings that trigger the email mode for .html / .htm files
_EMAIL_PATTERNS = re.compile(
    r"(email|e-mail|mail|template|newsletter|campaign|drip)",
    re.IGNORECASE,
)


def get_mode(file_path: str) -> str:
    """
    Detect the editing mode from the file extension and name.
    Returns one of: code, csv, txt, pdf, doc, conversation, email.
    Falls back to 'txt' for unknown extensions.
    Falls back to 'code' for typical code-like contexts.

    All decisions are logged at DEBUG level so the TUI log panel shows them.
    """
    log  = get_logger()
    name = os.path.basename(file_path).lower()
    ext  = os.path.splitext(name)[1]

    mode = _EXT_MAP.get(ext, "txt")

    # secondary pattern overrides
    if mode in ("txt", "doc") and _CONVO_PATTERNS.search(name):
        mode = "conversation"
    elif mode in ("email",) and not _EMAIL_PATTERNS.search(name):
        # .html/.htm that does not look like an email template -> treat as code
        mode = "code"

    log.debug(
        "AGENT",
        f"mode detection: {name!r}  ext={ext!r}  -> mode={mode!r}",
    )
    return mode


def _load_api_key() -> str:
    log = get_logger()
    try:
        with open(CONFIG_PATH, "r", encoding="utf-8") as f:
            config = json.load(f)
        keys_path = config.get("keys_path", "")
        key_files = config.get("key_files", {})
        key_file  = key_files.get("anthropic", "anthropic_key.txt")
        key_path  = os.path.join(keys_path, key_file)
        with open(key_path, "r", encoding="utf-8") as f:
            key = f.read().strip()
        if not key:
            raise ValueError("anthropic key file is empty")
        log.info("AGENT", f"loaded API key from {key_path}")
        return key
    except Exception as exc:
        log.error("AGENT", f"_load_api_key failed: {exc}")
        raise


def call_haiku(
    file_path: str,
    file_content: str,
    instruction: str,
    on_chunk: Optional[Callable[[int, str], None]] = None,
    mode: Optional[str] = None,
) -> Tuple[Optional[str], Optional[str]]:
    """
    Send file_content + instruction to Haiku using the streaming API.

    mode can be one of: code, csv, txt, pdf, doc, conversation, email.
    When None (default) the mode is auto-detected from file_path.

    on_chunk(chars_received, latest_snippet) is called on every text delta
    so callers can show a live byte counter without polling.

    Returns (new_content, None) on success or (None, error_message) on failure.
    emoji_clean + fence_clean are applied to the response before returning.
    """
    log = get_logger()
    try:
        import anthropic as _anthropic
    except ImportError as exc:
        err = f"anthropic SDK not installed: {exc}"
        log.error("AGENT", err)
        return None, err

    try:
        api_key = _load_api_key()
    except Exception as exc:
        return None, str(exc)

    # ------------------------------------------------------------------
    # Choose system prompt
    # ------------------------------------------------------------------
    resolved_mode = (mode or "").strip().lower()
    if resolved_mode and resolved_mode in PROMPTS:
        log.info("AGENT", f"prompt mode: {resolved_mode!r} (caller-supplied)")
    else:
        if resolved_mode and resolved_mode not in PROMPTS:
            log.warning(
                "AGENT",
                f"unknown mode {resolved_mode!r} -- auto-detecting instead",
            )
        resolved_mode = get_mode(file_path)
        log.info(
            "AGENT",
            f"prompt mode: {resolved_mode!r}  file={os.path.basename(file_path)!r}",
        )
    system_prompt = PROMPTS.get(resolved_mode, _PROMPT_CODE)

    user_message = (
        f"File: {os.path.basename(file_path)}\n\n"
        f"Instruction: {instruction}\n\n"
        f"Current file content:\n"
        f"{file_content}"
    )

    log.info("AGENT", f"calling {HAIKU_MODEL} for {os.path.basename(file_path)}")
    log.debug("AGENT", f"instruction: {instruction[:120]}")

    try:
        client = _anthropic.Anthropic(api_key=api_key)
        chunks: list[str] = []
        total_chars = 0

        with client.messages.stream(
            model=HAIKU_MODEL,
            max_tokens=4096,
            system=system_prompt,
            messages=[{"role": "user", "content": user_message}],
        ) as stream:
            for text_delta in stream.text_stream:
                chunks.append(text_delta)
                total_chars += len(text_delta)
                if on_chunk:
                    try:
                        on_chunk(total_chars, text_delta)
                    except Exception as cb_exc:
                        log.debug("AGENT", f"on_chunk callback error: {cb_exc}")

        raw          = "".join(chunks)
        after_emoji  = _emoji_clean(raw)
        cleaned      = _fence_clean(after_emoji)
        log.info(
            "AGENT",
            f"stream complete: {len(raw)} chars raw,"
            f" {len(after_emoji)} after emoji_clean,"
            f" {len(cleaned)} after fence_clean",
        )
        return cleaned, None

    except Exception as exc:
        err = f"API call failed: {exc}"
        log.error("AGENT", err)
        return None, err
