#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import json
import subprocess
import sys
import textwrap
import webbrowser
from pathlib import Path


def _script_dir() -> Path:
  script_path = Path(__file__)
  if script_path.is_absolute():
    return script_path.parent
  return (Path.cwd() / script_path).parent


UML_ROOT = _script_dir()
RENDER_SCRIPT = UML_ROOT / "render" / "render_diagram.jl"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Render UML JSON and open a responsive browser viewer.",
    )
    parser.add_argument("input", help="Path to UML JSON file")
    parser.add_argument("--output-dir", default=str(UML_ROOT / "out"), help="Output directory")
    parser.add_argument("--backend", default="auto", choices=["auto", "native", "python"], help="Renderer backend")
    parser.add_argument("--width", type=int, default=None, help="Override viewport width")
    parser.add_argument("--height", type=int, default=None, help="Override viewport height")
    parser.add_argument("--lib", default="", help="Optional path to render_core shared library")
    parser.add_argument("--no-open", action="store_true", help="Do not open browser automatically")
    return parser.parse_args()


def run_cmd(cmd: list[str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        text=True,
        capture_output=True,
        check=False,
    )


def build_viewer_html(ppm_base64: str, title: str, command_count: int | None) -> str:
    escaped_title = json.dumps(title)
    escaped_ppm = json.dumps(ppm_base64)
    escaped_command_count = json.dumps(command_count if command_count is not None else "-")
    return textwrap.dedent(
        f"""\
        <!doctype html>
        <html lang=\"en\">
        <head>
          <meta charset=\"utf-8\" />
          <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />
          <title>{title} - UML Viewer</title>
          <style>
            :root {{
              /* xcm-style-tokens: spacing */
              --sp1: 4px; --sp2: 8px; --sp3: 12px; --sp4: 16px; --sp5: 24px;
              --sp-fluid: clamp(12px, 1.2vw + 8px, 22px);
              /* xcm-style-tokens: line-height */
              --lh-tight: 1.2; --lh-normal: 1.45;
              /* xcm-style-tokens: type */
              --font-ui: "Inter", "Segoe UI", Helvetica, Arial, sans-serif;
              --font-body: "Noto Sans", "Segoe UI", Helvetica, Arial, sans-serif;
              --font-mono: "JetBrains Mono", Consolas, "Courier New", monospace;
              --fs-body: clamp(0.88rem, 0.18vw + 0.82rem, 0.97rem);
              --fs-title: clamp(1rem, 1.1vw + 0.7rem, 1.22rem);
              /* palette */
              --bg: #080f1c;
              --surface: #0e1828;
              --surface-hi: #162034;
              --border: #243452;
              --border-hi: #3b5280;
              --ink: #dde7f8;
              --ink-dim: #8da0be;
              --accent: #4e7fd5;
            }}
            *, *::before, *::after {{ box-sizing: border-box; margin: 0; padding: 0; }}
            html {{
              height: 100%;
              overflow-x: hidden;
            }}
            body {{
              height: 100%;
              overflow-x: hidden;
              font-family: var(--font-ui);
              font-size: var(--fs-body);
              line-height: var(--lh-normal);
              color: var(--ink);
              background:
                radial-gradient(ellipse 900px 420px at 12% -8%, #172646 0%, transparent 58%),
                radial-gradient(ellipse 600px 380px at 88% 2%, #1c2f48 0%, transparent 58%),
                linear-gradient(164deg, #060d18 0%, #080f1c 52%, #070c17 100%);
              display: flex;
              flex-direction: column;
            }}
            header {{
              flex: 0 0 auto;
              padding: var(--sp3) var(--sp4);
              border-bottom: 1px solid var(--border);
              background: rgba(14, 24, 40, 0.92);
              backdrop-filter: blur(8px);
              -webkit-backdrop-filter: blur(8px);
              display: flex;
              align-items: center;
              justify-content: space-between;
              gap: var(--sp3);
              flex-wrap: wrap;
            }}
            .hdr-left {{ display: flex; flex-direction: column; gap: 3px; }}
            h1 {{
              font-family: var(--font-ui);
              font-size: var(--fs-title);
              font-weight: 600;
              letter-spacing: 0.015em;
              color: var(--ink);
            }}
            .meta {{
              font-family: var(--font-mono);
              font-size: 0.78rem;
              color: var(--ink-dim);
              letter-spacing: 0.01em;
            }}
            .controls {{ display: flex; gap: var(--sp2); align-items: center; }}
            button {{
              font-family: var(--font-ui);
              font-size: 0.82rem;
              font-weight: 500;
              border: 1px solid var(--border-hi);
              background: linear-gradient(180deg, var(--surface-hi), var(--surface));
              color: var(--ink);
              border-radius: 7px;
              padding: 6px 14px;
              cursor: pointer;
              transition: border-color 0.15s, background 0.15s;
            }}
            button:hover {{
              border-color: var(--accent);
              background: linear-gradient(180deg, #1c2e4a, #162034);
            }}
            main {{
              flex: 1 1 0;
              min-height: 0;
              padding: var(--sp-fluid);
              display: flex;
              align-items: flex-start;
              justify-content: center;
            }}
            .stage {{
              width: min(calc(100vw - 2 * var(--sp-fluid)), 1680px);
              height: min(calc(100vh - 90px), 940px);
              border: 1px solid var(--border);
              border-radius: 12px;
              background: linear-gradient(178deg, var(--surface-hi) 0%, var(--surface) 100%);
              box-shadow: 0 20px 44px rgba(0, 0, 0, 0.5), inset 0 1px 0 rgba(255,255,255,0.04);
              overflow: auto;
              position: relative;
              padding: var(--sp3);
            }}
            /* Inner centering wrapper — only takes up as much space as the canvas needs */
            .canvas-wrap {{
              display: inline-flex;
              min-width: 100%;
              min-height: 100%;
              align-items: center;
              justify-content: center;
            }}
            canvas {{
              display: block;
              border-radius: 8px;
              box-shadow: 0 12px 28px rgba(0, 0, 0, 0.4);
              image-rendering: auto;
            }}
            .hint {{
              position: absolute;
              bottom: var(--sp2);
              right: var(--sp3);
              color: var(--ink-dim);
              font-family: var(--font-mono);
              font-size: 0.72rem;
              pointer-events: none;
            }}
            @media (max-width: 640px) {{
              .stage {{ height: calc(100vh - 100px); }}
              .hint {{ display: none; }}
            }}
          </style>
        </head>
        <body>
          <header>
            <div class=\"hdr-left\">
              <h1 id=\"title\"></h1>
              <div class=\"meta\" id=\"meta\"></div>
            </div>
            <div class=\"controls\">
              <button id=\"fit\" type=\"button\">Fit</button>
              <button id=\"one\" type=\"button\">100 %</button>
            </div>
          </header>
          <main>
            <section class=\"stage\" id=\"stage\">
              <div class=\"canvas-wrap\" id=\"wrap\">
                <canvas id=\"canvas\" aria-label=\"Rendered UML Diagram\"></canvas>
              </div>
              <div class=\"hint\">Fit / 100 % to zoom</div>
            </section>
          </main>
          <script>
            const title = {escaped_title};
            const ppmBase64 = {escaped_ppm};
            const commandCount = {escaped_command_count};

            const canvas  = document.getElementById("canvas");
            const ctx     = canvas.getContext("2d");
            const titleEl = document.getElementById("title");
            const metaEl  = document.getElementById("meta");
            const fitBtn  = document.getElementById("fit");
            const oneBtn  = document.getElementById("one");
            const stage   = document.getElementById("stage");
            const wrap    = document.getElementById("wrap");

            titleEl.textContent = title;

            const src = document.createElement("canvas");
            let zoom = -1; // -1 = fit

            /* ---- PPM decoder ---- */
            function isWs(c) {{ return c === 9 || c === 10 || c === 13 || c === 32; }}

            function readToken(bytes, st) {{
              while (st.i < bytes.length) {{
                if (bytes[st.i] === 35) {{
                  while (st.i < bytes.length && bytes[st.i] !== 10) st.i++;
                }} else if (isWs(bytes[st.i])) {{
                  st.i++;
                }} else {{
                  break;
                }}
              }}
              const s = st.i;
              while (st.i < bytes.length && !isWs(bytes[st.i])) st.i++;
              return new TextDecoder().decode(bytes.slice(s, st.i));
            }}

            function decodePpm(b64) {{
              const bin = atob(b64);
              const bytes = new Uint8Array(bin.length);
              for (let i = 0; i < bin.length; i++) bytes[i] = bin.charCodeAt(i);

              const st = {{ i: 0 }};
              const magic = readToken(bytes, st);
              const w = +readToken(bytes, st);
              const h = +readToken(bytes, st);
              const mx = +readToken(bytes, st);
              if (magic !== "P6") throw new Error("Not a P6 PPM: " + magic);
              if (!isFinite(w) || !isFinite(h) || mx !== 255) throw new Error("Bad PPM header");
              while (st.i < bytes.length && isWs(bytes[st.i])) st.i++;

              const rgb = bytes.slice(st.i, st.i + w * h * 3);
              if (rgb.length !== w * h * 3) throw new Error("PPM payload truncated");

              const rgba = new Uint8ClampedArray(w * h * 4);
              for (let s = 0, d = 0; s < rgb.length; s += 3, d += 4) {{
                rgba[d] = rgb[s]; rgba[d+1] = rgb[s+1]; rgba[d+2] = rgb[s+2]; rgba[d+3] = 255;
              }}

              src.width = w; src.height = h;
              src.getContext("2d").putImageData(new ImageData(rgba, w, h), 0, 0);
              return {{ w, h }};
            }}

            /* ---- Canvas rendering ---- */
            function paint() {{
              if (!src.width || !src.height) return;
              const dpr = window.devicePixelRatio || 1;
              const sw = src.width, sh = src.height;
              const aw = Math.max(1, stage.clientWidth  - 24);
              const ah = Math.max(1, stage.clientHeight - 24);
              const fit = Math.min(aw / sw, ah / sh);
              const scale = zoom > 0 ? zoom : Math.min(1, fit);

              /* CSS (logical) size of the canvas element */
              const cssW = Math.max(1, Math.round(sw * scale));
              const cssH = Math.max(1, Math.round(sh * scale));

              /* Physical pixel size — scaled up for HiDPI screens */
              const physW = Math.round(cssW * dpr);
              const physH = Math.round(cssH * dpr);

              canvas.width  = physW;
              canvas.height = physH;
              canvas.style.width  = cssW + "px";
              canvas.style.height = cssH + "px";

              /* All drawing commands operate in logical (CSS) coordinates */
              ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
              ctx.clearRect(0, 0, cssW, cssH);

              /* Smooth only when the image visually compresses (scale < 1).
                 At 1:1 or zoom-in, disable smoothing so pixels stay crisp.
                 Compare logical/CSS scale, not physical, so DPR does not affect the decision. */
              const needsSmooth = scale < 0.999;
              ctx.imageSmoothingEnabled = needsSmooth;
              if (needsSmooth) ctx.imageSmoothingQuality = "high";
              ctx.drawImage(src, 0, 0, cssW, cssH);

              /* wrap drives internal scrolling for 100 % mode */
              if (zoom > 0) {{
                wrap.style.minWidth  = cssW + "px";
                wrap.style.minHeight = cssH + "px";
              }} else {{
                wrap.style.minWidth  = "";
                wrap.style.minHeight = "";
              }}
            }}

            fitBtn.addEventListener("click", () => {{ zoom = -1; paint(); }});
            oneBtn.addEventListener("click", () => {{ zoom =  1; paint(); }});
            window.addEventListener("resize", () => {{ requestAnimationFrame(paint); }});
            new ResizeObserver(() => requestAnimationFrame(paint)).observe(stage);

            /* ---- Boot ---- */
            requestAnimationFrame(() => {{
              try {{
                const dims = decodePpm(ppmBase64);
                metaEl.textContent =
                  dims.w + " x " + dims.h + " px  |  scene commands: " + commandCount;
                paint();
              }} catch (err) {{
                metaEl.textContent = String(err && err.message ? err.message : err);
              }}
            }});
          </script>
        </body>
        </html>
        """
    )


def main() -> int:
    args = parse_args()

    input_path = Path(args.input).expanduser()
    if not input_path.is_absolute():
      input_path = (Path.cwd() / input_path).absolute()
    if not input_path.is_file():
        raise FileNotFoundError(f"Input JSON not found: {input_path}")

    out_dir = Path(args.output_dir).expanduser()
    if not out_dir.is_absolute():
      out_dir = (Path.cwd() / out_dir).absolute()
    out_dir.mkdir(parents=True, exist_ok=True)

    stem = input_path.stem
    ppm_out = out_dir / f"{stem}.ppm"
    scene_out = out_dir / f"{stem}.scene.json"
    html_out = out_dir / f"{stem}.viewer.html"

    render_cmd = [
        "julia",
        str(RENDER_SCRIPT),
        "--input",
        str(input_path),
        "--output",
        str(ppm_out),
        "--backend",
        args.backend,
    ]
    if args.width:
        render_cmd.extend(["--width", str(args.width)])
    if args.height:
        render_cmd.extend(["--height", str(args.height)])
    if args.lib:
        render_cmd.extend(["--lib", args.lib])

    render_proc = run_cmd(render_cmd, cwd=UML_ROOT)
    if render_proc.returncode != 0:
        sys.stderr.write(render_proc.stdout)
        sys.stderr.write(render_proc.stderr)
        raise RuntimeError("Render command failed")

    scene_cmd = ["julia", str(RENDER_SCRIPT), "--input", str(input_path), "--dry-run"]
    if args.width:
        scene_cmd.extend(["--width", str(args.width)])
    if args.height:
        scene_cmd.extend(["--height", str(args.height)])

    scene_proc = run_cmd(scene_cmd, cwd=UML_ROOT)
    command_count: int | None = None
    if scene_proc.returncode == 0 and scene_proc.stdout.strip().startswith("{"):
        scene_out.write_text(scene_proc.stdout, encoding="utf-8")
        try:
            scene_data = json.loads(scene_proc.stdout)
            commands = scene_data.get("commands")
            if isinstance(commands, list):
                command_count = len(commands)
        except json.JSONDecodeError:
            command_count = None

    ppm_base64 = base64.b64encode(ppm_out.read_bytes()).decode("ascii")
    html_out.write_text(
        build_viewer_html(ppm_base64, input_path.stem, command_count),
        encoding="utf-8",
    )

    print(f"Rendered image: {ppm_out}")
    print(f"Viewer html:    {html_out}")
    if scene_out.exists():
        print(f"Scene json:     {scene_out}")

    if not args.no_open:
        webbrowser.open(html_out.absolute().as_uri())

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
