"""
_NetworkMixin: TX/RX throughput display and color-picker methods.
"""

import subprocess

from PyQt6.QtCore import QTimer
from PyQt6.QtGui import QColor


class _NetworkMixin:
    """Methods for network throughput display and the CP eyedropper."""

    # -- Network throughput -----------------------------------------------

    def _net_active_iface(self) -> str:
        """Return the interface used by the default route (e.g. 'en3').
        Result is cached for 30 seconds to avoid spawning route constantly."""
        import time as _time
        now = _time.monotonic()
        if self._net_iface_cache and (now - self._net_iface_ts) < 30.0:
            return self._net_iface_cache
        try:
            out = subprocess.check_output(
                ['route', '-n', 'get', 'default'],
                stderr=subprocess.DEVNULL, timeout=1,
            ).decode(errors='replace')
            for line in out.splitlines():
                line = line.strip()
                if line.startswith('interface:'):
                    self._net_iface_cache = line.split()[-1]
                    self._net_iface_ts = now
                    return self._net_iface_cache
        except Exception:
            pass
        return self._net_iface_cache or 'en0'

    def _net_read_counters(self) -> tuple:
        """Read cumulative TX/RX byte counters from netstat -ib for the
        active interface.  Returns (tx_bytes, rx_bytes)."""
        iface = self._net_active_iface()
        try:
            out = subprocess.check_output(
                ['netstat', '-ib'],
                stderr=subprocess.DEVNULL,
                timeout=1,
            ).decode(errors='replace')
            for line in out.splitlines():
                parts = line.split()
                if parts and parts[0] == iface and len(parts) >= 10:
                    try:
                        rx = int(parts[6])
                        tx = int(parts[9])
                        self._net_prev_tx = tx
                        self._net_prev_rx = rx
                        return tx, rx
                    except ValueError:
                        continue
        except Exception:
            pass
        return self._net_prev_tx, self._net_prev_rx

    def _kick_net_update(self):
        """Timer callback (main thread). Spawns a background thread to read
        counters so the main thread is never blocked by subprocess calls."""
        if self._net_pending:
            return
        self._net_pending = True
        import threading
        threading.Thread(target=self._bg_net_update, daemon=True).start()

    def _bg_net_update(self):
        """Background thread: read counters, then signal the main thread."""
        try:
            prev_tx = self._net_prev_tx
            prev_rx = self._net_prev_rx
            cur_tx, cur_rx = self._net_read_counters()
            delta_tx = max(0, cur_tx - prev_tx)
            delta_rx = max(0, cur_rx - prev_rx)
        except Exception:
            delta_tx = delta_rx = 0
        self._net_update_ready.emit(delta_tx, delta_rx)

    def _apply_net_display(self, delta_tx: int, delta_rx: int):
        """Main thread: update the label with pre-computed deltas."""
        self._net_pending = False

        def _fmt(bps: int) -> str:
            if bps >= 1_048_576:
                return f'{bps / 1_048_576:.1f} MB/s'
            if bps >= 1024:
                return f'{bps / 1024:.0f} KB/s'
            return f'{bps} B/s'

        tx_str = _fmt(delta_tx)
        rx_str = _fmt(delta_rx)

        if delta_tx > 51_200:
            color = '#fb923c'
        elif delta_rx > 204_800:
            color = '#38bdf8'
        else:
            color = '#4ade80'

        self._net_label.setStyleSheet(
            f'QLabel {{ color:{color}; font-family:"JetBrains Mono",monospace;'
            f'font-size:11px; padding: 0 8px; }}'
        )
        self._net_label.setText(f'  TX: {tx_str}   RX: {rx_str}  ')

    # -- Color picker (eyedropper) ----------------------------------------

    def _start_color_pick(self):
        """Launch the color-picker overlay."""
        from PyQt6.QtWidgets import QApplication as _QApp
        from ._color_picker import _ColorPickOverlay
        # Grab the screenshot while the browser is still fully visible,
        # before any overlay window is created and forces a focus change.
        screen = _QApp.primaryScreen()
        pre_shot = screen.grabWindow(0) if screen else None
        self._pick_overlay = _ColorPickOverlay(self, pre_shot=pre_shot)
        self._pick_overlay.color_picked.connect(self._on_color_picked)
        self._pick_overlay.show()
        self._pick_overlay.raise_()
        self._pick_overlay.activateWindow()

    def _on_color_picked(self, hex_color: str):
        """Receives the picked hex string, copies to clipboard, shows in status bar."""
        self._pick_overlay = None  # release overlay reference
        from PyQt6.QtWidgets import QApplication as _QApp
        clipboard = _QApp.clipboard()
        if clipboard:
            clipboard.setText(hex_color)
        self._status_bar.showMessage(f'Copied {hex_color}', 4000)
        self._cp_btn.setStyleSheet(
            f'QToolButton {{ color:{hex_color}; background:transparent; border:none;'
            f'font-family:"JetBrains Mono",monospace; font-size:11px; padding:0 6px; }}'
            f'QToolButton:hover {{ color:#e9d5ff; }}'
        )
        QTimer.singleShot(4000, lambda: self._cp_btn.setStyleSheet(
            'QToolButton { color:#c084fc; background:transparent; border:none;'
            'font-family:"JetBrains Mono",monospace; font-size:11px; padding:0 6px; }'
            'QToolButton:hover { color:#e9d5ff; }'
        ))
