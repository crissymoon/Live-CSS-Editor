/*
 * css_parser.cpp  --  CSS tokenizer + rule tree parser
 */

#include "css_parser.h"
#include <cctype>
#include <algorithm>
#include <sstream>

namespace xcm {

// -------------------------------------------------------------------------
// Tokenizer helpers
// -------------------------------------------------------------------------
static void skip_ws(const char* s, std::size_t len, std::size_t& i) {
    while (i < len && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
}

static std::string to_lower_str(std::string_view sv) {
    std::string out(sv);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// -------------------------------------------------------------------------
// Low-level tokeniser: extract string inside {} block
// Returns position after closing '}'.
// -------------------------------------------------------------------------
static std::string read_block(const char* s, std::size_t len, std::size_t& i) {
    // i should be positioned at '{' -- skip it.
    if (i < len && s[i] == '{') ++i;
    std::string block;
    int depth = 1;
    while (i < len && depth > 0) {
        char c = s[i++];
        if (c == '{') { ++depth; block += c; }
        else if (c == '}') { --depth; if (depth > 0) block += c; }
        else if (c == '\'' || c == '"') {
            // String -- pass through quotes.
            char q = c; block += c;
            while (i < len && s[i] != q) {
                if (s[i] == '\\') { block += s[i++]; }
                if (i < len) block += s[i++];
            }
            if (i < len) { block += s[i++]; } // closing quote
        }
        else { block += c; }
    }
    return block;
}

// -------------------------------------------------------------------------
// Parse declarations from a block string (content between { }).
// -------------------------------------------------------------------------
std::vector<CssDecl> parse_declarations(const char* s, std::size_t len) {
    std::vector<CssDecl> decls;
    std::size_t i = 0;
    while (i < len) {
        skip_ws(s, len, i);
        if (i >= len) break;
        // Read property name.
        std::size_t name_start = i;
        while (i < len && s[i] != ':' && s[i] != ';' && s[i] != '}') ++i;
        std::string prop = to_lower_str(std::string_view(s + name_start, i - name_start));
        // Trim trailing ws.
        while (!prop.empty() && std::isspace(static_cast<unsigned char>(prop.back())))
            prop.pop_back();
        if (i >= len || s[i] != ':') { ++i; continue; }
        ++i; // skip ':'
        // Read value until ';' or '}', respecting parens and strings.
        skip_ws(s, len, i);
        std::size_t val_start = i;
        int paren_depth = 0;
        bool in_str = false; char str_char = 0;
        while (i < len) {
            char c = s[i];
            if (in_str) {
                if (c == str_char) in_str = false;
                else if (c == '\\') ++i;
            } else if (c == '\'' || c == '"') {
                in_str = true; str_char = c;
            } else if (c == '(') {
                ++paren_depth;
            } else if (c == ')') {
                --paren_depth;
            } else if ((c == ';' || c == '}') && paren_depth == 0) {
                break;
            }
            ++i;
        }
        std::string val(s + val_start, i - val_start);
        // Trim trailing ws.
        while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back())))
            val.pop_back();
        bool important = false;
        // Check for !important.
        std::size_t ii = val.rfind("!important");
        if (ii != std::string::npos) {
            important = true;
            val = val.substr(0, ii);
            while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back())))
                val.pop_back();
        }
        if (!prop.empty() && !val.empty()) {
            decls.push_back({prop, val, important});
        }
        if (i < len && s[i] == ';') ++i;
    }
    return decls;
}

// -------------------------------------------------------------------------
// Specificity computation
// Selector format: "tag.class#id[attr]:pseudo ::pseudo-element"
// -------------------------------------------------------------------------
Specificity compute_specificity(const std::string& selector) {
    uint8_t a = 0, b = 0, c = 0; // id, class/attr/pseudo-class, element/pseudo-elem
    for (std::size_t i = 0; i < selector.size(); ) {
        char ch = selector[i];
        if (ch == '#') {
            ++a; ++i;
            while (i < selector.size() && (std::isalnum(static_cast<unsigned char>(selector[i])) || selector[i] == '-' || selector[i] == '_')) ++i;
        } else if (ch == '.') {
            ++b; ++i;
            while (i < selector.size() && (std::isalnum(static_cast<unsigned char>(selector[i])) || selector[i] == '-' || selector[i] == '_')) ++i;
        } else if (ch == '[') {
            ++b;
            while (i < selector.size() && selector[i] != ']') ++i;
            ++i;
        } else if (ch == ':') {
            ++i;
            if (i < selector.size() && selector[i] == ':') {
                // pseudo-element
                ++c; ++i;
            } else {
                // pseudo-class
                ++b;
            }
            while (i < selector.size() && (std::isalnum(static_cast<unsigned char>(selector[i])) || selector[i] == '-')) ++i;
        } else if (ch == '*' || ch == '>' || ch == '+' || ch == '~' || ch == ' ' || ch == ',') {
            ++i;
        } else if (std::isalpha(static_cast<unsigned char>(ch))) {
            ++c;
            while (i < selector.size() && (std::isalnum(static_cast<unsigned char>(selector[i])) || selector[i] == '-')) ++i;
        } else {
            ++i;
        }
    }
    return spec(a, b, c);
}

