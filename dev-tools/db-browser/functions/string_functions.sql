-- String Functions for Data Analysis
-- Text manipulation and pattern matching

-- ============================================
-- BASIC STRING OPERATIONS
-- ============================================

-- Length of string
SELECT 
    text_column,
    LENGTH(text_column) as string_length
FROM texts;

-- Concatenation
SELECT 
    first_name || ' ' || last_name as full_name,
    'Hello, ' || name || '!' as greeting
FROM people;

-- ============================================
-- CASE CONVERSION
-- ============================================

-- Uppercase
SELECT 
    UPPER(text_column) as uppercase
FROM texts;

-- Lowercase
SELECT 
    LOWER(text_column) as lowercase
FROM texts;

-- Title case (first letter uppercase)
SELECT 
    UPPER(SUBSTR(text_column, 1, 1)) || LOWER(SUBSTR(text_column, 2)) as title_case
FROM texts;

-- ============================================
-- SUBSTRING EXTRACTION
-- ============================================

-- SUBSTR(string, start, length)
-- Note: SQLite uses 1-based indexing

SELECT 
    SUBSTR(text_column, 1, 5) as first_5_chars,
    SUBSTR(text_column, -5) as last_5_chars,
    SUBSTR(email, 1, INSTR(email, '@') - 1) as username_from_email,
    SUBSTR(email, INSTR(email, '@') + 1) as domain_from_email
FROM data;

-- ============================================
-- TRIMMING WHITESPACE
-- ============================================

-- Trim both sides
SELECT 
    TRIM(text_column) as trimmed,
    LTRIM(text_column) as left_trimmed,
    RTRIM(text_column) as right_trimmed
FROM texts;

-- Trim specific characters
SELECT 
    TRIM(text_column, 'x') as trim_x,
    TRIM(text_column, '.') as trim_dots
FROM texts;

-- ============================================
-- REPLACE
-- ============================================

-- Replace substring
SELECT 
    REPLACE(text_column, 'old', 'new') as replaced,
    REPLACE(phone, '-', '') as phone_no_dashes,
    REPLACE(REPLACE(REPLACE(text, ',', ''), '.', ''), '!', '') as remove_punctuation
FROM texts;

-- ============================================
-- PADDING
-- ============================================

-- Left pad with zeros
SELECT 
    id,
    SUBSTR('00000' || id, -5) as padded_id
FROM items;

-- Right pad
SELECT 
    text_column || SUBSTR('          ', 1, 10 - LENGTH(text_column)) as right_padded
FROM texts
WHERE LENGTH(text_column) < 10;

-- ============================================
-- PATTERN MATCHING (LIKE)
-- ============================================

-- Wildcards: % (any chars), _ (single char)

-- Starts with
SELECT * FROM products
WHERE product_name LIKE 'Apple%';

-- Ends with
SELECT * FROM products
WHERE product_name LIKE '%Pro';

-- Contains
SELECT * FROM products
WHERE description LIKE '%wireless%';

-- Pattern with single character wildcard
SELECT * FROM codes
WHERE code LIKE 'A_C';  -- Matches ABC, A1C, etc.

-- Case-insensitive search
SELECT * FROM products
WHERE LOWER(product_name) LIKE '%iphone%';

-- ============================================
-- GLOB (Unix-style pattern matching)
-- ============================================

-- * = any chars, ? = single char, [] = character class

SELECT * FROM files
WHERE filename GLOB '*.txt';  -- Files ending in .txt

SELECT * FROM codes
WHERE code GLOB '[A-Z][0-9]*';  -- Letter followed by digits

-- ============================================
-- INSTR (Find position)
-- ============================================

-- Find first occurrence of substring (1-based, returns 0 if not found)
SELECT 
    email,
    INSTR(email, '@') as at_position,
    INSTR(email, '.') as dot_position,
    CASE 
        WHEN INSTR(email, '@') > 0 THEN 'Valid'
        ELSE 'Invalid'
    END as email_validation
FROM contacts;

-- ============================================
-- STRING SPLITTING
-- ============================================

-- Extract first part (before delimiter)
SELECT 
    full_name,
    SUBSTR(full_name, 1, INSTR(full_name, ' ') - 1) as first_name
FROM people
WHERE INSTR(full_name, ' ') > 0;

-- Extract second part (after delimiter)
SELECT 
    full_name,
    SUBSTR(full_name, INSTR(full_name, ' ') + 1) as last_name
