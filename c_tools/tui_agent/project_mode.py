"""
project_mode.py
Full project workflow mode for the TUI agent.

Flow
----
1. ProjectScanner   -- recursively walks a root directory, collects source files,
                       runs outliner.py on each to produce per-file summaries.
2. ProjectPlanner   -- sends tree + outlines to GPT-4o mini; drip-feeds unused
                       phrases during the wait; parses the JSON response into
                         {"prompt": "<haiku instruction>", "phrases": [...]}
                       and saves both halves to project_prompt.json /
                       project_phrases.json inside the tui_agent folder.
3. ProjectExecutor  -- sends the plan prompt to Haiku 4.5 together with the full
                       file tree and working directory context. Haiku is asked to
                       return a JSON object:
                         {"changes": [{"file": "rel/path", "content": "..."}],
                          "commands": ["cmd1", ...]}
                       Each changed file is staged via Merger.propose() -> SQLite.
                       Shell commands (optional) are run in the project root and
                       their output is logged.
4. ProjectSession   -- orchestrates the three stages; exposes:
                         run(root_dir, instruction, on_log, on_diff)
                         approve_all(root_dir)
                         reject_all(root_dir)

All heavy work runs in a daemon thread so the TUI stays responsive.
Console fallback print() calls are included for every error path so
debugging is possible outside curses.
"""

import json
import os
import re
import subprocess
import sys
import threading
from typing import Callable, Dict, List, Optional, Tuple

HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------------------
# Lazy imports -- guard against missing packages gracefully
# ---------------------------------------------------------------------------

try:
    from log_util import get_logger as _get_logger
    _log = _get_logger()
except Exception as _le:
    import logging
    _log_inner = logging.getLogger("project_mode")
    class _FakeLog:
        def info(self, tag, msg):    _log_inner.info("[%s] %s", tag, msg)
        def warning(self, tag, msg): _log_inner.warning("[%s] %s", tag, msg)
        def error(self, tag, msg):   _log_inner.error("[%s] %s", tag, msg)
        def debug(self, tag, msg):   _log_inner.debug("[%s] %s", tag, msg)
    _log = _FakeLog()
    print(f"[project_mode] log_util import failed: {_le}", file=sys.stderr)

try:
    from db import AgentDB
except Exception as _dbe:
    AgentDB = None
    print(f"[project_mode] db import failed: {_dbe}", file=sys.stderr)

try:
    from merger import Merger
except Exception as _me:
    Merger = None
    print(f"[project_mode] merger import failed: {_me}", file=sys.stderr)

# outliner lives one level up (c_tools/)
_OUTLINER_DIR = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, _OUTLINER_DIR)
try:
    from outliner import outline as _outline, Node as _OutlineNode
    _HAS_OUTLINER = True
except Exception as _oe:
    _HAS_OUTLINER = False
    print(f"[project_mode] outliner import failed: {_oe}", file=sys.stderr)

_CONFIG_PATH   = os.path.normpath(os.path.join(HERE, "../../ai/config.json"))
_PHRASES_DB    = os.path.normpath(os.path.join(HERE, "../../ai/data/phrases.db"))
_PLAN_PROMPT   = os.path.join(HERE, "project_prompt.json")
_PLAN_PHRASES  = os.path.join(HERE, "project_phrases.json")

_SOURCE_EXTS = {
    ".py", ".js", ".ts", ".jsx", ".tsx",
    ".c", ".h", ".cpp", ".css", ".php",
    ".json", ".md", ".sh", ".txt", ".rs",
}
_SKIP_DIRS = {
    ".git", ".venv", "venv", "__pycache__", "node_modules",
    "target", "build", "dist", ".mypy_cache", ".pytest_cache",
}

# map file extension -> outliner language name
_LANG_MAP: dict = {
    ".py":   "python",
    ".js":   "javascript",
    ".ts":   "javascript",
    ".jsx":  "javascript",
    ".tsx":  "javascript",
    ".css":  "css",
    ".php":  "php",
    ".c":    "c",
    ".h":    "c",
    ".cpp":  "c",
}

