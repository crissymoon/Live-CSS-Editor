#include "function_categories.h"

static const SQLFunction aggregate_functions[] = {
    {
        "SUM - Total of Values",
        "Calculate the sum of all values in a column",
        "SELECT SUM(column_name) as total FROM table_name;"
    },
    {
        "AVG - Average Value",
        "Calculate the average of values in a column",
        "SELECT AVG(column_name) as average FROM table_name;"
    },
    {
        "COUNT - Count Rows",
        "Count the number of rows or non-null values",
        "SELECT COUNT(*) as total_rows FROM table_name;\n"
        "-- Count non-null values:\n"
        "SELECT COUNT(column_name) as non_null_count FROM table_name;"
    },
    {
        "MIN/MAX - Minimum and Maximum",
        "Find the minimum and maximum values",
        "SELECT \n"
        "    MIN(column_name) as minimum,\n"
        "    MAX(column_name) as maximum\n"
        "FROM table_name;"
    },
    {
        "GROUP BY - Grouped Aggregates",
        "Calculate aggregates for each group",
        "SELECT \n"
        "    category_column,\n"
        "    COUNT(*) as count,\n"
        "    SUM(value_column) as total,\n"
        "    AVG(value_column) as average\n"
        "FROM table_name\n"
        "GROUP BY category_column\n"
        "ORDER BY total DESC;"
    },
    {
        "HAVING - Filter Aggregates",
        "Filter groups based on aggregate conditions",
        "SELECT \n"
        "    category_column,\n"
        "    COUNT(*) as count,\n"
        "    AVG(value_column) as average\n"
        "FROM table_name\n"
        "GROUP BY category_column\n"
        "HAVING COUNT(*) > 5\n"
        "ORDER BY count DESC;"
    },
    {
        "Conditional SUM",
        "Sum values based on conditions",
        "SELECT \n"
        "    SUM(CASE WHEN condition_column = 'value' THEN amount ELSE 0 END) as conditional_sum,\n"
        "    SUM(amount) as total_sum\n"
        "FROM table_name;"
    },
    {
        "Running Total",
        "Calculate cumulative sum over rows",
        "SELECT \n"
        "    date_column,\n"
        "    amount,\n"
        "    SUM(amount) OVER (ORDER BY date_column) as running_total\n"
        "FROM table_name\n"
        "ORDER BY date_column;"
    },
    {
        "GROUP_CONCAT - Combine Strings",
        "Concatenate strings from multiple rows",
        "SELECT \n"
        "    category_column,\n"
        "    GROUP_CONCAT(name_column, ', ') as combined_names,\n"
        "    COUNT(*) as count\n"
        "FROM table_name\n"
        "GROUP BY category_column;"
    },
    {
        "Percentage Calculation",
        "Calculate percentages of totals",
        "SELECT \n"
        "    category_column,\n"
        "    COUNT(*) as count,\n"
        "    ROUND(COUNT(*) * 100.0 / (SELECT COUNT(*) FROM table_name), 2) as percentage\n"
        "FROM table_name\n"
        "GROUP BY category_column\n"
        "ORDER BY percentage DESC;"
    }
};

const FunctionCategory aggregate_category = {
    "Aggregate Functions",
    aggregate_functions,
    sizeof(aggregate_functions) / sizeof(aggregate_functions[0])
};
