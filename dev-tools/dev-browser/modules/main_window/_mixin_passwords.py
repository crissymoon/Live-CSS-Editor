"""
_PasswordsMixin: Fill Dev -- loads xcm_auth/dev-credentials.json and fills
the pb_admin login form at https://localhost:8443/pb_admin/login.php.
"""

import os
import json

from PyQt6.QtCore import QUrl
from PyQt6.QtWidgets import QMessageBox

_DEV_CREDS_FILE = os.path.normpath(
    os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        '..', '..', '..', 'page-builder', 'xcm_auth', 'dev-credentials.json',
    )
)
_LOGIN_PATH    = '/page-builder/pb_admin/login.php'
_DEV_LOGIN_URL = 'https://localhost:8443' + _LOGIN_PATH


class _PasswordsMixin:
    """Dev credential fill: reads xcm_auth/dev-credentials.json, no vault."""

    def autofill_credentials(self):
        """Load dev-credentials.json and fill the pb_admin login form."""
        try:
            with open(_DEV_CREDS_FILE, 'r') as _f:
                creds = json.load(_f)
        except Exception as exc:
            QMessageBox.warning(
                self, 'Fill Dev',
                f'Could not read dev-credentials.json:\n{exc}',
            )
            return

        username = creds.get('username', '')
        password = creds.get('password', '')

        browser = self.tabs.currentWidget()
        if not browser:
            return

        # Escape single quotes so they don't break the JS string literals.
        u = username.replace("'", "\\'")
        p = password.replace("'", "\\'")

        js = (
            "(function() {\n"
            f"    var u = '{u}';\n"
            f"    var p = '{p}';\n"
            "    document.querySelectorAll('input[type=\"password\"]')"
            "        .forEach(function(f) {\n"
            "        f.value = p;\n"
            "        f.dispatchEvent(new Event('input',  {bubbles:true}));\n"
            "        f.dispatchEvent(new Event('change', {bubbles:true}));\n"
            "    });\n"
            "    var uSels = [\n"
            "        'input[autocomplete=\"username\"]',\n"
            "        'input[autocomplete=\"email\"]',\n"
            "        'input[type=\"email\"]',\n"
            "        'input[name*=\"user\"]',\n"
            "        'input[name*=\"email\"]',\n"
            "        'input[name*=\"login\"]',\n"
            "        'input[id*=\"user\"]',\n"
            "        'input[id*=\"email\"]',\n"
            "        'input[id*=\"login\"]',\n"
            "    ];\n"
            "    for (var i = 0; i < uSels.length; i++) {\n"
            "        var f = document.querySelector(uSels[i]);\n"
            "        if (f) {\n"
            "            f.value = u;\n"
            "            f.dispatchEvent(new Event('input',  {bubbles:true}));\n"
            "            f.dispatchEvent(new Event('change', {bubbles:true}));\n"
            "            break;\n"
            "        }\n"
            "    }\n"
"})();\n"
        )

        def _inject(ok=True):
            browser.page().runJavaScript(js, lambda _: None)

        # Compare by URL path only -- avoids localhost vs 127.0.0.1 mismatches.
        if browser.url().path() == _LOGIN_PATH:
            _inject()
        else:
            # Navigate to the login page, then inject once it finishes loading.
            _slot = [None]

            def _on_load_finished(ok, _s=_slot):
                if _s[0] is not None:
                    try:
                        browser.loadFinished.disconnect(_s[0])
                    except Exception:
                        pass
                    _s[0] = None
                _inject(ok)

            _slot[0] = _on_load_finished
            browser.loadFinished.connect(_on_load_finished)
            browser.setUrl(QUrl(_DEV_LOGIN_URL))