HAIKU_MAX_TOKENS = 8192

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _load_api_keys() -> Tuple[Optional[str], Optional[str]]:
    """Return (openai_key, anthropic_key) or None for each on failure."""
    try:
        with open(_CONFIG_PATH, "r", encoding="utf-8") as f:
            cfg = json.load(f)
        keys_path = cfg.get("keys_path", "")
        key_files = cfg.get("key_files", {})

        def _read(fname):
            p = os.path.join(keys_path, fname)
            try:
                return open(p, "r", encoding="utf-8").read().strip() or None
            except Exception as exc:
                print(f"[project_mode] key read failed {p}: {exc}", file=sys.stderr)
                return None

        openai_key    = _read(key_files.get("openai", "openai_key.txt"))
        anthropic_key = _read(key_files.get("anthropic", "anthropic_key.txt"))
        return openai_key, anthropic_key
    except Exception as exc:
        print(f"[project_mode] _load_api_keys failed: {exc}", file=sys.stderr)
        return None, None


def _get_unused_phrases(n: int = 8) -> List[str]:
    """Return up to n low-use-count phrases from phrases.db."""
    try:
        import sqlite3 as _sqlite3
        conn = _sqlite3.connect(_PHRASES_DB)
        rows = conn.execute(
            "SELECT text FROM phrases ORDER BY use_count ASC, id ASC LIMIT ?", (n,)
        ).fetchall()
        ids_to_inc = conn.execute(
            "SELECT id FROM phrases ORDER BY use_count ASC, id ASC LIMIT ?", (n,)
        ).fetchall()
        if ids_to_inc:
            conn.executemany(
                "UPDATE phrases SET use_count = use_count + 1 WHERE id = ?",
                ids_to_inc,
            )
            conn.commit()
        conn.close()
        return [r[0] for r in rows]
    except Exception as exc:
        _log.warning("PROJECT", f"phrases load failed: {exc}")
        print(f"[project_mode] _get_unused_phrases failed: {exc}", file=sys.stderr)
        return []


# ---------------------------------------------------------------------------
# Stage 1 -- ProjectScanner
# ---------------------------------------------------------------------------

class ProjectScanner:
    """
    Walks root_dir recursively, collects source files, and runs the outliner
    to produce per-file node summaries.
    """

    def __init__(self, on_log: Optional[Callable[[str], None]] = None):
        self._on_log = on_log or (lambda m: None)

    def _log_msg(self, msg: str):
        _log.info("SCANNER", msg)
        self._on_log(msg)

    def scan(self, root_dir: str) -> Dict:
        """
        Returns a tree dict:
          {
            "root": root_dir,
            "files": [
              {"path": "/abs/path.py", "rel": "rel/path.py",
               "ext": ".py", "size": 1234,
               "outline": [{"line":1,"kind":"function","name":"foo"}, ...]},
              ...
            ]
          }
        """
        root_dir = os.path.realpath(root_dir)
        self._log_msg(f"scanning {root_dir}")
        files = []

        for dirpath, dirnames, filenames in os.walk(root_dir, topdown=True):
            # filter out skip dirs in-place so os.walk does not recurse into them
            dirnames[:] = [
                d for d in dirnames
                if d not in _SKIP_DIRS and not d.startswith(".")
            ]
            for fname in sorted(filenames):
                ext = os.path.splitext(fname)[1].lower()
                if ext not in _SOURCE_EXTS:
                    continue
                fpath = os.path.join(dirpath, fname)
                rel   = os.path.relpath(fpath, root_dir)
                try:
                    size = os.path.getsize(fpath)
                except Exception:
                    size = 0

                outline_nodes = []
                if _HAS_OUTLINER and size < 200_000:
                    lang = _LANG_MAP.get(ext)
                    if lang:
                        try:
                            with open(fpath, "r", encoding="utf-8", errors="replace") as _f:
                                text = _f.read()
                            nodes = _outline(fpath, text, language=lang, include_comments=False)
                            outline_nodes = [n.to_dict() for n in nodes]
                        except Exception as oexc:
                            _log.warning("SCANNER", f"outliner failed {fname}: {oexc}")
                            print(f"[project_mode] outline failed {fname}: {oexc}", file=sys.stderr)

                files.append({
                    "path":    fpath,
                    "rel":     rel,
                    "ext":     ext,
                    "size":    size,
                    "outline": outline_nodes,
                })
                self._log_msg(f"  scanned {rel}  ({len(outline_nodes)} nodes)")

        self._log_msg(f"scan complete: {len(files)} source files")
        return {"root": root_dir, "files": files}


