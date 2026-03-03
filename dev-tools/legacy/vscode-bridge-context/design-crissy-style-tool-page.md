# Design Spec: Crissy's Style Tool Web Page

Source: `custom_design_assets/TEST.md`
Last updated: 2026-03-02

This file is a design brief for the Crissy Style Tool web page.
It is NOT for the Tauri desktop app.
Read this file any time you are asked to build, extend, or review
sections/pages for the style tool marketing or landing page.

---

## Asset registry

All paths are relative to `PROJECT_ROOT` (`/Users/mac/Documents/live-css`).

| Slot           | File                                                               |
|----------------|--------------------------------------------------------------------|
| Page background| `custom_design_assets/backgrounds/bg-purple.webp`                 |
| Header image   | `custom_design_assets/ui-elements/left-top-header.webp`           |
| Logo           | `custom_design_assets/ui-elements/style-tool-logo.webp`           |
| Footer image   | `custom_design_assets/ui-elements/bottom-right-footer.webp`       |

Console error-handling requirement: every fetch or image load that touches
these assets must have an `.onerror` / `.catch` that writes to `console.error`
so failures are visible in DevTools without digging into network tabs.

---

## Layout spec

### Page background
- Full-page background: `bg-purple.webp`
- `background-size: cover`, `background-attachment: fixed`
- Applied on `body` or the outermost wrapper

### Header (oversized, left-aligned)
- Placement: top of page, anchored to the left
- Size: oversized relative to a standard 56px bar -- minimum height ~220px
- Navigation: NO traditional nav bar; leave it empty for now
- Decorative image: `left-top-header.webp`, covers the full header area
  from the left edge
- Logo image: `style-tool-logo.webp` displayed inside the header, top-left area
- Wordmark text: "Crissy's Style Tool" next to or below the logo
- CSS selector target: `.site-header`, `.site-header__inner`,
  `.site-header__logo-img`, `.site-header__wordmark`

### Mid section
- NONE at this time; leave a structural placeholder only

### Footer (oversized, bottom-right)
- Placement: bottom of page, decorative image anchored to the bottom-right
- Size: oversized -- minimum height ~200px
- Decorative image: `bottom-right-footer.webp`, anchored `right bottom`
- CSS selector target: `.site-footer`, `.site-footer__inner`

---

## HTML skeleton (reference -- used by build.php or static pages)

```html
<body class="st-page">

  <header class="site-header">
    <div class="site-header__inner">
      <img
        class="site-header__logo-img"
        src="/custom_design_assets/ui-elements/style-tool-logo.webp"
        alt="Crissy's Style Tool logo"
        onerror="console.error('[design] logo failed to load:', this.src)"
      >
      <h1 class="site-header__wordmark">Crissy's Style Tool</h1>
    </div>
  </header>

  <!-- no mid section yet -->
  <main class="site-main"></main>

  <footer class="site-footer">
    <div class="site-footer__inner"></div>
  </footer>

</body>
```

---

## CSS home

The styles for this design live in `style-sheets/xcm-contact.css`.
That file is managed by the MCP bridge and can be read or updated via
`read_stylesheet("xcm-contact.css")` and `update_stylesheet(...)`.

---

## Checklist status (from TEST.md)

- [ ] Page background -- bg-purple.webp
- [ ] Oversized header -- left-top-header.webp (left-aligned, no nav)
- [ ] Logo + wordmark -- style-tool-logo.webp, "Crissy's Style Tool"
- [ ] No mid section yet
- [ ] Oversized footer -- bottom-right-footer.webp (bottom-right)

Update this checklist as sections are completed.

---

## How to push design changes via the bridge

The bridge connects VSCode Copilot to all three editors and the live preview
inside the Crissy Style Tool (Tauri app).

### Recommended: save_project (all 3 editors at once)

Call `save_project` with `name`, `html`, `css`, and optionally `js`. This writes
the full project to SQLite. The browser polls for new saves every ~3.6 seconds
and auto-loads the project into all 3 editors. The user can also click Load
in the app to browse saved projects and load them manually.

Previous versions are backed up automatically in the database. Projects saved
by Copilot are tagged `[from Copilot]` in the Load modal.

Example workflow:
1. `save_project({ name: "style-tool-landing", html: "...", css: "...", js: "" })`
2. The app detects the save within 4 seconds, loads it into all 3 editors.
3. To iterate, call `save_project` again with the same name. The old version
   is backed up before overwrite.

### Individual editor push tools

Use these when you only need to update one editor at a time:

| What to change | MCP tool to call |
|---|---|
| CSS styles | `update_stylesheet("xcm-contact.css", css)` -- CSS editor updates + preview refreshes |
| HTML structure | `push_html_content({ html })` -- HTML editor updates + preview refreshes |
| JS behavior | `push_js_content({ js })` -- JS editor updates + preview refreshes |

All three propagate to the live editors within 3 seconds of the MCP call,
as long as the bridge toggle is ON in the app.

### Other useful tools

| Tool | Use case |
|---|---|
| `list_projects` | See what is already saved in the database |
| `load_project` | Read a saved project before editing it |
| `refresh_preview` | Force a full page reload in the browser |
| `list_debug_tickets` | Check for open bugs before making changes |

### Debugging bridge issues

Open DevTools in the Tauri app and look for `[BridgeSync]` console messages.
Every poll and apply logs there. If a push is not landing, check:

```javascript
// Run in the Tauri app DevTools console
console.log('bridge active:', typeof BridgeSync !== 'undefined' && BridgeSync.isActive());
// If false, run: BridgeSync.enable()
// Then trigger a manual poll: BridgeSync.poll()
```

---



```javascript
[
  '/custom_design_assets/backgrounds/bg-purple.webp',
  '/custom_design_assets/ui-elements/left-top-header.webp',
  '/custom_design_assets/ui-elements/style-tool-logo.webp',
  '/custom_design_assets/ui-elements/bottom-right-footer.webp'
].forEach(function(src) {
  var img = new Image();
  img.onload  = function() { console.log('[design asset ok]', src, img.naturalWidth + 'x' + img.naturalHeight); };
  img.onerror = function() { console.error('[design asset MISSING]', src); };
  img.src = src;
});
```