FROM people
WHERE INSTR(full_name, ' ') > 0;

-- Extract domain from email
SELECT 
    email,
    SUBSTR(email, INSTR(email, '@') + 1) as domain
FROM contacts;

-- Extract extension from filename
SELECT 
    filename,
    SUBSTR(filename, LENGTH(filename) - INSTR(REVERSE(filename), '.') + 2) as extension
FROM files
WHERE INSTR(filename, '.') > 0;

-- ============================================
-- REVERSE STRING
-- ============================================

-- Reverse function (custom implementation needed)
WITH RECURSIVE reverse_cte(original, reversed, pos) AS (
    SELECT text_column, '', LENGTH(text_column) FROM texts
    UNION ALL
    SELECT original, reversed || SUBSTR(original, pos, 1), pos - 1
    FROM reverse_cte
    WHERE pos > 0
)
SELECT original, reversed FROM reverse_cte WHERE pos = 0;

-- ============================================
-- COUNT OCCURRENCES
-- ============================================

-- Count occurrences of substring
SELECT 
    text_column,
    LENGTH(text_column) - LENGTH(REPLACE(text_column, 'a', '')) as count_of_a
FROM texts;

-- Count words (approximate)
SELECT 
    text_column,
    LENGTH(text_column) - LENGTH(REPLACE(text_column, ' ', '')) + 1 as word_count_approx
FROM texts;

-- ============================================
-- INITIALS
-- ============================================

-- Get initials from full name
SELECT 
    full_name,
    UPPER(SUBSTR(full_name, 1, 1)) || 
    UPPER(SUBSTR(full_name, INSTR(full_name, ' ') + 1, 1)) as initials
FROM people;

-- ============================================
-- STRING COMPARISON
-- ============================================

-- Case-sensitive comparison
SELECT * FROM items
WHERE name = 'Product';

-- Case-insensitive comparison
SELECT * FROM items
WHERE LOWER(name) = LOWER('product');

-- Collation
SELECT * FROM items
WHERE name COLLATE NOCASE = 'product';

-- ============================================
-- VALIDATION
-- ============================================

-- Email validation (basic)
SELECT 
    email,
    CASE 
        WHEN email LIKE '%@%.%' 
         AND LENGTH(email) > 5 
         AND INSTR(email, '@') > 1
         AND INSTR(email, '.') > INSTR(email, '@') + 1
        THEN 'Valid'
        ELSE 'Invalid'
    END as email_status
FROM contacts;

-- Phone number validation (10 digits)
SELECT 
    phone,
    CASE 
        WHEN LENGTH(REPLACE(REPLACE(REPLACE(phone, '-', ''), '(', ''), ')', '')) = 10
        THEN 'Valid'
        ELSE 'Invalid'
    END as phone_status
FROM contacts;

-- ============================================
-- FORMATTING
-- ============================================

-- Format phone number
SELECT 
    phone_digits,
    '(' || SUBSTR(phone_digits, 1, 3) || ') ' ||
    SUBSTR(phone_digits, 4, 3) || '-' ||
    SUBSTR(phone_digits, 7, 4) as formatted_phone
FROM contacts
WHERE LENGTH(phone_digits) = 10;

-- Format credit card (mask middle digits)
SELECT 
    card_number,
    SUBSTR(card_number, 1, 4) || '-****-****-' || SUBSTR(card_number, -4) as masked_card
FROM payments;

-- Format currency
SELECT 
    amount,
    '$' || REPLACE(
        PRINTF('%.2f', amount),
        SUBSTR(PRINTF('%.2f', amount), 1, LENGTH(PRINTF('%.0f', amount)) - LENGTH(PRINTF('%.0f', amount)) % 3),
        REPLACE(SUBSTR(PRINTF('%.2f', amount), 1, LENGTH(PRINTF('%.0f', amount)) - LENGTH(PRINTF('%.0f', amount)) % 3), '', ',')
    ) as formatted_currency
FROM transactions;

-- ============================================
-- TEXT CLEANING
-- ============================================

-- Remove extra spaces
SELECT 
    TRIM(REPLACE(REPLACE(REPLACE(text_column, '  ', ' '), '  ', ' '), '  ', ' ')) as cleaned
FROM texts;

-- Remove special characters
SELECT 
    REPLACE(
        REPLACE(
            REPLACE(
                REPLACE(text_column, '!', ''),
            '@', ''),
        '#', ''),
    '$', '') as cleaned