# ---------------------------------------------------------------------------
# Stage 2 -- ProjectPlanner
# ---------------------------------------------------------------------------

_PLANNER_SYSTEM = """\
You are a senior software engineer reviewing a codebase.
You will receive:
  - A recursive directory tree of source files with per-file outlines (function/class names).
  - A user instruction describing what should be fixed or improved.

Your job is to output a single JSON object with this exact structure:
{
  "prompt": "<detailed instruction for the code agent that will make the changes>",
  "phrases": ["<short working phrase 1>", "<short working phrase 2>", ...]
}

Rules:
- "prompt" should be a precise, actionable instruction written in second person
  that tells the code agent exactly what to do, referencing specific file names,
  function names, and the change strategy. It must include the working directory
  and file tree context.
- "phrases" should be 8-12 short conversational phrases (3-10 words each) that
  describe what you are doing as you work, in a casual technical tone. No emojis.
  No em-dashes. Example: "tracing the ledger balance logic", "fixing off-by-one in reports.py".
- Output raw JSON only. No markdown. No commentary outside the JSON.
""".strip()


class ProjectPlanner:
    """
    Calls GPT-4o mini to generate a plan prompt + phrases from the file tree.
    Drip-feeds unused phrases via on_log while waiting for the API.
    """

    def __init__(
        self,
        openai_key: str,
        on_log: Optional[Callable[[str], None]] = None,
    ):
        self._key    = openai_key
        self._on_log = on_log or (lambda m: None)

    def _log_msg(self, msg: str):
        _log.info("PLANNER", msg)
        self._on_log(msg)

    def plan(self, tree: Dict, instruction: str) -> Optional[Dict]:
        """
        Send tree + instruction to GPT-4o mini.
        Returns {"prompt": str, "phrases": list[str]} or None on failure.
        Also saves outputs to project_prompt.json and project_phrases.json.
        """
        self._log_msg("building file tree summary for GPT-4o mini")

        # build compact text representation of the tree
        tree_lines = [f"ROOT: {tree['root']}"]
        for f in tree["files"]:
            tree_lines.append(f"  {f['rel']}  ({f['size']} bytes)")
            for node in f["outline"][:20]:  # cap per file to keep prompt size sane
                tree_lines.append(
                    f"    L{node.get('line',0):>4}  {node.get('kind','?'):<12} {node.get('name','')}"
                )
        tree_text = "\n".join(tree_lines)

        # drip-feed unused phrases while waiting
        phrases_fallback = _get_unused_phrases(8)
        drip_idx = [0]

        def _drip_phrase():
            if phrases_fallback and drip_idx[0] < len(phrases_fallback):
                self._on_log(phrases_fallback[drip_idx[0]])
                drip_idx[0] += 1

        user_msg = (
            f"Working directory: {tree['root']}\n\n"
            f"User instruction:\n{instruction}\n\n"
            f"File tree with outlines:\n{tree_text}"
        )

        self._log_msg("calling GPT-4o mini for project plan")
        _drip_phrase()

        try:
            import openai as _openai
        except ImportError as exc:
            self._log_msg(f"openai SDK not installed: {exc}")
            print(f"[project_mode] openai import failed: {exc}", file=sys.stderr)
            return None

        try:
            client = _openai.OpenAI(api_key=self._key)
            _drip_phrase()
            resp = client.chat.completions.create(
                model="gpt-4o-mini",
                max_tokens=2048,
                messages=[
                    {"role": "system", "content": _PLANNER_SYSTEM},
                    {"role": "user",   "content": user_msg},
                ],
                response_format={"type": "json_object"},
            )
            _drip_phrase()
            raw_json = resp.choices[0].message.content or ""
        except Exception as exc:
            self._log_msg(f"GPT-4o mini call failed: {exc}")
            print(f"[project_mode] GPT call failed: {exc}", file=sys.stderr)
            return None

        try:
            plan = json.loads(raw_json)
        except Exception as exc:
            self._log_msg(f"JSON parse failed: {exc}")
            print(f"[project_mode] plan JSON parse failed: {exc}\nraw: {raw_json[:200]}", file=sys.stderr)
            return None

        if "prompt" not in plan:
            self._log_msg("plan JSON missing 'prompt' key")
            print(f"[project_mode] plan missing prompt key: {raw_json[:200]}", file=sys.stderr)
            return None

        if "phrases" not in plan or not isinstance(plan["phrases"], list):
            plan["phrases"] = []

        # save both halves to disk
        try:
            with open(_PLAN_PROMPT, "w", encoding="utf-8") as f:
                json.dump({"prompt": plan["prompt"]}, f, indent=2, ensure_ascii=False)
            _log.info("PLANNER", f"saved plan prompt to {_PLAN_PROMPT}")
        except Exception as exc:
            _log.warning("PLANNER", f"could not save project_prompt.json: {exc}")
            print(f"[project_mode] save project_prompt.json failed: {exc}", file=sys.stderr)

        try:
            with open(_PLAN_PHRASES, "w", encoding="utf-8") as f:
                json.dump({"phrases": plan["phrases"]}, f, indent=2, ensure_ascii=False)
            _log.info("PLANNER", f"saved plan phrases to {_PLAN_PHRASES}")
        except Exception as exc:
            _log.warning("PLANNER", f"could not save project_phrases.json: {exc}")
            print(f"[project_mode] save project_phrases.json failed: {exc}", file=sys.stderr)

        for ph in plan["phrases"]:
            self._on_log(str(ph))

        self._log_msg(f"plan ready: {len(plan['prompt'])} chars prompt, {len(plan['phrases'])} phrases")
        return plan


