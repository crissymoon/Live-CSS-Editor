/*
 * style_resolver.cpp  --  CSS cascade, selector matching, computed style
 */

#include "style_resolver.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <unordered_map>
#include <string_view>

namespace xcm {

// =========================================================================
// parse_color
// =========================================================================
static uint8_t hex2(char hi, char lo) {
    auto h = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return 0;
    };
    return (h(hi) << 4) | h(lo);
}

Color parse_color(const std::string& raw) {
    std::string val = raw;
    // Trim.
    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.front()))) val.erase(val.begin());
    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back())))  val.pop_back();
    std::string lv;
    for (char c : val) lv += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (lv == "transparent") return Color::transparent();
    if (lv == "white")     return {255,255,255,255};
    if (lv == "black")     return {0,0,0,255};
    if (lv == "red")       return {255,0,0,255};
    if (lv == "green")     return {0,128,0,255};
    if (lv == "blue")      return {0,0,255,255};
    if (lv == "yellow")    return {255,255,0,255};
    if (lv == "orange")    return {255,165,0,255};
    if (lv == "purple")    return {128,0,128,255};
    if (lv == "pink")      return {255,192,203,255};
    if (lv == "gray" || lv == "grey") return {128,128,128,255};
    if (lv == "darkgray" || lv == "darkgrey") return {169,169,169,255};
    if (lv == "lightgray" || lv == "lightgrey") return {211,211,211,255};
    if (lv == "silver")    return {192,192,192,255};
    if (lv == "navy")      return {0,0,128,255};
    if (lv == "teal")      return {0,128,128,255};
    if (lv == "maroon")    return {128,0,0,255};
    if (lv == "lime")      return {0,255,0,255};
    if (lv == "aqua" || lv == "cyan") return {0,255,255,255};
    if (lv == "fuchsia" || lv == "magenta") return {255,0,255,255};
    if (lv == "currentcolor") return Color::black(); // fallback

    if (lv[0] == '#') {
        std::string h = lv.substr(1);
        if (h.size() == 3) {
            return {hex2(h[0],h[0]), hex2(h[1],h[1]), hex2(h[2],h[2]), 255};
        }
        if (h.size() == 4) {
            return {hex2(h[0],h[0]), hex2(h[1],h[1]), hex2(h[2],h[2]), hex2(h[3],h[3])};
        }
        if (h.size() == 6) {
            return {hex2(h[0],h[1]), hex2(h[2],h[3]), hex2(h[4],h[5]), 255};
        }
        if (h.size() == 8) {
            return {hex2(h[0],h[1]), hex2(h[2],h[3]), hex2(h[4],h[5]), hex2(h[6],h[7])};
        }
    }
    // rgb(...) / rgba(...)
    if (lv.substr(0,4) == "rgb(" || lv.substr(0,5) == "rgba(") {
        auto s1 = lv.find('(');
        auto s2 = lv.rfind(')');
        if (s1 == std::string::npos || s2 == std::string::npos) return Color::black();
        std::string inner = lv.substr(s1+1, s2-s1-1);
        // Split by comma or space.
        std::vector<float> nums;
        std::istringstream is(inner);
        std::string tok;
        while (std::getline(is, tok, ',')) {
            while (!tok.empty() && std::isspace(static_cast<unsigned char>(tok.front()))) tok.erase(tok.begin());
            while (!tok.empty() && std::isspace(static_cast<unsigned char>(tok.back()))) tok.pop_back();
            if (!tok.empty()) {
                try { nums.push_back(std::stof(tok)); } catch (...) { nums.push_back(0); }
            }
        }
        Color c;
        c.r = nums.size() > 0 ? static_cast<uint8_t>(std::clamp(nums[0], 0.f, 255.f)) : 0;
        c.g = nums.size() > 1 ? static_cast<uint8_t>(std::clamp(nums[1], 0.f, 255.f)) : 0;
        c.b = nums.size() > 2 ? static_cast<uint8_t>(std::clamp(nums[2], 0.f, 255.f)) : 0;
        c.a = nums.size() > 3 ? static_cast<uint8_t>(std::clamp(nums[3] * 255.f, 0.f, 255.f)) : 255;
        return c;
    }
    return Color::black();
}

