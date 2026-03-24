<?php
/**
 * php pb_admin/tools/smoke_live_editor.php
 */
$root = dirname(dirname(__DIR__));

$required = [
    $root . '/js/pb-live-editor.js',
    $root . '/js/pb-ai-assist.js',
    $root . '/js/ts/pb-live-editor.ts',
    $root . '/js/ts/pb-ai-assist.ts',
    $root . '/pb_admin/live_editor.php',
    $root . '/pb_admin/ai_assist_proxy.php',
    $root . '/pb_admin/tools/09-live-editor-ai.php',
];

$fail = [];
foreach ($required as $f) {
    if (!file_exists($f)) $fail[] = "missing file: {$f}";
}

$proxy = $root . '/pb_admin/ai_assist_proxy.php';
if (file_exists($proxy)) {
    $txt = file_get_contents($proxy);
    if ($txt === false) {
        $fail[] = 'cannot read ai_assist_proxy.php';
    } else {
        if (strpos($txt, 'gpt-4o-mini') === false) $fail[] = 'proxy missing gpt-4o-mini model mapping';
        if (strpos($txt, 'gpt-4o') === false) $fail[] = 'proxy missing gpt-4o model mapping';
        if (strpos($txt, 'Desktop') === false || strpos($txt, 'keys') === false) {
            $fail[] = 'proxy missing Desktop keys external path hint';
        }
    }
}

if (!empty($fail)) {
    fwrite(STDERR, "[page-builder smoke] FAIL\n" . implode("\n", $fail) . "\n");
    exit(1);
}

echo "[page-builder smoke] OK: live editor + ai helper wiring present\n";