# ---------------------------------------------------------------------------
# Stage 3 -- ProjectExecutor
# ---------------------------------------------------------------------------

_EXECUTOR_SYSTEM = """\
You are an autonomous code editing agent working on a multi-file project.
You have been given a working directory, a list of source files with their
outlines, and a specific instruction.

Your task is to produce updated file content for every file that needs changes.

You MUST return your answer as a single raw JSON object with this structure:
{
  "changes": [
    {
      "file": "<relative path from working directory>",
      "content": "<complete updated file content>"
    }
  ],
  "commands": ["<shell command 1>", "<shell command 2>"]
}

Rules:
- "changes" lists every file you want to modify. Include the FULL updated content
  of each file. Do not include unchanged files.
- "commands" is an optional list of shell commands to run after applying changes
  (e.g. "python -m pytest", "python cli.py init"). Keep it short. Can be empty [].
- Output raw JSON only. No markdown fences. No explanations outside the JSON.
- File paths in "changes" must be relative to the working directory.
- Do not invent files that do not exist unless creating a new one is required.
""".strip()


class ProjectExecutor:
    """
    Sends the plan prompt to Haiku 4.5 with full project context.
    Stages each returned file change via Merger.propose().
    Optionally runs shell commands in root_dir.
    """

    def __init__(
        self,
        anthropic_key: str,
        merger: "Merger",
        on_log: Optional[Callable[[str], None]] = None,
        on_diff: Optional[Callable[[str, str], None]] = None,
        on_chunk: Optional[Callable[[int, str], None]] = None,
    ):
        self._key      = anthropic_key
        self._merger   = merger
        self._on_log   = on_log   or (lambda m: None)
        self._on_diff  = on_diff  or (lambda p, d: None)
        self._on_chunk = on_chunk or (lambda c, s: None)

    def _log_msg(self, msg: str):
        _log.info("EXECUTOR", msg)
        self._on_log(msg)

    def execute(
        self,
        tree: Dict,
        plan: Dict,
        plan_phrases: Optional[List[str]] = None,
    ) -> bool:
        """
        Send plan + tree to Haiku. Stage changes. Run commands.
        Returns True if at least one change was staged.
        """
        root_dir = tree["root"]

        # build tree text (same as planner but include full paths for Haiku)
        tree_lines = [f"WORKING_DIR: {root_dir}"]
        file_list_text = []
        for f in tree["files"]:
            line = f"  {f['rel']}  ({f['size']} bytes)"
            tree_lines.append(line)
            file_list_text.append(f['rel'])
            for node in f["outline"][:15]:
                tree_lines.append(
                    f"    L{node.get('line',0):>4}  {node.get('kind','?'):<12} {node.get('name','')}"
                )
        tree_text = "\n".join(tree_lines)

        plan_prompt = plan.get("prompt", "")
        user_message = (
            f"Working directory: {root_dir}\n\n"
            f"File tree:\n{tree_text}\n\n"
            f"Instruction:\n{plan_prompt}"
        )

        # drip phrases while waiting
        phrases = plan_phrases or plan.get("phrases", [])
        drip_idx = [0]

        def _drip():
            if drip_idx[0] < len(phrases):
                self._on_log(phrases[drip_idx[0]])
                drip_idx[0] += 1

        self._log_msg("calling Haiku 4.5 with project plan")
        _drip()

        try:
            import anthropic as _anthropic
        except ImportError as exc:
            self._log_msg(f"anthropic SDK not installed: {exc}")
            print(f"[project_mode] anthropic import failed: {exc}", file=sys.stderr)
            return False

        try:
            client      = _anthropic.Anthropic(api_key=self._key)
            chunks: list[str] = []
            total_chars = 0
            _drip()

            with client.messages.stream(
                model="claude-haiku-4-5-20251001",
                max_tokens=HAIKU_MAX_TOKENS,
                system=_EXECUTOR_SYSTEM,
                messages=[{"role": "user", "content": user_message}],
            ) as stream:
                for delta in stream.text_stream:
                    chunks.append(delta)
                    total_chars += len(delta)
                    self._on_chunk(total_chars, delta)
                    # drip a phrase every ~1000 chars
                    if total_chars % 1000 < len(delta):
                        _drip()

            raw = "".join(chunks)
        except Exception as exc:
            self._log_msg(f"Haiku call failed: {exc}")
            print(f"[project_mode] Haiku call failed: {exc}", file=sys.stderr)
            return False

        self._log_msg(f"Haiku returned {len(raw)} chars")

        # strip any accidental fence
        clean = re.sub(r"^```[a-zA-Z]*\n?", "", raw.strip())
        clean = re.sub(r"\n?```$", "", clean.strip())

        try:
            result = json.loads(clean)
        except Exception as exc:
            self._log_msg(f"executor JSON parse failed: {exc}")
            print(f"[project_mode] executor JSON parse error: {exc}\nraw[:400]: {raw[:400]}", file=sys.stderr)
            return False

        changes  = result.get("changes", [])
        commands = result.get("commands", [])

        if not isinstance(changes, list):
            self._log_msg("executor response: 'changes' is not a list")
            print(f"[project_mode] changes is not a list: {type(changes)}", file=sys.stderr)
            return False

        staged = 0
        for item in changes:
            rel_path   = item.get("file", "")
            new_content = item.get("content", "")
            if not rel_path or not new_content:
                self._log_msg(f"skipping malformed change entry: {str(item)[:80]}")
                continue

            # guard: must be inside root_dir
            abs_path = os.path.realpath(os.path.join(root_dir, rel_path))
            if not abs_path.startswith(root_dir):
                self._log_msg(f"blocked path outside root: {rel_path}")
                print(f"[project_mode] blocked outside-root path: {abs_path}", file=sys.stderr)
                continue

            if not os.path.isfile(abs_path):
                self._log_msg(f"target file not found: {rel_path}")
                print(f"[project_mode] target missing: {abs_path}", file=sys.stderr)
                continue

            change_id = self._merger.propose(abs_path, new_content, source_label="project-haiku")
            if change_id < 0:
                self._log_msg(f"merger.propose failed for {rel_path}")
                print(f"[project_mode] merge propose failed: {abs_path}", file=sys.stderr)
                continue
            if change_id == 0:
                self._log_msg(f"no changes needed: {rel_path}")
                continue

            staged += 1
            diff_text = self._merger.preview(abs_path)
            self._on_diff(abs_path, diff_text)
            self._log_msg(f"staged change id={change_id} for {rel_path}")

        # run optional shell commands
        if isinstance(commands, list):
            for cmd in commands:
                if not isinstance(cmd, str) or not cmd.strip():
                    continue
                self._log_msg(f"running command: {cmd}")
                try:
                    proc = subprocess.run(
                        cmd,
                        shell=True,
                        cwd=root_dir,
                        capture_output=True,
                        text=True,
                        timeout=60,
                    )
                    out = (proc.stdout or "").strip()
                    err = (proc.stderr or "").strip()
                    if out:
                        for line in out.splitlines()[:20]:
                            self._log_msg(f"  {line}")
                    if err:
                        for line in err.splitlines()[:10]:
                            self._log_msg(f"  STDERR: {line}")
                    self._log_msg(f"  exit code: {proc.returncode}")
                except subprocess.TimeoutExpired:
                    self._log_msg(f"  command timed out: {cmd}")
                    print(f"[project_mode] cmd timeout: {cmd}", file=sys.stderr)
                except Exception as exc:
                    self._log_msg(f"  command error: {exc}")
                    print(f"[project_mode] cmd error: {cmd}: {exc}", file=sys.stderr)

        self._log_msg(f"executor done: {staged} file(s) staged")
        return staged > 0


