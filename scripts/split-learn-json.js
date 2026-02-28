#!/usr/bin/env node
/**
 * Splits learn.json into:
 *   style-sheets/theme_handler.json  — lightweight routing manifest
 *   style-sheets/themes/{key}.json   — full per-theme detail files
 */

const fs   = require('fs');
const path = require('path');

const BASE        = path.resolve(__dirname, '..', 'style-sheets');
const LEARN_FILE  = path.join(BASE, 'learn.json');
const HANDLER_OUT = path.join(BASE, 'theme_handler.json');
const THEMES_DIR  = path.join(BASE, 'themes');

// Fields that stay in theme_handler.json (lightweight, needed for scoring/routing)
const HANDLER_KEYS = [
  'prefix', 'title', 'description', 'best_for', 'avoid_for',
  'visual_identity', 'components_available', 'is_dark_by_default',
  'has_dark_mode', 'body_class', 'file'
];

// Fields that go in the individual theme file (heavy detail)
const DETAIL_KEYS = [
  'palette', 'variables', 'variable_count', 'unique_components', 'line_count'
];

// ── load ──────────────────────────────────────────────────────────────────────
const learn = JSON.parse(fs.readFileSync(LEARN_FILE, 'utf8'));

// ── create themes/ dir ────────────────────────────────────────────────────────
if (!fs.existsSync(THEMES_DIR)) {
  fs.mkdirSync(THEMES_DIR, { recursive: true });
  console.log('Created:', THEMES_DIR);
}

// ── split each theme ──────────────────────────────────────────────────────────
const handlerThemes = {};

for (const [key, theme] of Object.entries(learn.themes || {})) {
  // Lightweight stub for handler
  const stub = { theme_file: `themes/${key}.json` };
  for (const k of HANDLER_KEYS) {
    if (k in theme) stub[k] = theme[k];
  }
  handlerThemes[key] = stub;

  // Heavy detail file
  const detail = {};
  for (const k of DETAIL_KEYS) {
    if (k in theme) detail[k] = theme[k];
  }

  const detailPath = path.join(THEMES_DIR, `${key}.json`);
  fs.writeFileSync(detailPath, JSON.stringify(detail, null, 2));
  console.log(`  wrote themes/${key}.json`);
}

// ── write theme_handler.json ──────────────────────────────────────────────────
const handler = {
  version:            learn.version   ?? '1.0.0',
  description:        'Master index for all CSS theme data. Contains routing metadata, component coverage, and palette keywords. Individual theme detail (palette, variables) is in themes/*.json.',
  notes:              learn.notes     ?? [],
  component_coverage: learn.component_coverage ?? {},
  palette_keywords:   learn.palette_keywords   ?? {},
  themes:             handlerThemes
};

fs.writeFileSync(HANDLER_OUT, JSON.stringify(handler, null, 2));
console.log('\nWrote:', HANDLER_OUT);
console.log('Done. theme_handler.json +', Object.keys(handlerThemes).length, 'theme files.');
