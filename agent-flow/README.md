# agent-flow

> STATUS: EXPERIMENTAL -- This folder is under active development.
> Features may change, break, or be removed without notice.
> Keep checking back for updates.

A drag-and-drop visual agent builder that runs on top of the moon-lang interpreter.

## What it does

You place nodes on a canvas, connect them with edges, and the system generates and
runs a `.moon` script via the moon binary. The workflow maps directly to moon-lang
primitives: prompts, AI calls (ava.anthropic / ava.openai), conditionals, loops,
memory (SQLite via ava.keep / ava.recall), and output.

## Node types

| Type       | Moon primitive         | Description                          |
|------------|------------------------|--------------------------------------|
| prompt     | variable assignment    | Set a text prompt or variable value  |
| ai-call    | ava.anthropic / ava.openai | Call an AI provider              |
| condition  | If ... then { }        | Branch on a value                    |
| loop       | loop thru i to N       | Repeat N times                       |
| memory     | ava.keep / ava.recall  | Persist or retrieve data             |
| tool       | shell / file ops       | Run a command or file operation      |
| output     | p(...)                 | Print / display a value              |

## Requirements

- PHP 8+
- moon binary built and available at `/Users/mac/Desktop/xcm-editor/moon-lang/moon`
  or on PATH as `moon`
- Write permissions on `flows/` directory (auto-created)

## Quick start

Serve this folder with PHP:

```bash
php -S localhost:9090 -t /Users/mac/Documents/live-css/agent-flow
```

Then open http://localhost:9090 in a browser.

## File layout

```
agent-flow/
  index.php          main drag-and-drop UI
  js/flow.js         canvas, node drag-drop, edge drawing, moon codegen
  css/flow.css       styles (dark, consistent with Live CSS Editor theme)
  api/
    validate.php     checks moon binary is available, returns JSON
    run.php          accepts flow JSON, generates .moon script, runs it, streams output
    save.php         saves named flow JSON to flows/
    load.php         loads named flow JSON from flows/
    list.php         lists saved flows
  flows/             saved flow JSON files (git-ignored if sensitive)
```

## How the codegen works

`flow.js` serializes the node graph to JSON. `api/run.php` receives that JSON,
walks the graph in topological order, and emits moon-lang source. The script is
written to a temp file and run with the moon binary. stdout and stderr are both
captured and returned so the UI can show them.

## Notes

- The moon binary path is resolved in `api/run.php` -- update `MOON_BIN` if yours
  is somewhere else.
- Provider keys must be configured in the moon-lang keys file (not stored here).
- This folder is safe to make public; it contains no secrets.
