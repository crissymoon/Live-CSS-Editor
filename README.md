# ADDING AI TO THIS

# Crissy's Style Tool

A desktop CSS/HTML/JavaScript live editor with real-time preview, built with Tauri v2 and PHP.

**Author:** Crissy Deutsch  
**Company:** XcaliburMoon Web Development  
**Website:** https://xcaliburmoon.net/  
**License:** MIT

---

## Overview

Crissy's Style Tool is a floating-panel code editor designed for rapid CSS prototyping and front-end development. Three resizable CodeMirror editors (CSS, HTML, JS) sit alongside a live preview iframe. All panels are freely draggable, minimizable, and restore their positions between sessions.

---

## Features

### Editors
- Three independent CodeMirror 5 editors: CSS, HTML, JavaScript
- Syntax highlighting and linting (CSSLint, JSHint, HTMLHint)
- Code folding with fold gutter
- Per-panel inline search with match count and orange highlights
- Undo/redo buttons in each panel header
- Custom indent guides with color, opacity, thickness, style, and step settings
- Column ruler with configurable position and color
- Autosave every 1.5 seconds to localStorage

### Fuzzy Autocomplete
- Context-aware CSS completions: property names before `:`, value keywords after `:`
- JavaScript and HTML completions
- Dropdown flips above cursor when near the bottom of the editor
- Arrow key navigation with scroll-into-view
- Capture-phase keyboard interception so arrow keys never move the editor cursor while the dropdown is open

### Inline Widgets
- Color swatch diamonds next to every color value -- click to open a color picker
- Size slider diamonds next to every numeric measurement -- drag to adjust values live
- Supports all CSS units: px, em, rem, %, vh, vw, vmin, vmax, pt, pc, ch, ex, cm, mm, in, fr, s, ms, deg, turn
- Bare numbers (flex: 1, gap: 0) are also detected

### Color Harmony Tool
- Floating panel with seven harmony modes: Complementary, Analogous, Triadic, Split-Comp, Tetradic, Square, Monochromatic
- Base color picker with hex readout
- Click any swatch to insert it into the active editor

### Wireframe Tool
- Full-canvas layout prototyping tool (1200x900 canvas)
- Add, move, and resize rectangles with mouse drag or arrow keys (1px; Shift+arrow for 10px)
- Adjust per-element label, position, size, margin, padding, background color, border color, border width, and border radius
- Nest elements inside each other via Parent selector
- Anti-overlap enforcement: siblings cannot touch or overlap (including margin areas)
- Children are constrained inside parent padding
- Horizontal and vertical ruler strips with tick marks every 25px, labels every 100px
- Drag from a ruler strip to create guide lines; drag guides to reposition; double-click to delete
- Save wireframe as JSON, load from JSON, and Copy Context (CSS comment block for use as AI/documentation context)
- Auto-saves to localStorage on every change

### Session Management
- Save/load named projects to localStorage
- Session history restore bar on startup (last 10 sessions)
- Manual save and load modals
- Reset Layout button restores default panel positions

---

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Desktop shell | Tauri v2 (Rust) |
| Backend | PHP 8.x built-in server |
| Editors | CodeMirror 5.65.16 (local vendor) |
| Linters | CSSLint 1.0.5, JSHint 2.13.6, HTMLHint 0.16.3 |
| JS modules | Vanilla ES5, namespaced under `window.LiveCSS` |
| CSS | Custom dark theme, no framework |

---

## Project Structure

```
live-css/
  index.php              Main HTML shell and PHP data bridge
  style.css              Global styles
  css/                   Modular CSS files
    base.css
    layout.css           CodeMirror panel layout
    fuzzy.css            Autocomplete dropdown
    wireframe.css        Wireframe tool
    color-tools.css
    ...
  js/                    Modular JavaScript files
    app.js               Boot and wiring
    editor.js            CodeMirror setup and live preview
    fuzzy.js             Fuzzy autocomplete
    color-swatch.js      Inline color swatch diamonds
    size-slider.js       Inline size slider diamonds
    color-harmony.js     Color harmony tool
    wireframe.js         Wireframe / layout prototyping tool
    gutter.js            Resizable/draggable panel system
    editor-search.js     Per-panel inline search
    indent-guide.js      Custom indent guides
    storage.js           localStorage persistence
    modal-save.js
    modal-load.js
    property-lookup.js   CSS properties reference
    native-bridge.js     Tauri native file/bridge integration
    cdn-loader.js        Local-first CodeMirror loader with CDN fallback
    utils.js             Shared utilities
  data/
    css-properties.php   CSS property reference data
    property-values.php  CSS property value keyword map
    default-content.php  Default editor content
  vendor/
    codemirror/          CodeMirror 5.65.16 (local copy)
    linters/             CSSLint, JSHint, HTMLHint
  src-tauri/             Tauri Rust application
  scripts/               Build and deploy scripts
    copy-www.js          Copies web assets to src-tauri/www
    gen-icon.js          App icon generator
```

---

## Running Locally (PHP server)

```bash
php -S localhost:8080
```

Then open http://localhost:8080 in a browser.

---

## Building the Desktop App

```bash
npm run tauri build
```

Requires Rust, Cargo, and the Tauri CLI. Run `node scripts/copy-www.js` before building to sync the web assets into `src-tauri/www/`.

---

## License

MIT License -- see [LICENSE](LICENSE) for full text.

Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development  
https://xcaliburmoon.net/
