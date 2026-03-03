"""
AppsManager -- local PHP server, app discovery, and Selenium automation bridge.

Directory layout
----------------
apps/
    _template/          (skipped by list_apps -- leading underscore)
        manifest.json
        index.php
    <slug>/
        manifest.json   -- required: {"name": "...", "slug": "...", "description": "..."}
        index.php       -- PHP frontend served at http://127.0.0.1:PHP_PORT/<slug>/
        automation.py   -- optional Selenium automation:
                              def run(driver, params: dict, job) -> dict

The PHP built-in server is started on PHP_PORT (default 9879) and serves the
entire apps/ directory.  Each app is accessed at:
    http://127.0.0.1:9879/<slug>/

Automation jobs
---------------
An app's automation.py must expose a top-level function:

    def run(driver: selenium.webdriver.Chrome, params: dict, job) -> dict:
        ...
        return {"key": "value", ...}

The function receives:
  driver -- a headless Chrome WebDriver instance (already created)
  params -- the dict passed by the caller (e.g. {"url": "https://..."})
  job    -- the AutomationJob; call job.append_log("msg") to stream log lines

Screenshots are automatically saved to apps/<slug>/screenshots/ and served by
the PHP server so the frontend can display them at:
    http://127.0.0.1:9879/<slug>/screenshots/latest.png

The command server exposes:
    GET  /apps/list
    POST /apps/automation/run   body: {"app": "<slug>", "params": {...}}
    GET  /apps/automation/status?id=<job_id>
    GET  /apps/automation/jobs
"""

import os
import json
import uuid
import time
import threading
import subprocess
import importlib.util
from typing import Optional

_HERE      = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APPS_DIR   = os.path.join(_HERE, 'apps')
PHP_PORT   = 9879

_manager_instance: Optional['AppsManager'] = None


# ---------------------------------------------------------------------------
# AutomationJob
# ---------------------------------------------------------------------------

class AutomationJob:
    """Represents a running or completed Selenium automation job."""

    def __init__(self, job_id: str, app_slug: str, params: dict):
        self.job_id      = job_id
        self.app_slug    = app_slug
        self.params      = params
        self.status      = 'queued'     # queued | running | done | error
        self.result      = None
        self.error       = None
        self.log: list   = []
        self.started_at  = time.time()
        self.finished_at: Optional[float] = None

    def append_log(self, msg: str):
        self.log.append(msg)

    def to_dict(self) -> dict:
        return {
            'job_id':      self.job_id,
            'app_slug':    self.app_slug,
            'params':      self.params,
            'status':      self.status,
            'result':      self.result,
            'error':       self.error,
            'log':         self.log,
            'started_at':  self.started_at,
            'finished_at': self.finished_at,
        }


# ---------------------------------------------------------------------------
# AppsManager
# ---------------------------------------------------------------------------

