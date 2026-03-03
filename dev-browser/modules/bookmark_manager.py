"""
Bookmarks: persistent JSON file stored at ~/.xcaliburmoon_bookmarks.json
"""

import os
import json


class BookmarkManager:
    def __init__(self):
        self.bookmark_file = os.path.expanduser('~/.xcaliburmoon_bookmarks.json')
        self.bookmarks = self._load()

    def _load(self) -> dict:
        if not os.path.exists(self.bookmark_file):
            return {}
        try:
            with open(self.bookmark_file, 'r') as f:
                return json.load(f)
        except Exception:
            return {}

    def _save(self):
        with open(self.bookmark_file, 'w') as f:
            json.dump(self.bookmarks, f, indent=2)
        os.chmod(self.bookmark_file, 0o600)

    def save_bookmark(self, title: str, url: str) -> bool:
        if not title or not url:
            return False
        self.bookmarks[title] = url
        self._save()
        return True

    def delete_bookmark(self, title: str) -> bool:
        if title not in self.bookmarks:
            return False
        del self.bookmarks[title]
        self._save()
        return True

    def edit_bookmark(self, old_title: str, new_title: str, new_url: str) -> bool:
        """Rename or re-URL an existing bookmark.  Old entry is removed first."""
        if old_title not in self.bookmarks:
            return False
        if not new_title or not new_url:
            return False
        del self.bookmarks[old_title]
        self.bookmarks[new_title] = new_url
        self._save()
        return True

    def get_bookmarks(self) -> dict:
        return self.bookmarks.copy()
