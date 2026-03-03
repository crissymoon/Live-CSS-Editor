# Page Builder - Section Schema Reference

This document tells an AI everything needed to create valid page-builder section
JSON files that work with the Crissy Style Tool page builder system.

---

## What a section file is

Each section is a single JSON file that lives in one of two places:

- **Template library** -- `page-builder/sections/{type}/name.json`
  These are reusable templates. They always include a `_meta` block which is
  stripped when the template is copied into a page.

- **Page-specific file** -- `page-builder/pages/{page-name}/{file}.json`
  These are live section files for a specific page. The `_meta` block is absent.
  These are what `build.php` reads to render the final HTML.

---

## Four section types

| type     | where it lives in the page        | typical file name      |
|----------|------------------------------------|------------------------|
| `header` | pinned top - nav bar               | `header.json`          |
| `footer` | bottom of page                     | `footer.json`          |
| `section`| body content block                 | `section-N-name.json`  |
| `panel`  | body block with explicit columns   | `section-N-name.json`  |

A `panel` is technically a `section` with a two-column `layout: "row"` and
`columns: 2`. The type value inside the file is `"section"` -- the word
"panel" is just a composer label.

---

## Top-level shape per type

### type: header

```json
{
    "_meta": { "name": "...", "type": "header", "description": "..." },
    "type": "header",
    "settings": {
        "bg":          "#0d0d18",
        "borderColor": "rgba(99,102,241,0.2)",
        "height":      "56px",
        "maxWidth":    "1100px",
        "padding":     "0 28px"
    },
    "brand": {
        "text":          "Brand",
        "href":          "#",
        "color":         "#6366f1",
        "fontSize":      "15px",
        "fontWeight":    "700",
        "letterSpacing": "0.08em"
    },
    "nav": [
        { "label": "Home",    "href": "#" },
        { "label": "About",   "href": "#" },
        { "label": "Contact", "href": "#", "cta": true }
    ],
    "navSettings": {
        "linkColor":      "#a5a5c0",
        "ctaColor":       "#6366f1",
        "ctaBorder":      "rgba(99,102,241,0.5)",
        "gap":            "4px",
        "fontSize":       "12px",
        "navCollapseAt":  640
    }
}
```

Required keys: `type`, `settings`, `brand`, `nav`, `navSettings`

- `nav` items can have a `"cta": true` flag -- rendered as a bordered button link
- `navCollapseAt` is the px breakpoint below which the nav collapses (not yet wired
  to a hamburger; items simply hide below that width)
- `brand.href` defaults to `#` for templates; real pages replace it

---

### type: footer

```json
{
    "_meta": { "name": "...", "type": "footer", "description": "..." },
    "type": "footer",
    "settings": {
        "bg":        "#0d0d18",
        "borderTop": "1px solid rgba(255,255,255,0.06)",
        "padding":   "24px 28px",
        "maxWidth":  "1100px"
    },
    "copy": {
        "id":   "footer-copy",
        "text": "2026 Your Brand",
        "settings": {
            "color":         "#5555a0",
            "fontSize":      "12px",
            "letterSpacing": "0.06em"
        }
    },
    "links": [
        { "id": "footer-privacy", "label": "Privacy", "href": "#" },
        { "id": "footer-terms",   "label": "Terms",   "href": "#" }
    ],
    "linkSettings": {
        "color":    "#5555a0",
        "fontSize": "11px",
        "gap":      "4px"
    }
}
```

Required keys: `type`, `settings`, `copy`
Optional keys: `links`, `linkSettings`

- Each link needs a unique `id`, a `label`, and an `href`
- `copy.id` must be unique per page so overrides can target it

---

### type: section

```json
{
    "_meta": { "name": "...", "type": "section", "description": "..." },
    "type": "section",
    "id": "hero",
    "layout": "column",
    "settings": {
        "bg":       "#0a0a14",
        "padding":  "100px 24px",
        "maxWidth": "760px",
        "align":    "center",
        "gap":      "24px"
    },
    "blocks": [ ... ]
}
```

