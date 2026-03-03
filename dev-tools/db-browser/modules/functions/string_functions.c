#include "function_categories.h"

static const SQLFunction string_functions[] = {
    {
        "UPPER and LOWER - Case Conversion",
        "Convert text to upper or lower case",
        "SELECT \n"
        "    text_column,\n"
        "    UPPER(text_column) as uppercase,\n"
        "    LOWER(text_column) as lowercase\n"
        "FROM table_name;"
    },
    {
        "LENGTH - String Length",
        "Get the length of strings",
        "SELECT \n"
        "    text_column,\n"
        "    LENGTH(text_column) as length\n"
        "FROM table_name\n"
        "ORDER BY length DESC;"
    },
    {
        "SUBSTR - Substring Extraction",
        "Extract parts of strings",
        "SELECT \n"
        "    text_column,\n"
        "    SUBSTR(text_column, 1, 10) as first_10_chars,\n"
        "    SUBSTR(text_column, -5) as last_5_chars\n"
        "FROM table_name;"
    },
    {
        "TRIM - Remove Whitespace",
        "Remove leading and trailing spaces",
        "SELECT \n"
        "    text_column,\n"
        "    TRIM(text_column) as trimmed,\n"
        "    LTRIM(text_column) as left_trimmed,\n"
        "    RTRIM(text_column) as right_trimmed\n"
        "FROM table_name;"
    },
    {
        "REPLACE - String Replacement",
        "Replace text within strings",
        "SELECT \n"
        "    text_column,\n"
        "    REPLACE(text_column, 'old_text', 'new_text') as replaced\n"
        "FROM table_name;"
    },
    {
        "LIKE - Pattern Matching",
        "Search for patterns in strings",
        "SELECT * FROM table_name\n"
        "WHERE text_column LIKE '%search_term%';  -- Contains\n"
        "\n"
        "-- Other patterns:\n"
        "-- LIKE 'A%'    (starts with A)\n"
        "-- LIKE '%Z'    (ends with Z)\n"
        "-- LIKE 'A%Z'   (starts with A, ends with Z)\n"
        "-- LIKE '_A%'   (second char is A)"
    },
    {
        "Concatenation",
        "Combine multiple strings",
        "SELECT \n"
        "    first_name || ' ' || last_name as full_name,\n"
        "    'ID: ' || CAST(id_column AS TEXT) as id_label\n"
        "FROM table_name;"
    },
    {
        "INSTR - Find Position",
        "Find position of substring",
        "SELECT \n"
        "    text_column,\n"
        "    INSTR(text_column, 'search') as position\n"
        "FROM table_name\n"
        "WHERE INSTR(text_column, 'search') > 0;"
    },
    {
        "Extract Domain from Email",
        "Parse email addresses",
        "SELECT \n"
        "    email_column,\n"
        "    SUBSTR(email_column, 1, INSTR(email_column, '@') - 1) as username,\n"
        "    SUBSTR(email_column, INSTR(email_column, '@') + 1) as domain\n"
        "FROM table_name\n"
        "WHERE email_column LIKE '%@%';"
    },
    {
        "String Aggregation",
        "Combine strings from multiple rows",
        "SELECT \n"
        "    category_column,\n"
        "    GROUP_CONCAT(text_column, ', ') as combined_text,\n"
        "    GROUP_CONCAT(DISTINCT text_column) as unique_values\n"
        "FROM table_name\n"
        "GROUP BY category_column;"
    }
};

const FunctionCategory string_category = {
    "String Functions",
    string_functions,
    sizeof(string_functions) / sizeof(string_functions[0])
};