class AppsManager:
    """Manages the local PHP server, app discovery, and Selenium jobs."""

    PHP_PORT = PHP_PORT
    APPS_DIR = APPS_DIR

    def __init__(self):
        self._php_proc: Optional[subprocess.Popen] = None
        self._jobs: dict = {}
        self._jobs_lock  = threading.Lock()

    # ── PHP server ────────────────────────────────────────────────

    def start_php_server(self) -> bool:
        """Start php -S serving APPS_DIR on PHP_PORT. Returns True on success."""
        if self._php_proc and self._php_proc.poll() is None:
            return True  # already running
        try:
            os.makedirs(self.APPS_DIR, exist_ok=True)
            self._php_proc = subprocess.Popen(
                ['php', '-S', f'127.0.0.1:{self.PHP_PORT}', '-t', self.APPS_DIR],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            return True
        except FileNotFoundError:
            return False  # php binary not found
        except Exception:
            return False

    def stop_php_server(self):
        if self._php_proc and self._php_proc.poll() is None:
            self._php_proc.terminate()
            self._php_proc = None

    def php_running(self) -> bool:
        return self._php_proc is not None and self._php_proc.poll() is None

    # ── App discovery ─────────────────────────────────────────────

    def list_apps(self) -> list:
        """Return list of app manifests sorted by name, skipping _ prefixed dirs."""
        apps = []
        if not os.path.isdir(self.APPS_DIR):
            return apps
        for entry in sorted(os.listdir(self.APPS_DIR)):
            if entry.startswith('_'):
                continue
            app_dir = os.path.join(self.APPS_DIR, entry)
            if not os.path.isdir(app_dir):
                continue
            manifest_path = os.path.join(app_dir, 'manifest.json')
            if not os.path.exists(manifest_path):
                continue
            try:
                with open(manifest_path, 'r') as f:
                    manifest = json.load(f)
                manifest.setdefault('slug', entry)
                manifest.setdefault('name', entry)
                manifest.setdefault('description', '')
                manifest.setdefault('version', '1.0')
                manifest['has_automation'] = os.path.exists(
                    os.path.join(app_dir, 'automation.py')
                )
                apps.append(manifest)
            except Exception:
                pass
        return apps

    def get_app_url(self, slug: str) -> str:
        return f'http://127.0.0.1:{self.PHP_PORT}/{slug}/'

    def get_app_dir(self, slug: str) -> str:
        return os.path.join(self.APPS_DIR, slug)

    def create_app_from_template(self, name: str) -> str:
        """
        Copy the _template app to a new slug directory.
        Returns the new slug.
        """
        import shutil
        slug     = name.strip().lower().replace(' ', '-').replace('/', '-')
        app_dir  = os.path.join(self.APPS_DIR, slug)
        template = os.path.join(self.APPS_DIR, '_template')

        if os.path.isdir(template):
            shutil.copytree(template, app_dir, dirs_exist_ok=True)
        else:
            os.makedirs(app_dir, exist_ok=True)
            with open(os.path.join(app_dir, 'index.php'), 'w') as f:
                f.write(_minimal_php(name))

        manifest = {
            'name':        name,
            'slug':        slug,
            'description': '',
            'version':     '1.0',
        }
        with open(os.path.join(app_dir, 'manifest.json'), 'w') as f:
            json.dump(manifest, f, indent=2)

        return slug

    # ── Automation ────────────────────────────────────────────────

    def run_automation(self, app_slug: str, params: dict = None) -> str:
        """Queue and start a Selenium automation job. Returns job_id."""
        params = params or {}
        job_id = str(uuid.uuid4())
        job    = AutomationJob(job_id, app_slug, params)
        with self._jobs_lock:
            self._jobs[job_id] = job
        t = threading.Thread(target=self._execute_job, args=(job,), daemon=True)
        t.start()
        return job_id

    def get_job(self, job_id: str) -> Optional[AutomationJob]:
        with self._jobs_lock:
            return self._jobs.get(job_id)

    def list_jobs(self) -> list:
        with self._jobs_lock:
            return [j.to_dict() for j in self._jobs.values()]

    def _execute_job(self, job: AutomationJob):
        automation_path = os.path.join(self.APPS_DIR, job.app_slug, 'automation.py')
        if not os.path.exists(automation_path):
            job.status = 'error'
            job.error  = f'No automation.py in app "{job.app_slug}"'
            job.finished_at = time.time()
            return

        job.status = 'running'
        job.append_log(f'[{_ts()}] Starting automation for "{job.app_slug}"')

        driver = None
        try:
            # ── Load the automation module ────────────────────────
            spec = importlib.util.spec_from_file_location(
                f'_app_{job.app_slug}', automation_path)
            mod  = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)

            if not hasattr(mod, 'run'):
                raise AttributeError('automation.py must define run(driver, params, job)')

            # ── Set up headless Chrome via Selenium ───────────────
            from selenium import webdriver
            from selenium.webdriver.chrome.options import Options
            from selenium.webdriver.chrome.service import Service
            from webdriver_manager.chrome import ChromeDriverManager

            opts = Options()
            opts.add_argument('--headless=new')
            opts.add_argument('--no-sandbox')
            opts.add_argument('--disable-dev-shm-usage')
            opts.add_argument('--window-size=1280,800')
            opts.add_argument('--disable-gpu')
            # Point Selenium at the macOS Chrome app binary explicitly so
            # webdriver-manager can match the correct ChromeDriver version.
            opts.binary_location = (
                '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'
            )

            _service = Service(ChromeDriverManager().install())

            shots_dir = os.path.join(self.APPS_DIR, job.app_slug, 'screenshots')
            os.makedirs(shots_dir, exist_ok=True)

            driver = webdriver.Chrome(service=_service, options=opts)
            # Expose helpers to automation.py via driver attributes
            driver._job       = job
            driver._shots_dir = shots_dir

            job.append_log(f'[{_ts()}] Headless Chrome started')

            result     = mod.run(driver, job.params, job)
            job.result = result
            job.status = 'done'
            job.append_log(f'[{_ts()}] Done')

        except Exception as exc:
            job.status = 'error'
            job.error  = str(exc)
            job.append_log(f'[{_ts()}] ERROR: {exc}')
        finally:
            if driver:
                try:
                    driver.quit()
                except Exception:
                    pass
            job.finished_at = time.time()


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _ts() -> str:
    return time.strftime('%H:%M:%S')


def _minimal_php(name: str) -> str:
    return (
        '<?php\n'
        '// Generated by Crissy\'s Browser Apps\n'
        '?>\n'
        '<!DOCTYPE html>\n'
        '<html lang="en">\n'
        '<head><meta charset="UTF-8">\n'
        f'<title>{name}</title>\n'
        '<style>body{{font-family:sans-serif;padding:2rem;background:#0f0f1a;color:#e2e8f0}}</style>\n'
        '</head>\n'
        '<body>\n'
        f'<h1>{name}</h1>\n'
        '<p>Edit this file to build your app.</p>\n'
        '</body>\n'
        '</html>\n'
    )


# ---------------------------------------------------------------------------
# Module-level singleton
# ---------------------------------------------------------------------------

def get_manager() -> AppsManager:
    global _manager_instance
    if _manager_instance is None:
        _manager_instance = AppsManager()
    return _manager_instance