// =========================================================================
// parse_length
// =========================================================================
Length parse_length(const std::string& raw) {
    std::string val = raw;
    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.front()))) val.erase(val.begin());
    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back())))  val.pop_back();
    std::string lv;
    for (char c : val) lv += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (lv == "auto")   return {0, LengthUnit::AUTO};
    if (lv == "none")   return {0, LengthUnit::NONE};
    if (lv == "0")      return {0, LengthUnit::PX};

    try {
        std::size_t end = 0;
        float n = std::stof(lv, &end);
        std::string unit = lv.substr(end);
        if (unit == "px" || unit.empty()) return {n, LengthUnit::PX};
        if (unit == "%")                  return {n, LengthUnit::PERCENT};
        if (unit == "em")                 return {n, LengthUnit::EM};
        if (unit == "rem")                return {n, LengthUnit::REM};
        if (unit == "vw")                 return {n, LengthUnit::VW};
        if (unit == "vh")                 return {n, LengthUnit::VH};
        if (unit == "pt")                 return {n * 96.f / 72.f, LengthUnit::PX};
        if (unit == "cm")                 return {n * 37.795f, LengthUnit::PX};
        if (unit == "mm")                 return {n * 3.7795f, LengthUnit::PX};
        if (unit == "in")                 return {n * 96.f, LengthUnit::PX};
        return {n, LengthUnit::PX};
    } catch (...) {}
    return {0, LengthUnit::AUTO};
}

float Length::resolve(float parent_px, float font_size_px, float root_font_px,
                      float vw, float vh) const {
    switch (unit) {
    case LengthUnit::PX:      return value;
    case LengthUnit::PERCENT: return value * parent_px / 100.f;
    case LengthUnit::EM:      return value * font_size_px;
    case LengthUnit::REM:     return value * root_font_px;
    case LengthUnit::VW:      return value * vw / 100.f;
    case LengthUnit::VH:      return value * vh / 100.f;
    default:                  return 0.f;
    }
}

// =========================================================================
// Selector matcher
// =========================================================================
// Simplified but handles: tag, .class, #id, [attr], [attr=val],
// descendant (space), child (>), adjacent (+), sibling (~), :first-child,
// :last-child, :nth-child(n), :not(s), ::before/::after (ignored).
//
// Returns true if the given node matches the selector.
// =========================================================================

static std::string_view trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))  sv.remove_suffix(1);
    return sv;
}

// Forward declaration.
static bool match_simple(const Node* node, std::string_view simple);

static int child_index(const Node* node) {
    int idx = 1;
    const Node* sib = node->prev_sibling;
    while (sib) {
        if (sib->kind == NodeKind::ELEMENT) ++idx;
        sib = sib->prev_sibling;
    }
    return idx;
}

static int child_count(const Node* node) {
    int count = 0;
    const Node* sib = node->parent ? node->parent->first_child : nullptr;
    while (sib) {
        if (sib->kind == NodeKind::ELEMENT) ++count;
        sib = sib->next_sibling;
    }
    return count;
}

