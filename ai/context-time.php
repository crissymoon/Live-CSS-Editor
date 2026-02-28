<?php
/**
 * Temporal and cultural context endpoint.
 *
 * Returns a JSON object describing the current date, time, timezone, broad
 * cultural region, season, and time-of-day band.  This data is injected into
 * AI system prompts so the model is grounded in the real world moment.
 *
 * The client should send an X-Timezone header containing a valid IANA timezone
 * string (e.g. "America/New_York") so server time is expressed in the user zone.
 * The Accept-Language request header is used to infer cultural region.
 *
 * No external dependencies.  PHP 8.0 or later required.
 */

declare(strict_types=1);

header('Content-Type: application/json');
header('Cache-Control: no-store, no-cache');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Headers: Content-Type, X-Timezone');

// --------------------------------------------------------------------------
// Resolve timezone from client header or fall back to UTC
// --------------------------------------------------------------------------

$requestedTZ   = trim($_SERVER['HTTP_X_TIMEZONE'] ?? '');
$validZones    = array_flip(DateTimeZone::listIdentifiers());
$useTZ         = ($requestedTZ !== '' && isset($validZones[$requestedTZ]))
                 ? $requestedTZ
                 : 'UTC';

$tz  = new DateTimeZone($useTZ);
$now = new DateTimeImmutable('now', $tz);

$utcOffsetHours = (int) round($now->getOffset() / 3600, 0);

// --------------------------------------------------------------------------
// Locale and cultural region from Accept-Language
// --------------------------------------------------------------------------

$acceptLang    = $_SERVER['HTTP_ACCEPT_LANGUAGE'] ?? 'en-US';
$primaryTag    = strtolower(explode(',', explode(';', $acceptLang)[0])[0]);
$parts         = explode('-', $primaryTag);
$langCode      = $parts[0];
$regionCode    = isset($parts[1]) ? strtoupper($parts[1]) : '';

$culturalArea  = resolveCulturalArea($langCode, $regionCode);
$direction     = resolveWritingDirection($langCode);

// --------------------------------------------------------------------------
// Time-of-day band (based on local hour)
// --------------------------------------------------------------------------

$localHour  = (int) $now->format('G');
$timeOfDay  = resolveTimeOfDay($localHour);

// --------------------------------------------------------------------------
// Season (accounts for Southern Hemisphere)
// --------------------------------------------------------------------------

$southernRegions = ['AU', 'NZ', 'ZA', 'AR', 'BR', 'CL', 'BO', 'PE', 'EC', 'CO', 'PY', 'UY'];
$isSouthern      = in_array($regionCode, $southernRegions, true);
$season          = resolveSeason((int) $now->format('n'), $isSouthern);

// --------------------------------------------------------------------------
// ISO week
// --------------------------------------------------------------------------

$isoWeek     = (int) $now->format('W');
$isoWeekYear = (int) $now->format('o');

// --------------------------------------------------------------------------
// Build summary sentence for AI prompt injection
// --------------------------------------------------------------------------

$shortOffset  = $utcOffsetHours >= 0 ? '+' . $utcOffsetHours : (string) $utcOffsetHours;
$promptLine   = sprintf(
    'Current date and time: %s %s (UTC%s, %s). Season: %s. Cultural region: %s. Locale: %s.',
    $now->format('l, F j, Y'),
    $now->format('H:i'),
    $shortOffset,
    $useTZ,
    $season,
    $culturalArea,
    $primaryTag
);

// --------------------------------------------------------------------------
// Respond
// --------------------------------------------------------------------------

echo json_encode([
    'iso8601'       => $now->format(DateTimeInterface::ATOM),
    'date_long'     => $now->format('l, F j, Y'),
    'time_24'       => $now->format('H:i'),
    'day_of_week'   => $now->format('l'),
    'month'         => $now->format('F'),
    'year'          => (int) $now->format('Y'),
    'timezone'      => $useTZ,
    'utc_offset'    => $utcOffsetHours,
    'time_of_day'   => $timeOfDay,
    'iso_week'      => $isoWeek,
    'iso_week_year' => $isoWeekYear,
    'season'        => $season,
    'locale'        => $primaryTag,
    'lang'          => $langCode,
    'region'        => $regionCode ?: 'US',
    'cultural_area' => $culturalArea,
    'direction'     => $direction,
    'prompt_line'   => $promptLine,
], JSON_UNESCAPED_UNICODE);


