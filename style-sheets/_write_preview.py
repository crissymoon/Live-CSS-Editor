import os

code = r'''// CSS Preview Generator
// Reads a CSS stylesheet and generates a self-contained HTML preview
// showing every component and element defined in the stylesheet.
//
// Prefix-aware: detects the theme prefix from class names (e.g. "ng" for
// neon-grid, "aa" for atom-age, "" for clean-system) and uses it for all
// generated demo HTML. Works with any new theme the AI randomizer creates.
//
// Usage:
//   go run preview.go atom-age.css
//   go run preview.go               (all .css files in current directory)

package main

import (
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"strings"
)

// --------------------------------------------------------------------------
// Data structures
// --------------------------------------------------------------------------

type Selector struct {
	Raw       string
	ClassName string
	IsPseudo  bool
	IsAttr    bool // true for attribute-only selectors like [data-ng-tip]
}

type RuleBlock struct {
	Selector Selector
	Body     string
}

type Section struct {
	Title  string
	Blocks []RuleBlock
}

// ThemeInfo holds detected information about the current CSS theme.
type ThemeInfo struct {
	Prefix     string            // e.g. "ng", "aa", "" for clean-system
	PrefixDash string            // e.g. "ng-", "aa-", "" for clean-system
	AllClasses map[string]bool   // every class name found in the stylesheet
}

// p returns a prefixed class name: p("btn") -> "ng-btn" or "btn" (no prefix).
func (t ThemeInfo) p(name string) string {
	if t.Prefix == "" {
		return name
	}
	return t.Prefix + "-" + name
}

// cls finds the first class among the given component names that exists
// in the stylesheet. Falls back to prefixing the first name.
func (t ThemeInfo) cls(names ...string) string {
	for _, n := range names {
		full := t.p(n)
		if t.AllClasses[full] {
			return full
		}
	}
	return t.p(names[0])
}

// firstVariant returns the first color/accent variant of a component found
// in the CSS. For "btn" it might return "ng-btn-cyan" or "aa-btn-aqua".
// If no variant is found, returns the base class.
func (t ThemeInfo) firstVariant(component string) string {
	base := t.p(component) + "-"
	skip := map[string]bool{
		"sm": true, "lg": true, "xl": true, "group": true, "icon": true,
		"circle": true, "ghost": true, "outline": true, "square": true,
		"hard": true, "header": true, "title": true, "footer": true,
		"body": true, "close": true, "fill": true, "wrapper": true,
		"bar": true, "track": true, "thumb": true, "slider": true,
		"knob": true, "label": true, "head": true, "subtitle": true,
		"item": true, "sep": true, "end": true, "brand": true,
		"link": true, "active": true, "text": true, "content": true,
		"vertical": true, "v": true, "wrap": true, "striped": true,
		"row": true, "step": true, "steps": true, "error": true,
		"success": true, "hover": true, "pressed": true, "flat": true,
	}
	for cls := range t.AllClasses {
		if strings.HasPrefix(cls, base) {
			variant := lastSeg(cls)
			if !skip[variant] {
				return cls
			}
		}
	}
	return t.p(component)
}

// combo returns "base variant" if variant differs from base, else just base.
// Prevents duplicate classes like "ng-btn ng-btn".
func (t ThemeInfo) combo(base, variant string) string {
	if variant == base || variant == "" {
		return base
	}
	return base + " " + variant
}

func buildThemeInfo(sections []Section) ThemeInfo {
	prefix := detectPrefix(sections)
	allClasses := map[string]bool{}
	for _, s := range sections {
		for _, b := range s.Blocks {
			cn := b.Selector.ClassName
			if !strings.HasPrefix(cn, "[") && !strings.HasPrefix(cn, "@") {
				allClasses[cn] = true
			}
		}
	}
	dash := ""
	if prefix != "" {
		dash = prefix + "-"
	}
	return ThemeInfo{
		Prefix:     prefix,
		PrefixDash: dash,
		AllClasses: allClasses,
	}
}

// --------------------------------------------------------------------------
// Helpers: exact word-segment matching
// --------------------------------------------------------------------------

// seg splits a class name by hyphens.
func seg(cn string) []string { return strings.Split(cn, "-") }

// hasSeg checks whether a class name contains exactly the given hyphen-segment.
func hasSeg(cn, s string) bool {
	for _, p := range seg(cn) {
		if p == s {
			return true
		}
	}
	return false
}

// lastSeg returns the last hyphen segment.
func lastSeg(cn string) string {
	parts := seg(cn)
	return parts[len(parts)-1]
}

// afterPrefix strips the first segment (e.g. "aa") and returns the rest.
func afterPrefix(cn string) string {
	idx := strings.Index(cn, "-")
	if idx == -1 {
		return cn
	}
	return cn[idx+1:]
}

// --------------------------------------------------------------------------
// CSS parsing
// --------------------------------------------------------------------------

var (
	reClassName = regexp.MustCompile(`[.#][\w-]+`)
	rePseudo    = regexp.MustCompile(`:(?:hover|focus|active|checked|disabled|visited|before|after|first-child|last-child|nth-child)`)
	reAttrSel   = regexp.MustCompile(`^\[[\w-]+\]`)
)

func isMajorSection(raw string) (string, bool) {
	inner := strings.TrimPrefix(raw, "/*")
	inner = strings.TrimSuffix(inner, "*/")
	inner = strings.TrimSpace(inner)
	if strings.Count(inner, "=") < 4 {
		return "", false
	}
	for _, line := range strings.Split(inner, "\n") {
		line = strings.TrimSpace(line)
		stripped := strings.Trim(line, "=* \t-~")
		stripped = strings.TrimSpace(stripped)
		if len(stripped) >= 3 && len(stripped) <= 60 &&
			!strings.Contains(stripped, "{") &&
			!strings.Contains(stripped, ":") {
			return toTitle(stripped), true
		}
	}
	return "", false
}

func toTitle(s string) string {
	stop := map[string]bool{"a": true, "an": true, "the": true, "and": true, "or": true, "of": true, "in": true}
	words := strings.Fields(strings.ToLower(s))
	for i, w := range words {
		if i == 0 || !stop[w] {
			if len(w) > 0 {
				words[i] = strings.ToUpper(w[:1]) + w[1:]
			}
		}
	}
	return strings.Join(words, " ")
}

func parseCSS(src string) []Section {
	lines := strings.Split(src, "\n")
	var sections []Section
	current := Section{}

	i := 0
	for i < len(lines) {
		line := strings.TrimSpace(lines[i])

		// Single-line block comment
		if strings.HasPrefix(line, "/*") && strings.Contains(line, "*/") {
			if title, ok := isMajorSection(line); ok {
				if len(current.Blocks) > 0 || current.Title != "" {
					sections = append(sections, current)
				}
				current = Section{Title: title}
			}
			i++
			continue
		}

		// Multi-line block comment
		if strings.HasPrefix(line, "/*") {
			collected := []string{line}
			j := i + 1
			for j < len(lines) {
				collected = append(collected, strings.TrimSpace(lines[j]))
				if strings.Contains(lines[j], "*/") {
					j++
					break
				}
				j++
			}
			full := strings.Join(collected, "\n")
			if title, ok := isMajorSection(full); ok {
				if len(current.Blocks) > 0 || current.Title != "" {
					sections = append(sections, current)
				}
				current = Section{Title: title}
			}
			i = j
			continue
		}

		// CSS rule block
		if strings.Contains(line, "{") && !strings.HasPrefix(line, "//") {
			block := line
			depth := strings.Count(line, "{") - strings.Count(line, "}")
			j := i + 1
			for depth > 0 && j < len(lines) {
				block += "\n" + lines[j]
				depth += strings.Count(lines[j], "{") - strings.Count(lines[j], "}")
				j++
			}
			i = j

			braceIdx := strings.Index(block, "{")
			if braceIdx < 0 {
				continue
			}
			selectorStr := strings.TrimSpace(block[:braceIdx])
			body := ""
			end := strings.LastIndex(block, "}")
			if end > braceIdx {
				body = strings.TrimSpace(block[braceIdx+1 : end])
			}

			// Skip structural selectors and at-rules
			if strings.HasPrefix(selectorStr, ":root") ||
				strings.HasPrefix(selectorStr, "html") ||
				strings.HasPrefix(selectorStr, "body") ||
				strings.HasPrefix(selectorStr, "*") ||
				strings.HasPrefix(selectorStr, "@") {
				continue
			}

			for _, sel := range strings.Split(selectorStr, ",") {
				sel = strings.TrimSpace(sel)
				if sel == "" {
					continue
				}
				if strings.Contains(sel, "[data-theme") {
					continue
				}

				isPseudo := rePseudo.MatchString(sel)
				isAttr := strings.HasPrefix(sel, "[")

				className := ""
				if isAttr {
					// Attribute selector like [data-ng-tip]
					m := reAttrSel.FindString(sel)
					if m != "" {
						className = m // keep brackets for identification
					} else {
						className = strings.Fields(sel)[0]
					}
				} else {
					m := reClassName.FindString(sel)
					if m != "" {
						className = strings.TrimLeft(m, ".#")
					}
					if className == "" && len(strings.Fields(sel)) > 0 {
						className = strings.Fields(sel)[0]
					}
				}

				if className == "" {
					continue
				}

				current.Blocks = append(current.Blocks, RuleBlock{
					Selector: Selector{Raw: sel, ClassName: className, IsPseudo: isPseudo, IsAttr: isAttr},
					Body:     body,
				})
			}
			continue
		}

		i++
	}

	if len(current.Blocks) > 0 || current.Title != "" {
		sections = append(sections, current)
	}

	var out []Section
	for _, s := range sections {
		if s.Title != "" {
			out = append(out, s)
		}
	}
	return out
}

// --------------------------------------------------------------------------
// Deduplication
// --------------------------------------------------------------------------

func dedupBlocks(blocks []RuleBlock) []RuleBlock {
	seen := map[string]bool{}
	var out []RuleBlock
	for _, b := range blocks {
		if seen[b.Selector.ClassName] {
			continue
		}
		seen[b.Selector.ClassName] = true
		out = append(out, b)
	}
	return out
}

// --------------------------------------------------------------------------
// Prefix detection
// --------------------------------------------------------------------------

// detectPrefix finds the dominant prefix among class names. For themes like
// neon-grid (ng-btn, ng-card...) it returns "ng". For clean-system (btn,
// card...) it returns "". Works for any new AI-generated theme.
func detectPrefix(sections []Section) string {
	freq := map[string]int{}
	total := 0
	for _, s := range sections {
		for _, b := range s.Blocks {
			cn := b.Selector.ClassName
			if strings.HasPrefix(cn, "[") || strings.HasPrefix(cn, "@") {
				continue
			}
			parts := strings.SplitN(cn, "-", 2)
			if len(parts) == 2 && len(parts[0]) >= 2 {
				freq[parts[0]]++
				total++
			}
		}
	}
	if total == 0 {
		return ""
	}

	best := ""
	bestCount := 0
	for k, v := range freq {
		if v > bestCount {
			best = k
			bestCount = v
		}
	}

	// If the winner has >50% of all multi-segment classes, treat it as a
	// real prefix. Otherwise it is just a common component root name
	// (e.g. "btn" in clean-system) and there is no prefix.
	if bestCount > total/2 {
		return best
	}
	return ""
}

// --------------------------------------------------------------------------
// Demo inference -- fully prefix-aware
// --------------------------------------------------------------------------

type DemoSpec struct {
	Tag      string
	Attrs    string
	Content  string
	Children string
	Skip     bool
}

func inferDemo(rb RuleBlock, section Section, info ThemeInfo) DemoSpec {
	cn := rb.Selector.ClassName
	cnl := strings.ToLower(cn)
	raw := rb.Selector.Raw

	// Always skip pseudo-class variants (:hover, :focus, etc.)
	if rb.Selector.IsPseudo {
		return DemoSpec{Skip: true}
	}

	// Skip data-theme context rules
	if strings.Contains(raw, "[data-theme") {
		return DemoSpec{Skip: true}
	}

	// Handle attribute-only selectors like [data-ng-tip]
	if rb.Selector.IsAttr || strings.HasPrefix(cn, "[") {
		if strings.Contains(raw, "tip") {
			re := regexp.MustCompile(`\[([\w-]+)\]`)
			m := re.FindStringSubmatch(raw)
			if len(m) > 1 {
				return DemoSpec{
					Tag:     "span",
					Attrs:   fmt.Sprintf(`%s="Tooltip preview"`, m[1]),
					Content: "Hover for tooltip",
				}
			}
		}
		return DemoSpec{Skip: true}
	}

	// ---- Button group (before generic btn) ----
	if hasSeg(cnl, "btn") && hasSeg(cnl, "group") {
		baseBtn := info.p("btn")
		accentBtn := info.firstVariant("btn")
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<button class="%s">Alpha</button>`+
					`<button class="%s">Beta</button>`+
					`<button class="%s">Gamma</button>`,
				baseBtn, baseBtn, info.combo(baseBtn, accentBtn)),
		}
	}

	// ---- Buttons ----
	if hasSeg(cnl, "btn") {
		label := guessLabel(cn)
		if lastSeg(cnl) == "sm" {
			label = "Small"
		} else if lastSeg(cnl) == "lg" {
			label = "Large"
		}
		base := info.p("btn")
		return DemoSpec{
			Tag:     "button",
			Attrs:   fmt.Sprintf(`class="%s"`, info.combo(base, cn)),
			Content: label,
		}
	}

	// ---- Toggles ----
	if hasSeg(cnl, "toggle") {
		if hasSeg(cnl, "track") || hasSeg(cnl, "thumb") ||
			hasSeg(cnl, "slider") || hasSeg(cnl, "knob") {
			return DemoSpec{Skip: true}
		}
		trackCls := info.cls("toggle-track", "toggle-slider")
		thumbCls := info.cls("toggle-thumb", "toggle-knob")
		return DemoSpec{
			Tag:   "label",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<div class="%s on"><div class="%s"></div></div><span>Toggle on</span>`,
				trackCls, thumbCls),
		}
	}

	// ---- Range slider ----
	if hasSeg(cnl, "range") {
		return DemoSpec{
			Tag:   "input",
			Attrs: fmt.Sprintf(`class="%s" type="range" min="0" max="100" value="65"`, cn),
		}
	}

	// ---- Textarea ----
	if hasSeg(cnl, "textarea") {
		return DemoSpec{
			Tag:   "textarea",
			Attrs: fmt.Sprintf(`class="%s" rows="3" placeholder="Enter text here..."`, cn),
		}
	}

	// ---- Select ----
	if hasSeg(cnl, "select") {
		return DemoSpec{
			Tag:      "select",
			Attrs:    fmt.Sprintf(`class="%s"`, cn),
			Children: `<option>Option Alpha</option><option>Option Beta</option><option>Option Gamma</option>`,
		}
	}

	// ---- Checkbox / radio group ----
	if hasSeg(cnl, "check") && hasSeg(cnl, "group") {
		return DemoSpec{
			Tag:      "label",
			Attrs:    fmt.Sprintf(`class="%s"`, cn),
			Children: `<input type="checkbox" checked> Checkbox option`,
		}
	}

	// ---- Inputs ----
	if hasSeg(cnl, "input") {
		ls := lastSeg(cnl)
		base := info.p("input")
		if ls == "error" {
			return DemoSpec{
				Tag:   "input",
				Attrs: fmt.Sprintf(`class="%s" type="text" value="Invalid entry"`, info.combo(base, cn)),
			}
		}
		if ls == "success" {
			return DemoSpec{
				Tag:   "input",
				Attrs: fmt.Sprintf(`class="%s" type="text" value="Valid entry"`, info.combo(base, cn)),
			}
		}
		return DemoSpec{
			Tag:   "input",
			Attrs: fmt.Sprintf(`class="%s" type="text" placeholder="Type something..."`, info.combo(base, cn)),
		}
	}

	// ---- Form group ----
	if hasSeg(cnl, "form") && hasSeg(cnl, "group") {
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<label class="%s">Field Label</label>`+
					`<input class="%s" type="text" placeholder="Enter value...">`,
				info.p("label"), info.p("input")),
		}
	}

	// ---- Field row (clean-system) ----
	if hasSeg(cnl, "field") {
		if hasSeg(cnl, "row") {
			return DemoSpec{
				Tag:   "div",
				Attrs: fmt.Sprintf(`class="%s"`, cn),
				Children: fmt.Sprintf(
					`<label class="%s">Label</label>`+
						`<input class="%s" type="text" placeholder="Value">`,
					info.cls("label", "label-caps"), info.p("input")),
			}
		}
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<label class="%s">Field Label</label>`+
					`<input class="%s" type="text" placeholder="Enter value...">`,
				info.p("label"), info.p("input")),
		}
	}

	// ---- Hint ----
	if lastSeg(cnl) == "hint" {
		return DemoSpec{
			Tag:     "p",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: "Helper hint text appears below an input field.",
		}
	}

	// ---- Navigation (excluding breadcrumb) ----
	if hasSeg(cnl, "nav") && !hasSeg(cnl, "breadcrumb") {
		if hasSeg(cnl, "brand") || hasSeg(cnl, "link") ||
			hasSeg(cnl, "end") || hasSeg(cnl, "item") {
			return DemoSpec{Skip: true}
		}
		navLink := info.cls("nav-link", "nav-item")
		return DemoSpec{
			Tag:   "nav",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<span class="%s">Brand</span>`+
					`<a class="%s active" href="#">Home</a>`+
					`<a class="%s" href="#">About</a>`+
					`<a class="%s" href="#">Work</a>`+
					`<div class="%s"><button class="%s %s">Launch</button></div>`,
				info.cls("nav-brand"),
				navLink, navLink, navLink,
				info.cls("nav-end"),
				info.p("btn"), info.cls("btn-sm")),
		}
	}

	// ---- Breadcrumb ----
	if hasSeg(cnl, "breadcrumb") {
		if hasSeg(cnl, "item") || hasSeg(cnl, "sep") {
			return DemoSpec{Skip: true}
		}
		bItem := info.cls("breadcrumb-item")
		bSep := info.cls("breadcrumb-sep")
		return DemoSpec{
			Tag:   "nav",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<span class="%s">Home</span>`+
					`<span class="%s">/</span>`+
					`<span class="%s">Section</span>`+
					`<span class="%s">/</span>`+
					`<span class="%s active">Current</span>`,
				bItem, bSep, bItem, bSep, bItem),
		}
	}

	// ---- Badges ----
	if hasSeg(cnl, "badge") {
		base := info.p("badge")
		return DemoSpec{
			Tag:     "span",
			Attrs:   fmt.Sprintf(`class="%s"`, info.combo(base, cn)),
			Content: toTitle(lastSeg(cn)),
		}
	}

	// ---- Tags ----
	if hasSeg(cnl, "tag") {
		if hasSeg(cnl, "close") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:     "span",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: "Component Tag",
		}
	}

	// ---- Alerts ----
	if hasSeg(cnl, "alert") {
		if hasSeg(cnl, "title") || hasSeg(cnl, "body") || hasSeg(cnl, "message") {
			return DemoSpec{Skip: true}
		}
		title, msg := alertContent(cnl)
		base := info.p("alert")
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, info.combo(base, cn)),
			Children: fmt.Sprintf(
				`<div><div class="%s">%s</div><div class="%s">%s</div></div>`,
				info.cls("alert-title"),
				title,
				info.cls("alert-body", "alert-message"),
				msg),
		}
	}

	// ---- Dividers ----
	if hasSeg(cnl, "divider") {
		if hasSeg(cnl, "label") {
			return DemoSpec{
				Tag:     "div",
				Attrs:   fmt.Sprintf(`class="%s"`, cn),
				Content: "Section Break",
			}
		}
		if hasSeg(cnl, "vertical") || lastSeg(cnl) == "v" {
			return DemoSpec{
				Tag:   "div",
				Attrs: fmt.Sprintf(`class="%s" style="height:40px;display:inline-block"`, cn),
			}
		}
		return DemoSpec{Tag: "hr", Attrs: fmt.Sprintf(`class="%s"`, cn)}
	}

	// ---- Tables ----
	if hasSeg(cnl, "table") {
		if hasSeg(cnl, "wrap") || hasSeg(cnl, "wrapper") {
			return DemoSpec{
				Tag:   "div",
				Attrs: fmt.Sprintf(`class="%s"`, cn),
				Children: fmt.Sprintf(
					`<table class="%s">`+
						`<thead><tr><th>Module</th><th>Version</th><th>Status</th></tr></thead>`+
						`<tbody>`+
						`<tr><td>core-engine</td><td>4.2.0</td><td>Stable</td></tr>`+
						`<tr><td>ui-layer</td><td>2.1.3</td><td>Beta</td></tr>`+
						`<tr><td>data-layer</td><td>1.9.1</td><td>Stable</td></tr>`+
						`</tbody></table>`,
					info.p("table")),
			}
		}
		if hasSeg(cnl, "striped") {
			wrapCls := info.cls("table-wrap", "table-wrapper")
			return DemoSpec{
				Tag:   "div",
				Attrs: fmt.Sprintf(`class="%s"`, wrapCls),
				Children: fmt.Sprintf(
					`<table class="%s %s">`+
						`<thead><tr><th>Column A</th><th>Column B</th></tr></thead>`+
						`<tbody>`+
						`<tr><td>Row 1</td><td>Value</td></tr>`+
						`<tr><td>Row 2</td><td>Value</td></tr>`+
						`</tbody></table>`,
					info.p("table"), cn),
			}
		}
		return DemoSpec{Skip: true}
	}

	// ---- Progress ----
	if hasSeg(cnl, "progress") {
		if hasSeg(cnl, "bar") {
			return DemoSpec{Skip: true}
		}
		if hasSeg(cnl, "step") {
			if lastSeg(cnl) == "steps" {
				stepCls := info.p("progress-step")
				return DemoSpec{
					Tag:   "div",
					Attrs: fmt.Sprintf(`class="%s"`, cn),
					Children: fmt.Sprintf(
						`<div class="%s done"></div>`+
							`<div class="%s done"></div>`+
							`<div class="%s done"></div>`+
							`<div class="%s"></div>`+
							`<div class="%s"></div>`,
						stepCls, stepCls, stepCls, stepCls, stepCls),
				}
			}
			return DemoSpec{Skip: true}
		}
		base := info.p("progress")
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, info.combo(base, cn)),
			Children: fmt.Sprintf(
				`<div class="%s" style="width:72%%"></div>`,
				info.p("progress-bar")),
		}
	}

	// ---- Tooltips (class-based) ----
	if hasSeg(cnl, "tooltip") {
		if hasSeg(cnl, "content") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:     "span",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: "Hover for tooltip",
		}
	}

	// ---- Cards ----
	if hasSeg(cnl, "card") {
		if hasSeg(cnl, "header") || hasSeg(cnl, "title") ||
			hasSeg(cnl, "footer") || hasSeg(cnl, "subtitle") ||
			hasSeg(cnl, "elevated") {
			return DemoSpec{Skip: true}
		}
		if lastSeg(cnl) == "hover" || lastSeg(cnl) == "pressed" {
			return DemoSpec{Skip: true}
		}
		variant := toTitle(afterPrefix(cn))
		accentBadge := info.firstVariant("badge")
		baseBadge := info.p("badge")
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<div class="%s">`+
					`<span class="%s">%s</span>`+
					`<span class="%s">Active</span>`+
					`</div>`+
					`<p style="font-size:0.875rem;line-height:1.6">`+
					`Preview card with structured header, body and action footer.`+
					`</p>`+
					`<div class="%s">`+
					`<button class="%s %s">Action</button>`+
					`<button class="%s %s">Cancel</button>`+
					`</div>`,
				info.cls("card-header"),
				info.cls("card-title"),
				variant,
				info.combo(baseBadge, accentBadge),
				info.cls("card-footer"),
				info.p("btn"), info.cls("btn-sm"),
				info.p("btn"), info.cls("btn-sm")),
		}
	}

	// ---- Panels ----
	if hasSeg(cnl, "panel") {
		if hasSeg(cnl, "head") || hasSeg(cnl, "body") || hasSeg(cnl, "header") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<div class="%s">Panel Header</div>`+
					`<div class="%s">Panel body content goes here.</div>`,
				info.cls("panel-head", "panel-header"),
				info.cls("panel-body")),
		}
	}

	// ---- Stat widget ----
	if hasSeg(cnl, "stat") {
		if hasSeg(cnl, "value") || hasSeg(cnl, "label") ||
			hasSeg(cnl, "change") || hasSeg(cnl, "num") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<div class="%s">8,247</div><div class="%s">Active Sessions</div>`,
				info.p("stat-value"), info.p("stat-label")),
		}
	}

	// ---- Section heading ----
	if hasSeg(cnl, "section") && hasSeg(cnl, "head") {
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<h3 class="%s">Section Header</h3>`+
					`<button class="%s %s">Action</button>`,
				info.p("h3"),
				info.p("btn"), info.cls("btn-sm")),
		}
	}

	// ---- Typography: heading classes ----
	switch lastSeg(cnl) {
	case "h1":
		return DemoSpec{Tag: "h1", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Heading Level One"}
	case "h2":
		return DemoSpec{Tag: "h2", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Heading Level Two"}
	case "h3":
		return DemoSpec{Tag: "h3", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Heading Level Three"}
	case "h4":
		return DemoSpec{Tag: "h4", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Heading Level Four"}
	case "display":
		return DemoSpec{Tag: "p", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Display Type Scale"}
	case "label":
		return DemoSpec{Tag: "span", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Label Text"}
	case "overline":
		return DemoSpec{Tag: "span", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Overline Text"}
	}

	// ---- Text utilities ----
	if hasSeg(cnl, "text") {
		last := lastSeg(cnl)
		colorWords := map[string]bool{
			"aqua": true, "orange": true, "yellow": true, "muted": true,
			"cyan": true, "pink": true, "green": true, "purple": true,
			"ice": true, "lavender": true, "bright": true, "dim": true,
			"primary": true, "success": true, "warning": true, "danger": true,
			"error": true, "info": true, "accent": true, "red": true, "blue": true,
		}
		if colorWords[last] {
			return DemoSpec{
				Tag:     "span",
				Attrs:   fmt.Sprintf(`class="%s"`, cn),
				Content: toTitle(lastSeg(cn)) + " text sample",
			}
		}
		if last == "upper" || last == "caps" {
			return DemoSpec{Tag: "span", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Uppercase text sample"}
		}
		if last == "light" {
			return DemoSpec{
				Tag:     "span",
				Attrs:   fmt.Sprintf(`class="%s" style="background:#333;padding:0.2em 0.4em"`, cn),
				Content: "Light text",
			}
		}
		if last == "sm" || last == "xs" || last == "lg" {
			return DemoSpec{Tag: "span", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: toTitle(last) + " size text"}
		}
		return DemoSpec{Tag: "span", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Sample text"}
	}

	// ---- Modals / overlays ----
	if hasSeg(cnl, "modal") || hasSeg(cnl, "overlay") {
		if hasSeg(cnl, "header") || hasSeg(cnl, "title") ||
			hasSeg(cnl, "body") || hasSeg(cnl, "footer") || hasSeg(cnl, "close") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: guessLabel(cn),
		}
	}

	// ---- Avatars ----
	if hasSeg(cnl, "avatar") {
		if hasSeg(cnl, "initials") || hasSeg(cnl, "group") ||
			hasSeg(cnl, "status") || hasSeg(cnl, "wrapper") {
			return DemoSpec{Skip: true}
		}
		base := info.p("avatar")
		return DemoSpec{
			Tag:      "div",
			Attrs:    fmt.Sprintf(`class="%s"`, info.combo(base, cn)),
			Children: fmt.Sprintf(`<span class="%s">AB</span>`, info.cls("avatar-initials")),
		}
	}

	// ---- Tabs ----
	if hasSeg(cnl, "tabs") || (hasSeg(cnl, "tab") && !hasSeg(cnl, "table")) {
		if lastSeg(cnl) == "tab" || hasSeg(cnl, "active") || hasSeg(cnl, "vertical") {
			return DemoSpec{Skip: true}
		}
		tabCls := info.p("tab")
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<div class="%s active">Tab One</div>`+
					`<div class="%s">Tab Two</div>`+
					`<div class="%s">Tab Three</div>`,
				tabCls, tabCls, tabCls),
		}
	}

	// ---- Checkbox / Radio (class-based, not group) ----
	if hasSeg(cnl, "checkbox") || hasSeg(cnl, "radio") {
		if hasSeg(cnl, "wrapper") {
			return DemoSpec{Skip: true}
		}
		inputType := "checkbox"
		if hasSeg(cnl, "radio") {
			inputType = "radio"
		}
		wrapperCls := info.cls(inputType + "-wrapper")
		return DemoSpec{
			Tag:      "label",
			Attrs:    fmt.Sprintf(`class="%s"`, wrapperCls),
			Children: fmt.Sprintf(`<input class="%s" type="%s" checked> Option`, cn, inputType),
		}
	}

	// ---- Terminal ----
	if hasSeg(cnl, "terminal") {
		if hasSeg(cnl, "header") || hasSeg(cnl, "body") || hasSeg(cnl, "line") ||
			hasSeg(cnl, "prompt") || hasSeg(cnl, "output") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<div class="%s">Terminal</div>`+
					`<div class="%s">`+
					`<div class="%s"><span class="%s">$</span> ls -la</div>`+
					`<div class="%s">drwxr-xr-x  4 user  staff  128 Jan  1 12:00 .</div>`+
					`</div>`,
				info.cls("terminal-header"),
				info.cls("terminal-body"),
				info.cls("terminal-line"),
				info.cls("terminal-prompt"),
				info.cls("terminal-output", "terminal-line")),
		}
	}

	// ---- Keyboard key (keyboard-ui) ----
	if hasSeg(cnl, "key") && !hasSeg(cnl, "keyboard") {
		label := guessLabel(cn)
		if len(label) > 12 {
			label = toTitle(lastSeg(cn))
		}
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: label,
		}
	}
	if hasSeg(cnl, "keyboard") {
		return DemoSpec{Skip: true}
	}

	// ---- Touchbar (keyboard-ui) ----
	if hasSeg(cnl, "touchbar") {
		if hasSeg(cnl, "btn") || hasSeg(cnl, "divider") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: guessLabel(cn),
		}
	}

	// ---- Shortcut (keyboard-ui) ----
	if hasSeg(cnl, "shortcut") {
		if hasSeg(cnl, "key") || hasSeg(cnl, "plus") {
			return DemoSpec{Skip: true}
		}
		keyCls := info.cls("shortcut-key")
		plusCls := info.cls("shortcut-plus")
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<span class="%s">Cmd</span>`+
					`<span class="%s">+</span>`+
					`<span class="%s">S</span>`,
				keyCls, plusCls, keyCls),
		}
	}

	// ---- Chrome surface (atom-age) ----
	if hasSeg(cnl, "chrome") {
		if hasSeg(cnl, "bar") {
			return DemoSpec{Tag: "div", Attrs: fmt.Sprintf(`class="%s" style="width:100%%"`, cn)}
		}
		return DemoSpec{Tag: "div", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Chrome surface"}
	}

	// ---- Star rule (atom-age) ----
	if hasSeg(cnl, "star") && hasSeg(cnl, "rule") {
		return DemoSpec{Tag: "div", Attrs: fmt.Sprintf(`class="%s"`, cn), Content: "Star Rule"}
	}

	// ---- Dot grid (atom-age) ----
	if hasSeg(cnl, "dot") && hasSeg(cnl, "grid") {
		return DemoSpec{Tag: "div", Attrs: fmt.Sprintf(`class="%s" style="width:100%%;height:80px"`, cn)}
	}

	// ---- Orbit ring (atom-age) ----
	if hasSeg(cnl, "orbit") && hasSeg(cnl, "ring") {
		return DemoSpec{Tag: "div", Attrs: fmt.Sprintf(`class="%s"`, cn)}
	}

	// ---- Accent strips ----
	if hasSeg(cnl, "accent") && hasSeg(cnl, "strip") {
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s %s"`, info.p("card"), cn),
			Content: "Card with " + toTitle(lastSeg(cn)) + " accent strip",
		}
	}

	// ---- Glass effects (crystal-ui) ----
	if hasSeg(cnl, "glass") || hasSeg(cnl, "aurora") || hasSeg(cnl, "prism") || hasSeg(cnl, "shimmer") {
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s" style="padding:1.5rem;min-height:60px"`, cn),
			Content: guessLabel(cn),
		}
	}
	if hasSeg(cnl, "orbs") {
		return DemoSpec{Tag: "div", Attrs: fmt.Sprintf(`class="%s" style="position:relative;height:100px"`, cn)}
	}
	if hasSeg(cnl, "gradient") && hasSeg(cnl, "text") {
		return DemoSpec{
			Tag:     "span",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: "Gradient Text",
		}
	}

	// ---- Shadow / glow utilities ----
	if hasSeg(cnl, "shadow") || hasSeg(cnl, "glow") {
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s %s"`, info.p("card"), cn),
			Content: guessLabel(cn),
		}
	}

	// ---- Neumorphic utilities ----
	if hasSeg(cnl, "raised") || hasSeg(cnl, "pressed") || hasSeg(cnl, "flat") ||
		hasSeg(cnl, "hard") || hasSeg(cnl, "border") {
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s" style="padding:1rem 1.5rem"`, cn),
			Content: guessLabel(cn),
		}
	}

	// ---- Code display ----
	if hasSeg(cnl, "code") {
		if hasSeg(cnl, "inline") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: "Code",
		}
	}

	// ---- Neon-grid decorations ----
	if hasSeg(cnl, "scanlines") || hasSeg(cnl, "hud") || hasSeg(cnl, "bracket") ||
		hasSeg(cnl, "pulse") || hasSeg(cnl, "flicker") {
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: guessLabel(cn),
		}
	}

	// ---- Skeleton loaders ----
	if hasSeg(cnl, "skeleton") {
		if hasSeg(cnl, "text") || hasSeg(cnl, "title") ||
			hasSeg(cnl, "avatar") || hasSeg(cnl, "image") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s" style="width:100%%;height:60px"`, cn),
		}
	}

	// ---- Loaders ----
	if hasSeg(cnl, "loader") {
		if hasSeg(cnl, "bars") || hasSeg(cnl, "pulse") ||
			hasSeg(cnl, "dots") || hasSeg(cnl, "key") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{Tag: "div", Attrs: fmt.Sprintf(`class="%s"`, cn)}
	}

	// ---- Clean-system specific ----
	if hasSeg(cnl, "toolbar") {
		if hasSeg(cnl, "title") || hasSeg(cnl, "sep") || hasSeg(cnl, "spacer") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<span class="%s">Toolbar</span>`+
					`<span class="%s"></span>`+
					`<span class="%s"></span>`+
					`<button class="%s %s">Action</button>`,
				info.cls("toolbar-title"),
				info.cls("toolbar-sep"),
				info.cls("toolbar-spacer"),
				info.p("btn"), info.cls("btn-primary")),
		}
	}

	if hasSeg(cnl, "status") && hasSeg(cnl, "bar") {
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<span class="%s %s"></span> Ready`+
					`<span class="%s %s"></span> Active`,
				info.cls("status-dot"), info.cls("dot-ok"),
				info.cls("status-dot"), info.cls("dot-active")),
		}
	}
	if hasSeg(cnl, "status") && hasSeg(cnl, "dot") {
		return DemoSpec{Skip: true}
	}
	if hasSeg(cnl, "dot") && (hasSeg(cnl, "ok") || hasSeg(cnl, "warn") ||
		hasSeg(cnl, "error") || hasSeg(cnl, "active")) {
		return DemoSpec{Skip: true}
	}

	// ---- Chips (neumorphism) ----
	if hasSeg(cnl, "chip") {
		if hasSeg(cnl, "close") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:     "span",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: "Chip Label",
		}
	}

	// ---- Segmented control (neumorphism) ----
	if hasSeg(cnl, "segmented") {
		if hasSeg(cnl, "item") {
			return DemoSpec{Skip: true}
		}
		itemCls := info.cls("segmented-item")
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<div class="%s active">Option A</div>`+
					`<div class="%s">Option B</div>`+
					`<div class="%s">Option C</div>`,
				itemCls, itemCls, itemCls),
		}
	}

	// ---- Player (neumorphism) ----
	if hasSeg(cnl, "player") {
		if hasSeg(cnl, "btn") || hasSeg(cnl, "progress") || hasSeg(cnl, "main") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: guessLabel(cn),
		}
	}

	// ---- Clock (neumorphism) ----
	if hasSeg(cnl, "clock") {
		if hasSeg(cnl, "inner") || hasSeg(cnl, "center") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: guessLabel(cn),
		}
	}

	// ---- Sys-toast (clean-system) ----
	if hasSeg(cnl, "toast") {
		if hasSeg(cnl, "stack") || hasSeg(cnl, "success") ||
			hasSeg(cnl, "error") || hasSeg(cnl, "warn") || hasSeg(cnl, "visible") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: "Toast notification message",
		}
	}

	// ---- Sys-modal (clean-system) ----
	if hasSeg(cnl, "sys") {
		if hasSeg(cnl, "overlay") || hasSeg(cnl, "header") ||
			hasSeg(cnl, "title") || hasSeg(cnl, "body") ||
			hasSeg(cnl, "footer") {
			return DemoSpec{Skip: true}
		}
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: guessLabel(cn),
		}
	}

	// ---- List (clean-system) ----
	if hasSeg(cnl, "list") {
		if hasSeg(cnl, "item") || hasSeg(cnl, "active") {
			return DemoSpec{Skip: true}
		}
		itemCls := info.cls("list-item")
		return DemoSpec{
			Tag:   "div",
			Attrs: fmt.Sprintf(`class="%s"`, cn),
			Children: fmt.Sprintf(
				`<div class="%s">List item one</div>`+
					`<div class="%s %s">List item two (active)</div>`+
					`<div class="%s">List item three</div>`,
				itemCls, itemCls, info.cls("item-active"), itemCls),
		}
	}

	// ---- Layout utilities: skip ----
	for _, skip := range []string{"container", "grid", "flex", "stack", "cluster", "row", "col"} {
		if hasSeg(cnl, skip) {
			return DemoSpec{Skip: true}
		}
	}

	// ---- Focus/accessibility utilities: skip ----
	if hasSeg(cnl, "focus") || hasSeg(cnl, "sr") || lastSeg(cnl) == "truncate" {
		return DemoSpec{Skip: true}
	}

	// ---- Rounded utilities (neumorphism) ----
	if hasSeg(cnl, "rounded") {
		return DemoSpec{Skip: true}
	}

	// ---- Icon-box (dark-neu, neumorphism) ----
	if hasSeg(cnl, "icon") && hasSeg(cnl, "box") {
		return DemoSpec{
			Tag:     "div",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: guessLabel(cn),
		}
	}

	// ---- FAB ----
	if lastSeg(cnl) == "fab" {
		return DemoSpec{
			Tag:     "button",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: "+",
		}
	}

	// ---- KBD ----
	if lastSeg(cnl) == "kbd" {
		return DemoSpec{
			Tag:     "kbd",
			Attrs:   fmt.Sprintf(`class="%s"`, cn),
			Content: "Ctrl",
		}
	}

	// ---- Generic fallback ----
	return DemoSpec{
		Tag:     "div",
		Attrs:   fmt.Sprintf(`class="%s"`, cn),
		Content: guessLabel(cn),
	}
}

func guessLabel(cn string) string {
	base := afterPrefix(cn)
	return toTitle(strings.ReplaceAll(base, "-", " "))
}

func alertContent(cnl string) (title, msg string) {
	if hasSeg(cnl, "success") {
		return "Success", "Operation completed without errors."
	}
	if hasSeg(cnl, "warning") {
		return "Warning", "Proceed with caution."
	}
	if hasSeg(cnl, "danger") {
		return "Error", "A critical error occurred."
	}
	return "Information", "This is an informational message."
}

// --------------------------------------------------------------------------
// HTML rendering
// --------------------------------------------------------------------------

func renderDemo(d DemoSpec) string {
	if d.Skip {
		return ""
	}
	inner := d.Children
	if inner == "" {
		inner = d.Content
	}
	if d.Tag == "hr" || d.Tag == "input" {
		return fmt.Sprintf("<%s %s>", d.Tag, d.Attrs)
	}
	return fmt.Sprintf("<%s %s>%s</%s>", d.Tag, d.Attrs, inner, d.Tag)
}

func htmlEscape(s string) string {
	s = strings.ReplaceAll(s, "&", "&amp;")
	s = strings.ReplaceAll(s, "<", "&lt;")
	s = strings.ReplaceAll(s, ">", "&gt;")
	return s
}

func sectionID(title string) string {
	id := strings.ToLower(title)
	id = strings.ReplaceAll(id, " ", "-")
	return regexp.MustCompile(`[^a-z0-9\-]`).ReplaceAllString(id, "")
}

func renderSection(s Section, info ThemeInfo) string {
	blocks := dedupBlocks(s.Blocks)
	var sb strings.Builder

	id := sectionID(s.Title)
	sb.WriteString(fmt.Sprintf("<section class=\"preview-section\" id=\"%s\">\n<h2 class=\"preview-section-title\">%s</h2>\n<div class=\"preview-grid\">\n", id, s.Title))

	for _, b := range blocks {
		d := inferDemo(b, s, info)
		html := renderDemo(d)
		if html == "" {
			continue
		}
		label := b.Selector.ClassName
		if !b.Selector.IsAttr {
			label = strings.TrimLeft(b.Selector.Raw, ".")
		}
		sb.WriteString(fmt.Sprintf("<div class=\"preview-item\">\n  <div class=\"preview-canvas\">%s</div>\n  <div class=\"preview-label\">%s</div>\n</div>\n", html, htmlEscape(label)))
	}

	sb.WriteString("</div></section>\n")
	return sb.String()
}

func countVisible(sections []Section, info ThemeInfo) (classes, rules int) {
	for _, s := range sections {
		rules += len(s.Blocks)
		for _, b := range dedupBlocks(s.Blocks) {
			if !inferDemo(b, s, info).Skip {
				classes++
			}
		}
	}
	return
}

// --------------------------------------------------------------------------
// Metadata extraction
// --------------------------------------------------------------------------

func detectBodyClass(src string) string {
	re := regexp.MustCompile(`body\.([\w-]+)\s*[{,\s]`)
	m := re.FindStringSubmatch(src)
	if len(m) > 1 {
		return m[1]
	}
	return ""
}

func detectTitle(src, filename string) string {
	lines := strings.Split(src, "\n")
	n := 20
	if len(lines) < n {
		n = len(lines)
	}
	for _, line := range lines[:n] {
		line = strings.TrimSpace(line)
		stripped := strings.Trim(line, "/*= \t-~")
		stripped = strings.TrimSpace(stripped)
		if len(stripped) > 4 && len(stripped) < 80 &&
			!strings.Contains(stripped, "{") &&
			!strings.Contains(stripped, ":") &&
			strings.Count(stripped, " ") >= 1 {
			return stripped
		}
	}
	base := strings.TrimSuffix(filepath.Base(filename), ".css")
	return toTitle(strings.ReplaceAll(base, "-", " "))
}

// --------------------------------------------------------------------------
// HTML page template
//
// Uses {{PREFIX_DASH}} which expands to "ng-", "aa-", "" etc. so that
// canvas CSS selectors and toggle JS match the actual theme classes.
// --------------------------------------------------------------------------

const pageTemplate = `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{{TITLE}} -- Component Preview</title>
<link rel="stylesheet" href="{{CSSFILE}}">
<style>
*, *::before, *::after { box-sizing: border-box; }

body.preview-shell {
  font-family: system-ui, -apple-system, 'Segoe UI', sans-serif;
  background: #0d0f14;
  color: #c8d0e0;
  margin: 0;
  padding: 0;
  line-height: 1.5;
}

.preview-topbar {
  background: #13161d;
  border-bottom: 1px solid #1e2433;
  padding: 0.875rem 2rem;
  display: flex;
  align-items: center;
  gap: 1.25rem;
  position: sticky;
  top: 0;
  z-index: 1000;
}

.preview-topbar-title {
  font-size: 0.78rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.16em;
  color: #a8b8d8;
}

.preview-topbar-file {
  font-size: 0.7rem;
  color: #3a5070;
  font-family: 'Courier New', monospace;
  margin-left: auto;
}

.preview-topbar-mark {
  width: 7px;
  height: 7px;
  background: #3a8fb0;
  display: inline-block;
  flex-shrink: 0;
}

.preview-sidebar {
  position: fixed;
  top: 44px;
  left: 0;
  bottom: 0;
  width: 200px;
  background: #0c0f16;
  border-right: 1px solid #181f2e;
  overflow-y: auto;
  padding: 1rem 0 2rem;
  z-index: 900;
}

.preview-sidebar-heading {
  font-size: 0.58rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.2em;
  color: #2a3a52;
  padding: 0.75rem 1.25rem 0.4rem;
}

.preview-nav-link {
  display: block;
  padding: 0.4rem 1.25rem;
  font-size: 0.73rem;
  color: #4a5a72;
  text-decoration: none;
  transition: color 0.1s, background 0.1s;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.preview-nav-link:hover {
  color: #7ab0cc;
  background: rgba(255,255,255,0.025);
}

.preview-nav-link.is-active {
  color: #4fa8c8;
  background: rgba(79,168,200,0.07);
  border-left: 2px solid #4fa8c8;
  padding-left: calc(1.25rem - 2px);
}

.preview-main {
  margin-left: 200px;
  padding: 2.5rem 2.75rem 5rem;
  min-height: 100vh;
}

.preview-page-title {
  font-size: 1.4rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  color: #ccd8f0;
  margin: 0 0 0.3rem;
}

.preview-page-meta {
  font-size: 0.72rem;
  color: #3a4a62;
  margin: 0 0 2.5rem;
  font-family: 'Courier New', monospace;
}

.preview-stats {
  display: flex;
  gap: 2rem;
  margin-bottom: 3rem;
  padding: 1rem 1.5rem;
  background: #0f1218;
  border: 1px solid #1a2030;
}

.preview-stat {
  display: flex;
  flex-direction: column;
  gap: 0.2rem;
}

.preview-stat-num {
  font-size: 1.3rem;
  font-weight: 700;
  color: #4fa8c8;
  line-height: 1;
  font-family: 'Courier New', monospace;
}

.preview-stat-name {
  font-size: 0.58rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.16em;
  color: #2a3a52;
}

.preview-section {
  margin-bottom: 4rem;
}

.preview-section-title {
  font-size: 0.65rem;
  font-weight: 700;
  text-transform: uppercase;
  letter-spacing: 0.2em;
  color: #3a8fb0;
  margin: 0 0 1.5rem;
  padding-bottom: 0.55rem;
  border-bottom: 1px solid #181f2e;
  scroll-margin-top: 56px;
}

.preview-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(290px, 1fr));
  gap: 1.25rem;
}

.preview-item {
  background: #10131c;
  border: 1px solid #1a2030;
  overflow: hidden;
}

.preview-canvas {
  padding: 1.5rem 1.25rem;
  min-height: 80px;
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 0.5rem;
}

.preview-canvas > nav,
.preview-canvas > .{{PREFIX_DASH}}nav,
.preview-canvas > [class*="{{PREFIX_DASH}}table-wrap"],
.preview-canvas > .{{PREFIX_DASH}}alert,
.preview-canvas > [class$="{{PREFIX_DASH}}alert"],
.preview-canvas > .{{PREFIX_DASH}}breadcrumb,
.preview-canvas > .{{PREFIX_DASH}}section-head,
.preview-canvas > .{{PREFIX_DASH}}panel,
.preview-canvas > .{{PREFIX_DASH}}stat,
.preview-canvas > .{{PREFIX_DASH}}form-group,
.preview-canvas > .{{PREFIX_DASH}}field,
.preview-canvas > .{{PREFIX_DASH}}field-row,
.preview-canvas > .{{PREFIX_DASH}}toolbar,
.preview-canvas > .{{PREFIX_DASH}}status-bar,
.preview-canvas > .{{PREFIX_DASH}}shortcut,
.preview-canvas > .{{PREFIX_DASH}}terminal,
.preview-canvas > .{{PREFIX_DASH}}list { width: 100%; }

.preview-canvas > .{{PREFIX_DASH}}input,
.preview-canvas > input[class*="{{PREFIX_DASH}}input"],
.preview-canvas > .{{PREFIX_DASH}}textarea,
.preview-canvas > textarea,
.preview-canvas > .{{PREFIX_DASH}}select,
.preview-canvas > select,
.preview-canvas > input[type="range"],
.preview-canvas > .{{PREFIX_DASH}}range,
.preview-canvas > hr,
.preview-canvas > [class*="{{PREFIX_DASH}}divider"],
.preview-canvas > .{{PREFIX_DASH}}star-rule { width: 100%; }

.preview-canvas > [class*="{{PREFIX_DASH}}progress"] { width: 100%; }
.preview-canvas > [class*="{{PREFIX_DASH}}card"] { width: 100%; font-size: 0.82rem; }

.preview-label {
  padding: 0.4rem 0.875rem;
  font-size: 0.64rem;
  font-family: 'Courier New', monospace;
  color: #2e3e58;
  background: #0a0d14;
  border-top: 1px solid #181f2e;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

::-webkit-scrollbar { width: 5px; height: 5px; }
::-webkit-scrollbar-track { background: #0a0c12; }
::-webkit-scrollbar-thumb { background: #1a2232; }
::-webkit-scrollbar-thumb:hover { background: #243045; }

@media (max-width: 860px) {
  .preview-sidebar { display: none; }
  .preview-main { margin-left: 0; padding: 1.5rem; }
  .preview-grid { grid-template-columns: 1fr; }
}
</style>
</head>
<body class="preview-shell {{BODYCLASS}}">

<header class="preview-topbar">
  <span class="preview-topbar-mark"></span>
  <span class="preview-topbar-title">Component Preview</span>
  <span class="preview-topbar-file">{{CSSFILE}}</span>
</header>

<nav class="preview-sidebar">
  <div class="preview-sidebar-heading">Sections</div>
  {{NAVLINKS}}
</nav>

<main class="preview-main">
  <h1 class="preview-page-title">{{TITLE}}</h1>
  <p class="preview-page-meta">{{CSSFILE}} -- {{CLASSCOUNT}} components across {{SECTIONCOUNT}} sections</p>

  <div class="preview-stats">
    <div class="preview-stat">
      <span class="preview-stat-num">{{CLASSCOUNT}}</span>
      <span class="preview-stat-name">Components</span>
    </div>
    <div class="preview-stat">
      <span class="preview-stat-num">{{SECTIONCOUNT}}</span>
      <span class="preview-stat-name">Sections</span>
    </div>
    <div class="preview-stat">
      <span class="preview-stat-num">{{RULECOUNT}}</span>
      <span class="preview-stat-name">Rule Blocks</span>
    </div>
  </div>

  {{SECTIONS}}
</main>

<script>
(function() {
  var links = [].slice.call(document.querySelectorAll('.preview-nav-link'));
  var items = links.map(function(l) {
    var id = l.getAttribute('href').replace('#','');
    return { link: l, el: document.getElementById(id) };
  }).filter(function(x) { return x.el; });

  function update() {
    var y = window.scrollY + 70;
    var active = null;
    items.forEach(function(it) {
      if (it.el.getBoundingClientRect().top + window.scrollY <= y) active = it;
    });
    links.forEach(function(l) { l.classList.remove('is-active'); });
    if (active) active.link.classList.add('is-active');
  }
  window.addEventListener('scroll', update, { passive: true });
  update();

  document.querySelectorAll('.{{PREFIX_DASH}}toggle').forEach(function(t) {
    t.addEventListener('click', function() {
      var track = t.querySelector('.{{PREFIX_DASH}}toggle-track, .{{PREFIX_DASH}}toggle-slider');
      if (track) track.classList.toggle('on');
    });
  });
})();
</script>
</body>
</html>
`

// --------------------------------------------------------------------------
// Main
// --------------------------------------------------------------------------

func processFile(cssPath string) error {
	src, err := os.ReadFile(cssPath)
	if err != nil {
		return fmt.Errorf("read %s: %w", cssPath, err)
	}
	content := string(src)

	sections := parseCSS(content)
	info := buildThemeInfo(sections)
	bodyClass := detectBodyClass(content)
	title := detectTitle(content, cssPath)
	classes, rules := countVisible(sections, info)

	var navSB strings.Builder
	for _, s := range sections {
		id := sectionID(s.Title)
		navSB.WriteString(fmt.Sprintf("<a class=\"preview-nav-link\" href=\"#%s\">%s</a>\n", id, s.Title))
	}

	var secSB strings.Builder
	for _, s := range sections {
		secSB.WriteString(renderSection(s, info))
	}

	cssFilename := filepath.Base(cssPath)
	outPath := strings.TrimSuffix(cssPath, ".css") + "-preview.html"

	page := pageTemplate
	page = strings.ReplaceAll(page, "{{TITLE}}", title)
	page = strings.ReplaceAll(page, "{{CSSFILE}}", cssFilename)
	page = strings.ReplaceAll(page, "{{BODYCLASS}}", bodyClass)
	page = strings.ReplaceAll(page, "{{NAVLINKS}}", navSB.String())
	page = strings.ReplaceAll(page, "{{SECTIONS}}", secSB.String())
	page = strings.ReplaceAll(page, "{{CLASSCOUNT}}", fmt.Sprintf("%d", classes))
	page = strings.ReplaceAll(page, "{{SECTIONCOUNT}}", fmt.Sprintf("%d", len(sections)))
	page = strings.ReplaceAll(page, "{{RULECOUNT}}", fmt.Sprintf("%d", rules))
	page = strings.ReplaceAll(page, "{{PREFIX_DASH}}", info.PrefixDash)

	if err := os.WriteFile(outPath, []byte(page), 0644); err != nil {
		return fmt.Errorf("write %s: %w", outPath, err)
	}

	pfxLabel := info.Prefix
	if pfxLabel == "" {
		pfxLabel = "(none)"
	}
	fmt.Printf("  wrote  %s\n         prefix: %s  |  %d sections  |  %d components  |  %d rule blocks\n",
		filepath.Base(outPath), pfxLabel, len(sections), classes, rules)
	return nil
}

func main() {
	args := os.Args[1:]
	var files []string

	if len(args) == 0 {
		matches, _ := filepath.Glob("*.css")
		files = matches
	} else {
		for _, a := range args {
			m, err := filepath.Glob(a)
			if err != nil || len(m) == 0 {
				files = append(files, a)
			} else {
				files = append(files, m...)
			}
		}
	}

	if len(files) == 0 {
		fmt.Fprintln(os.Stderr, "No CSS files found.")
		os.Exit(1)
	}

	for _, f := range files {
		fmt.Printf("Processing: %s\n", f)
		if err := processFile(f); err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
		}
	}
}
'''

with open('/Users/mac/Documents/live-css/style-sheets/preview.go', 'w') as f:
    f.write(code)

print(f"Written {len(code)} bytes, {code.count(chr(10))} lines")