Required keys: `type`, `id`, `layout`, `settings`, `blocks`

- `id` -- short slug, unique per page (e.g. `"hero"`, `"features"`, `"cta"`)
- `layout` -- `"column"` for stacked blocks, `"row"` for side-by-side columns
- `settings.align` -- `"center"` centers the inner max-width wrapper; omit for
  left-aligned
- `settings.gap` -- flex gap between blocks, e.g. `"24px"`
- `settings.columns` -- number of columns when `layout: "row"`, e.g. `3`
- A section can optionally have a top-level `"heading"` block outside `blocks`
  for a section heading that spans the full width above the columns:

```json
"heading": {
    "id":   "features-heading",
    "text": "Why choose us",
    "tag":  "h2",
    "settings": { "color": "#c7c7f0", "fontSize": "28px", "fontWeight": "600" }
}
```

---

### type: panel (two-column section)

Identical shape to `type: section` with `layout: "row"` and `columns: 2`.
The `blocks` array should have exactly two entries, typically both `"card"` type,
one for main content and one for the sidebar.

---

## Block types (used inside `blocks` arrays)

### heading

```json
{
    "type": "heading",
    "id":   "hero-heading",
    "text": "Build Something Beautiful",
    "tag":  "h1",
    "settings": {
        "color":         "#e8e8f0",
        "fontSize":      "52px",
        "fontWeight":    "700",
        "textAlign":     "center",
        "lineHeight":    "1.15",
        "letterSpacing": "0.02em",
        "marginBottom":  "8px"
    }
}
```

- `tag` -- any heading tag: `"h1"` through `"h6"`
- All `settings` keys become inline CSS via `build.php`

---

### text

```json
{
    "type": "text",
    "id":   "hero-sub",
    "text": "A clean, focused way to craft fast, lightweight pages.",
    "settings": {
        "color":      "#8888a0",
        "fontSize":   "18px",
        "lineHeight": "1.6",
        "textAlign":  "center"
    }
}
```

Rendered as a `<p>` tag.

---

### button

```json
{
    "type": "button",
    "id":   "hero-cta",
    "text": "Get Started",
    "href": "#",
    "settings": {
        "bg":            "#6366f1",
        "color":         "#ffffff",
        "padding":       "13px 32px",
        "fontSize":      "14px",
        "fontWeight":    "600",
        "borderRadius":  "4px",
        "letterSpacing": "0.06em"
    }
}
```

Rendered as an `<a>` tag styled as a button. `href` is required.

---

### card

A container block that wraps child blocks. Used when `layout: "row"` splits
blocks into columns. The card itself is a `<div>` with its own settings, and
its `children` array contains `heading`, `text`, or `button` blocks.

```json
{
    "type": "card",
    "id":   "feature-1",
    "settings": {
        "bg":           "#111122",
        "border":       "1px solid rgba(255,255,255,0.07)",
        "padding":      "24px",
        "borderRadius": "0"
    },
    "children": [
        { "type": "heading", "id": "f1-title", "text": "Fast", "tag": "h3",
          "settings": { "color": "#6366f1", "fontSize": "16px", "fontWeight": "600", "marginBottom": "8px" }},
        { "type": "text",    "id": "f1-desc",  "text": "Pages build in milliseconds.",
          "settings": { "color": "#8888a0", "fontSize": "13px", "lineHeight": "1.65" }}
    ]
}
```

---

## Settings key reference

All `settings` objects map to inline CSS via `build.php`'s `toStyle()` function.
The mapping is:

