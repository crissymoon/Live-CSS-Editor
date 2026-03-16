# xcm_binding.jl
# Julia ccall binding to the xcm render_core shared library.
#
# Uses Libdl for dynamic library loading and Julia ccall for all C calls.
# No Julia packages required beyond the standard library.
#
# Resolution order for the shared library:
#   1. XCM_LIB_PATH env var  (full path to .dll/.dylib/.so)
#   2. RENDER_ENG_ROOT env var + standard build subdirs
#   3. Sibling render_eng/ repo auto-discovered from this file's location
#   4. System linker search path (bare name "render_core")

module XcmBinding

using Libdl

# ---------------------------------------------------------------------------
# Library discovery
# ---------------------------------------------------------------------------

function find_library()::String
    env_path = get(ENV, "XCM_LIB_PATH", "")
    if !isempty(env_path)
        isfile(env_path) || error("XCM_LIB_PATH is set but file not found: $env_path")
        return env_path
    end

    ext = Sys.iswindows() ? ".dll" : Sys.isapple() ? ".dylib" : ".so"

    build_subdirs = [
        joinpath("build", "native-vcpkg", "Release"),
        joinpath("build", "native-vs",    "Release"),
        joinpath("build", "native",       "Release"),
        joinpath("build", "native"),
    ]

    function try_root(root::String)
        for sub in build_subdirs
            p = joinpath(root, sub, "render_core" * ext)
            ispath(p) && return p
        end
        return ""
    end

    # Path: render/xcm_binding.jl -> render/ -> uml/ -> code-review/ ->
    #        dev-tools/ -> dev_tools/ -> Desktop/ -> render_eng/
    script_dir = dirname(abspath(@__FILE__))
    desktop    = normpath(joinpath(script_dir, "..", "..", "..", "..", ".."))
    sibling    = joinpath(desktop, "render_eng")

    result = try_root(sibling)
    !isempty(result) && return result

    env_root = get(ENV, "RENDER_ENG_ROOT", "")
    if !isempty(env_root)
        result = try_root(env_root)
        !isempty(result) && return result
    end

    # Fall through to system linker
    return "render_core"
end

# ---------------------------------------------------------------------------
# Context struct
# ---------------------------------------------------------------------------

mutable struct XcmCtx
    lib::Ptr{Nothing}   # dlopen handle
    ptr::Ptr{Cvoid}     # xcm context pointer
    init_w::Int
    init_h::Int
end

# ---------------------------------------------------------------------------
# create / destroy
# ---------------------------------------------------------------------------

function create(w::Int, h::Int; lib_path::Union{String, Nothing}=nothing)::XcmCtx
    path = lib_path === nothing ? find_library() : lib_path
    lib  = Libdl.dlopen(path, Libdl.RTLD_LOCAL)

    sym = Libdl.dlsym(lib, :xcm_create)
    ptr = ccall(sym, Ptr{Cvoid}, (Cint, Cint), Cint(w), Cint(h))
    ptr == C_NULL && error("xcm_create($w, $h) returned NULL")

    return XcmCtx(lib, ptr, w, h)
end

function destroy!(c::XcmCtx)
    c.ptr == C_NULL && return
    sym = Libdl.dlsym(c.lib, :xcm_destroy)
    ccall(sym, Cvoid, (Ptr{Cvoid},), c.ptr)
    c.ptr = C_NULL
    Libdl.dlclose(c.lib)
    return nothing
end

# ---------------------------------------------------------------------------
# Scene JSON render path
# xcm_render_scene_json(ctx, utf8_buf, byte_len, scroll_y) -> int
# ---------------------------------------------------------------------------

function render_scene_json!(c::XcmCtx, scene_json::String;
                             scroll_y::Float32=0f0)::Tuple{Bool, String}
    sym    = Libdl.dlsym(c.lib, :xcm_render_scene_json)
    buf    = collect(codeunits(scene_json))
    rc     = ccall(sym, Cint,
                   (Ptr{Cvoid}, Ptr{UInt8}, Cint, Cfloat),
                   c.ptr, buf, Cint(length(buf)), scroll_y)
    if rc != 0
        err_sym = Libdl.dlsym(c.lib, :xcm_scene_last_error)
        msg_ptr = ccall(err_sym, Cstring, (Ptr{Cvoid},), c.ptr)
        msg     = msg_ptr == C_NULL ? "(unknown error)" : unsafe_string(msg_ptr)
        return false, msg
    end
    return true, ""
end

# ---------------------------------------------------------------------------
# Pixel / dimension accessors
# ---------------------------------------------------------------------------

function get_width(c::XcmCtx)::Int
    sym = Libdl.dlsym(c.lib, :xcm_width)
    Int(ccall(sym, Cint, (Ptr{Cvoid},), c.ptr))
end

function get_height(c::XcmCtx)::Int
    sym = Libdl.dlsym(c.lib, :xcm_height)
    Int(ccall(sym, Cint, (Ptr{Cvoid},), c.ptr))
end

# Returns a copy of the RGBA pixel buffer (width * height * 4 bytes).
function get_pixels(c::XcmCtx)::Vector{UInt8}
    sym = Libdl.dlsym(c.lib, :xcm_pixels)
    ptr = ccall(sym, Ptr{UInt8}, (Ptr{Cvoid},), c.ptr)
    ptr == C_NULL && error("xcm_pixels returned NULL")
    n = get_width(c) * get_height(c) * 4
    return copy(unsafe_wrap(Array, ptr, n))
end

end  # module XcmBinding
