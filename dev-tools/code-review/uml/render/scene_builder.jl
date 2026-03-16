# scene_builder.jl
# Converts UML JSON dicts (parsed from input files) into Scene JSON strings
# for xcm render_core (xcm_render_scene_json API).
#
# Supports:
#   class_diagram    -- classes with attributes/methods and typed relationships
#   sequence_diagram -- participants with ordered message arrows and lifelines
#
# The colour palette matches the Code Review TUI dark-purple theme.

module SceneBuilder

# ---------------------------------------------------------------------------
# Colour palette  (dark-purple theme mirroring review_tui/main.lua)
# ---------------------------------------------------------------------------
const BG          = "#0c071cff"
const PANEL       = "#111827ff"
const PANEL_MID   = "#1f2937ff"
const HEADER_BG   = "#1e1442ff"
const BOX_BG      = "#140e2cff"
const BORDER      = "#3b1f6fff"
const BORDER_HI   = "#5c2fa0ff"
const TEXT        = "#eceaf6ff"
const TEXT_DIM    = "#9896a6ff"
const TEXT_BRIGHT = "#ffffffff"
const VIOLET      = "#aa55ffff"
const LAVENDER    = "#ccb2ffff"
const GREEN       = "#66cc77ff"
const CYAN        = "#66ccdfff"
const YELLOW      = "#ffd966ff"
const ORANGE      = "#ff8c26ff"
const RED         = "#ff4d4dff"
const LINE_ASSOC  = "#6636aaff"
const LINE_RETURN = "#55aaffff"

# ---------------------------------------------------------------------------
# Scene JSON command builders  (return strings, no allocations beyond that)
# ---------------------------------------------------------------------------

j_clear(color)             = """{"op":"clear","color":"$color"}"""
j_rect(x,y,w,h,color)     = """{"op":"rect","x":$x,"y":$y,"w":$w,"h":$h,"color":"$color"}"""
j_outline(x,y,w,h,lw,col) = """{"op":"outline","x":$x,"y":$y,"w":$w,"h":$h,"lineWidth":$lw,"color":"$col"}"""
j_line(x0,y0,x1,y1,lw,col)= """{"op":"line","x0":$x0,"y0":$y0,"x1":$x1,"y1":$y1,"lineWidth":$lw,"color":"$col"}"""

# Primary font family — matches the system font registered in python_scene_render.py.
# Falls back to renderer FONT5x7 if no font is registered.
const UI_FONT = "Segoe UI"

# Font sizes chosen for clean stb_truetype rendering without hinting.
# Below ~18 px, TrueType outlines produce visibly rough letterforms; 18–24 px
# is the practical floor for comfortable on-screen display.
const SZ_TITLE = 32   # diagram title  (header bar)
const SZ_NAME  = 24   # class / participant names  (bold)
const SZ_BODY  = 18   # attributes, methods, messages
const SZ_META  = 14   # small labels, footer text

function j_text(x, y, txt, sz, color, weight=400, family=UI_FONT)
    safe = replace(string(txt), "\\" => "\\\\", "\"" => "\\\"")
    """{"op":"text","x":$x,"y":$y,"fontSize":$sz,"text":"$safe","color":"$color","fontFamily":"$family","fontWeight":$weight}"""
end

# ---------------------------------------------------------------------------
# Dashed line helpers (simulate dashes via short solid segments)
# ---------------------------------------------------------------------------

function dashes_v(cmds, x, y0, y1, lw, dash, gap, color)
    y   = Float64(y0 < y1 ? y0 : y1)
    yto = Float64(y0 < y1 ? y1 : y0)
    on  = true
    while y < yto
        seg = min(y + (on ? dash : gap), yto)
        on && push!(cmds, j_line(round(Int,x), round(Int,y),
                                 round(Int,x), round(Int,seg), lw, color))
        y  = seg
        on = !on
    end
end

function dashes_h(cmds, y, x0, x1, lw, dash, gap, color)
    x   = Float64(x0 < x1 ? x0 : x1)
    xto = Float64(x0 < x1 ? x1 : x0)
    on  = true
    while x < xto
        seg = min(x + (on ? dash : gap), xto)
        on && push!(cmds, j_line(round(Int,x), round(Int,y),
                                 round(Int,seg), round(Int,y), lw, color))
        x  = seg
        on = !on
    end
end

# ---------------------------------------------------------------------------
# Arrowheads
# ---------------------------------------------------------------------------

