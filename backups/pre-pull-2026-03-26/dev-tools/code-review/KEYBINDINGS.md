# Code Review TUI - Keyboard Shortcuts

## Editor

Note: On macOS, `Cmd` is treated as `Ctrl` for most editor shortcuts.

### Platform Notes

| Platform | Shortcut mapping |
|---|---|
| macOS | `Cmd` works as the main control modifier for editor actions |
| Linux / Windows | Use `Ctrl` for the same actions |
| Turbo movement | `Cmd+Shift+I/J/K/L` on macOS, `Ctrl+Shift+I/J/K/L` on Linux/Windows |

### File / Tabs

| Keys | Action |
|---|---|
| Cmd+Left / Cmd+Right | Switch to previous / next tab |
| Cmd+T | Open a new empty tab |
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
| Ctrl+Up / Ctrl+Down | Jump 8 lines |
| Left / Right | Move one character (crosses line boundary at edges) |
| Ctrl+Left / Ctrl+Right | Jump by word |
| Cmd+Shift+I / Cmd+Shift+K | Turbo move up/down 8 lines |
| Cmd+Shift+J / Cmd+Shift+L | Turbo word-jump left/right |
| Home | Smart home: first non-whitespace, then column 1 |
| Ctrl+Home | Jump to start of file (line 1, column 1) |
| End | End of current line |
| Ctrl+End | Jump to end of file |
| PageUp / PageDown | Move one visible page |

### Selection Movement

| Keys | Action |
|---|---|
| Shift+Up / Shift+Down | Extend selection by one line |
| Ctrl+Shift+Up / Ctrl+Shift+Down | Extend selection by 8 lines |
| Shift+Left / Shift+Right | Extend selection by one character |
| Ctrl+Shift+Left / Ctrl+Shift+Right | Extend selection by word |
| Shift+Home | Extend selection with smart home |
| Shift+End | Extend selection to end of line |

### Mouse

| Action | Effect |
|---|---|
| Click in content area | Move cursor to clicked position |
| Click and drag in content area | Select text |
| Click tab label | Switch to that tab |
| Click tab X button | Close that tab |
| Right-click in content | Context menu: Select All / Copy / Cut / Paste |
| Scroll wheel | Scroll content |

---

## File Browser

### Keyboard

| Keys | Action |
|---|---|
| Up / Down | Move browser selection |
| Left | Collapse selected folder if open, otherwise go up one directory |
| Right | Expand selected folder |
| Return / Enter | Toggle selected folder, or open selected file |
| / | Open fuzzy finder input |
| Ctrl+P | Open fuzzy finder and focus browser |
| Escape | Leave browser keyboard focus |

### Mouse

| Action | Effect |
|---|---|
| Click root bar | Go up one directory |
| Click a recent path (R) | Set browser root and scan path to that location |
| Click a folder | Expand / collapse it |
| Click a file | Open file in editor |
| Right-click any entry | Context menu: Edit file / Copy path / Set as scan path |
| Scroll wheel | Scroll browser |

### Notes

| Item | Behavior |
|---|---|
| Recent paths | Last used working paths are shown at the top of the browser list |
| Recent-path storage | Saved in dev-tools/code-review/.browser_recent_paths |
| Fuzzy finder matching | Case-insensitive subsequence match across name/path |
| Fuzzy match highlight | Matching characters are rendered brighter in the results list |
| Fuzzy finder behavior | While open: type to filter, Backspace to edit, Enter to open selected result, Escape to clear/close |

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
| Click Orphans | Run orphaned code scan |
| Click C Safe | Run C memory safety scan |
| Click Terminal | Toggle integrated terminal panel |
| Click > All | Run all scans |

---

## Terminal

### Keyboard

| Keys | Action |
|---|---|
| Return / Enter | Run current command in active tab |
| Ctrl+T | New terminal tab |
| Ctrl+W | Close current terminal tab |
| Ctrl+Left / Ctrl+Right | Switch terminal tab |
| Up / Down | Command history navigation |
| PageUp / PageDown | Scroll terminal output |
| Escape | Hide terminal panel |

### Mouse

| Action | Effect |
|---|---|
| Click terminal tab | Activate tab |
| Click + | Open a new terminal tab |
| Click Hide | Close terminal panel (state preserved) |
| Scroll wheel | Scroll terminal output |

### Notes

| Item | Behavior |
|---|---|
| Per-tab state | Each terminal tab keeps its own cwd, history, and output |
| Persistence file | Saved in dev-tools/code-review/.terminal_state.lua |
| Restore on reopen | Tabs and panel visibility are restored on app start |

---

## Global

| Keys | Action |
|---|---|
| Cmd+Q | Quit |
| Escape | Close menu / modal |
