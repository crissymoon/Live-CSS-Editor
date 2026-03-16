#!/usr/bin/env julia
# render_diagram.jl  --  CLI entry point for the UML renderer.
#
# Usage:
#   julia render_diagram.jl --input <file.json>
#   julia render_diagram.jl --input <file.json> --output <file.ppm>
#   julia render_diagram.jl --input <file.json> --width 1400 --height 900
#   julia render_diagram.jl --input <file.json> --lib /path/to/render_core.dll
#   julia render_diagram.jl --input <file.json> --dry-run   # emit Scene JSON only
#   julia render_diagram.jl --setup                         # install dependencies
#
# Output is written to uml/out/ by default, named after the input file.
# Default format: PPM (no dependencies).
# PNG: pass --output foo.png; requires Images.jl + FileIO.jl in the environment.

# ---------------------------------------------------------------------------
# Auto-activate the project so JSON.jl is available
# ---------------------------------------------------------------------------
import Pkg

const RENDER_DIR = dirname(abspath(@__FILE__))

let activated = false
    try
        Pkg.activate(RENDER_DIR; io=devnull)
        activated = true
    catch
    end
    if activated
        try Pkg.instantiate(; io=devnull) catch end
    end
end

# ---------------------------------------------------------------------------
# Dependency check / setup mode
# ---------------------------------------------------------------------------
if "--setup" in ARGS
    println("Setting up UML renderer dependencies...")
    Pkg.activate(RENDER_DIR)
    Pkg.add("JSON")
    println("Done.  Run diagrams with:")
    println("  julia render_diagram.jl --input <path/to/diagram.json>")
    exit(0)
end

try
    using JSON
catch
    @info "JSON.jl not found -- installing..."
    Pkg.activate(RENDER_DIR)
    Pkg.add("JSON")
    using JSON
end

include(joinpath(RENDER_DIR, "xcm_binding.jl"))
include(joinpath(RENDER_DIR, "scene_builder.jl"))

using .XcmBinding
using .SceneBuilder

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

struct Args
    input    :: Union{String, Nothing}
    output   :: Union{String, Nothing}
    width    :: Union{Int, Nothing}
    height   :: Union{Int, Nothing}
    lib      :: Union{String, Nothing}
    dry_run  :: Bool
end

function parse_args(argv) :: Args
    input   = nothing
    output  = nothing
    width   = nothing
    height  = nothing
    lib     = nothing
    dry_run = false

    i = 1
    while i <= length(argv)
        arg = argv[i]
        function next_val(flag)
            i >= length(argv) && error("$flag requires a value")
            argv[i + 1]
        end
        if     arg == "--input"  ;  input   = next_val(arg); i += 2
        elseif arg == "--output" ;  output  = next_val(arg); i += 2
        elseif arg == "--width"  ;  width   = parse(Int, next_val(arg)); i += 2
        elseif arg == "--height" ;  height  = parse(Int, next_val(arg)); i += 2
        elseif arg == "--lib"    ;  lib     = next_val(arg); i += 2
        elseif arg == "--dry-run";  dry_run = true; i += 1
        else
            @warn "Unknown argument: $arg"
            i += 1
        end
    end

    return Args(input, output, width, height, lib, dry_run)
end

# ---------------------------------------------------------------------------
# Image writers
# ---------------------------------------------------------------------------

function save_ppm(path::String, w::Int, h::Int, rgba::Vector{UInt8})
    header = Vector{UInt8}("P6\n$w $h\n255\n")
    rgb    = Vector{UInt8}(undef, w * h * 3)
    for p in 0:(w * h - 1)
        rgb[p*3+1] = rgba[p*4+1]
        rgb[p*3+2] = rgba[p*4+2]
        rgb[p*3+3] = rgba[p*4+3]
    end
    open(path, "w") do f
        write(f, header)
        write(f, rgb)
    end
end

function save_image(path::String, w::Int, h::Int, rgba::Vector{UInt8})
    if endswith(lowercase(path), ".png")
        png_ok = false
        try
            # Use Images + FileIO if available in the environment
            @eval begin
                using Images, FileIO
                img = reinterpret(reshape, Images.RGBA{Images.N0f8},
                                  reshape($rgba, 4, $w, $h))
                FileIO.save($path, collect(img'))
            end
            png_ok = true
        catch e
            @warn "PNG save failed ($(typeof(e))). Writing PPM instead. " *
                  "To enable PNG: julia -e 'import Pkg; Pkg.add([\"Images\",\"FileIO\",\"ImageIO\"])'"
        end
        png_ok && return
        path = replace(path, r"\.png$"i => ".ppm")
    end
    save_ppm(path, w, h, rgba)
end

# ---------------------------------------------------------------------------
# Derive output path from input path
# ---------------------------------------------------------------------------

function default_output(input_path::String) :: String
    out_dir = joinpath(RENDER_DIR, "..", "out")
    mkpath(out_dir)
    base = splitext(basename(input_path))[1]
    return joinpath(out_dir, base * ".ppm")
end

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

function main()
    if isempty(ARGS)
        println("""
UML Renderer -- Code Review TUI

Usage:
  julia render_diagram.jl --input <file.json> [options]

Options:
  --input  <path>    UML JSON file to render  (required)
  --output <path>    Output image path        (default: uml/out/<name>.ppm)
  --width  <n>       Override viewport width
  --height <n>       Override viewport height
  --lib    <path>    Path to render_core .dll/.dylib/.so
  --dry-run          Print Scene JSON to stdout instead of rendering
  --setup            Install Julia dependencies (run once)

Examples:
  julia render_diagram.jl --input examples/auth_service_class.json
  julia render_diagram.jl --input examples/login_flow_sequence.json --output out/login.ppm
        """)
        return
    end

    args = parse_args(ARGS)

    args.input === nothing && error("--input is required")
    isfile(args.input) || error("Input file not found: $(args.input)")

    # Parse UML JSON
    uml = JSON.parsefile(args.input)

    # Apply CLI viewport overrides
    if args.width !== nothing || args.height !== nothing
        vp = get(uml, "viewport", Dict())
        if args.width  !== nothing; vp["width"]  = args.width  end
        if args.height !== nothing; vp["height"] = args.height end
        uml["viewport"] = vp
    end

    # Build Scene JSON
    @info "Building scene JSON for $(basename(args.input))..."
    scene_json = SceneBuilder.build_scene(uml)

    # Dry-run: emit Scene JSON only
    if args.dry_run
        println(scene_json)
        return
    end

    # Render via xcm render_core
    @info "Rendering..."
    ctx = XcmBinding.create(
        get(get(uml, "viewport", Dict()), "width",  1200),
        get(get(uml, "viewport", Dict()), "height",  800);
        lib_path = args.lib
    )

    try
        ok, err_msg = XcmBinding.render_scene_json!(ctx, scene_json)
        if !ok
            error("xcm_render_scene_json failed: $err_msg")
        end

        w    = XcmBinding.get_width(ctx)
        h    = XcmBinding.get_height(ctx)
        rgba = XcmBinding.get_pixels(ctx)

        out_path = args.output !== nothing ? args.output : default_output(args.input)
        mkpath(dirname(abspath(out_path)))
        save_image(out_path, w, h, rgba)

        @info "Wrote $w x $h image -> $out_path"
        println("Rendered: $out_path")
    finally
        XcmBinding.destroy!(ctx)
    end
end

main()