# Open arrowhead pointing to (x, y) from the left (arrow going right).
function arrow_right!(cmds, x, y, sz, lw, color)
    push!(cmds, j_line(x, y, x-sz, y-sz÷2, lw, color))
    push!(cmds, j_line(x, y, x-sz, y+sz÷2, lw, color))
end

# Open arrowhead pointing to (x, y) from the right (arrow going left).
function arrow_left!(cmds, x, y, sz, lw, color)
    push!(cmds, j_line(x, y, x+sz, y-sz÷2, lw, color))
    push!(cmds, j_line(x, y, x+sz, y+sz÷2, lw, color))
end

# Hollow triangle arrowhead (inheritance) pointing to (x, y) from the left.
function tri_right!(cmds, x, y, sz, lw, color)
    push!(cmds, j_line(x,    y,    x-sz, y-sz÷2, lw, color))
    push!(cmds, j_line(x,    y,    x-sz, y+sz÷2, lw, color))
    push!(cmds, j_line(x-sz, y-sz÷2, x-sz, y+sz÷2, lw, color))
end

# Hollow triangle pointing to (x, y) from the right.
function tri_left!(cmds, x, y, sz, lw, color)
    push!(cmds, j_line(x,    y,    x+sz, y-sz÷2, lw, color))
    push!(cmds, j_line(x,    y,    x+sz, y+sz÷2, lw, color))
    push!(cmds, j_line(x+sz, y-sz÷2, x+sz, y+sz÷2, lw, color))
end

# Downward arrow (self-call / vertical relations).
function arrow_down!(cmds, x, y, sz, lw, color)
    push!(cmds, j_line(x, y, x-sz÷2, y-sz, lw, color))
    push!(cmds, j_line(x, y, x+sz÷2, y-sz, lw, color))
end

# ---------------------------------------------------------------------------
# Class diagram
# ---------------------------------------------------------------------------

const CLS_W       = 340   # box width
const CLS_HDR_H   = 76   # header zone height  (fits SZ_NAME(24) + SZ_BODY(18) + padding)
const CLS_ROW_H   = 32   # height per attribute / method row (fits SZ_BODY(18) + leading)
const CLS_PAD     = 18   # inner vertical padding
const CLS_SEP     = 10   # pixel gap between attr section and method section
const CLS_MARGIN  = 64   # gap between boxes
const N_COLS      = 3    # max classes per row

function _str(d::Dict, key, default="") :: String
    v = get(d, key, default)
    v === nothing ? default : string(v)
end

function _arr(d::Dict, key) :: Vector
    v = get(d, key, nothing)
    (v isa Vector) ? v : Any[]
end

function visibility_sym(v)
    v == "private"   ? "-" :
    v == "protected" ? "#" :
    v == "package"   ? "~" : "+"
end

function class_box_h(cls) :: Int
    attrs = _arr(cls, "attributes")
    meths = _arr(cls, "methods")
    n     = length(attrs) + length(meths)
    sep   = (!isempty(attrs) && !isempty(meths)) ? CLS_SEP : 0
    return CLS_HDR_H + CLS_PAD + n * CLS_ROW_H + sep + CLS_PAD
end

function draw_class!(cmds, x, y, cls)
    h     = class_box_h(cls)
    name  = _str(cls, "name", "Class")
    ster  = _str(cls, "stereotype")
    attrs = _arr(cls, "attributes")
    meths = _arr(cls, "methods")

    # Box background
    push!(cmds, j_rect(x, y, CLS_W, h, BOX_BG))
    # Header background
    push!(cmds, j_rect(x, y, CLS_W, CLS_HDR_H, HEADER_BG))
    # Outer border
    push!(cmds, j_outline(x, y, CLS_W, h, 1, BORDER_HI))
    # Header divider
    push!(cmds, j_line(x, y+CLS_HDR_H, x+CLS_W, y+CLS_HDR_H, 1, BORDER_HI))

    if !isempty(ster)
        push!(cmds, j_text(x + 12, y + 8,  "«$ster»", SZ_BODY, TEXT_DIM))
        push!(cmds, j_text(x + 12, y + 34, name,       SZ_NAME, TEXT_BRIGHT, 700))
    else
        push!(cmds, j_text(x + 12, y + 26, name, SZ_NAME, TEXT_BRIGHT, 700))
    end

    cy = y + CLS_HDR_H + CLS_PAD

    for attr in attrs
        aname = _str(attr, "name", "field")
        atype = _str(attr, "type")
        avis  = _str(attr, "visibility", "private")
        label = "$(visibility_sym(avis)) $aname$(isempty(atype) ? "" : ": $atype")"
        push!(cmds, j_text(x + 12, cy, label, SZ_BODY, CYAN))
        cy += CLS_ROW_H
    end

    if !isempty(attrs) && !isempty(meths)
        push!(cmds, j_line(x, cy, x+CLS_W, cy, 1, BORDER))
        cy += CLS_SEP
    end

    for meth in meths
        mname = _str(meth, "name", "method()")
        mret  = _str(meth, "return_type", "void")
        mvis  = _str(meth, "visibility",  "public")
        label = "$(visibility_sym(mvis)) $mname: $mret"
        push!(cmds, j_text(x + 12, cy, label, SZ_BODY, GREEN))
        cy += CLS_ROW_H
    end

    return h
