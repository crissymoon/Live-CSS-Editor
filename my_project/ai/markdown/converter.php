<?php
/**
 * Markdown Converter
 * Lightweight, dependency-free markdown-to-HTML converter.
 * Reusable module: any file can require_once this and call MarkdownConverter::toHtml().
 * Handles: headings, bold, italic, inline code, fenced code blocks, blockquotes,
 *          ordered/unordered lists, horizontal rules, links, images, and paragraphs.
 */

class MarkdownConverter
{
    /**
     * Convert a markdown string to safe HTML.
     *
     * @param string $markdown  Raw markdown input.
     * @param bool   $sanitize  When true, strips any raw HTML tags in the input first.
     * @return string           HTML string ready for insertion into the DOM.
     */
    public static function toHtml(string $markdown, bool $sanitize = true): string
    {
        if ($sanitize) {
            $markdown = strip_tags($markdown);
        }

        $lines  = explode("\n", $markdown);
        $output = [];
        $i      = 0;
        $total  = count($lines);

        while ($i < $total) {
            $line = $lines[$i];

            // Fenced code block  ```lang ... ```
            if (preg_match('/^```(\w*)/', $line, $m)) {
                $lang  = htmlspecialchars($m[1], ENT_QUOTES, 'UTF-8');
                $code  = [];
                $i++;
                while ($i < $total && !preg_match('/^```/', $lines[$i])) {
                    $code[] = htmlspecialchars($lines[$i], ENT_QUOTES, 'UTF-8');
                    $i++;
                }
                $langAttr = $lang ? ' class="language-' . $lang . '"' : '';
                $output[] = '<pre><code' . $langAttr . '>' . implode("\n", $code) . '</code></pre>';
                $i++;
                continue;
            }

            // Headings  # ## ###
            if (preg_match('/^(#{1,6})\s+(.+)/', $line, $m)) {
                $level    = strlen($m[1]);
                $text     = self::inline($m[2]);
                $output[] = "<h{$level}>{$text}</h{$level}>";
                $i++;
                continue;
            }

            // Horizontal rule  --- *** ___
            if (preg_match('/^[-*_]{3,}\s*$/', $line)) {
                $output[] = '<hr>';
                $i++;
                continue;
            }

            // Blockquote  > text
            if (preg_match('/^>\s?(.*)/', $line, $m)) {
                $inner = [$m[1]];
                $i++;
                while ($i < $total && preg_match('/^>\s?(.*)/', $lines[$i], $bm)) {
                    $inner[] = $bm[1];
                    $i++;
                }
                $bqContent = self::toHtml(implode("\n", $inner), false);
                $output[]  = '<blockquote>' . $bqContent . '</blockquote>';
                continue;
            }

            // Unordered list  - * +
            if (preg_match('/^[-*+]\s+(.+)/', $line, $m)) {
                $items = [self::inline($m[1])];
                $i++;
                while ($i < $total && preg_match('/^[-*+]\s+(.+)/', $lines[$i], $lm)) {
                    $items[] = self::inline($lm[1]);
                    $i++;
                }
                $lis      = implode('', array_map(fn($t) => "<li>{$t}</li>", $items));
                $output[] = '<ul>' . $lis . '</ul>';
                continue;
            }

            // Ordered list  1. 2. ...
            if (preg_match('/^\d+\.\s+(.+)/', $line, $m)) {
                $items = [self::inline($m[1])];
                $i++;
                while ($i < $total && preg_match('/^\d+\.\s+(.+)/', $lines[$i], $lm)) {
                    $items[] = self::inline($lm[1]);
                    $i++;
                }
                $lis      = implode('', array_map(fn($t) => "<li>{$t}</li>", $items));
                $output[] = '<ol>' . $lis . '</ol>';
                continue;
            }

            // Empty line -- paragraph break
            if (trim($line) === '') {
                $output[] = '';
                $i++;
                continue;
            }

            // Paragraph: collect consecutive non-empty, non-special lines
            $para = [self::inline($line)];
            $i++;
            while ($i < $total) {
                $next = $lines[$i];
                if (
                    trim($next) === '' ||
                    preg_match('/^(#{1,6}\s|[-*+]\s|\d+\.\s|>|```|[-*_]{3})/', $next)
                ) {
                    break;
                }
                $para[] = self::inline($next);
                $i++;
            }
            $output[] = '<p>' . implode(' ', $para) . '</p>';
        }

        return implode("\n", array_filter($output, fn($l) => $l !== ''));
    }

    /**
     * Process inline markdown: bold, italic, code, links, images, strikethrough.
     */
    private static function inline(string $text): string
    {
        // Escape HTML first (except already-escaped content from code blocks)
        $text = htmlspecialchars($text, ENT_QUOTES, 'UTF-8');

        // Inline code  `code`
        $text = preg_replace('/`([^`]+)`/', '<code>$1</code>', $text);

        // Bold  **text** or __text__
        $text = preg_replace('/\*\*(.+?)\*\*|__(.+?)__/', '<strong>$1$2</strong>', $text);

        // Italic  *text* or _text_
        $text = preg_replace('/\*(.+?)\*|_(.+?)_/', '<em>$1$2</em>', $text);

        // Strikethrough  ~~text~~
        $text = preg_replace('/~~(.+?)~~/', '<del>$1</del>', $text);

        // Image  ![alt](url)
        $text = preg_replace(
            '/!\[([^\]]*)\]\(([^)]+)\)/',
            '<img src="$2" alt="$1">',
            $text
        );

        // Link  [text](url)
        $text = preg_replace(
            '/\[([^\]]+)\]\(([^)]+)\)/',
            '<a href="$2" target="_blank" rel="noopener noreferrer">$1</a>',
            $text
        );

        return $text;
    }

    /**
     * Strip all markdown syntax and return plain text.
     * Useful for generating preview snippets or aria-labels.
     */
    public static function toPlainText(string $markdown): string
    {
        return strip_tags(self::toHtml($markdown));
    }
}
