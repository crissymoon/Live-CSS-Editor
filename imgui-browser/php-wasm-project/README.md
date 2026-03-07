# 🐘 PHP × WebAssembly

Run a **full PHP 8.3 interpreter entirely in the browser** via WebAssembly.
No server required — PHP scripts execute client-side inside a sandboxed WASM runtime.

---

## Table of Contents

1. [How it works](#how-it-works)
2. [Project structure](#project-structure)
3. [Prerequisites](#prerequisites)
4. [Quick start (Docker — recommended)](#quick-start-docker--recommended)
5. [Quick start (local Emscripten)](#quick-start-local-emscripten)
6. [Build options](#build-options)
7. [Running the demo](#running-the-demo)
8. [Adding your own PHP code](#adding-your-own-php-code)
9. [JS API reference](#js-api-reference)
10. [Architecture decisions](#architecture-decisions)
11. [Troubleshooting](#troubleshooting)
12. [Contributing](#contributing)

---

## How it works

```
Browser
  │
  ├── public/index.html      ← UI shell (URL bar, output tabs)
  ├── public/ui.js           ← DOM event wiring
  └── public/php-wasm.js     ← JS ↔ WASM bridge (phpExec)
                                     │
                              wasm/php.js  (Emscripten glue)
                              wasm/php.wasm (PHP interpreter + VFS)
                                     │
                         ┌───────────┴────────────┐
                         │   php_wasm_shim.c       │
                         │   (C SAPI wrapper)      │
                         └───────────┬────────────┘
                                     │
                              libphp (static)
                              compiled with emcc
```

1. **Emscripten** compiles the PHP interpreter (`php-src/`) into a static library `libphp.a`.
2. `src/php_wasm_shim.c` wraps the PHP embed SAPI into a handful of exported C functions.
3. `emcc` links the shim + library into `wasm/php.js` + `wasm/php.wasm`.
4. The PHP source files in `src/` are baked into the WASM virtual filesystem at compile time.
5. In the browser, `public/php-wasm.js` calls `PHP.ccall()` to simulate HTTP requests, then hands the HTML/JSON response back to `ui.js` for display.

---

## Project structure

```
php-wasm-project/
│
├── build.sh                  ← Top-level build orchestrator
├── Makefile                  ← Convenience make targets
├── .gitignore
│
├── src/                      ← PHP application (embedded into WASM VFS)
│   ├── index.php             ← Entry-point router
│   ├── bootstrap.php         ← Runtime init (error reporting, autoloader)
│   ├── php_wasm_shim.c       ← C shim / custom SAPI
│   ├── lib/
│   │   ├── router.php        ← Minimal HTTP router class
│   │   └── renderer.php      ← Template renderer + e() helper
│   └── views/
│       └── home.php          ← Home page template
│
├── public/                   ← Static files served to the browser
│   ├── index.html            ← Demo UI shell
│   ├── style.css             ← UI styles
│   ├── php-wasm.js           ← WASM bridge module (import me)
│   └── ui.js                 ← DOM controller
│
├── wasm/                     ← Build output (git-ignored)
│   ├── php.js                ← Emscripten JS glue (generated)
│   └── php.wasm              ← The PHP interpreter (generated)
│
├── scripts/
│   ├── fetch-php.sh          ← Download PHP source tarball
│   ├── apply-patches.sh      ← Apply patches/ to php-src/
│   ├── configure-php.sh      ← emconfigure ./configure
│   └── link-wasm.sh          ← emcc link step
│
├── patches/                  ← Unified-diff patches for PHP source
│   └── README.md
│
└── docker/
    ├── Dockerfile            ← Build environment image
    └── docker-compose.yml    ← Compose shortcuts
```

---

## Prerequisites

### For the Docker build (recommended)

| Tool | Version |
|------|---------|
| Docker | 20+ |
| docker compose | v2 |

### For a local build

| Tool | Version | Install |
|------|---------|---------|
| Emscripten SDK (emsdk) | 3.1.56 | [emsdk docs](https://emscripten.org/docs/getting_started/downloads.html) |
| autoconf | 2.69+ | `brew install autoconf` / `apt install autoconf` |
| bison | 3.0+ | `brew install bison` / `apt install bison` |
| re2c | 1.0+ | `brew install re2c` / `apt install re2c` |
| Python 3 | 3.8+ | For the dev server |

---

## Quick start (Docker — recommended)

```bash
# 1. Clone / enter the project
cd php-wasm-project

# 2. Build PHP as WebAssembly (takes ~10–20 min on first run)
make docker-build

# 3. Start the local dev server
make serve
# → open http://localhost:8080
```

That's it. Docker handles all toolchain dependencies.

---

## Quick start (local Emscripten)

```bash
# 1. Activate the Emscripten SDK
source /path/to/emsdk/emsdk_env.sh

# 2. Build
./build.sh

# 3. Serve
make serve
```

---

## Build options

```bash
./build.sh --help

# Build a specific PHP version
./build.sh --php-version 8.2.18

# Skip re-downloading sources (if you already have php-src/)
./build.sh --skip-fetch

# Skip configure + make (re-link only — fastest iteration)
./build.sh --skip-fetch --skip-configure --skip-make

# Full clean rebuild
./build.sh --clean
```

### Make targets

| Target | Description |
|--------|-------------|
| `make build` | Full build (emcc on PATH) |
| `make docker-build` | Full build inside Docker |
| `make serve` | Dev server on `localhost:8080` |
| `make clean` | Remove `wasm/php.js` + `wasm/php.wasm` |
| `make clean-all` | Remove build output + `php-src/` |
| `make fetch` | Download PHP sources only |
| `make configure` | Run `emconfigure ./configure` only |
| `make link` | Re-run `emcc` link step only |

---

## Running the demo

After `make serve`, open **http://localhost:8080** in your browser.

The UI provides:

- **Path** input — type any route (`/`, `/json`, `/info`) and press **Run**.
- **Method** selector — switch to POST to send a request body.
- **Rendered** tab — renders PHP HTML output in a sandboxed iframe.
- **Raw output** tab — shows the raw text returned by PHP.
- **Headers** tab — shows the HTTP response headers PHP sent.

---

## Adding your own PHP code

1. Edit files in `src/` — add routes in `src/index.php`, add view templates in `src/views/`.
2. Re-run the **link step only** (no recompile needed for PHP code changes):

```bash
make link          # fast — just re-embeds the PHP VFS
# or
./build.sh --skip-fetch --skip-configure --skip-make
```

3. Reload the page.

> **Why is a re-link needed?** PHP source files are baked into the WASM binary's virtual filesystem at link time via Emscripten's `--embed-file` flag. Changing them requires a fresh `emcc` invocation, but not a PHP recompile.

---

## JS API reference

Import `phpExec` from `public/php-wasm.js`:

```js
import { phpExec } from './php-wasm.js';

const response = await phpExec({
    uri:    '/json',        // path  (required)
    method: 'GET',          // "GET" | "POST"  (default "GET")
    body:   '',             // raw POST body   (default "")
    headers: {},            // extra request headers
});

console.log(response.status);   // 200
console.log(response.body);     // JSON string
console.log(response.headers);  // { "Content-Type": "application/json" }
console.log(response.exitCode); // 0
```

### `phpExec(request)` → `Promise<PHPResponse>`

| Field | Type | Description |
|-------|------|-------------|
| `request.uri` | `string` | URL path, e.g. `/json` |
| `request.method` | `string` | `GET` or `POST` |
| `request.body` | `string` | Raw POST body |
| `request.headers` | `object` | Extra `HTTP_*` headers |

| Response field | Type | Description |
|----------------|------|-------------|
| `status` | `number` | HTTP status code |
| `headers` | `object` | Parsed response headers |
| `body` | `string` | Raw PHP output |
| `exitCode` | `number` | WASM exit code (0 = success) |

---

## Architecture decisions

### Why a custom C SAPI instead of the CLI SAPI?

The CLI SAPI writes to stdout/stderr. In Emscripten those map to the terminal
(Node) or nowhere (browser). A custom SAPI (`php_wasm_shim.c`) intercepts
`ub_write`, `send_header`, and `read_post` so the browser JS layer can capture
them without DOM tricks.

### Why embed PHP files at link time?

Emscripten's `--embed-file` bakes files into the WASM binary's linear memory
before the module runs. This avoids async file fetching from within PHP, which
the embed SAPI does not support out of the box.

### Why `ALLOW_MEMORY_GROWTH`?

PHP allocates heap memory dynamically. A fixed 64 MB initial heap is enough to
boot but real scripts may need more. `ALLOW_MEMORY_GROWTH=1` lets the WASM
linear memory expand (up to 512 MB) transparently.

---

## Troubleshooting

### `emcc: command not found`

Activate the Emscripten SDK: `source /path/to/emsdk/emsdk_env.sh`
Or use `make docker-build`.

### Build fails with `undefined reference to ...`

Ensure the PHP configure step completed without errors.
Check `/tmp/php-configure.log` and `/tmp/php-make.log`.

### Browser shows "PHP WASM not loaded — run build.sh first"

`wasm/php.js` and `wasm/php.wasm` have not been generated yet.
Run `make build` or `make docker-build`.

### `SharedArrayBuffer` errors in browser console

Some browsers require cross-origin isolation headers for `SharedArrayBuffer`.
Add these headers to your server:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

The Python dev server (`make serve`) does not send these by default.
Use `scripts/serve-coop.py` (if provided) or a proper static server.

### Out of memory (WASM trap)

Increase `INITIAL_MEMORY` or `MAXIMUM_MEMORY` in `scripts/link-wasm.sh`.

---

## Contributing

1. Fork the repo.
2. Create a feature branch: `git checkout -b feat/my-feature`.
3. Add patches to `patches/` if you need to modify PHP source.
4. Open a PR with a description of what changed and why.

---

## License

MIT — see [LICENSE](LICENSE) for details.