// ==========================================================================
// Functions
// ==========================================================================

/**
 * Resolve a broad cultural area label from BCP 47 language and region codes.
 * This is intentionally coarse for prompt grounding purposes.
 */
function resolveCulturalArea(string $lang, string $region): string
{
    static $eastAsian  = ['zh', 'ja', 'ko'];
    static $southAsian = ['hi', 'ur', 'bn', 'ta', 'te', 'mr', 'gu', 'kn', 'ml', 'pa', 'si'];
    static $rtlLangs   = ['ar', 'he', 'fa', 'ps', 'dv', 'yi'];
    static $nordic     = ['sv', 'no', 'nb', 'nn', 'da', 'fi', 'is', 'et', 'lv', 'lt'];
    static $slavic     = ['ru', 'pl', 'cs', 'sk', 'uk', 'bg', 'sr', 'hr', 'sl', 'mk', 'bs', 'be'];
    static $latin      = ['es', 'pt', 'fr', 'it', 'ro', 'ca', 'gl', 'oc'];
    static $germanic   = ['de', 'nl', 'af', 'lb'];
    static $latamCarib = ['MX','GT','BZ','HN','SV','NI','CR','PA','CU','DO','PR','JM','TT','HT'];
    static $latamSouth = ['AR','BO','BR','CL','CO','EC','PE','PY','UY','VE','GY','SR'];
    static $australas  = ['AU','NZ'];
    static $subSaharan = ['SW','KE','TZ','GH','NG','SN','ET','CM','CI','UG','ZW','ZM','RW','MZ'];

    if (in_array($lang, $eastAsian,  true)) { return 'East Asia'; }
    if (in_array($lang, $southAsian, true)) { return 'South Asia'; }
    if (in_array($lang, $rtlLangs,   true)) { return 'Middle East / North Africa'; }
    if (in_array($lang, $nordic,     true)) { return 'Northern Europe'; }
    if (in_array($lang, $slavic,     true)) { return 'Eastern Europe'; }
    if (in_array($lang, $germanic,   true)) { return 'Western Europe / Germanic'; }

    if (in_array($lang, $latin, true)) {
        if (in_array($region, $latamCarib, true)) { return 'Latin America / Caribbean'; }
        if (in_array($region, $latamSouth, true)) { return 'Latin America / South'; }
        return 'Western Europe / Latin';
    }

    if (in_array($region, $australas,  true)) { return 'Australasia'; }
    if (in_array($region, $subSaharan, true)) { return 'Sub-Saharan Africa'; }
    if ($region === 'IN')                     { return 'South Asia'; }
    if (in_array($region, ['CN','TW','HK','SG','MY'], true)) { return 'East Asia'; }

    return 'North America / English';
}

/**
 * Resolve the dominant writing direction for a language code.
 */
function resolveWritingDirection(string $lang): string
{
    static $rtl = ['ar', 'he', 'fa', 'ur', 'ps', 'dv', 'yi', 'syr', 'nqo', 'mende'];
    return in_array($lang, $rtl, true) ? 'rtl' : 'ltr';
}

/**
 * Resolve the season for a given month, with hemisphere inversion.
 */
function resolveSeason(int $month, bool $southern): string
{
    if ($southern) {
        if ($month >= 12 || $month <= 2) { return 'Summer'; }
        if ($month >= 3  && $month <= 5) { return 'Autumn'; }
        if ($month >= 6  && $month <= 8) { return 'Winter'; }
        return 'Spring';
    }
    if ($month >= 12 || $month <= 2) { return 'Winter'; }
    if ($month >= 3  && $month <= 5) { return 'Spring'; }
    if ($month >= 6  && $month <= 8) { return 'Summer'; }
    return 'Autumn';
}

/**
 * Return the time-of-day band for a local hour (24-hour).
 */
function resolveTimeOfDay(int $hour): string
{
    if ($hour >= 5  && $hour < 12) { return 'Morning'; }
    if ($hour >= 12 && $hour < 17) { return 'Afternoon'; }
    if ($hour >= 17 && $hour < 21) { return 'Evening'; }
    return 'Night';
}
