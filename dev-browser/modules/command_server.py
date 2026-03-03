"""
HTTP command server that the mind-map frontend talks to.
Runs in a background daemon thread; enqueues commands for the Qt main thread.
"""

import json
import queue
import threading
from urllib.parse import urlparse, parse_qs
from http.server import HTTPServer, BaseHTTPRequestHandler

# Thread-safe command queue: HTTP thread enqueues, Qt main thread dequeues
CMD_QUEUE: queue.Queue = queue.Queue()


class BrowserCommandHandler(BaseHTTPRequestHandler):
    """Minimal HTTP server that accepts control commands from the mind-map app."""

    def log_message(self, format, *args):
        pass  # suppress access log noise

    def do_GET(self):
        parsed = urlparse(self.path)
        params = parse_qs(parsed.query)
        path   = parsed.path

        def ok(body=b'ok'):
            self.send_response(200)
            self.send_header('Content-Type', 'text/plain')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(body)

        def err(msg):
            self.send_response(400)
            self.send_header('Content-Type', 'text/plain')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(msg.encode())

        if path == '/ping':
            ok()

        elif path == '/navigate':
            nav_url = params.get('url', [''])[0]
            if nav_url:
                CMD_QUEUE.put(('navigate', nav_url))
                ok()
            else:
                err('missing url')

        elif path == '/geometry':
            try:
                x = int(float(params.get('x', ['0'])[0]))
                y = int(float(params.get('y', ['0'])[0]))
                w = int(float(params.get('w', ['800'])[0]))
                h = int(float(params.get('h', ['600'])[0]))
                CMD_QUEUE.put(('geometry', x, y, w, h))
                ok()
            except (ValueError, TypeError):
                err('invalid geometry params')

        elif path == '/show':
            CMD_QUEUE.put(('show',))
            ok()

        elif path == '/hide':
            CMD_QUEUE.put(('hide',))
            ok()

        elif path == '/back':
            CMD_QUEUE.put(('back',))
            ok()

        elif path == '/forward':
            CMD_QUEUE.put(('forward',))
            ok()

        elif path == '/reload':
            CMD_QUEUE.put(('reload',))
            ok()

        elif path == '/visual_mode_launch':
            q = params.get('q', [''])[0]
            try:
                from .visual_mode_server import start_server, create_session, get_session_url
                import urllib.parse as _up
                port = start_server()
                sid  = create_session('', 'Chat')
                url  = get_session_url(sid)
                if q:
                    url += '&q=' + _up.quote(q)
                self._json({'url': url})
            except Exception as exc:
                self._json({'error': str(exc)}, status=500)

        elif path == '/close':
            CMD_QUEUE.put(('close',))
            ok()

        # ── Apps / automation ─────────────────────────────────────
        elif path == '/apps/list':
            from .apps_manager import get_manager
            apps = get_manager().list_apps()
            self._json(apps)

        elif path == '/apps/automation/status':
            job_id = params.get('id', [''])[0]
            from .apps_manager import get_manager
            job = get_manager().get_job(job_id)
            if job:
                self._json(job.to_dict())
            else:
                self._json({'error': 'job not found', 'status': 'unknown'})

        elif path == '/apps/automation/jobs':
            from .apps_manager import get_manager
            self._json(get_manager().list_jobs())

        else:
            self.send_response(404)
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()

    def do_OPTIONS(self):
        """CORS preflight for fetch() calls from localhost PHP pages."""
        self.send_response(204)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def do_POST(self):
        parsed = urlparse(self.path)
        path   = parsed.path
        length = int(self.headers.get('Content-Length', 0))
        body   = self.rfile.read(length) if length else b'{}'

        try:
            payload = json.loads(body) if body.strip() else {}
        except Exception:
            payload = {}

        if path == '/apps/automation/run':
            app_slug = payload.get('app', '')
            params   = payload.get('params', {})
            if not app_slug:
                self._json({'error': 'missing app'}, status=400)
                return
            from .apps_manager import get_manager
            job_id = get_manager().run_automation(app_slug, params)
            self._json({'job_id': job_id})
        else:
            self.send_response(404)
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()

    def _json(self, data, status=200):
        body = json.dumps(data).encode()
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def start_command_server(port: int) -> HTTPServer:
    """Start the command HTTP server on the given port and return the server instance."""
    server = HTTPServer(('127.0.0.1', port), BrowserCommandHandler)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server
