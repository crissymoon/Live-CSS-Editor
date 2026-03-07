# Code Review TUI - Keyboard Shortcuts

## Editor

### File / Tabs

| Keys | Action |
|---|---|
| Cmd+Left / Cmd+Right | Switch to previous / next tab |
| Ctrl+S | Save current file |
| Ctrl+W | Close current tab |

### Editing

| Keys | Action |
|---|---|
| Type normally | Insert character at cursor |
| Return | New line, preserving indentation |
| Tab | Insert 4 spaces |
| Backspace | Delete character before cursor |
| Ctrl+Backspace | Delete word before cursor |
| Delete | Delete character after cursor |
| Ctrl+C | Copy selection (or current line if nothing selected) |
| Ctrl+X | Cut selection |
| Ctrl+V | Paste clipboard |
| Ctrl+A | Select all |
| Escape | Clear selection |

### Cursor Movement

| Keys | Action |
|---|---|
| Up / Down | Move one line |
| Shift+Up / Shift+Down | Jump 8 lines (turbo) |
| Left / Right | Move one character |
| Ctrl+Left / Ctrl+Right | Jump one word |
| Shift+Left / Shift+Right | Jump one word (turbo) |
| Home | Smart home: first non-whitespace, then column 1 |
| Shift+Home | Jump to start of file (line 1, column 1) |
| End | End of current line |
| Shift+End | Jump to end of file |
| PageUp / PageDown | Move one visible page |

### Mouse

| Action | Effect |
|---|---|
| Click in content area | Move cursor to clicked position |
| Click tab label | Switch to that tab |
| Click tab X button | Close that tab |
| Right-click in content | Context menu: Select All / Copy / Cut / Paste |
| Scroll wheel | Scroll content |

---

## File Browser

| Action | Effect |
|---|---|
| Click root bar | Go up one directory |
| Click a folder | Expand / collapse it |
| Click a file | Open file in editor |
| Right-click any entry | Context menu: Edit file / Copy path / Set as scan path |
| Scroll wheel | Scroll browser |

---

## Results Panel

| Action | Effect |
|---|---|
| Click and drag | Select result lines |
| Right-click | Context menu: Copy selection / Copy all / Clear results |
| Ctrl+C | Copy selected result lines |
| Scroll wheel | Scroll results |

---

## Toolbar / Scans

| Action | Effect |
|---|---|
| Click path box | Edit scan path |
| Return (in path box) | Confirm new scan path |
| Escape (in path box) | Cancel edit |
| Click Security | Run security scan |
| Click God Funcs | Run god functions scan |
| Click Lines | Run line count scan |
| Click Py Audit | Run Python audit |
| Click Smells | Run code smells scan |
| Click > All | Run all scans |

---

## Global

| Keys | Action |
|---|---|
| Cmd+Q | Quit |
| Escape | Close menu / modal |