// -------------------------------------------------------------------------
// Main CSS parser
// -------------------------------------------------------------------------
StyleSheet parse_css(const char* css, std::size_t len, int source_index) {
    StyleSheet sheet;
    sheet.source_index = source_index;
    std::size_t i = 0;
    int rule_order = 0;

    while (i < len) {
        skip_ws(css, len, i);
        if (i >= len) break;

        // Skip comments: /* ... */
        if (i + 1 < len && css[i] == '/' && css[i+1] == '*') {
            i += 2;
            while (i + 1 < len && !(css[i] == '*' && css[i+1] == '/')) ++i;
            i += 2;
            continue;
        }

        // At-rules
        if (css[i] == '@') {
            ++i;
            // Read at-keyword.
            std::string at_kw;
            while (i < len && std::isalpha(static_cast<unsigned char>(css[i])))
                at_kw += static_cast<char>(std::tolower(static_cast<unsigned char>(css[i++])));
            skip_ws(css, len, i);

            if (at_kw == "import") {
                // Skip to ;
                while (i < len && css[i] != ';') ++i;
                ++i;
                continue;
            }
            if (at_kw == "charset") {
                while (i < len && css[i] != ';') ++i;
                ++i;
                continue;
            }
            if (at_kw == "keyframes" || at_kw == "-webkit-keyframes" ||
                at_kw == "-moz-keyframes") {
                // Skip the name then consume the block.
                while (i < len && css[i] != '{') ++i;
                if (i < len) read_block(css, len, i);
                continue;
            }
            if (at_kw == "media" || at_kw == "supports" || at_kw == "layer") {
                // Read the condition (everything up to '{').
                while (i < len && css[i] != '{') ++i;
                // Recurse into the block.
                std::string inner = read_block(css, len, i);
                StyleSheet inner_sheet = parse_css(inner.c_str(), inner.size(), source_index);
                for (auto& r : inner_sheet.rules) sheet.rules.push_back(std::move(r));
                continue;
            }
            // Unknown at-rule with block -- skip it.
            if (i < len && css[i] == '{') { read_block(css, len, i); continue; }
            while (i < len && css[i] != ';') ++i;
            if (i < len) ++i;
            continue;
        }

        // Read selector(s) up to '{'.
        std::string selector_str;
        while (i < len && css[i] != '{') {
            // Skip comments within selector.
            if (i + 1 < len && css[i] == '/' && css[i+1] == '*') {
                i += 2;
                while (i + 1 < len && !(css[i] == '*' && css[i+1] == '/')) ++i;
                i += 2;
                continue;
            }
            selector_str += css[i++];
        }
        if (i >= len) break;
        // Read declarations block.
        std::string block = read_block(css, len, i);
        auto decls = parse_declarations(block.c_str(), block.size());
        if (decls.empty()) continue;

        // Split comma-separated selectors.
        std::istringstream ss(selector_str);
        std::string sel_part;
        while (std::getline(ss, sel_part, ',')) {
            // Trim.
            std::size_t s0 = sel_part.find_first_not_of(" \t\n\r");
            std::size_t s1 = sel_part.find_last_not_of(" \t\n\r");
            if (s0 == std::string::npos) continue;
            std::string sel = sel_part.substr(s0, s1 - s0 + 1);
            if (sel.empty()) continue;

            CssRule rule;
            rule.selector    = sel;
            rule.specificity = compute_specificity(sel);
            rule.decls       = decls;
            rule.source_order = rule_order++;
            sheet.rules.push_back(std::move(rule));
        }
    }
    return sheet;
}

// -------------------------------------------------------------------------
// Inline style parser (no selector, just declarations)
// -------------------------------------------------------------------------
std::vector<CssDecl> parse_inline_style(const char* style, std::size_t len) {
    return parse_declarations(style, len);
}

} // namespace xcm
