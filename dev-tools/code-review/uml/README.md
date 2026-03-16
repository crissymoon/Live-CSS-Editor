# UML Renderer

Converts JSON-defined UML diagrams into rendered images using the
`xcm render_core` graphics API via a Julia front-end.

## Diagram types

| Type | `"type"` value |
|---|---|
| Class diagram | `"class_diagram"` |
| Sequence diagram | `"sequence_diagram"` |

## Folder layout

```
uml/
  README.md
  schema/            -- JSON schema docs for each diagram type
  examples/          -- ready-to-run example diagrams
  render/            -- Julia renderer (xcm binding + scene builder + CLI)
    Project.toml     -- Julia package manifest (JSON.jl dependency)
    render_diagram.jl
    xcm_binding.jl
    scene_builder.jl
  out/               -- generated images (gitignored)
```

## Requirements

- Julia 1.9 or later (`julia` on PATH)
- `render_core` shared library built from the `render_eng` repo
  - The renderer auto-discovers the library if `render_eng/` is a sibling
    of your `dev_tools/` workspace folder, or set `XCM_LIB_PATH` to the
    full path of `render_core.dll / .dylib / .so`.

## First-time setup

```powershell
julia render/render_diagram.jl --setup
# or manually in the render/ directory:
julia -e 'import Pkg; Pkg.activate("."); Pkg.instantiate()'
```

## Rendering a diagram

```powershell
# Render to out/ folder (PPM image)
julia render/render_diagram.jl --input examples/auth_service_class.json

# Override output path and viewport
julia render/render_diagram.jl \
    --input  examples/login_flow_sequence.json \
    --output out/login.ppm \
    --width  1400 \
    --height 900

# Force a specific library path
julia render/render_diagram.jl \
    --input examples/auth_service_class.json \
    --lib   C:/render_eng/build/native-vcpkg/Release/render_core.dll

# Emit Scene JSON only (no render) for debugging
julia render/render_diagram.jl --input examples/auth_service_class.json --dry-run

# Force Python bindings backend (uses bindings/python)
julia render/render_diagram.jl --input examples/auth_service_class.json --backend python

# Auto backend (default): native first, Python fallback if native load fails
julia render/render_diagram.jl --input examples/auth_service_class.json --backend auto
```

The Python backend calls `bindings/python/src/xcm_render_core` and renders
Scene JSON via `xcm_render_scene_json`, then writes `.ppm` (or `.png` when Pillow is available).

## Browser Viewer CLI

The UML folder includes a dedicated CLI that renders JSON, emits a responsive
browser viewer HTML, and opens it automatically.

```powershell
# from code-review/uml
python uml_cli.py examples/xcm_styles_usage_sequence.json

# windows convenience launcher
uml.cmd examples/xcm_styles_integration_class.json

# disable auto-open
python uml_cli.py examples/auth_service_class.json --no-open
```

Generated artifacts:

- `<name>.ppm` (renderer output)
- `<name>.scene.json` (scene command output)
- `<name>.viewer.html` (responsive browser viewer)

## PPM to PNG

PPM is the default output format (no extra dependencies).
Convert via ImageMagick:

```bash
magick out/auth_service_class.ppm out/auth_service_class.png
```

Or, if `Images.jl` and `FileIO.jl` are available in the Julia environment,
pass `--output foo.png` and the renderer uses those packages automatically.

## Adding diagrams

Place a `.json` file in `uml/` or any subdirectory.  It must have:

```json
{ "version": 1, "type": "<diagram_type>", ... }
```

See `schema/` for the full field reference and `examples/` for working files.

## Environment variables

| Variable | Purpose |
|---|---|
| `XCM_LIB_PATH` | Full path to `render_core` shared library |
| `RENDER_ENG_ROOT` | Root of the `render_eng` repo (used for lib auto-discovery) |
