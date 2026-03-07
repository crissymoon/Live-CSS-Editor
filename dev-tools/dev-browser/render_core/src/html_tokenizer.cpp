/*
 * html_tokenizer.cpp  --  HTML5 tokenizer + tree construction
 *
 * State machine following the HTML5 parsing spec (simplified).
 * Handles: start tags, end tags, attributes, text nodes, comments,
 * DOCTYPE, <script>/<style>/<textarea> raw text, self-closing void elements,
 * implied open/close for p, li, dt, dd, and the html/head/body frame.
 */

#include "html_tokenizer.h"
#include <cctype>
#include <cstring>
#include <algorithm>
#include <unordered_set>

namespace xcm {

// -------------------------------------------------------------------------
// Void elements (self-closing, cannot have children)
// -------------------------------------------------------------------------
static const std::unordered_set<std::string_view> VOID_ELEMENTS = {
    "area","base","br","col","embed","hr","img","input","link",
    "meta","param","source","track","wbr",
};

// Elements whose open implies closing a matching ancestor.
static const std::unordered_set<std::string_view> PARA_ELEMS = {
    "address","article","aside","blockquote","details","div","dl",
    "fieldset","figcaption","figure","footer","h1","h2","h3","h4",
    "h5","h6","header","hgroup","hr","main","menu","nav","ol","p",
    "pre","section","summary","table","ul",
};

static const std::unordered_set<std::string_view> RAW_TEXT_ELEMENTS = {
    "script","style","textarea","title",
};

// -------------------------------------------------------------------------
// Simple in-place ASCII lower-case
// -------------------------------------------------------------------------
static std::string to_lower(const char* s, std::size_t len) {
    std::string out(s, len);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// -------------------------------------------------------------------------
// Tokenizer
// -------------------------------------------------------------------------
enum class TokState {
    DATA,
    TAG_OPEN,
    END_TAG_OPEN,
    TAG_NAME,
    END_TAG_NAME,
    BEFORE_ATTR_NAME,
    ATTR_NAME,
    AFTER_ATTR_NAME,
    BEFORE_ATTR_VAL,
    ATTR_VAL_DOUBLE,
    ATTR_VAL_SINGLE,
    ATTR_VAL_UNQUOTED,
    AFTER_ATTR_VAL,
    SELF_CLOSING,
    COMMENT_START,
    COMMENT,
    COMMENT_END_DASH,
    COMMENT_END,
    DOCTYPE,
    RAW_TEXT,
    RAW_TEXT_END_TAG_OPEN,
    RAW_TEXT_END_TAG_NAME,
    CHAR_REF,
};

// Pending token types.
enum class TType { START_TAG, END_TAG, COMMENT, DOCTYPE, TEXT, EOF_TOK };

struct Token {
    TType ttype = TType::TEXT;
    std::string tag;  // lower-cased
    std::vector<std::pair<std::string,std::string>> attrs;
    std::string text;
    bool self_closing = false;

    void reset_tag(TType tt) {
        ttype = tt; tag.clear(); attrs.clear();
        text.clear(); self_closing = false;
    }
};

// -------------------------------------------------------------------------
// Tokenize + Build tree in one pass
// -------------------------------------------------------------------------
struct Parser {
    const char*  src;
    std::size_t  src_len;
    std::size_t  pos = 0;

    Document*    doc;
    TokState     state = TokState::DATA;

    Token        tok;
    std::string  text_buf;       // accumulates DATA-state text
    std::string  raw_text_buf;   // RAW_TEXT accumulator
    std::string  raw_end_tag;    // which tag we're looking for to close raw text
    std::string  current_raw_tag;// tag that opened the raw-text section

    std::string  cur_attr_name;
    std::string  cur_attr_val;

    // Open element stack (foster parenting happens implicitly here).
    std::vector<Node*> open_stack;

    Node* current_node() {
        return open_stack.empty() ? doc->root : open_stack.back();
    }

    // -----------------------------------------------------------------------
    void flush_text() {
        if (text_buf.empty()) return;
        // Skip if only whitespace when directly inside <head>.
        bool only_ws = true;
        for (char c : text_buf) if (c != ' ' && c != '\t' && c != '\n' && c != '\r') { only_ws = false; break; }

        if (!only_ws || (current_node()->kind == NodeKind::ELEMENT &&
                         std::string_view(current_node()->tag) != "head")) {
            Node* tn = doc->make_text(text_buf.c_str(), text_buf.size());
            current_node()->append_child(doc->arena, tn);
        }
        text_buf.clear();
    }

    // -----------------------------------------------------------------------
    void maybe_close_p_for(std::string_view tag) {
        if (PARA_ELEMS.count(tag)) {
            // Close any open <p> first.
            for (int i = static_cast<int>(open_stack.size()) - 1; i >= 0; --i) {
                if (std::string_view(open_stack[i]->tag) == "p") {
                    open_stack.erase(open_stack.begin() + i,
                                     open_stack.end());
                    break;
                }
                // Stop at formatting / non-block context.
                auto t = std::string_view(open_stack[i]->tag);
                if (t == "div" || t == "body" || t == "html") break;
            }
        }
    }

    // -----------------------------------------------------------------------
    void push_attr() {
        if (!cur_attr_name.empty()) {
            tok.attrs.emplace_back(std::move(cur_attr_name), std::move(cur_attr_val));
            cur_attr_name.clear(); cur_attr_val.clear();
        }
    }

    // -----------------------------------------------------------------------
    void emit_token() {
        if (tok.ttype == TType::START_TAG) {
            flush_text();

            std::string_view tv = tok.tag;
            maybe_close_p_for(tv);

            // Ensure html/head/body exist.
            if (tv != "html" && !doc->html) ensure_frame("html");
            if (tv != "head" && tv != "body" && !doc->body) {
                if (tv == "title" || tv == "meta" || tv == "link" ||
                    tv == "style" || tv == "script" || tv == "base") {
                    ensure_frame("head");
                } else {
                    ensure_head_closed();
                    ensure_frame("body");
                }
            }

            Node* el = doc->make_element(tok.tag.c_str(), tok.tag.size());
            // Copy attributes.
            for (auto& [k,v] : tok.attrs) {
                Attr a;
                a.name  = doc->arena.strdup(k.c_str(), k.size());
                a.value = doc->arena.strdup(v.c_str(), v.size());
                el->attrs.push(doc->arena, a);
            }

            current_node()->append_child(doc->arena, el);

            if (!VOID_ELEMENTS.count(tv) && !tok.self_closing) {
                open_stack.push_back(el);
                if (tv == "html") doc->html = el;
                else if (tv == "head") doc->head = el;
                else if (tv == "body") doc->body = el;

                if (RAW_TEXT_ELEMENTS.count(tv)) {
                    current_raw_tag = std::string(tv);
                    raw_text_buf.clear();
                    raw_end_tag = "</" + current_raw_tag;
                    state = TokState::RAW_TEXT;
                    return;
                }
            }
        } else if (tok.ttype == TType::END_TAG) {
            flush_text();
            std::string_view tv = tok.tag;
            // Pop open stack until we find matching tag.
            for (int i = static_cast<int>(open_stack.size()) - 1; i >= 0; --i) {
                if (std::string_view(open_stack[i]->tag) == tv) {
                    open_stack.erase(open_stack.begin() + i, open_stack.end());
                    break;
                }
            }
        } else if (tok.ttype == TType::COMMENT) {
            flush_text();
            Node* cn = doc->make_comment(tok.text.c_str(), tok.text.size());
            current_node()->append_child(doc->arena, cn);
        }
        // DOCTYPE and EOF: nothing extra to do.
    }

    // -----------------------------------------------------------------------
    void ensure_frame(const char* tag_name) {
        std::string_view tv{tag_name};
        for (auto* n : open_stack) if (std::string_view(n->tag) == tv) return;

        Node* el = doc->make_element(tag_name, std::strlen(tag_name));
        current_node()->append_child(doc->arena, el);
        open_stack.push_back(el);
        if (tv == "html") doc->html = el;
        else if (tv == "head") doc->head = el;
        else if (tv == "body") doc->body = el;
    }

    void ensure_head_closed() {
        if (!doc->head) return;
        for (int i = static_cast<int>(open_stack.size()) - 1; i >= 0; --i) {
            if (std::string_view(open_stack[i]->tag) == "head") {
                open_stack.erase(open_stack.begin() + i, open_stack.end());
                return;
            }
        }
    }

    // -----------------------------------------------------------------------
    // Main tokenize loop
    // -----------------------------------------------------------------------
    void run() {
        while (pos <= src_len) {
            char c = (pos < src_len) ? src[pos] : '\0';

            switch (state) {

            case TokState::DATA:
                if (pos == src_len) { flush_text(); return; }
                if (c == '<') {
                    state = TokState::TAG_OPEN;
                } else if (c == '&') {
                    text_buf += decode_char_ref();
                    continue; // pos already advanced inside helper
                } else {
                    text_buf += c;
                }
                break;

            case TokState::TAG_OPEN:
                if (c == '/') {
                    state = TokState::END_TAG_OPEN;
                } else if (c == '!') {
                    // Could be comment or DOCTYPE.
                    state = TokState::COMMENT_START;
                } else if (c == '?') {
                    // Processing instruction -- treat as comment.
                    state = TokState::COMMENT;
                    tok.reset_tag(TType::COMMENT);
                } else if (std::isalpha(static_cast<unsigned char>(c))) {
                    tok.reset_tag(TType::START_TAG);
                    tok.tag += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    state = TokState::TAG_NAME;
                } else {
                    // Bogus < -- output as text.
                    text_buf += '<';
                    state = TokState::DATA;
                    continue;
                }
                break;

            case TokState::END_TAG_OPEN:
                if (std::isalpha(static_cast<unsigned char>(c))) {
                    tok.reset_tag(TType::END_TAG);
                    tok.tag += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    state = TokState::END_TAG_NAME;
                } else if (c == '>') {
                    state = TokState::DATA;
                } else {
                    state = TokState::COMMENT;
                    tok.reset_tag(TType::COMMENT);
                }
                break;

            case TokState::TAG_NAME:
                if (c == '>') {
                    emit_token(); state = TokState::DATA;
                } else if (c == '/') {
                    tok.self_closing = true; state = TokState::SELF_CLOSING;
                } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    state = TokState::BEFORE_ATTR_NAME;
                } else {
                    tok.tag += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                break;

            case TokState::END_TAG_NAME:
                if (c == '>') {
                    emit_token(); state = TokState::DATA;
                } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    state = TokState::BEFORE_ATTR_NAME;
                } else {
                    tok.tag += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                break;

            case TokState::BEFORE_ATTR_NAME:
                if (c == '>') {
                    emit_token(); state = TokState::DATA;
                } else if (c == '/') {
                    tok.self_closing = true; state = TokState::SELF_CLOSING;
                } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    push_attr();
                    cur_attr_name.clear();
                    cur_attr_val.clear();
                    cur_attr_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    state = TokState::ATTR_NAME;
                }
                break;

            case TokState::ATTR_NAME:
                if (c == '=') {
                    state = TokState::BEFORE_ATTR_VAL;
                } else if (c == '>') {
                    push_attr(); emit_token(); state = TokState::DATA;
                } else if (c == '/') {
                    push_attr(); tok.self_closing = true; state = TokState::SELF_CLOSING;
                } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    state = TokState::AFTER_ATTR_NAME;
                } else {
                    cur_attr_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                break;

            case TokState::AFTER_ATTR_NAME:
                if (c == '=') {
                    state = TokState::BEFORE_ATTR_VAL;
                } else if (c == '>') {
                    push_attr(); emit_token(); state = TokState::DATA;
                } else if (c == '/') {
                    push_attr(); tok.self_closing = true; state = TokState::SELF_CLOSING;
                } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    push_attr();
                    cur_attr_name.clear(); cur_attr_val.clear();
                    cur_attr_name += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    state = TokState::ATTR_NAME;
                }
                break;

            case TokState::BEFORE_ATTR_VAL:
                if (c == '"') {
                    state = TokState::ATTR_VAL_DOUBLE;
                } else if (c == '\'') {
                    state = TokState::ATTR_VAL_SINGLE;
                } else if (c == '>') {
                    push_attr(); emit_token(); state = TokState::DATA;
                } else if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    cur_attr_val += c;
                    state = TokState::ATTR_VAL_UNQUOTED;
                }
                break;

            case TokState::ATTR_VAL_DOUBLE:
                if (c == '"') { state = TokState::AFTER_ATTR_VAL; }
                else if (c == '&') { cur_attr_val += decode_char_ref(); continue; }
                else { cur_attr_val += c; }
                break;

            case TokState::ATTR_VAL_SINGLE:
                if (c == '\'') { state = TokState::AFTER_ATTR_VAL; }
                else if (c == '&') { cur_attr_val += decode_char_ref(); continue; }
                else { cur_attr_val += c; }
                break;

            case TokState::ATTR_VAL_UNQUOTED:
                if (c == '>') { push_attr(); emit_token(); state = TokState::DATA; }
                else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    state = TokState::BEFORE_ATTR_NAME;
                }
                else if (c == '&') { cur_attr_val += decode_char_ref(); continue; }
                else { cur_attr_val += c; }
                break;

            case TokState::AFTER_ATTR_VAL:
                push_attr();
                if (c == '>') { emit_token(); state = TokState::DATA; }
                else if (c == '/') { tok.self_closing = true; state = TokState::SELF_CLOSING; }
                else { state = TokState::BEFORE_ATTR_NAME; continue; }
                break;

            case TokState::SELF_CLOSING:
                if (c == '>') { emit_token(); state = TokState::DATA; }
                else { state = TokState::BEFORE_ATTR_NAME; continue; }
                break;

            case TokState::COMMENT_START:
                // After <!
                tok.reset_tag(TType::COMMENT);
                if (c == '-' && pos + 1 < src_len && src[pos + 1] == '-') {
                    pos += 2;
                    state = TokState::COMMENT;
                    continue;
                } else {
                    // DOCTYPE or bogus.
                    std::string ahead;
                    for (int k = 0; k < 6 && pos + k < src_len; ++k)
                        ahead += static_cast<char>(std::tolower(static_cast<unsigned char>(src[pos + k])));
                    if (ahead == "doctyp") {
                        // Skip to >.
                        while (pos < src_len && src[pos] != '>') ++pos;
                        state = TokState::DATA;
                    } else if (ahead.substr(0, 7) == "[cdata[") {
                        pos += 7;
                        state = TokState::COMMENT;
                    } else {
                        state = TokState::COMMENT;
                        continue;
                    }
                }
                break;

            case TokState::COMMENT:
                if (c == '-') { state = TokState::COMMENT_END_DASH; }
                else if (c == '>' && tok.text.size() > 0 &&
                         tok.text.back() == '-') {
                    // bogus end
                    tok.text += c;
                } else if (pos == src_len) {
                    emit_token(); return;
                } else {
                    tok.text += c;
                }
                break;

            case TokState::COMMENT_END_DASH:
                if (c == '-') { state = TokState::COMMENT_END; }
                else { tok.text += '-'; tok.text += c; state = TokState::COMMENT; }
                break;

            case TokState::COMMENT_END:
                if (c == '>') { emit_token(); state = TokState::DATA; }
                else if (c == '-') { tok.text += '-'; }
                else { tok.text += '-'; tok.text += '-'; tok.text += c; state = TokState::COMMENT; }
                break;

            case TokState::RAW_TEXT:
                if (c == '<') {
                    state = TokState::RAW_TEXT_END_TAG_OPEN;
                } else if (pos == src_len) {
                    // EOF inside raw text -- emit as text child.
                    flush_raw_text();
                    return;
                } else {
                    raw_text_buf += c;
                }
                break;

            case TokState::RAW_TEXT_END_TAG_OPEN:
                if (c == '/') {
                    state = TokState::RAW_TEXT_END_TAG_NAME;
                } else {
                    raw_text_buf += '<';
                    raw_text_buf += c;
                    state = TokState::RAW_TEXT;
                }
                break;

            case TokState::RAW_TEXT_END_TAG_NAME: {
                // Accumulate until > or whitespace, then check tag name.
                std::string closing_tag;
                while (pos < src_len && src[pos] != '>' && src[pos] != ' ') {
                    closing_tag += static_cast<char>(std::tolower(static_cast<unsigned char>(src[pos])));
                    ++pos;
                }
                if (pos < src_len) {
                    while (pos < src_len && src[pos] != '>') ++pos;
                }
                if (closing_tag == current_raw_tag) {
                    flush_raw_text();
                    // Pop open stack.
                    for (int i = static_cast<int>(open_stack.size()) - 1; i >= 0; --i) {
                        if (std::string_view(open_stack[i]->tag) == current_raw_tag) {
                            open_stack.erase(open_stack.begin() + i, open_stack.end());
                            break;
                        }
                    }
                    state = TokState::DATA;
                } else {
                    raw_text_buf += "</" + closing_tag + ">";
                    state = TokState::RAW_TEXT;
                }
                break;
            }

            default:
                break;
            }
            ++pos;
        }
    }

    void flush_raw_text() {
        if (raw_text_buf.empty()) return;
        Node* tn = doc->make_text(raw_text_buf.c_str(), raw_text_buf.size());
        current_node()->append_child(doc->arena, tn);
        raw_text_buf.clear();
    }

    // Very small named character reference decoder (covers the most common ones).
    std::string decode_char_ref() {
        ++pos; // skip '&'
        if (pos >= src_len) return "&";
        std::string ref;
        bool numeric = false;
        bool hex     = false;
        if (src[pos] == '#') {
            numeric = true; ++pos;
            if (pos < src_len && (src[pos] == 'x' || src[pos] == 'X')) {
                hex = true; ++pos;
            }
        }
        while (pos < src_len && src[pos] != ';' && src[pos] != ' ' && src[pos] != '<') {
            ref += src[pos++];
        }
        if (pos < src_len && src[pos] == ';') ++pos;
        // Now decode.
        if (numeric) {
            unsigned long cp = 0;
            {
                char* endp = nullptr;
                cp = std::strtoul(ref.c_str(), &endp, hex ? 16 : 10);
                if (endp == ref.c_str()) return "&";
            }
            // Encode as UTF-8.
            std::string out;
            if (cp < 0x80) {
                out += static_cast<char>(cp);
            } else if (cp < 0x800) {
                out += static_cast<char>(0xC0 | (cp >> 6));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                out += static_cast<char>(0xE0 | (cp >> 12));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                out += static_cast<char>(0xF0 | (cp >> 18));
                out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            return out;
        }
        // Named references -- most common subset.
        struct NR { const char* name; const char* val; };
        static const NR NAMED[] = {
            {"amp","&"},{"lt","<"},{"gt",">"},{"quot","\""},{"apos","'"},
            {"nbsp","\xc2\xa0"},{"copy","\xc2\xa9"},{"reg","\xc2\xae"},
            {"trade","\xe2\x84\xa2"},{"mdash","\xe2\x80\x94"},
            {"ndash","\xe2\x80\x93"},{"laquo","\xc2\xab"},
            {"raquo","\xc2\xbb"},{"hellip","\xe2\x80\xa6"},
            {"euro","\xe2\x82\xac"},{"pound","\xc2\xa3"},
            {"yen","\xc2\xa5"},{"bull","\xe2\x80\xa2"},
            {nullptr,nullptr}
        };
        for (int i = 0; NAMED[i].name; ++i) {
            if (ref == NAMED[i].name) return NAMED[i].val;
        }
        return "&" + ref + ";";
    }
};

// -------------------------------------------------------------------------
// Public entry point
// -------------------------------------------------------------------------
Document* parse_html(const char* html, std::size_t len, Arena& arena) {
    Document* doc = arena.make<Document>(arena);

    Parser p;
    p.src     = html;
    p.src_len = len;
    p.doc     = doc;
    p.run();

    // Ensure at minimum: html > body exist.
    if (!doc->html) {
        Node* h = doc->make_element("html", 4);
        doc->root->append_child(doc->arena, h);
        doc->html = h;
    }
    if (!doc->body && doc->html) {
        if (!doc->head) {
            Node* hd = doc->make_element("head", 4);
            doc->html->append_child(doc->arena, hd);
            doc->head = hd;
        }
        Node* bd = doc->make_element("body", 4);
        doc->html->append_child(doc->arena, bd);
        doc->body = bd;
    }

    return doc;
}

} // namespace xcm
