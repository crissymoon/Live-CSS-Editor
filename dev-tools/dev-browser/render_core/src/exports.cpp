/*
 * exports.cpp  --  Emscripten WASM / native C exports
 *
 * API exported to JavaScript (and usable from Python via wasmtime):
 *
 *   xcm_ctx*  xcm_create(int width, int height)
 *   void      xcm_destroy(xcm_ctx* ctx)
 *   int       xcm_render(xcm_ctx* ctx,
 *                        const char* html, int html_len,
 *                        const char* css,  int css_len,
 *                        float scroll_y)
 *   uint8_t*  xcm_pixels(xcm_ctx* ctx)
 *   int       xcm_width(xcm_ctx* ctx)
 *   int       xcm_height(xcm_ctx* ctx)
 *   int       xcm_doc_height(xcm_ctx* ctx)    -- full scrollable height
 *   const char* xcm_metrics_json(xcm_ctx* ctx) -- layout metrics as JSON
 *   const char* xcm_ansi_frame(xcm_ctx* ctx, int cols, int rows)
 *   const char* xcm_render_ansi(xcm_ctx* ctx,
 *                               const char* html, int html_len,
 *                               const char* css,  int css_len,
 *                               float scroll_y,
 *                               int cols, int rows)
 *
 *   -- Memory helpers for JS caller (pass/receive string buffers)--
 *   uint8_t*  xcm_alloc(int n)    -- allocate n bytes in WASM heap
 *   void      xcm_free(uint8_t*)  -- free a xcm_alloc()ed buffer
 *
 * Build with Emscripten:
 *   emcc -O3 -std=c++17 -sWASM=1 -sEXPORTED_FUNCTIONS=@exports.json \
 *        -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,stringToUTF8 \
 *        -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432 \
 *        -sSTACK_SIZE=2097152 \
 *        html_tokenizer.cpp css_parser.cpp style_resolver.cpp \
 *        layout.cpp paint.cpp exports.cpp \
 *        -o render_core.js
 */

#include "html_tokenizer.h"
#include "css_parser.h"
#include "style_resolver.h"
#include "layout.h"
#include "paint.h"
#include "xcm_tui.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <memory>
#include <algorithm>

#ifdef __EMSCRIPTEN__
  #include <emscripten.h>
  #define XCM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
  #define XCM_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {

// -------------------------------------------------------------------------
// Context
// -------------------------------------------------------------------------
struct xcm_ctx {
    int viewport_w = 0;
    int viewport_h = 0;

    // Persistent arenas -- soft-reset each render, zero malloc/free on hot path.
    xcm::Arena         dom_arena;
    xcm::Arena         layout_arena;

    xcm::Document*     doc         = nullptr;
    xcm::LayoutBox*    layout_root = nullptr;
    xcm::LayoutEngine* layout_eng  = nullptr;
    xcm::PaintEngine*  paint_eng   = nullptr;

    // CSS cache -- skip re-parse when CSS hasn't changed.
    std::string        last_css;
    xcm::StyleSheet    cached_css;

    // Cached metrics JSON.
    char metrics_json[8192] = {};

    // Cached ANSI frame for terminal rendering.
    std::string ansi_frame;

    void reset() {
        // Soft-reset: rewind bump pointers without freeing slabs.
        // Zero malloc/free cost -- eliminates the per-frame allocator variance.
        dom_arena.reset();
        layout_arena.reset();
        doc = nullptr; layout_root = nullptr; layout_eng = nullptr;
        // paint_eng pixel buffer is reused -- just cleared by PaintEngine::paint()
    }

    ~xcm_ctx() { delete paint_eng; }
};

// -------------------------------------------------------------------------
XCM_EXPORT xcm_ctx* xcm_create(int width, int height) {
    auto* ctx = new xcm_ctx();
    ctx->viewport_w = width;
    ctx->viewport_h = height;
    ctx->paint_eng  = new xcm::PaintEngine(width, height);
    return ctx;
}

// -------------------------------------------------------------------------
XCM_EXPORT void xcm_destroy(xcm_ctx* ctx) {
    delete ctx;
}

