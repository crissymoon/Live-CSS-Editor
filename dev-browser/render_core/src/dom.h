#pragma once
/*
 * dom.h  --  DOM node tree (Document Object Model)
 *
 * All nodes are allocated from a shared Arena owned by RenderContext.
 * No dynamic allocation outside the arena.
 *
 * Node kinds:
 *   DOCUMENT  --  root, always one per tree
 *   DOCTYPE
 *   ELEMENT   --  <div>, <p>, …
 *   TEXT      --  raw text content
 *   COMMENT   --  <!-- … -->
 *
 * Namespace is stored compactly as an enum; attributes use a flat ArenaVec
 * of Attr structs (name/value string_pair stored as arena-duped C-strings).
 */

#include <cstdint>
#include <cstring>
#include <string_view>
#include "arena.h"

namespace xcm {

// -------------------------------------------------------------------------
// Attr
// -------------------------------------------------------------------------
struct Attr {
    const char* name  = nullptr;
    const char* value = nullptr;

    std::string_view name_sv()  const { return name  ? name  : ""; }
    std::string_view value_sv() const { return value ? value : ""; }
};

// -------------------------------------------------------------------------
// Node kinds
// -------------------------------------------------------------------------
enum class NodeKind : uint8_t {
    DOCUMENT = 0,
    DOCTYPE,
    ELEMENT,
    TEXT,
    COMMENT,
};

// -------------------------------------------------------------------------
// Display / NS hint (used by layout engine; resolved during parsing)
// -------------------------------------------------------------------------
enum class Ns : uint8_t { HTML = 0, SVG, MATHML };

// -------------------------------------------------------------------------
// Node
// -------------------------------------------------------------------------
struct Node {
    NodeKind kind = NodeKind::DOCUMENT;
    Ns       ns   = Ns::HTML;

    // ELEMENT: tag name (e.g. "div"), already lower-cased by tokenizer.
    // TEXT/COMMENT: nullptr.
    const char* tag = nullptr;

    // Text content (TEXT and COMMENT nodes).
    const char* text = nullptr;

    // Attribute list (ELEMENT nodes only).
    ArenaVec<Attr> attrs;

    // Tree links.
    Node* parent       = nullptr;
    Node* first_child  = nullptr;
    Node* last_child   = nullptr;
    Node* next_sibling = nullptr;
    Node* prev_sibling = nullptr;

    // Computed style handle -- filled by StyleResolver.
    void* computed_style = nullptr;

    // Layout box handle -- filled by LayoutEngine.
    void* layout_box = nullptr;

    // -----------------------------------------------------------------------
    // Tree helpers
    // -----------------------------------------------------------------------
    void append_child(Arena& arena, Node* child) {
        child->parent = this;
        child->prev_sibling = last_child;
        child->next_sibling = nullptr;
        if (last_child) last_child->next_sibling = child;
        else            first_child = child;
        last_child = child;
        (void)arena; // arena not needed here but kept for symmetry
    }

    std::string_view tag_sv() const { return tag ? tag : ""; }

    const char* attr(std::string_view name) const {
        for (const Attr& a : attrs) {
            if (a.name_sv() == name) return a.value;
        }
        return nullptr;
    }

    bool has_class(std::string_view cls) const {
        const char* cv = attr("class");
        if (!cv) return false;
        std::string_view sv{cv};
        std::size_t pos = 0;
        while (pos < sv.size()) {
            // skip spaces
            while (pos < sv.size() && sv[pos] == ' ') ++pos;
            std::size_t end = pos;
            while (end < sv.size() && sv[end] != ' ') ++end;
            if (sv.substr(pos, end - pos) == cls) return true;
            pos = end;
        }
        return false;
    }

    bool has_id(std::string_view id) const {
        const char* iv = attr("id");
        return iv && std::string_view{iv} == id;
    }
};

// -------------------------------------------------------------------------
// Document (root of the parse tree)
// -------------------------------------------------------------------------
struct Document {
    Arena arena;
    Node* root = nullptr;   // DOCUMENT node
    Node* html = nullptr;   // <html> element
    Node* head = nullptr;   // <head> element
    Node* body = nullptr;   // <body> element

    Document() {
        root = arena.make<Node>();
        root->kind = NodeKind::DOCUMENT;
    }

    Node* make_element(const char* tag_name, std::size_t tag_len) {
        Node* n  = arena.make<Node>();
        n->kind  = NodeKind::ELEMENT;
        n->tag   = arena.strdup(tag_name, tag_len);
        return n;
    }

    Node* make_text(const char* text, std::size_t len) {
        Node* n  = arena.make<Node>();
        n->kind  = NodeKind::TEXT;
        n->text  = arena.strdup(text, len);
        return n;
    }

    Node* make_comment(const char* text, std::size_t len) {
        Node* n  = arena.make<Node>();
        n->kind  = NodeKind::COMMENT;
        n->text  = arena.strdup(text, len);
        return n;
    }
};

} // namespace xcm
