#pragma once
/*
 * html_tokenizer.h  --  HTML5 tokenizer + tree constructor
 *
 * Implements a subset of the HTML5 parsing algorithm sufficient for
 * real-world pages:
 *   - All standard tokenizer states (DATA through CDATA)
 *   - Character references (named + numeric)
 *   - Optional/implied tag handling (html, head, body, p, li, td, ...)
 *   - <script>/<style>/<textarea> raw-text content
 *   - DOCTYPE processing
 *
 * Output: a fully constructed xcm::Document tree.
 */

#include <string>
#include <vector>
#include <string_view>
#include "dom.h"

namespace xcm {

Document* parse_html(const char* html, std::size_t len, Arena& arena);

} // namespace xcm