end

function build_class_diagram(uml::Dict) :: String
    vp     = get(uml, "viewport", Dict())
    vp_w   = Int(get(vp, "width",  1200))
    vp_h   = Int(get(vp, "height",  780))
    title  = _str(uml, "title", "Class Diagram")
    classes = _arr(uml, "classes")
    rels    = _arr(uml, "relationships")

    cmds = String[]

    push!(cmds, j_clear(BG))
    push!(cmds, j_rect(0, 0, vp_w, 64, HEADER_BG))
    push!(cmds, j_text(16, 16, title, SZ_TITLE, LAVENDER, 700))
    push!(cmds, j_line(0, 64, vp_w, 64, 1, BORDER_HI))

    # ---- Layout: place classes in a grid, record box positions ----
    positions = Dict{String, @NamedTuple{x::Int, y::Int, w::Int, h::Int}}()
    n         = length(classes)
    n_rows    = (n - 1) ÷ N_COLS + 1
    row_h     = zeros(Int, n_rows)

    for (i, cls) in enumerate(classes)
        row = (i-1) ÷ N_COLS
        row_h[row+1] = max(row_h[row+1], class_box_h(cls))
    end

    row_y = Vector{Int}(undef, n_rows)
    acc   = 84   # 64 px header + 20 px gap
    for r in 1:n_rows
        row_y[r] = acc
        acc      += row_h[r] + CLS_MARGIN
    end

    # Auto-size viewport: width = N_COLS columns + margins; height = all rows + footer
    computed_vp_w = 20 + N_COLS * (CLS_W + CLS_MARGIN) + 20
    vp_w = max(vp_w, computed_vp_w)
    computed_vp_h = acc + 34   # acc already accounts for last row + CLS_MARGIN; add footer
    vp_h = max(vp_h, computed_vp_h)

    for (i, cls) in enumerate(classes)
        col = (i-1) % N_COLS
        row = (i-1) ÷ N_COLS
        x   = 20 + col * (CLS_W + CLS_MARGIN)
        y   = row_y[row+1]
        h   = class_box_h(cls)
        id  = _str(cls, "id", "cls_$i")
        positions[id] = (x=x, y=y, w=CLS_W, h=h)
    end

    # ---- Draw boxes ----
    for cls in classes
        id = _str(cls, "id")
        haskey(positions, id) && draw_class!(cmds, positions[id].x, positions[id].y, cls)
    end

    # ---- Draw relationships ----
    for rel in rels
        fid   = _str(rel, "from")
        tid   = _str(rel, "to")
        rtype = _str(rel, "type", "association")
        label = _str(rel, "label")

        (!haskey(positions, fid) || !haskey(positions, tid)) && continue
        pf = positions[fid]
        pt = positions[tid]

        # Self-call loop
        if fid == tid
            lx = pf.x + CLS_W + 4
            my = pf.y + pf.h ÷ 2
            push!(cmds, j_line(pf.x+CLS_W, my, lx+20, my,       1, LINE_ASSOC))
            push!(cmds, j_line(lx+20,       my, lx+20, my+24,    1, LINE_ASSOC))
            push!(cmds, j_line(lx+20,       my+24, pf.x+CLS_W, my+24, 1, LINE_ASSOC))
            arrow_left!(cmds, pf.x+CLS_W, my+24, 7, 1, LINE_ASSOC)
            !isempty(label) && push!(cmds, j_text(lx+22, my+4, label, SZ_META, TEXT_DIM))
            continue
        end

        inherit = rtype in ("inheritance","extends","implementation","implements")
        dashed  = rtype in ("implementation","implements","dependency")
        lcolor  = inherit ? LAVENDER : LINE_ASSOC

        # Determine preferred connection sides
        if pf.x + CLS_W <= pt.x          # source is to the left
            x0, y0 = pf.x + CLS_W, pf.y + pf.h ÷ 2
            x1, y1 = pt.x,          pt.y + pt.h ÷ 2
            head!  = inherit ? tri_right! : arrow_right!
        elseif pf.x >= pt.x + pt.w       # source is to the right
            x0, y0 = pf.x,              pf.y + pf.h ÷ 2
            x1, y1 = pt.x + pt.w,       pt.y + pt.h ÷ 2
            head!  = inherit ? tri_left! : arrow_left!
        else                              # source above or below
            x0, y0 = pf.x + CLS_W ÷ 2,  pf.y + pf.h
            x1, y1 = pt.x + CLS_W ÷ 2,  pt.y
            head!  = arrow_down!
        end

        if dashed
            mid = (x0 + x1) ÷ 2
            dashes_h(cmds, y0, x0, mid, 1, 6, 4, lcolor)
            dashes_v(cmds, mid, y0, y1,  1, 6, 4, lcolor)
            dashes_h(cmds, y1, mid, x1,  1, 6, 4, lcolor)
        else
            push!(cmds, j_line(x0, y0, x1, y1, 1, lcolor))
        end

        head!(cmds, x1, y1, 9, 1, lcolor)

        if !isempty(label)
            mx = (x0 + x1) ÷ 2
            my = (y0 + y1) ÷ 2 - 16
            push!(cmds, j_text(mx, my, label, SZ_META, TEXT_DIM))
        end
    end

    # ---- Footer ----
    push!(cmds, j_line(0, vp_h - 26, vp_w, vp_h - 26, 1, BORDER))
    push!(cmds, j_text(10, vp_h - 14,
                       "Generated by UML Renderer  --  Code Review TUI", SZ_META, TEXT_DIM))

    return _scene_wrap(cmds, vp_w, vp_h)
