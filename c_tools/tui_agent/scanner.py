"""
scanner.py
Python mirror of folder_scanner.h / folder_scanner.c

Scans a directory for files and verifies they still exist
before returning them to the caller.
"""

import os
import stat
from typing import List, Optional, Tuple

from log_util import get_logger

SOURCE_EXTENSIONS = {
    ".js", ".ts", ".py", ".c", ".h", ".cpp", ".css", ".php",
    ".html", ".json", ".md", ".sh", ".txt", ".rs", ".go",
}


class FileEntry:
    __slots__ = ("path", "name", "extension", "size", "is_dir")

    def __init__(self, path: str, name: str, extension: str, size: int, is_dir: bool):
        self.path      = path
        self.name      = name
        self.extension = extension
        self.size      = size
        self.is_dir    = is_dir

    def __repr__(self):
        kind = "DIR" if self.is_dir else "FILE"
        return f"<{kind} {self.name} {self.size}B>"


class FolderScanner:
    def __init__(self, recursive: bool = False,
                 ext_filter: Optional[List[str]] = None):
        self._entries:   List[FileEntry] = []
        self._recursive  = recursive
        self._ext_filter = [e.lower() for e in ext_filter] if ext_filter else []
        self._log        = get_logger()

    # ------------------------------------------------------------------
    def scan(self, root_path: str) -> int:
        """Scan root_path and populate internal list. Returns file count."""
        self._entries = []
        if not os.path.isdir(root_path):
            self._log.error("SCANNER", f"not a directory: {root_path}")
            return 0

        self._log.info("SCANNER", f"scanning {root_path} (recursive={self._recursive})")
        try:
            self._walk(root_path)
            self._log.info("SCANNER", f"found {len(self._entries)} entries")
            return len(self._entries)
        except Exception as exc:
            self._log.error("SCANNER", f"scan failed: {exc}")
            return 0

    def _walk(self, path: str):
        try:
            for entry in sorted(os.scandir(path), key=lambda e: (e.is_dir(), e.name.lower())):
                try:
                    st = entry.stat()
                except OSError as exc:
                    self._log.warning("SCANNER", f"stat failed {entry.path}: {exc}")
                    continue

                name = entry.name
                if name.startswith("."):
                    continue

                ext = os.path.splitext(name)[1].lower()
                is_dir = entry.is_dir(follow_symlinks=False)

                if is_dir:
                    fe = FileEntry(entry.path, name, "", 0, True)
                    self._entries.append(fe)
                    if self._recursive:
                        self._walk(entry.path)
                else:
                    if self._ext_filter and ext not in self._ext_filter:
                        continue
                    fe = FileEntry(entry.path, name, ext, st.st_size, False)
                    self._entries.append(fe)
        except PermissionError as exc:
            self._log.warning("SCANNER", f"permission denied: {path}: {exc}")

    # ------------------------------------------------------------------
    def get_files(self, dirs: bool = False) -> List[FileEntry]:
        """Return entries, verifying each still exists. Optionally include dirs."""
        result = []
        for fe in self._entries:
            if fe.is_dir and not dirs:
                continue
            if not os.path.exists(fe.path):
                self._log.warning("SCANNER", f"file no longer exists: {fe.path}")
                continue
            result.append(fe)
        return result

    def filter_by_ext(self, ext: str) -> List[FileEntry]:
        ext = ext.lower()
        return [fe for fe in self.get_files() if fe.extension == ext]

    def stats(self) -> Tuple[int, int, int]:
        """Returns (file_count, dir_count, total_bytes)."""
        files = [fe for fe in self._entries if not fe.is_dir]
        dirs  = [fe for fe in self._entries if fe.is_dir]
        total = sum(fe.size for fe in files)
        return (len(files), len(dirs), total)

    def is_source_file(self, path: str) -> bool:
        return os.path.splitext(path)[1].lower() in SOURCE_EXTENSIONS