# ---------------------------------------------------------------------------
# Stage 4 -- ProjectSession (orchestrator)
# ---------------------------------------------------------------------------

class ProjectSession:
    """
    Orchestrates scan -> plan -> execute.
    All work runs in a background daemon thread.
    """

    def __init__(self, db: "AgentDB", merger: "Merger"):
        self._db     = db
        self._merger = merger
        self._session_id: Optional[int] = None
        self._tree: Optional[Dict] = None

    def run(
        self,
        root_dir: str,
        instruction: str,
        on_log: Callable[[str], None],
        on_diff: Callable[[str, str], None],
        on_done: Callable[[bool], None],
        on_chunk: Optional[Callable[[int, str], None]] = None,
    ):
        """
        Non-blocking. Launches a daemon thread for the full workflow.
        on_done(success: bool) is called when the session completes or fails.
        """
        t = threading.Thread(
            target=self._run_inner,
            args=(root_dir, instruction, on_log, on_diff, on_done, on_chunk or (lambda c, s: None)),
            daemon=True,
        )
        t.start()

    def _run_inner(self, root_dir, instruction, on_log, on_diff, on_done, on_chunk):
        try:
            on_log("project mode started")
            _log.info("PROJECT", f"starting session for {root_dir}")

            # create DB session row
            session_id = -1
            if self._db:
                session_id = self._db.create_project_session(
                    root_dir=root_dir,
                    instruction=instruction,
                    status="scanning",
                )
                self._session_id = session_id
                if session_id < 0:
                    on_log("warning: could not create project session in DB")
                    print("[project_mode] create_project_session returned -1", file=sys.stderr)

            # --- Stage 1: scan -----------------------------------------------
            scanner = ProjectScanner(on_log=on_log)
            tree    = scanner.scan(root_dir)
            self._tree = tree

            if self._db and session_id >= 0:
                self._db.update_project_session(
                    session_id,
                    file_tree_json=json.dumps({"files": [f["rel"] for f in tree["files"]]}),
                    status="planning",
                )

            # --- Stage 2: plan -----------------------------------------------
            openai_key, anthropic_key = _load_api_keys()

            if not openai_key:
                on_log("openai key missing -- skipping GPT planner; using raw instruction")
                plan = {"prompt": instruction, "phrases": []}
            else:
                planner = ProjectPlanner(openai_key=openai_key, on_log=on_log)
                plan    = planner.plan(tree, instruction)
                if plan is None:
                    on_log("GPT planner failed -- using raw instruction as fallback")
                    plan = {"prompt": instruction, "phrases": []}

            if self._db and session_id >= 0:
                self._db.update_project_session(
                    session_id,
                    plan_prompt=plan.get("prompt", ""),
                    status="executing",
                )

            # --- Stage 3: execute --------------------------------------------
            if not anthropic_key:
                on_log("anthropic key missing -- cannot call Haiku; aborting project run")
                _log.error("PROJECT", "anthropic key not loaded")
                if self._db and session_id >= 0:
                    self._db.update_project_session(session_id, status="failed")
                on_done(False)
                return

            executor = ProjectExecutor(
                anthropic_key=anthropic_key,
                merger=self._merger,
                on_log=on_log,
                on_diff=on_diff,
                on_chunk=on_chunk,
            )
            success = executor.execute(tree, plan)

            status = "complete" if success else "no_changes"
            if self._db and session_id >= 0:
                self._db.update_project_session(session_id, status=status)

            on_log(f"project mode done -- status: {status}")
            _log.info("PROJECT", f"session {session_id} status={status}")
            on_done(success)

        except Exception as exc:
            import traceback
            tb = traceback.format_exc()
            on_log(f"project mode error: {exc}")
            _log.error("PROJECT", f"unhandled exception: {exc}\n{tb[:400]}")
            print(f"[project_mode] _run_inner unhandled: {exc}\n{tb}", file=sys.stderr)
            if self._db and getattr(self, "_session_id", -1) >= 0:
                try:
                    self._db.update_project_session(self._session_id, status="error")
                except Exception:
                    pass
            on_done(False)

    def approve_all(self, root_dir: str, on_log: Callable[[str], None]) -> int:
        """
        Approve and apply all pending changes under root_dir.
        Returns number of changes applied.
        """
        if self._tree is None:
            on_log("no active tree -- run the project session first")
            return 0
        applied = 0
        for finfo in self._tree.get("files", []):
            fpath = finfo["path"]
            pending = self._db.get_pending_changes(fpath) if self._db else []
            for change in pending:
                ok = self._merger.apply(fpath, change["id"])
                if ok:
                    applied += 1
                    on_log(f"applied change id={change['id']} to {finfo['rel']}")
                else:
                    on_log(f"apply failed for id={change['id']} {finfo['rel']}")
                    print(f"[project_mode] apply failed id={change['id']} {fpath}", file=sys.stderr)
        return applied

    def reject_all(self, root_dir: str, on_log: Callable[[str], None]) -> int:
        """
        Reject all pending changes under root_dir.
        Returns number rejected.
        """
        if self._tree is None:
            on_log("no active tree -- run the project session first")
            return 0
        rejected = 0
        for finfo in self._tree.get("files", []):
            fpath = finfo["path"]
            pending = self._db.get_pending_changes(fpath) if self._db else []
            for change in pending:
                ok = self._merger.reject(fpath, change["id"])
                if ok:
                    rejected += 1
                    on_log(f"rejected change id={change['id']} for {finfo['rel']}")
        return rejected