| JSON key        | CSS property        | Notes                                  |
|-----------------|---------------------|----------------------------------------|
| `bg`            | `background`        |                                        |
| `borderColor`   | `border-color`      |                                        |
| `borderTop`     | `border-top`        | Shorthand, e.g. `"1px solid #eee"`    |
| `border`        | `border`            | Full shorthand                         |
| `fontWeight`    | `font-weight`       |                                        |
| `fontSize`      | `font-size`         |                                        |
| `lineHeight`    | `line-height`       |                                        |
| `letterSpacing` | `letter-spacing`    |                                        |
| `textTransform` | `text-transform`    |                                        |
| `textAlign`     | `text-align`        |                                        |
| `marginBottom`  | `margin-bottom`     |                                        |
| `marginTop`     | `margin-top`        |                                        |
| `flexDirection` | `flex-direction`    |                                        |
| `borderRadius`  | `border-radius`     |                                        |
| `color`         | `color`             |                                        |
| `padding`       | `padding`           |                                        |
| `height`        | `height`            |                                        |
| `href`          | *(ignored)*         | Used as anchor href, not a CSS prop    |
| `maxWidth`      | *(layout helper)*   | Skipped in inline style; used by renderer to set max-width on wrapper |
| `align`         | *(layout helper)*   | `"center"` makes wrapper `margin: 0 auto` |
| `gap`           | *(layout helper)*   | Applied to the flex container          |
| `columns`       | *(layout helper)*   | Sets grid/flex column count            |

Any key not in the explicit map is converted from camelCase to kebab-case
automatically (e.g. `"someProperty"` becomes `"some-property"`).

---

## ID uniqueness rules

- Every `id` in a page must be unique across all sections in that page.
- Template ids are placeholders -- when a template is copied into a page the
  section gets a generated `pb-{stem}` id for the manifest entry, but the
  internal block `id` values come directly from the template JSON.
- IDs are used by `overrides.json` for live-edit persistence and by
  `data-pb-id` attributes in the built HTML.

---

## page.json manifest format

Each page directory contains a `page.json` that controls section order:

```json
{
    "title": "My Page",
    "sections": [
        { "id": "pb-header",      "file": "header.json",        "type": "header",  "label": "Header" },
        { "id": "pb-sec-abc123",  "file": "section-abc123.json","type": "section", "label": "Hero" },
        { "id": "pb-footer",      "file": "footer.json",        "type": "footer",  "label": "Footer" }
    ]
}
```

- `id` -- stable identifier for drag-and-drop ordering and override targeting
- `file` -- the actual filename inside the page directory
- `type` -- `header`, `footer`, `section`, or `panel`
- `label` -- display name shown in the composer canvas

`build.php` reads this file to determine what sections to include and in
what order. If `page.json` is absent it falls back to globbing
`section-*.json` files with `header.json` at top and `footer.json` at bottom.

---

## File validation checklist

Before a section JSON can be used it must pass these checks:

- Valid JSON (no trailing commas, no comments)
- `"type"` is one of: `header`, `footer`, `section`
- All `id` values inside `blocks` are non-empty strings
- All `settings` values are strings (numbers must be quoted with their unit,
  e.g. `"16px"` not `16`)
- `"href"` values are present on every `button` block
- `"tag"` is present on every `heading` block
- `"children"` on `card` blocks is a non-empty array

---

## Project color palette (from style-context.txt)

These are the project-level design tokens. Use them as defaults when the AI
is creating new sections for this project:

- primary:    `#2d1c6e`
- secondary:  `#eceaf6`
- accent:     `#0c071c`
- text:       `#1b1825`
- background: `#fcfcfd`

For dark-mode sections (the default template style), these are the values
currently used across all existing templates:

- section bg (dark):  `#0a0a14` / `#0d0d18` / `#111122`
- text (primary):     `#e8e8f0`
- text (secondary):   `#c7c7f0`
- text (muted):       `#8888a0` / `#a5a5c0`
- text (faint):       `#5555a0`
- accent color:       `#6366f1`
- accent border:      `rgba(99,102,241,0.2)` / `rgba(99,102,241,0.5)`
- card bg:            `#111122`
- card border:        `rgba(255,255,255,0.07)`

---

## Where to place new template files

```
page-builder/
  sections/
    headers/     <-- header templates
    footers/     <-- footer templates
    sections/    <-- section templates
    panels/      <-- panel/two-column templates
```

Filename convention: `kebab-case.json`, e.g. `pricing-table.json`.

The section library API (`section-library.php?action=list`) auto-discovers
all files in these directories. No registration step is needed -- just drop
the file in the right folder.