// Match a simple selector (no combinators) against a node.
static bool match_simple(const Node* node, std::string_view sel) {
    if (node->kind != NodeKind::ELEMENT) return false;
    sel = trim(sel);
    if (sel.empty() || sel == "*") return true;

    std::size_t i = 0;
    // Optional universal/type selector first.
    if (!sel.empty() && sel[0] != '.' && sel[0] != '#' && sel[0] != '[' &&
        sel[0] != ':' && sel[0] != '*') {
        // Tag name.
        std::size_t j = i;
        while (j < sel.size() && sel[j] != '.' && sel[j] != '#' &&
               sel[j] != '[' && sel[j] != ':') ++j;
        std::string_view tag = sel.substr(i, j - i);
        if (!tag.empty() && tag != "*") {
            if (std::string_view(node->tag) != tag) return false;
        }
        i = j;
    } else if (!sel.empty() && sel[0] == '*') {
        ++i;
    }

    while (i < sel.size()) {
        char c = sel[i];
        if (c == '.') {
            ++i;
            std::size_t j = i;
            while (j < sel.size() && sel[j] != '.' && sel[j] != '#' &&
                   sel[j] != '[' && sel[j] != ':') ++j;
            if (!node->has_class(sel.substr(i, j - i))) return false;
            i = j;
        } else if (c == '#') {
            ++i;
            std::size_t j = i;
            while (j < sel.size() && sel[j] != '.' && sel[j] != '#' &&
                   sel[j] != '[' && sel[j] != ':') ++j;
            if (!node->has_id(sel.substr(i, j - i))) return false;
            i = j;
        } else if (c == '[') {
            ++i;
            std::size_t close = sel.find(']', i);
            if (close == std::string_view::npos) return false;
            std::string_view attr_expr = sel.substr(i, close - i);
            i = close + 1;
            // Parse attr_expr: attr, attr=val, attr~=val, attr|=val, attr^=val, attr$=val, attr*=val
            auto eq_pos = attr_expr.find('=');
            if (eq_pos == std::string_view::npos) {
                // Attribute presence.
                if (!node->attr(std::string(attr_expr))) return false;
            } else {
                char op = (eq_pos > 0) ? attr_expr[eq_pos - 1] : ' ';
                std::string_view aname = (op == '~' || op == '|' || op == '^' || op == '$' || op == '*')
                    ? attr_expr.substr(0, eq_pos - 1) : attr_expr.substr(0, eq_pos);
                std::string_view aval = attr_expr.substr(eq_pos + 1);
                // Remove quotes.
                if (!aval.empty() && (aval.front() == '"' || aval.front() == '\'')) aval.remove_prefix(1);
                if (!aval.empty() && (aval.back()  == '"' || aval.back()  == '\'')) aval.remove_suffix(1);
                const char* nv = node->attr(std::string(aname));
                if (!nv) return false;
                std::string_view node_attr_val{nv};
                bool ok = false;
                switch (op) {
                case '~': {
                    // Contains word.
                    std::size_t p = 0;
                    while (p < node_attr_val.size()) {
                        while (p < node_attr_val.size() && node_attr_val[p] == ' ') ++p;
                        std::size_t e = p;
                        while (e < node_attr_val.size() && node_attr_val[e] != ' ') ++e;
                        if (node_attr_val.substr(p, e-p) == aval) { ok = true; break; }
                        p = e;
                    }
                    break;
                }
                case '|': ok = node_attr_val == aval || (node_attr_val.size() > aval.size() && node_attr_val.substr(0, aval.size()) == aval && node_attr_val[aval.size()] == '-'); break;
                case '^': ok = node_attr_val.size() >= aval.size() && node_attr_val.substr(0, aval.size()) == aval; break;
                case '$': ok = node_attr_val.size() >= aval.size() && node_attr_val.substr(node_attr_val.size() - aval.size()) == aval; break;
                case '*': ok = node_attr_val.find(aval) != std::string_view::npos; break;
                default:  ok = node_attr_val == aval; break;
                }
                if (!ok) return false;
            }
        } else if (c == ':') {
            ++i;
            if (i < sel.size() && sel[i] == ':') {
                // pseudo-element -- skip
                ++i;
                while (i < sel.size() && sel[i] != '.' && sel[i] != '#' &&
                       sel[i] != '[' && sel[i] != ':') ++i;
            } else {
                // pseudo-class
                std::size_t j = i;
                while (j < sel.size() && sel[j] != '(' && sel[j] != '.' &&
                       sel[j] != '#' && sel[j] != '[' && sel[j] != ':') ++j;
                std::string_view pseudo = sel.substr(i, j - i);
                i = j;
                std::string_view arg;
                if (i < sel.size() && sel[i] == '(') {
                    ++i;
                    std::size_t close = sel.find(')', i);
                    arg = sel.substr(i, close - i);
                    i = (close == std::string_view::npos) ? sel.size() : close + 1;
                }
                if (pseudo == "first-child") {
                    if (child_index(node) != 1) return false;
                } else if (pseudo == "last-child") {
                    int cnt = child_count(node), idx = child_index(node);
                    if (idx != cnt) return false;
                } else if (pseudo == "nth-child") {
                    // Only handle simple numbers for now.
                    try {
                        std::string as(arg);
                        int n = std::stoi(as);
                        if (child_index(node) != n) return false;
                    } catch (...) {}
                } else if (pseudo == "not") {
                    if (match_simple(node, arg)) return false;
                } else if (pseudo == "hover" || pseudo == "focus" || pseudo == "active" ||
                           pseudo == "visited" || pseudo == "checked") {
                    // Dynamic pseudo-classes -- ignore (no browser state).
                }
            }
        } else {
            ++i; // unknown char, skip
        }
    }
    return true;
}

