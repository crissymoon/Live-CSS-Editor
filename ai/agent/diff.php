<?php
/**
 * Agent Diff Engine
 * Generates a unified line-by-line diff between two text strings.
 * Returns structured data that the JS diff renderer turns into HTML.
 * Also provides a convenience toHtml() for server-side rendering.
 */

class AgentDiff
{
    /**
     * Produce a structured diff between two texts.
     * Returns an array of hunks, each hunk an array of lines:
     *   [ 'type' => 'context'|'add'|'remove', 'line' => string, 'old_no' => int|null, 'new_no' => int|null ]
     */
    public static function diff(string $oldText, string $newText, int $context = 3): array
    {
        $oldLines = explode("\n", $oldText);
        $newLines = explode("\n", $newText);

        $matrix = self::buildLCS($oldLines, $newLines);
        $ops    = self::backtrack($matrix, $oldLines, $newLines, count($oldLines), count($newLines));

        return self::groupHunks($ops, $context);
    }

    /**
     * Render diff as HTML table rows. Suitable for injection into a <tbody>.
     */
    public static function toHtmlRows(array $hunks): string
    {
        $html = '';
        foreach ($hunks as $hunk) {
            if ($hunk['type'] === 'separator') {
                $html .= '<tr class="diff-separator"><td colspan="4">...</td></tr>';
                continue;
            }
            $type   = htmlspecialchars($hunk['type'],  ENT_QUOTES, 'UTF-8');
            $line   = htmlspecialchars($hunk['line'],  ENT_QUOTES, 'UTF-8');
            $oldNo  = $hunk['old_no'] !== null ? $hunk['old_no'] : '';
            $newNo  = $hunk['new_no'] !== null ? $hunk['new_no'] : '';
            $sign   = $hunk['type'] === 'add' ? '+' : ($hunk['type'] === 'remove' ? '-' : ' ');
            $html  .= '<tr class="diff-line diff-' . $type . '">'
                    . '<td class="diff-old-no">'  . $oldNo . '</td>'
                    . '<td class="diff-new-no">'  . $newNo . '</td>'
                    . '<td class="diff-sign">'    . $sign  . '</td>'
                    . '<td class="diff-content"><code>' . $line . '</code></td>'
                    . '</tr>';
        }
        return $html;
    }

    /**
     * Quick summary: how many lines added, removed, changed.
     */
    public static function summary(array $hunks): array
    {
        $added   = 0;
        $removed = 0;
        foreach ($hunks as $h) {
            if ($h['type'] === 'add')    { $added++;   }
            if ($h['type'] === 'remove') { $removed++; }
        }
        return ['added' => $added, 'removed' => $removed];
    }

    // -------------------------------------------------------------------------
    // LCS (Longest Common Subsequence)
    // -------------------------------------------------------------------------

    private static function buildLCS(array $a, array $b): array
    {
        $m = count($a);
        $n = count($b);
        // Use rolling two-row DP to reduce memory for large files
        $prev = array_fill(0, $n + 1, 0);
        $curr = array_fill(0, $n + 1, 0);
        $full = [];

        for ($i = 1; $i <= $m; $i++) {
            for ($j = 1; $j <= $n; $j++) {
                if ($a[$i - 1] === $b[$j - 1]) {
                    $curr[$j] = $prev[$j - 1] + 1;
                } else {
                    $curr[$j] = max($prev[$j], $curr[$j - 1]);
                }
            }
            $full[$i] = $curr;
            $prev = $curr;
            $curr = array_fill(0, $n + 1, 0);
        }

        return $full;
    }

    private static function backtrack(
        array $matrix,
        array $a,
        array $b,
        int   $i,
        int   $j
    ): array {
        $ops = [];
        $oldNo = 1;
        $newNo = 1;
        // Rebuild full matrix rows if not present (rolling optimization means we stored all)
        // Simple iterative approach: compare directly
        $aLines = $a;
        $bLines = $b;
        $m      = count($aLines);
        $n      = count($bLines);

        // Build full matrix for backtracking
        $dp = array_fill(0, $m + 1, array_fill(0, $n + 1, 0));
        for ($ii = 1; $ii <= $m; $ii++) {
            for ($jj = 1; $jj <= $n; $jj++) {
                if ($aLines[$ii - 1] === $bLines[$jj - 1]) {
                    $dp[$ii][$jj] = $dp[$ii - 1][$jj - 1] + 1;
                } else {
                    $dp[$ii][$jj] = max($dp[$ii - 1][$jj], $dp[$ii][$jj - 1]);
                }
            }
        }

        $ii = $m; $jj = $n;
        $ops = [];
        while ($ii > 0 || $jj > 0) {
            if ($ii > 0 && $jj > 0 && $aLines[$ii - 1] === $bLines[$jj - 1]) {
                array_unshift($ops, [
                    'type'   => 'context',
                    'line'   => $aLines[$ii - 1],
                    'old_no' => $ii,
                    'new_no' => $jj,
                ]);
                $ii--; $jj--;
            } elseif ($jj > 0 && ($ii === 0 || $dp[$ii][$jj - 1] >= $dp[$ii - 1][$jj])) {
                array_unshift($ops, [
                    'type'   => 'add',
                    'line'   => $bLines[$jj - 1],
                    'old_no' => null,
                    'new_no' => $jj,
                ]);
                $jj--;
            } else {
                array_unshift($ops, [
                    'type'   => 'remove',
                    'line'   => $aLines[$ii - 1],
                    'old_no' => $ii,
                    'new_no' => null,
                ]);
                $ii--;
            }
        }
        return $ops;
    }

    private static function groupHunks(array $ops, int $ctx): array
    {
        if (empty($ops)) { return []; }

        $total       = count($ops);
        $changeIdx   = [];
        foreach ($ops as $i => $op) {
            if ($op['type'] !== 'context') {
                $changeIdx[] = $i;
            }
        }

        if (empty($changeIdx)) { return []; }

        // Build inclusive ranges [start, end] of lines to show
        $ranges = [];
        foreach ($changeIdx as $ci) {
            $start = max(0, $ci - $ctx);
            $end   = min($total - 1, $ci + $ctx);
            if (empty($ranges)) {
                $ranges[] = [$start, $end];
            } else {
                $last = &$ranges[count($ranges) - 1];
                if ($start <= $last[1] + 1) {
                    $last[1] = max($last[1], $end);
                } else {
                    $ranges[] = [$start, $end];
                }
            }
        }

        $result   = [];
        $prevEnd  = -1;
        foreach ($ranges as [$start, $end]) {
            if ($prevEnd >= 0 && $start > $prevEnd + 1) {
                $result[] = ['type' => 'separator', 'line' => '', 'old_no' => null, 'new_no' => null];
            }
            for ($i = $start; $i <= $end; $i++) {
                $result[] = $ops[$i];
            }
            $prevEnd = $end;
        }
        return $result;
    }
}
