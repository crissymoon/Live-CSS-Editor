# VSCode Copilot Prompt - Create Page Builder Sections

Paste the block below into Copilot Chat (Agent mode recommended).
Replace the bracketed placeholders before sending.


---


## READY-TO-PASTE PROMPT

```
I am working on a PHP page builder system called Crissy's Style Tool.

PROJECT ROOT: /Users/mac/Documents/live-css

CONTEXT FILE TO READ FIRST:
Read the file at vscode-bridge/context/section-schema.md before doing anything
else. It contains the complete JSON schema, all block types, settings key
mappings, file placement rules, and the project color palette.

MY REQUEST:
[Describe the section you want here. Examples:
  - "Create a pricing table section with three tiers: Free, Pro, and Enterprise"
  - "Create a testimonials section with three quote cards in a row"
  - "Create a dark hero section for a SaaS product with a headline, sub-copy,
     and two buttons side by side (primary and ghost style)"
  - "Create a centered contact form section with name, email, message fields
     and a submit button"]

SECTION TYPE: [header / footer / section / panel]

SAVE LOCATION: page-builder/sections/[headers|footers|sections|panels]/[name].json

REQUIREMENTS:
- Output valid JSON only, no comments inside the JSON
- All numeric values must be strings with units, e.g. "16px" not 16
- Every block must have a unique "id" value (use short descriptive slugs)
- Every heading block must have a "tag" key
- Every button block must have an "href" key
- Every card block must have a non-empty "children" array
- Include _meta at the top with name, type, and description
- Use the dark-mode color palette from section-schema.md unless I specify
  otherwise (bg: #0a0a14 or #0d0d18, text: #e8e8f0, muted: #8888a0,
  accent: #6366f1)
- After writing the file, verify it passes JSON.parse by mentally checking
  for trailing commas or missing quotes

CONSOLE ERROR HANDLING NOTE:
This project requires all JS fetch calls that interact with section files to
have .catch(err => console.error('[section] error:', err)) fallbacks.
If you are also writing any companion JS (e.g. to wire up the section in the
composer), include that fallback on every fetch.

After creating the file, tell me:
1. The full path where you saved it
2. A brief summary of the blocks inside it
3. Any IDs I should know about for targeting with overrides.json later
```


---


## QUICK VARIANT - Add a section to a specific page

Use this when you want to add a section directly into a live page (not as a
reusable template).

```
I am working on the Crissy Style Tool page builder.

PROJECT ROOT: /Users/mac/Documents/live-css

READ FIRST: vscode-bridge/context/section-schema.md

PAGE NAME: [page-folder-name, e.g. "demo"]
PAGE DIRECTORY: page-builder/pages/[page-name]/

TASK:
1. Read page-builder/pages/[page-name]/page.json to see the current section
   manifest for this page.

2. Create a new section JSON file in that page directory. File naming:
   section-[short-slug].json

3. Add the new section to page.json's "sections" array. Insert it before the
   footer entry. Use this shape for the new manifest entry:
   { "id": "pb-[short-slug]", "file": "section-[short-slug].json",
     "type": "section", "label": "[Label]" }

4. Do NOT include "_meta" in page-specific section files. It is only for
   template files under sections/.

THE SECTION I WANT:
[Describe the section]

Use the project dark palette (bg #0a0a14, text #e8e8f0, accent #6366f1).
All block ids must be unique and not conflict with ids already in the page.

After finishing, list:
- The new file path
- The updated page.json sections array
- All block ids used (so I can add overrides later if needed)
```


---


## QUICK VARIANT - Review an existing section

```
I am working on the Crissy Style Tool page builder.

PROJECT ROOT: /Users/mac/Documents/live-css

READ FIRST: vscode-bridge/context/section-schema.md

TASK: Review the section file at:
page-builder/[sections/type/name.json OR pages/page-name/file.json]

Check for:
1. Valid JSON structure (no trailing commas, all strings quoted)
2. All required keys present per schema (type, id, layout, blocks for sections;
   type, settings, brand, nav, navSettings for headers; etc.)
3. All blocks have unique ids
4. All heading blocks have "tag"
5. All button blocks have "href"
6. All numeric values are quoted strings with units
7. _meta present if this is a template file; absent if page-specific

Report each issue with the JSON path where it occurs and the fix needed.
If no issues found, confirm the file is valid.
```