FROM texts;

-- Normalize whitespace to single space
SELECT 
    TRIM(
        REPLACE(
            REPLACE(
                REPLACE(text_column, CHAR(9), ' '),  -- tab to space
            CHAR(10), ' '),  -- newline to space
        CHAR(13), ' ')  -- carriage return to space
    ) as normalized
FROM texts;

-- ============================================
-- ACRONYMS
-- ============================================

-- Create acronym from phrase
SELECT 
    phrase,
    UPPER(
        SUBSTR(phrase, 1, 1) ||
        CASE WHEN INSTR(phrase, ' ') > 0 THEN SUBSTR(phrase, INSTR(phrase, ' ') + 1, 1) ELSE '' END
    ) as acronym
FROM phrases;

-- ============================================
-- SLUG GENERATION
-- ============================================

-- Create URL-friendly slug
SELECT 
    title,
    LOWER(REPLACE(REPLACE(REPLACE(title, ' ', '-'), '_', '-'), '--', '-')) as slug
FROM articles;

-- ============================================
-- TEXT STATISTICS
-- ============================================

-- Character distribution
SELECT 
    'Letters' as type,
    SUM(LENGTH(text_column) - LENGTH(REPLACE(REPLACE(LOWER(text_column), 
        'abcdefghijklmnopqrstuvwxyz', ''), 
        'ABCDEFGHIJKLMNOPQRSTUVWXYZ', ''))) as count
FROM texts
UNION ALL
SELECT 
    'Digits',
    SUM(LENGTH(text_column) - LENGTH(REPLACE(text_column, '0123456789', '')))
FROM texts
UNION ALL
SELECT 
    'Spaces',
    SUM(LENGTH(text_column) - LENGTH(REPLACE(text_column, ' ', '')))
FROM texts;

-- ============================================
-- LEVENSHTEIN DISTANCE (SIMILARITY)
-- ============================================

-- Note: SQLite doesn't have built-in Levenshtein distance
-- Can be implemented with extension or in application code

-- Simple similarity check (shared characters)
SELECT 
    string1,
    string2,
    (LENGTH(string1) + LENGTH(string2) - 
     LENGTH(REPLACE(string1, string2, ''))) * 100.0 / 
    GREATEST(LENGTH(string1), LENGTH(string2)) as similarity_percent
FROM string_pairs;

-- ============================================
-- PRINTF FORMATTING
-- ============================================

-- Formatted output
SELECT 
    PRINTF('%10s', name) as padded_name,           -- Right-aligned, width 10
    PRINTF('%-10s', name) as left_padded_name,     -- Left-aligned, width 10
    PRINTF('%05d', id) as zero_padded_id,          -- Zero-padded number
    PRINTF('%.2f', price) as two_decimal_price,    -- Two decimal places
    PRINTF('%e', large_number) as scientific,      -- Scientific notation
    PRINTF('%x', number) as hexadecimal           -- Hexadecimal
FROM items;

-- ============================================
-- HEX ENCODING
-- ============================================

SELECT 
    text_column,
    HEX(text_column) as hexadecimal,
    UNHEX(HEX(text_column)) as back_to_text
FROM texts;

-- ============================================
-- QUOTE (SQL Escaping)
-- ============================================

SELECT 
    QUOTE(text_column) as escaped_for_sql
FROM texts;

-- ============================================
-- AGGREGATING STRINGS
-- ============================================

-- Concatenate with GROUP_CONCAT
SELECT 
    category,
    GROUP_CONCAT(product_name, ', ') as products,
    GROUP_CONCAT(DISTINCT brand) as brands
FROM products
GROUP BY category;

-- Ordered concatenation
SELECT 
    category,
    GROUP_CONCAT(product_name, ', ') as products_ordered
FROM (
    SELECT category, product_name
    FROM products
    ORDER BY product_name
)
GROUP BY category;

-- ============================================
-- FUZZY MATCHING
-- ============================================

-- Soundex-like matching (first letter + pattern)
SELECT 
    name1,
    name2,
    CASE 
        WHEN UPPER(SUBSTR(name1, 1, 1)) = UPPER(SUBSTR(name2, 1, 1)) 
         AND LENGTH(name1) = LENGTH(name2)
        THEN 'Possible Match'
        ELSE 'Different'
    END as match_status
FROM name_pairs;