end

# ---------------------------------------------------------------------------
# Sequence diagram
# ---------------------------------------------------------------------------

const SEQ_TITLE_H   = 64
const SEQ_PART_H    = 78    # participant box height  (fits SZ_NAME(24) with padding)
const SEQ_PART_W    = 210   # participant box width
const SEQ_LANE_W    = 280   # centre-to-centre distance between lanes
const SEQ_MSG_STEP  = 90    # vertical gap between message arrows
const SEQ_ARROW_SZ  = 12

function participant_border_color(ptype)
    ptype == "actor"    ? VIOLET  :
    ptype == "database" ? CYAN    :
    ptype == "boundary" ? YELLOW  :
    ptype == "control"  ? ORANGE  :
    ptype == "entity"   ? GREEN   : BORDER_HI
end

function build_sequence_diagram(uml::Dict) :: String
    parts  = _arr(uml, "participants")
    msgs   = _arr(uml, "messages")
    title  = _str(uml, "title", "Sequence Diagram")
    n_p    = length(parts)
    n_m    = length(msgs)

    # Viewport auto-size (can be overridden in JSON)
    vp = get(uml, "viewport", Dict())
    vp_w = max(Int(get(vp, "width",  n_p * SEQ_LANE_W + 80)),  n_p * SEQ_LANE_W + 80)
    vp_h = max(Int(get(vp, "height", 0)),
               SEQ_TITLE_H + 14 + SEQ_PART_H + n_m * SEQ_MSG_STEP + SEQ_MSG_STEP + SEQ_PART_H + 30)

    cmds = String[]

    push!(cmds, j_clear(BG))
    push!(cmds, j_rect(0, 0, vp_w, SEQ_TITLE_H, HEADER_BG))
    push!(cmds, j_text(16, 16, title, SZ_TITLE, LAVENDER, 700))
    push!(cmds, j_line(0, SEQ_TITLE_H, vp_w, SEQ_TITLE_H, 1, BORDER_HI))

    # Lane centre X positions (evenly distributed across viewport width)
    margin   = (vp_w - n_p * SEQ_LANE_W) ÷ 2 + SEQ_LANE_W ÷ 2
    lane_xs  = [margin + (i-1) * SEQ_LANE_W for i in 1:n_p]
    part_map = Dict{String,Int}()
    for (i, p) in enumerate(parts)
        part_map[_str(p, "id", "p$i")] = i
    end

    part_top_y       = SEQ_TITLE_H + 10
    lifeline_start_y = part_top_y + SEQ_PART_H + 2
    lifeline_end_y   = lifeline_start_y + n_m * SEQ_MSG_STEP + SEQ_MSG_STEP

    # Draw participant boxes (top) and lifelines
    for (i, p) in enumerate(parts)
        name  = _str(p, "name", "P$i")
        ptype = _str(p, "type", "component")
        cx    = lane_xs[i]
        bx    = cx - SEQ_PART_W ÷ 2
        bcol  = participant_border_color(ptype)

        push!(cmds, j_rect(bx, part_top_y, SEQ_PART_W, SEQ_PART_H, BOX_BG))
        push!(cmds, j_outline(bx, part_top_y, SEQ_PART_W, SEQ_PART_H, 2, bcol))
        push!(cmds, j_text(bx + 12, part_top_y + 27, name, SZ_NAME, TEXT_BRIGHT, 700))

        dashes_v(cmds, cx, lifeline_start_y, lifeline_end_y, 1, 9, 5, bcol)
    end

    # Draw messages
    for (i, msg) in enumerate(msgs)
        fid   = _str(msg, "from")
        tid   = _str(msg, "to")
        label = _str(msg, "label")
        mtype = _str(msg, "type", "sync")

        fi = get(part_map, fid, 0)
        ti = get(part_map, tid, 0)
        (fi == 0 || ti == 0) && continue

        fx    = lane_xs[fi]
        tx    = lane_xs[ti]
        my    = lifeline_start_y + (i-1) * SEQ_MSG_STEP + SEQ_MSG_STEP ÷ 2
        is_ret = mtype == "return"
        lcolor = is_ret ? LINE_RETURN : LINE_ASSOC

        # Self-call (from and to are the same participant)
        if fi == ti
            loop_x = fx + SEQ_PART_W ÷ 2 + 4
            push!(cmds, j_line(fx, my,     loop_x+16, my,      1, lcolor))
            push!(cmds, j_line(loop_x+16, my,     loop_x+16, my+34, 1, lcolor))
            push!(cmds, j_line(loop_x+16, my+34,  fx,         my+34, 1, lcolor))
            arrow_left!(cmds, fx, my+34, SEQ_ARROW_SZ, 1, lcolor)
            !isempty(label) && push!(cmds, j_text(loop_x+18, my+6, label, SZ_BODY, TEXT))
            continue
        end

        if is_ret
            dashes_h(cmds, my, min(fx,tx), max(fx,tx), 1, 7, 4, lcolor)
        else
            push!(cmds, j_line(fx, my, tx, my, 1, lcolor))
        end

        if tx > fx
            arrow_right!(cmds, tx, my, SEQ_ARROW_SZ, 1, lcolor)
        else
            arrow_left!(cmds, tx, my, SEQ_ARROW_SZ, 1, lcolor)
        end

        if !isempty(label)
            lx = (fx + tx) ÷ 2 - min(length(label) * 7, 110)
            push!(cmds, j_text(lx, my - 22, label, SZ_BODY, TEXT))
        end
    end

    # Participant echo boxes at bottom of lifelines
    for (i, p) in enumerate(parts)
        name  = _str(p, "name", "P$i")
        ptype = _str(p, "type", "component")
        cx    = lane_xs[i]
        bx    = cx - SEQ_PART_W ÷ 2
        bcol  = participant_border_color(ptype)
        by    = lifeline_end_y
        push!(cmds, j_rect(bx, by, SEQ_PART_W, SEQ_PART_H, BOX_BG))
        push!(cmds, j_outline(bx, by, SEQ_PART_W, SEQ_PART_H, 2, bcol))
        push!(cmds, j_text(bx + 12, by + 27, name, SZ_NAME, TEXT_BRIGHT, 700))
    end

    push!(cmds, j_line(0, vp_h - 26, vp_w, vp_h - 26, 1, BORDER))
    push!(cmds, j_text(10, vp_h - 14,
                       "Generated by UML Renderer  --  Code Review TUI", SZ_META, TEXT_DIM))

    return _scene_wrap(cmds, vp_w, vp_h)
end

# ---------------------------------------------------------------------------
# Assemble Scene JSON envelope
# ---------------------------------------------------------------------------

function _scene_wrap(cmds::Vector{String}, w::Int, h::Int) :: String
    body = join(cmds, ",\n  ")
    return """{"version":1,"viewport":{"width":$w,"height":$h},"commands":[\n  $body\n]}"""
end

# ---------------------------------------------------------------------------
# Public dispatch
# ---------------------------------------------------------------------------

function build_scene(uml::Dict) :: String
    dtype = get(uml, "type", "")
    if dtype == "class_diagram"
        return build_class_diagram(uml)
    elseif dtype == "sequence_diagram"
        return build_sequence_diagram(uml)
    else
        error("Unknown diagram type \"$dtype\". Supported: class_diagram, sequence_diagram")
    end
end

end  # module SceneBuilder
