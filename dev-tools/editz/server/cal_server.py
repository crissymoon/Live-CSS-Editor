#!/usr/bin/env python3
"""
cal_server.py — Local SQLite backend for the Calendar Tool.

Usage:
    python server/cal_server.py

Listens on http://localhost:5050
SQLite DB path: ~/Desktop/my_keys/calendar.db

Endpoints:
    GET    /api/calendar  — return saved calendar JSON
    POST   /api/calendar  — save calendar JSON
    DELETE /api/calendar  — clear all saved data
    GET    /              — health check
"""

import os
import json
import sqlite3
from pathlib import Path
from flask import Flask, request, jsonify
from flask_cors import CORS

# ── Config ────────────────────────────────────────────────────────────────── #

PORT = 5050
DB_DIR  = Path.home() / 'Desktop' / 'my_keys'
DB_PATH = DB_DIR / 'calendar.db'

# ── Flask app ─────────────────────────────────────────────────────────────── #

app = Flask(__name__)
CORS(app)  # allow requests from file:// and localhost origins

# ── DB helpers ────────────────────────────────────────────────────────────── #

def get_db():
    """Return a sqlite3 connection, creating the DB file / schema as needed."""
    DB_DIR.mkdir(parents=True, exist_ok=True)
    conn = sqlite3.connect(str(DB_PATH))
    conn.row_factory = sqlite3.Row
    conn.execute("""
        CREATE TABLE IF NOT EXISTS calendar_store (
            key        TEXT PRIMARY KEY,
            value      TEXT NOT NULL,
            updated_at TEXT NOT NULL DEFAULT (datetime('now'))
        )
    """)
    conn.commit()
    return conn


def db_get(key: str):
    with get_db() as conn:
        row = conn.execute(
            "SELECT value FROM calendar_store WHERE key = ?", (key,)
        ).fetchone()
    return json.loads(row['value']) if row else None


def db_set(key: str, data: dict):
    with get_db() as conn:
        conn.execute("""
            INSERT INTO calendar_store (key, value, updated_at)
                 VALUES (?, ?, datetime('now'))
            ON CONFLICT(key) DO UPDATE SET
                value      = excluded.value,
                updated_at = excluded.updated_at
        """, (key, json.dumps(data)))


def db_delete(key: str):
    with get_db() as conn:
        conn.execute("DELETE FROM calendar_store WHERE key = ?", (key,))


# ── Routes ────────────────────────────────────────────────────────────────── #

CAL_KEY = 'calendar_main'


@app.get('/')
def health():
    return jsonify({'status': 'ok', 'db': str(DB_PATH)})


@app.get('/api/calendar')
def load_calendar():
    data = db_get(CAL_KEY)
    return jsonify(data or {})


@app.post('/api/calendar')
def save_calendar():
    payload = request.get_json(force=True, silent=True)
    if not payload:
        return jsonify({'error': 'invalid JSON'}), 400
    db_set(CAL_KEY, payload)
    return jsonify({'status': 'saved'})


@app.delete('/api/calendar')
def clear_calendar():
    db_delete(CAL_KEY)
    return jsonify({'status': 'cleared'})


# ── Main ──────────────────────────────────────────────────────────────────── #

if __name__ == '__main__':
    print(f'[cal_server] SQLite DB → {DB_PATH}')
    print(f'[cal_server] Listening on http://localhost:{PORT}')
    app.run(host='127.0.0.1', port=PORT, debug=False)