// -------------------------------------------------------------------------
XCM_EXPORT int xcm_render(xcm_ctx* ctx,
                          const char* html, int html_len,
                          const char* css,  int css_len,
                          float scroll_y)
{
    if (!ctx) return -1;
    ctx->reset();

    // 1. Parse HTML.
    ctx->doc = xcm::parse_html(html, static_cast<std::size_t>(html_len), ctx->dom_arena);
    if (!ctx->doc) return -2;

    // 2. Parse CSS (cached -- skip re-parse if CSS unchanged).
    {
        std::size_t new_len = (css && css_len > 0) ? static_cast<std::size_t>(css_len) : 0;
        std::string new_key(css && css_len > 0 ? css : "", new_len);
        if (new_key != ctx->last_css) {
            ctx->last_css   = new_key;
            ctx->cached_css = new_len > 0
                ? xcm::parse_css(css, new_len, 2)
                : xcm::StyleSheet{};
        }
    }
    std::vector<xcm::StyleSheet> sheets;
    if (!ctx->last_css.empty()) sheets.push_back(ctx->cached_css);

    // 3. Resolve styles.
    xcm::resolve_styles(ctx->doc, sheets,
                        static_cast<float>(ctx->viewport_w),
                        static_cast<float>(ctx->viewport_h));

    // 4. Layout.
    ctx->layout_eng = ctx->layout_arena.make<xcm::LayoutEngine>(
        static_cast<float>(ctx->viewport_w),
        static_cast<float>(ctx->viewport_h),
        ctx->layout_arena);
    ctx->layout_root = ctx->layout_eng->layout(ctx->doc);

    // 5. Paint.
    ctx->paint_eng->set_scroll(scroll_y);
    ctx->paint_eng->paint(ctx->layout_root);

    // 6. Build metrics JSON.
    float doc_h = ctx->layout_root ? ctx->layout_root->height : 0;
    std::snprintf(ctx->metrics_json, sizeof(ctx->metrics_json),
        "{"
        "\"viewport_w\":%d,"
        "\"viewport_h\":%d,"
        "\"doc_height\":%.1f,"
        "\"boxes\":%zu"
        "}",
        ctx->viewport_w, ctx->viewport_h,
        doc_h,
        ctx->layout_eng ? ctx->layout_eng->all_boxes.size() : 0);

    return 0; // success
}

// -------------------------------------------------------------------------
XCM_EXPORT uint8_t* xcm_pixels(xcm_ctx* ctx) {
    return ctx ? ctx->paint_eng->pixels() : nullptr;
}

XCM_EXPORT int xcm_width(xcm_ctx* ctx) {
    return ctx ? ctx->viewport_w : 0;
}

XCM_EXPORT int xcm_height(xcm_ctx* ctx) {
    return ctx ? ctx->viewport_h : 0;
}

XCM_EXPORT int xcm_doc_height(xcm_ctx* ctx) {
    if (!ctx || !ctx->layout_root) return 0;
    return static_cast<int>(ctx->layout_root->height);
}

XCM_EXPORT const char* xcm_metrics_json(xcm_ctx* ctx) {
    return ctx ? ctx->metrics_json : "{}";
}

// -------------------------------------------------------------------------
// ANSI terminal rendering.
// -------------------------------------------------------------------------
static inline int clampi(int v, int lo, int hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

static std::string rgba_to_ansi_background(const uint8_t* rgba,
                                           int src_w,
                                           int src_h,
                                           int cols,
                                           int rows) {
    if (!rgba || src_w <= 0 || src_h <= 0 || cols <= 0 || rows <= 0) return "";

    std::string out;
    out.reserve(static_cast<std::size_t>(cols * rows * 14));

    for (int y = 0; y < rows; ++y) {
        int py = clampi((y * src_h) / rows, 0, src_h - 1);
        int prev_r = -1, prev_g = -1, prev_b = -1;

        for (int x = 0; x < cols; ++x) {
            int px = clampi((x * src_w) / cols, 0, src_w - 1);
            const uint8_t* p = rgba + (py * src_w + px) * 4;
            int r = p[0], g = p[1], b = p[2];

            if (r != prev_r || g != prev_g || b != prev_b) {
                char esc[32];
                std::snprintf(esc, sizeof(esc), "\x1b[48;2;%d;%d;%dm", r, g, b);
                out.append(esc);
                prev_r = r;
                prev_g = g;
                prev_b = b;
            }
            out.push_back(' ');
        }
        out.append("\x1b[0m\n");
    }

    return out;
}

XCM_EXPORT const char* xcm_ansi_frame(xcm_ctx* ctx, int cols, int rows) {
    if (!ctx || !ctx->paint_eng) return "";

    ctx->ansi_frame = rgba_to_ansi_background(ctx->paint_eng->pixels(),
                                              ctx->viewport_w,
                                              ctx->viewport_h,
                                              cols,
                                              rows);
    return ctx->ansi_frame.c_str();
}

XCM_EXPORT const char* xcm_render_ansi(xcm_ctx* ctx,
                                       const char* html,
                                       int html_len,
                                       const char* css,
                                       int css_len,
                                       float scroll_y,
                                       int cols,
                                       int rows) {
    int rc = xcm_render(ctx, html, html_len, css, css_len, scroll_y);
    if (rc != 0) return "";
    return xcm_ansi_frame(ctx, cols, rows);
}

// -------------------------------------------------------------------------
// Memory helpers (used by JS to pass strings into WASM heap).
// -------------------------------------------------------------------------
XCM_EXPORT uint8_t* xcm_alloc(int n) {
    return static_cast<uint8_t*>(std::malloc(static_cast<std::size_t>(n)));
}

XCM_EXPORT void xcm_free(uint8_t* ptr) {
    std::free(ptr);
}

// -------------------------------------------------------------------------
// TUI API  --  callable from Python / Node / shell via ctypes or ffi
// -------------------------------------------------------------------------
struct xcm_tui_ctx {
    xcm::TuiRenderer renderer;
    xcm::TuiFramebuffer fb;
    std::string serial_out;
};

XCM_EXPORT xcm_tui_ctx* xcm_tui_create(int mode, int color_mode,
                                        int edge_pass, int shading_pass,
                                        int edge_thresh) {
    auto* t = new xcm_tui_ctx();
    t->renderer.mode        = static_cast<xcm::TuiMode>(mode);
    t->renderer.color_mode  = static_cast<xcm::TuiColorMode>(color_mode);
    t->renderer.edge_pass   = (edge_pass   != 0);
    t->renderer.shading_pass= (shading_pass != 0);
    t->renderer.edge_thresh = edge_thresh;
    return t;
}

XCM_EXPORT void xcm_tui_destroy(xcm_tui_ctx* t) {
    delete t;
}

// Render RGBA framebuffer into a TUI cell grid and return serialized ANSI.
// xcm_ctx must have been populated by xcm_render() first.
XCM_EXPORT const char* xcm_tui_render(xcm_tui_ctx* t, xcm_ctx* ctx,
                                       int cols, int rows) {
    if (!t || !ctx || !ctx->paint_eng) return "";
    t->fb.resize(cols, rows);
    t->renderer.render(ctx->paint_eng->pixels(),
                       ctx->viewport_w, ctx->viewport_h,
                       t->fb);
    t->renderer.serialize(t->fb, t->serial_out, true);
    return t->serial_out.c_str();
}

// Render a diff update relative to the previous xcm_tui_render() call.
XCM_EXPORT const char* xcm_tui_frame_diff(xcm_tui_ctx* t, xcm_ctx* ctx,
                                           int cols, int rows) {
    if (!t || !ctx || !ctx->paint_eng) return "";
    xcm::TuiFramebuffer prev = t->fb;
    t->fb.resize(cols, rows);
    t->renderer.render(ctx->paint_eng->pixels(),
                       ctx->viewport_w, ctx->viewport_h,
                       t->fb);
    t->renderer.serialize_diff(prev, t->fb, t->serial_out);
    return t->serial_out.c_str();
}

} // extern "C"