// Match a selector with combinators against a node.
static bool selector_matches(const Node* node, const std::string& selector) {
    // Split by combinators: ' ', '>', '+', '~'.
    // We work right-to-left (rightmost simple selector matches current node).
    // Build a list of (combinator, simple_selector) from right to left.
    std::string sel = selector;
    // Tokenize.
    struct Part { char combinator; std::string simple; };
    std::vector<Part> parts;
    std::size_t i = sel.size();
    std::string current_simple;
    char next_combinator = ' ';

    auto push_part = [&]() {
        if (!current_simple.empty()) {
            // Trim.
            std::size_t s = current_simple.find_first_not_of(" \t\n\r");
            std::size_t e = current_simple.find_last_not_of(" \t\n\r");
            if (s != std::string::npos) {
                parts.push_back({next_combinator, current_simple.substr(s, e-s+1)});
            }
            current_simple.clear();
            next_combinator = ' ';
        }
    };

    // Scan selector right to left.
    // First collect parts left-to-right then process.
    // Simpler: split on combinators.
    std::vector<std::pair<char, std::string>> fwd_parts; // (combinator before this, simple)
    {
        std::size_t p = 0;
        char comb = ' ';
        while (p <= sel.size()) {
            if (p == sel.size() || sel[p] == '>' || sel[p] == '+' || sel[p] == '~' ||
                (sel[p] == ' ' && p + 1 < sel.size())) {
                // Check if it's a meaningful space vs trailing.
                std::string simple_str = sel.substr(0, p);
                std::size_t s0 = simple_str.find_first_not_of(" \t\n\r");
                std::size_t s1 = simple_str.find_last_not_of(" \t\n\r");
                if (s0 != std::string::npos) {
                    fwd_parts.push_back({comb, simple_str.substr(s0, s1 - s0 + 1)});
                }
                if (p == sel.size()) break;
                char nc = sel[p];
                if (nc == ' ') {
                    // Skip whitespace.
                    while (p < sel.size() && sel[p] == ' ') ++p;
                    // Check next real char for combinator.
                    if (p < sel.size() && (sel[p] == '>' || sel[p] == '+' || sel[p] == '~')) {
                        comb = sel[p++];
                        while (p < sel.size() && sel[p] == ' ') ++p;
                    } else {
                        comb = ' '; // descendant
                    }
                } else {
                    comb = nc; ++p;
                    while (p < sel.size() && sel[p] == ' ') ++p;
                }
                sel = sel.substr(p);
                p = 0;
            } else {
                // Handle string and bracket content.
                if (sel[p] == '[') {
                    while (p < sel.size() && sel[p] != ']') ++p;
                } else if (sel[p] == '(') {
                    int d = 1; ++p;
                    while (p < sel.size() && d > 0) {
                        if (sel[p] == '(') ++d;
                        else if (sel[p] == ')') --d;
                        ++p;
                    }
                    continue;
                }
                ++p;
            }
        }
    }

    // Now match right-to-left.
    if (fwd_parts.empty()) return false;
    const Node* cur = node;
    for (int pi = static_cast<int>(fwd_parts.size()) - 1; pi >= 0; --pi) {
        const auto& [comb, simple] = fwd_parts[pi];
        if (pi == static_cast<int>(fwd_parts.size()) - 1) {
            // Rightmost -- must match 'cur' directly.
            if (!match_simple(cur, simple)) return false;
        } else {
            char c2 = fwd_parts[pi + 1].first;
            if (c2 == '>') {
                // Direct parent.
                if (!cur->parent || cur->parent->kind != NodeKind::ELEMENT) return false;
                cur = cur->parent;
                if (!match_simple(cur, simple)) return false;
            } else if (c2 == '+') {
                // Immediately preceding sibling.
                const Node* sib = cur->prev_sibling;
                while (sib && sib->kind != NodeKind::ELEMENT) sib = sib->prev_sibling;
                if (!sib || !match_simple(sib, simple)) return false;
                cur = sib;
            } else if (c2 == '~') {
                // Any preceding sibling.
                const Node* sib = cur->prev_sibling;
                bool found = false;
                while (sib) {
                    if (sib->kind == NodeKind::ELEMENT && match_simple(sib, simple)) {
                        found = true; cur = sib; break;
                    }
                    sib = sib->prev_sibling;
                }
                if (!found) return false;
            } else {
                // Descendant.
                const Node* anc = cur->parent;
                bool found = false;
                while (anc) {
                    if (anc->kind == NodeKind::ELEMENT && match_simple(anc, simple)) {
                        found = true; cur = anc; break;
                    }
                    anc = anc->parent;
                }
                if (!found) return false;
            }
        }
    }
    return true;
}