---


## QUICK VARIANT - Convert a Crissy Style Tool stylesheet into section defaults

Use this when you have a stylesheet open in the style tool and want a new
section template that inherits its colors and typography.

```
I am working on the Crissy Style Tool page builder.

PROJECT ROOT: /Users/mac/Documents/live-css

READ FIRST: vscode-bridge/context/section-schema.md

ALSO READ:
- style-sheets/[stylesheet-name].css  (the stylesheet to extract from)
- style-context.txt  (project-level color tokens)

TASK:
Extract the following from the stylesheet and use them to build a new section
template:

1. Background color  ->  sections: settings.bg
2. Primary text color  ->  heading blocks: settings.color
3. Secondary/muted text  ->  text blocks: settings.color
4. Accent/primary color  ->  button bg, heading accent color
5. Border color / card bg  ->  card settings.bg and settings.border
6. Font size scale  ->  heading and text block settings.fontSize
7. Border radius  ->  button and card settings.borderRadius

SECTION TO CREATE: [describe the section]
SECTION TYPE: [section / header / footer / panel]
SAVE TO: page-builder/sections/[type-folder]/[name].json

Output the JSON file with _meta included. After saving, list which CSS
variables or rules you pulled each color from.
```


---


## QUICK VARIANT - Build or extend the Crissy Style Tool web page design

Use this when working on the main style tool landing/web page, not the Tauri
app. The full design brief, asset paths, HTML skeleton, and CSS targets are
in the design context file below.

```
I am working on the Crissy Style Tool web page design.

PROJECT ROOT: /Users/mac/Documents/live-css

READ FIRST (in this order):
1. vscode-bridge/context/design-crissy-style-tool-page.md  -- full design spec
2. vscode-bridge/context/section-schema.md                 -- section JSON schema
3. style-sheets/xcm-contact.css                            -- current page CSS

TASK:
[Describe what you want to build or change.  Examples:
  - "Build the oversized header section as a page-builder header JSON"
  - "Build the oversized footer section as a page-builder footer JSON"
  - "Update the header CSS so the logo sits to the left of the wordmark
     on the same line with a 16px gap"
  - "Add a mid section hero with a headline and a CTA button"
  - "Mark the header checklist item as done in the design context file"
  - "Save the current design as a project called style-tool-landing"]

REQUIREMENTS:
- Follow the asset paths and CSS class names in design-crissy-style-tool-page.md
  exactly
- Every image element must have an onerror handler that calls console.error
  with the asset src so missing files are visible in the console
- Every fetch call must have a .catch(err => console.error('[design]', err))
- Do not add a traditional nav to the header; leave it empty until the spec says
  otherwise
- Use the project dark palette from section-schema.md unless the design spec
  says otherwise
- Prefer save_project to push the full design (HTML + CSS + JS) into the app.
  This writes to SQLite, the browser auto-loads it within 4 seconds if bridge
  sync is ON, and the user can also click Load to browse saved projects.
  Only use push_html_content / push_js_content / update_stylesheet when
  updating a single editor in isolation.
- After any file write, list:
  1. The file path written
  2. Any CSS class names or block ids I should know about
  3. Any checklist items in design-crissy-style-tool-page.md I should mark done
```


---


## Notes for best results

- Copilot Agent mode lets it read and write files directly. Prefer Agent mode
  over Ask mode for these tasks.

- If Copilot writes a file to disk, the section library picks it up immediately
  on the next ?action=list call - no restart needed.

- To test a newly created section, open the composer:
  http://localhost:8080/page-builder/composer.php?page=[page-name]
  The section should appear in the library panel under its type filter tab.

- If the section does not appear in the library, check:
  1. The file is in the correct subdirectory under page-builder/sections/
  2. The JSON is valid (open it in browser at
     http://localhost:8080/page-builder/section-library.php?action=get&path=
     and append the relative path)
  3. The "type" field matches the subdirectory name (headers/, footers/,
     sections/, panels/)

- To debug fetch errors in the composer, open browser DevTools console.
  All API calls log errors to console.error('[section-api]...') or
  '[section-library]...' prefixes.
