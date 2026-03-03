#include "function_categories.h"

static const SQLFunction datetime_functions[] = {
    {
        "Current Date and Time",
        "Get current date, time, and timestamp",
        "SELECT \n"
        "    DATE('now') as current_date,\n"
        "    TIME('now') as current_time,\n"
        "    DATETIME('now') as current_datetime,\n"
        "    STRFTIME('%s', 'now') as unix_timestamp;"
    },
    {
        "Date Parts Extraction",
        "Extract year, month, day from dates",
        "SELECT \n"
        "    date_column,\n"
        "    STRFTIME('%Y', date_column) as year,\n"
        "    STRFTIME('%m', date_column) as month,\n"
        "    STRFTIME('%d', date_column) as day,\n"
        "    STRFTIME('%w', date_column) as day_of_week,\n"
        "    STRFTIME('%j', date_column) as day_of_year\n"
        "FROM table_name;"
    },
    {
        "Date Arithmetic",
        "Add or subtract days, months, years",
        "SELECT \n"
        "    date_column,\n"
        "    DATE(date_column, '+1 day') as tomorrow,\n"
        "    DATE(date_column, '-1 day') as yesterday,\n"
        "    DATE(date_column, '+1 month') as next_month,\n"
        "    DATE(date_column, '+1 year') as next_year,\n"
        "    DATE(date_column, '-7 days') as week_ago\n"
        "FROM table_name;"
    },
    {
        "Date Difference",
        "Calculate days between dates",
        "SELECT \n"
        "    id_column,\n"
        "    start_date,\n"
        "    end_date,\n"
        "    JULIANDAY(end_date) - JULIANDAY(start_date) as days_between\n"
        "FROM table_name;"
    },
    {
        "Age Calculation",
        "Calculate age or time elapsed",
        "SELECT \n"
        "    id_column,\n"
        "    birth_date,\n"
        "    CAST((JULIANDAY('now') - JULIANDAY(birth_date)) / 365.25 AS INTEGER) as age_years,\n"
        "    CAST(JULIANDAY('now') - JULIANDAY(birth_date) AS INTEGER) as age_days\n"
        "FROM table_name;"
    },
    {
        "Date Formatting",
        "Format dates in different ways",
        "SELECT \n"
        "    date_column,\n"
        "    STRFTIME('%Y-%m-%d', date_column) as iso_format,\n"
        "    STRFTIME('%m/%d/%Y', date_column) as us_format,\n"
        "    STRFTIME('%d/%m/%Y', date_column) as eu_format,\n"
        "    STRFTIME('%Y-%m-%d %H:%M:%S', date_column) as full_datetime,\n"
        "    STRFTIME('%W', date_column) as week_number\n"
        "FROM table_name;"
    },
    {
        "Group by Date Period",
        "Aggregate by day, month, quarter, year",
        "SELECT \n"
        "    STRFTIME('%Y-%m', date_column) as year_month,\n"
        "    COUNT(*) as count,\n"
        "    SUM(value_column) as total\n"
        "FROM table_name\n"
        "GROUP BY year_month\n"
        "ORDER BY year_month;"
    },
    {
        "Start and End of Month",
        "Get first and last day of month",
        "SELECT \n"
        "    date_column,\n"
        "    DATE(date_column, 'start of month') as month_start,\n"
        "    DATE(date_column, 'start of month', '+1 month', '-1 day') as month_end\n"
        "FROM table_name;"
    }
};

const FunctionCategory datetime_category = {
    "Date and Time Functions",
    datetime_functions,
    sizeof(datetime_functions) / sizeof(datetime_functions[0])
};