// =========================================================================
// Apply declarations to a ComputedStyle.
// =========================================================================
static void apply_decl(ComputedStyle& cs, const CssDecl& d, Arena& arena,
                       float vw, float vh) {
    const std::string& p = d.property;
    const std::string& v = d.value;

    if (p == "display") {
        if (v == "block")        cs.display = Display::BLOCK;
        else if (v == "inline")  cs.display = Display::INLINE;
        else if (v == "inline-block") cs.display = Display::INLINE_BLOCK;
        else if (v == "flex")    cs.display = Display::FLEX;
        else if (v == "inline-flex") cs.display = Display::INLINE_FLEX;
        else if (v == "grid")    cs.display = Display::GRID;
        else if (v == "none")    cs.display = Display::NONE;
        else if (v == "table")   cs.display = Display::TABLE;
        else if (v == "table-row") cs.display = Display::TABLE_ROW;
        else if (v == "table-cell") cs.display = Display::TABLE_CELL;
        else if (v == "list-item") cs.display = Display::LIST_ITEM;
    } else if (p == "position") {
        if (v == "relative") cs.position = Position::RELATIVE;
        else if (v == "absolute") cs.position = Position::ABSOLUTE;
        else if (v == "fixed")    cs.position = Position::FIXED;
        else if (v == "sticky")   cs.position = Position::STICKY;
        else                       cs.position = Position::STATIC;
    } else if (p == "width")  { cs.width   = parse_length(v); }
    else if (p == "height") { cs.height  = parse_length(v); }
    else if (p == "min-width")  { cs.min_width  = parse_length(v); }
    else if (p == "min-height") { cs.min_height = parse_length(v); }
    else if (p == "max-width")  { cs.max_width  = parse_length(v); }
    else if (p == "max-height") { cs.max_height = parse_length(v); }
    else if (p == "margin") {
        // Parse shorthand: 1, 2, 3, or 4 values.
        std::istringstream ss(v); std::string tok;
        std::vector<Length> vals;
        while (ss >> tok) vals.push_back(parse_length(tok));
        if (vals.size() == 1) { for (int k=0;k<4;++k) cs.margin[k] = vals[0]; }
        else if (vals.size() == 2) { cs.margin[0]=cs.margin[2]=vals[0]; cs.margin[1]=cs.margin[3]=vals[1]; }
        else if (vals.size() == 3) { cs.margin[0]=vals[0]; cs.margin[1]=cs.margin[3]=vals[1]; cs.margin[2]=vals[2]; }
        else if (vals.size() >= 4) { for (int k=0;k<4;++k) cs.margin[k]=vals[k]; }
    } else if (p == "margin-top")    { cs.margin[0] = parse_length(v); }
    else if (p == "margin-right")    { cs.margin[1] = parse_length(v); }
    else if (p == "margin-bottom")   { cs.margin[2] = parse_length(v); }
    else if (p == "margin-left")     { cs.margin[3] = parse_length(v); }
    else if (p == "padding") {
        std::istringstream ss(v); std::string tok;
        std::vector<Length> vals;
        while (ss >> tok) vals.push_back(parse_length(tok));
        if (vals.size() == 1) { for (int k=0;k<4;++k) cs.padding[k] = vals[0]; }
        else if (vals.size() == 2) { cs.padding[0]=cs.padding[2]=vals[0]; cs.padding[1]=cs.padding[3]=vals[1]; }
        else if (vals.size() == 3) { cs.padding[0]=vals[0]; cs.padding[1]=cs.padding[3]=vals[1]; cs.padding[2]=vals[2]; }
        else if (vals.size() >= 4) { for (int k=0;k<4;++k) cs.padding[k]=vals[k]; }
    } else if (p == "padding-top")   { cs.padding[0] = parse_length(v); }
    else if (p == "padding-right")   { cs.padding[1] = parse_length(v); }
    else if (p == "padding-bottom")  { cs.padding[2] = parse_length(v); }
    else if (p == "padding-left")    { cs.padding[3] = parse_length(v); }
    else if (p == "color") { cs.color = parse_color(v); }
    else if (p == "background-color") { cs.background_color = parse_color(v); }
    else if (p == "background") {
        // Check if it contains url(...) for background-image.
        auto up = v.find("url(");
        if (up != std::string::npos) {
            auto ue = v.find(')', up);
            if (ue != std::string::npos) {
                std::string url = v.substr(up + 4, ue - up - 4);
                if (!url.empty() && (url.front() == '"' || url.front() == '\'')) url.erase(url.begin());
                if (!url.empty() && (url.back()  == '"' || url.back()  == '\'')) url.pop_back();
                cs.background_image = arena.strdup(url.c_str(), url.size());
            }
        }
        // Try to extract a color from the background shorthand.
        // Remove url(...) part and parse remainder as color.
        auto bg_no_url = v;
        if (up != std::string::npos) {
            auto ue2 = v.find(')', up);
            if (ue2 != std::string::npos) bg_no_url = v.substr(0, up) + v.substr(ue2+1);
        }
        // Trim and try color parse.
        while (!bg_no_url.empty() && std::isspace(static_cast<unsigned char>(bg_no_url.front()))) bg_no_url.erase(bg_no_url.begin());
        while (!bg_no_url.empty() && std::isspace(static_cast<unsigned char>(bg_no_url.back()))) bg_no_url.pop_back();
        if (!bg_no_url.empty()) {
            Color tc = parse_color(bg_no_url);
            if (tc.a > 0 || bg_no_url == "transparent") cs.background_color = tc;
        }
    } else if (p == "background-image") {
        if (v != "none") {
            auto up = v.find("url(");
            if (up != std::string::npos) {
                auto ue = v.find(')', up + 4);
                if (ue != std::string::npos) {
                    std::string url = v.substr(up + 4, ue - up - 4);
                    if (!url.empty() && (url.front() == '"' || url.front() == '\'')) url.erase(url.begin());
                    if (!url.empty() && (url.back()  == '"' || url.back()  == '\'')) url.pop_back();
                    cs.background_image = arena.strdup(url.c_str(), url.size());
                }
            }
        }
    }
    else if (p == "font-size") {
        if (v == "small") cs.font_size = 13.f;
        else if (v == "medium") cs.font_size = 16.f;
        else if (v == "large") cs.font_size = 18.f;
        else if (v == "x-large") cs.font_size = 24.f;
        else if (v == "xx-large") cs.font_size = 32.f;
        else if (v == "smaller") cs.font_size *= 0.833f;
        else if (v == "larger")  cs.font_size *= 1.2f;
        else {
            Length l = parse_length(v);
            if (l.unit == LengthUnit::PX)      cs.font_size = l.value;
            else if (l.unit == LengthUnit::EM)  cs.font_size *= l.value;
            else if (l.unit == LengthUnit::PERCENT) cs.font_size *= l.value / 100.f;
        }
    }
    else if (p == "font-weight") {
        if (v == "bold" || v == "700" || v == "800" || v == "900") cs.font_weight = FontWeight::BOLD;
        else cs.font_weight = FontWeight::NORMAL;
    }
    else if (p == "font-style") {
        if (v == "italic" || v == "oblique") cs.font_style = FontStyle::ITALIC;
    }
    else if (p == "font-family") {
        cs.font_family = arena.strdup(v.c_str(), v.size());
    }
    else if (p == "text-align") {
        if (v == "right")   cs.text_align = TextAlign::RIGHT;
        else if (v == "center") cs.text_align = TextAlign::CENTER;
        else if (v == "justify") cs.text_align = TextAlign::JUSTIFY;
        else cs.text_align = TextAlign::LEFT;
    }
    else if (p == "line-height") {
        try { cs.line_height = std::stof(v); } catch (...) {
            Length l = parse_length(v);
            if (l.unit == LengthUnit::PX) cs.line_height = l.value / cs.font_size;
        }
    }
    else if (p == "white-space") {
        if (v == "pre")       cs.white_space = WhiteSpace::PRE;
        else if (v == "pre-wrap") cs.white_space = WhiteSpace::PRE_WRAP;
        else if (v == "pre-line") cs.white_space = WhiteSpace::PRE_LINE;
        else if (v == "nowrap")   cs.white_space = WhiteSpace::NOWRAP;
        else                      cs.white_space = WhiteSpace::NORMAL;
    }
    else if (p == "overflow") {
        Overflow ov = Overflow::VISIBLE;
        if (v == "hidden") ov = Overflow::HIDDEN;
        else if (v == "scroll") ov = Overflow::SCROLL;
        else if (v == "auto")   ov = Overflow::AUTO;
        cs.overflow_x = cs.overflow_y = ov;
    }
    else if (p == "overflow-x") {
        if (v == "hidden") cs.overflow_x = Overflow::HIDDEN;
        else if (v == "scroll") cs.overflow_x = Overflow::SCROLL;
        else if (v == "auto")   cs.overflow_x = Overflow::AUTO;
        else cs.overflow_x = Overflow::VISIBLE;
    }
    else if (p == "overflow-y") {
        if (v == "hidden") cs.overflow_y = Overflow::HIDDEN;
        else if (v == "scroll") cs.overflow_y = Overflow::SCROLL;
        else if (v == "auto")   cs.overflow_y = Overflow::AUTO;
        else cs.overflow_y = Overflow::VISIBLE;
    }
    else if (p == "opacity") {
        try { cs.opacity = std::clamp(std::stof(v), 0.f, 1.f); } catch (...) {}
    }
    else if (p == "visibility") {
        cs.visible = (v != "hidden" && v != "collapse");
    }
    else if (p == "z-index") {
        try { cs.z_index = std::stoi(v); } catch (...) {}
    }
    else if (p == "box-sizing") {
        cs.box_sizing = (v == "border-box") ? BoxSizing::BORDER_BOX : BoxSizing::CONTENT_BOX;
    }
    // Flex properties.
    else if (p == "flex-direction") {
        if (v == "column")         cs.flex_direction = FlexDir::COLUMN;
        else if (v == "row-reverse") cs.flex_direction = FlexDir::ROW_REV;
        else if (v == "column-reverse") cs.flex_direction = FlexDir::COLUMN_REV;
        else cs.flex_direction = FlexDir::ROW;
    }
    else if (p == "flex-wrap") {
        if (v == "wrap") cs.flex_wrap = FlexWrap::WRAP;
        else if (v == "wrap-reverse") cs.flex_wrap = FlexWrap::WRAP_REV;
        else cs.flex_wrap = FlexWrap::NOWRAP;
    }
    else if (p == "justify-content") {
        if (v == "flex-end" || v == "end")   cs.justify_content = JustContent::FLEX_END;
        else if (v == "center")              cs.justify_content = JustContent::CENTER;
        else if (v == "space-between")       cs.justify_content = JustContent::SPACE_BETWEEN;
        else if (v == "space-around")        cs.justify_content = JustContent::SPACE_AROUND;
        else if (v == "space-evenly")        cs.justify_content = JustContent::SPACE_EVENLY;
        else cs.justify_content = JustContent::FLEX_START;
    }
    else if (p == "align-items") {
        if (v == "flex-end" || v == "end") cs.align_items = AlignItems::FLEX_END;
        else if (v == "center")            cs.align_items = AlignItems::CENTER;
        else if (v == "flex-start" || v == "start") cs.align_items = AlignItems::FLEX_START;
        else if (v == "baseline")          cs.align_items = AlignItems::BASELINE;
        else cs.align_items = AlignItems::STRETCH;
    }
    else if (p == "flex-grow")   { try { cs.flex_grow   = std::stof(v); } catch (...) {} }
    else if (p == "flex-shrink") { try { cs.flex_shrink = std::stof(v); } catch (...) {} }
    else if (p == "flex-basis")  { cs.flex_basis = parse_length(v); }
    else if (p == "flex") {
        // Shorthand: flex: grow shrink basis | flex: auto | flex: none | flex: 1
        if (v == "auto") { cs.flex_grow=1; cs.flex_shrink=1; cs.flex_basis={0,LengthUnit::AUTO}; }
        else if (v == "none") { cs.flex_grow=0; cs.flex_shrink=0; cs.flex_basis={0,LengthUnit::AUTO}; }
        else {
            std::istringstream ss(v); std::string tok;
            int n = 0;
            while (ss >> tok) {
                if (n == 0) { try { cs.flex_grow   = std::stof(tok); } catch (...) {} }
                if (n == 1) { try { cs.flex_shrink  = std::stof(tok); } catch (...) {} }
                if (n == 2) { cs.flex_basis = parse_length(tok); }
                ++n;
            }
            if (n == 1) { cs.flex_shrink = 1; cs.flex_basis = {0, LengthUnit::AUTO}; }
        }
    }
    // Border (simplified)
    else if (p == "border") {
        // Rough parse: "1px solid #ccc"
        std::istringstream ss(v); std::string tok;
        while (ss >> tok) {
            Length l = parse_length(tok);
            if (l.unit == LengthUnit::PX && l.value >= 0.f && l.value <= 50.f && tok != "solid" && tok != "dashed" && tok != "dotted" && tok != "none") {
                for (int k=0;k<4;++k) cs.border_width[k] = l.value;
                for (int k=0;k<4;++k) cs.border_style[k] = 1; // solid default
            } else if (tok == "solid") { for (int k=0;k<4;++k) cs.border_style[k] = 1; }
            else if (tok == "dashed") { for (int k=0;k<4;++k) cs.border_style[k] = 2; }
            else if (tok == "dotted") { for (int k=0;k<4;++k) cs.border_style[k] = 3; }
            else if (tok == "none")   { for (int k=0;k<4;++k) { cs.border_width[k]=0; cs.border_style[k]=0; } }
            else {
                Color bc = parse_color(tok);
                for (int k=0;k<4;++k) cs.border_color[k] = bc;
            }
        }
    }
    else if (p == "border-color") {
        Color bc = parse_color(v);
        for (int k=0;k<4;++k) cs.border_color[k] = bc;
    }
    else if (p == "border-width") {
        Length l = parse_length(v);
        float w = (l.unit == LengthUnit::PX) ? l.value : 0.f;
        for (int k=0;k<4;++k) cs.border_width[k] = w;
    }
    else if (p == "border-radius") {
        Length l = parse_length(v);
        float r = (l.unit == LengthUnit::PX) ? l.value : 0.f;
        for (int k=0;k<4;++k) cs.border_radius[k] = r;
    }
    else if (p == "left")   { cs.left   = parse_length(v); }
    else if (p == "right")  { cs.right  = parse_length(v); }
    else if (p == "top")    { cs.top    = parse_length(v); }
    else if (p == "bottom") { cs.bottom = parse_length(v); }
    else if (p == "transform") {
        cs.transform = arena.strdup(v.c_str(), v.size());
    }
    else if (p == "list-style" || p == "list-style-type") {
        cs.list_style_none = (v == "none");
    }
    // Ignored but common -- avoid warning: content, cursor, transition, animation, etc.
}

