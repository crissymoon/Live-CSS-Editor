"""
Password Vault: stores encrypted usernames + passwords per domain.

Storage format (all values are Fernet-encrypted, base64 strings):
  {
    "example.com": {
      "username": "<encrypted>",
      "password": "<encrypted>"
    },
    ...
  }

Backward-compatible with the old single-string format:
  { "example.com": "<encrypted_password_only>" }
which is silently upgraded on first save.
"""

import os
import json
from urllib.parse import urlparse
from cryptography.fernet import Fernet


class PasswordManager:
    def __init__(self):
        self.key_file  = os.path.expanduser('~/.xcaliburmoon_key')
        self.data_file = os.path.expanduser('~/.xcaliburmoon_passwords')
        self._cipher   = self._get_or_create_cipher()
        self._store    = self._load()   # {domain: {'username': str, 'password': str}}

    # ── Cipher ────────────────────────────────────────────────────

    def _get_or_create_cipher(self) -> Fernet:
        if os.path.exists(self.key_file):
            with open(self.key_file, 'rb') as f:
                key = f.read()
        else:
            key = Fernet.generate_key()
            with open(self.key_file, 'wb') as f:
                f.write(key)
            os.chmod(self.key_file, 0o600)
        return Fernet(key)

    def _enc(self, text: str) -> str:
        return self._cipher.encrypt(text.encode()).decode()

    def _dec(self, token: str) -> str:
        try:
            return self._cipher.decrypt(token.encode()).decode()
        except Exception:
            return ''

    # ── Persistence ───────────────────────────────────────────────

    def _load(self) -> dict:
        if not os.path.exists(self.data_file):
            return {}
        try:
            with open(self.data_file, 'r') as f:
                raw = json.load(f)
        except Exception:
            return {}

        result = {}
        for domain, val in raw.items():
            if isinstance(val, dict):
                # New format
                result[domain] = {
                    'username': self._dec(val.get('username', '')),
                    'password': self._dec(val.get('password', '')),
                }
            else:
                # Legacy format: plain encrypted password string
                result[domain] = {
                    'username': '',
                    'password': self._dec(val),
                }
        return result

    def _save(self):
        encrypted = {
            domain: {
                'username': self._enc(entry['username']),
                'password': self._enc(entry['password']),
            }
            for domain, entry in self._store.items()
        }
        with open(self.data_file, 'w') as f:
            json.dump(encrypted, f)
        os.chmod(self.data_file, 0o600)

    # ── Public API ────────────────────────────────────────────────

    @staticmethod
    def domain_from_url(url: str) -> str:
        return urlparse(url).netloc

    def save(self, url: str, username: str, password: str) -> bool:
        domain = self.domain_from_url(url)
        if not domain:
            return False
        self._store[domain] = {'username': username, 'password': password}
        self._save()
        return True

    def get(self, url: str) -> dict | None:
        """Return {'username': ..., 'password': ...} or None."""
        domain = self.domain_from_url(url)
        return self._store.get(domain)

    def get_all(self) -> dict:
        """Return copy of full store: {domain: {'username': ..., 'password': ...}}."""
        return {d: dict(v) for d, v in self._store.items()}

    def delete(self, domain: str) -> bool:
        if domain not in self._store:
            return False
        del self._store[domain]
        self._save()
        return True