// =========================================================================
// CLI smoke-test  (compiled only when XCM_CLI_TEST=1)
// Usage: xcm_render_test [width height [outfile.ppm]]
// =========================================================================
#ifdef XCM_CLI_TEST

#include <cstdio>

static const char* TEST_HTML = R"html(
<!DOCTYPE html>
<html>
<head><style>
  body { font-family: sans-serif; background: #f4f4f4; margin: 0; padding: 16px; }
  h1   { color: #1a1a2e; font-size: 28px; margin-bottom: 8px; }
  p    { color: #333; font-size: 14px; line-height: 1.6; }
  .box { display: flex; gap: 12px; margin-top: 16px; }
  .card{ background: white; border: 1px solid #ddd; padding: 12px;
         border-radius: 6px; flex: 1; }
  .card h2 { color: #e94560; font-size: 16px; margin: 0 0 6px; }
</style></head>
<body>
  <h1>XCM Render Core</h1>
  <p>C++17 layout and paint engine compiled to a native shared library.</p>
  <div class="box">
    <div class="card"><h2>HTML</h2><p>Full tokenizer with implied tags.</p></div>
    <div class="card"><h2>CSS</h2><p>Cascade, specificity, selectors.</p></div>
    <div class="card"><h2>Layout</h2><p>Block, inline, and flex.</p></div>
    <div class="card"><h2>Paint</h2><p>RGBA8 software rasterizer.</p></div>
  </div>
</body>
</html>
)html";

int main(int argc, char** argv) {
    int   w    = argc > 1 ? std::atoi(argv[1]) : 800;
    int   h    = argc > 2 ? std::atoi(argv[2]) : 600;
    const char* out = argc > 3 ? argv[3] : "xcm_test.ppm";

    xcm_ctx* ctx = xcm_create(w, h);
    if (!ctx) { std::fprintf(stderr, "xcm_create failed\n"); return 1; }

    int rc = xcm_render(ctx,
                        TEST_HTML, static_cast<int>(std::strlen(TEST_HTML)),
                        "", 0, 0.f);
    if (rc != 0) { std::fprintf(stderr, "xcm_render returned %d\n", rc); return 1; }

    std::fprintf(stderr, "metrics: %s\n", xcm_metrics_json(ctx));

    // Write PPM (P6 binary RGB -- drop alpha channel).
    int rw = xcm_width(ctx), rh = xcm_height(ctx);
    const uint8_t* px = xcm_pixels(ctx);
    FILE* f = std::fopen(out, "wb");
    if (!f) { std::fprintf(stderr, "cannot open %s\n", out); return 1; }
    std::fprintf(f, "P6\n%d %d\n255\n", rw, rh);
    for (int i = 0; i < rw * rh; ++i) {
        std::fputc(px[i*4+0], f);
        std::fputc(px[i*4+1], f);
        std::fputc(px[i*4+2], f);
    }
    std::fclose(f);

    std::fprintf(stderr, "wrote %dx%d PPM to %s\n", rw, rh, out);
    xcm_destroy(ctx);
    return 0;
}
#endif // XCM_CLI_TEST