// =========================================================================
// Inherit inheritable properties from parent.
// =========================================================================
static void inherit_from(ComputedStyle& cs, const ComputedStyle& parent) {
    cs.color        = parent.color;
    cs.font_size    = parent.font_size;
    cs.font_weight  = parent.font_weight;
    cs.font_style   = parent.font_style;
    cs.font_family  = parent.font_family;
    cs.text_align   = parent.text_align;
    cs.line_height  = parent.line_height;
    cs.letter_spacing = parent.letter_spacing;
    cs.white_space  = parent.white_space;
    cs.visible      = parent.visible;
    cs.list_style_none = parent.list_style_none;
}

// =========================================================================
// Default UA stylesheet rules.
// =========================================================================
static const char* UA_CSS = R"(
html, body { display: block; }
head, script, style, link, meta, title { display: none; }
div, p, section, article, main, header, footer, nav, aside,
h1, h2, h3, h4, h5, h6, ul, ol, dl, pre, blockquote,
figure, figcaption, form, fieldset, address, details, summary { display: block; }
span, a, em, strong, b, i, u, s, code, small, sup, sub,
abbr, cite, q, mark, time, var, kbd, samp, label, button,
img, input, select, textarea { display: inline; }
table   { display: table; }
tr      { display: table-row; }
td, th  { display: table-cell; }
li      { display: list-item; }
h1 { font-size: 32px; font-weight: bold; margin-top: 16px; margin-bottom: 16px; }
h2 { font-size: 24px; font-weight: bold; margin-top: 14px; margin-bottom: 14px; }
h3 { font-size: 18.72px; font-weight: bold; margin-top: 12px; margin-bottom: 12px; }
h4 { font-size: 16px; font-weight: bold; margin-top: 10px; margin-bottom: 10px; }
h5 { font-size: 13.28px; font-weight: bold; margin-top: 8px; margin-bottom: 8px; }
h6 { font-size: 10.72px; font-weight: bold; margin-top: 8px; margin-bottom: 8px; }
p  { margin-top: 8px; margin-bottom: 8px; }
ul, ol { margin-left: 40px; padding-left: 0; }
strong, b { font-weight: bold; }
em, i { font-style: italic; }
a  { color: #0000ee; }
pre, code { font-family: monospace; white-space: pre; }
body { margin: 8px; font-size: 16px; }
)";

// =========================================================================
// resolve_styles  --  main entry
// =========================================================================

static void resolve_node(Node* node,
                         const ComputedStyle* parent_style,
                         const std::vector<CssRule>& all_rules,
                         Arena& arena,
                         float vw, float vh,
                         float root_font_px)
{
    if (node->kind == NodeKind::TEXT || node->kind == NodeKind::COMMENT ||
        node->kind == NodeKind::DOCTYPE) {
        // Text nodes inherit from parent.
        if (parent_style) {
            ComputedStyle* cs = arena.make<ComputedStyle>();
            inherit_from(*cs, *parent_style);
            node->computed_style = cs;
        }
        // Recurse (text nodes have no children but keep consistent).
        return;
    }
    if (node->kind == NodeKind::DOCUMENT) {
        ComputedStyle* cs = arena.make<ComputedStyle>();
        node->computed_style = cs;
        for (Node* ch = node->first_child; ch; ch = ch->next_sibling)
            resolve_node(ch, cs, all_rules, arena, vw, vh, root_font_px);
        return;
    }

    // ELEMENT: compute style.
    ComputedStyle* cs = arena.make<ComputedStyle>();
    // Start with inherited values from parent.
    if (parent_style) inherit_from(*cs, *parent_style);

    // Collect matching rules, sorted by (source_index desc, specificity asc, order asc).
    struct Match { int source; Specificity spec; int order; const CssRule* rule; };
    std::vector<Match> matches;
    for (const auto& rule : all_rules) {
        if (selector_matches(node, rule.selector)) {
            matches.push_back({0, rule.specificity, rule.source_order, &rule});
        }
    }
    std::stable_sort(matches.begin(), matches.end(),
        [](const Match& a, const Match& b) {
            if (a.source  != b.source)  return a.source  < b.source;
            if (a.spec    != b.spec)    return a.spec    < b.spec;
            return a.order < b.order;
        });

    // Apply non-important first.
    for (const auto& m : matches) {
        for (const auto& d : m.rule->decls) {
            if (!d.important) apply_decl(*cs, d, arena, vw, vh);
        }
    }
    // Apply inline style.
    const char* inline_s = node->attr("style");
    if (inline_s) {
        auto idecls = parse_inline_style(inline_s, std::strlen(inline_s));
        for (const auto& d : idecls) {
            if (!d.important) apply_decl(*cs, d, arena, vw, vh);
        }
    }
    // Apply important declarations.
    for (const auto& m : matches) {
        for (const auto& d : m.rule->decls) {
            if (d.important) apply_decl(*cs, d, arena, vw, vh);
        }
    }
    if (inline_s) {
        auto idecls = parse_inline_style(inline_s, std::strlen(inline_s));
        for (const auto& d : idecls) {
            if (d.important) apply_decl(*cs, d, arena, vw, vh);
        }
    }

    node->computed_style = cs;

    // Recurse into children.
    for (Node* ch = node->first_child; ch; ch = ch->next_sibling)
        resolve_node(ch, cs, all_rules, arena, vw, vh, root_font_px);
}

void resolve_styles(Document* doc,
                    const std::vector<StyleSheet>& author_sheets,
                    float viewport_width,
                    float viewport_height)
{
    // Build merged rule list: UA first, then author.
    StyleSheet ua = parse_css(UA_CSS, std::strlen(UA_CSS), 0);
    std::vector<CssRule> all_rules;
    for (auto& r : ua.rules) all_rules.push_back(r);
    for (const auto& sh : author_sheets)
        for (const auto& r : sh.rules) all_rules.push_back(r);

    resolve_node(doc->root, nullptr, all_rules, doc->arena,
                 viewport_width, viewport_height, 16.f);
}

} // namespace xcm
