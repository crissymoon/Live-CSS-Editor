#include "function_categories.h"

static const SQLFunction window_functions[] = {
    {
        "ROW_NUMBER - Sequential Numbering",
        "Assign sequential numbers to rows",
        "SELECT \n"
        "    ROW_NUMBER() OVER (ORDER BY value_column DESC) as row_num,\n"
        "    id_column,\n"
        "    value_column\n"
        "FROM table_name\n"
        "ORDER BY row_num;"
    },
    {
        "RANK - Ranking with Gaps",
        "Rank rows with same values getting same rank",
        "SELECT \n"
        "    RANK() OVER (ORDER BY score DESC) as rank,\n"
        "    name_column,\n"
        "    score\n"
        "FROM table_name\n"
        "ORDER BY rank;"
    },
    {
        "DENSE_RANK - Ranking without Gaps",
        "Rank rows without gaps in rank numbers",
        "SELECT \n"
        "    DENSE_RANK() OVER (ORDER BY score DESC) as dense_rank,\n"
        "    name_column,\n"
        "    score\n"
        "FROM table_name\n"
        "ORDER BY dense_rank;"
    },
    {
        "LAG and LEAD - Previous/Next Values",
        "Access previous or next row values",
        "SELECT \n"
        "    date_column,\n"
        "    value_column,\n"
        "    LAG(value_column, 1) OVER (ORDER BY date_column) as previous_value,\n"
        "    LEAD(value_column, 1) OVER (ORDER BY date_column) as next_value,\n"
        "    value_column - LAG(value_column, 1) OVER (ORDER BY date_column) as change\n"
        "FROM table_name\n"
        "ORDER BY date_column;"
    },
    {
        "Running Total",
        "Calculate cumulative sum",
        "SELECT \n"
        "    date_column,\n"
        "    amount,\n"
        "    SUM(amount) OVER (ORDER BY date_column) as running_total,\n"
        "    ROUND(AVG(amount) OVER (ORDER BY date_column), 2) as running_average\n"
        "FROM table_name\n"
        "ORDER BY date_column;"
    },
    {
        "Moving Average",
        "Calculate rolling averages",
        "SELECT \n"
        "    date_column,\n"
        "    value_column,\n"
        "    AVG(value_column) OVER (\n"
        "        ORDER BY date_column \n"
        "        ROWS BETWEEN 6 PRECEDING AND CURRENT ROW\n"
        "    ) as moving_avg_7day\n"
        "FROM table_name\n"
        "ORDER BY date_column;"
    },
    {
        "NTILE - Divide into Buckets",
        "Divide rows into equal groups",
        "SELECT \n"
        "    id_column,\n"
        "    value_column,\n"
        "    NTILE(4) OVER (ORDER BY value_column) as quartile,\n"
        "    NTILE(10) OVER (ORDER BY value_column) as decile\n"
        "FROM table_name\n"
        "ORDER BY value_column;"
    },
    {
        "Top N per Group",
        "Get top records within each category",
        "WITH ranked AS (\n"
        "    SELECT \n"
        "        category_column,\n"
        "        name_column,\n"
        "        value_column,\n"
        "        ROW_NUMBER() OVER (\n"
        "            PARTITION BY category_column \n"
        "            ORDER BY value_column DESC\n"
        "        ) as rn\n"
        "    FROM table_name\n"
        ")\n"
        "SELECT category_column, name_column, value_column\n"
        "FROM ranked\n"
        "WHERE rn <= 3\n"
        "ORDER BY category_column, rn;"
    },
    {
        "FIRST_VALUE and LAST_VALUE",
        "Get first or last value in partition",
        "SELECT \n"
        "    category_column,\n"
        "    date_column,\n"
        "    value_column,\n"
        "    FIRST_VALUE(value_column) OVER (\n"
        "        PARTITION BY category_column \n"
        "        ORDER BY date_column\n"
        "    ) as first_value,\n"
        "    LAST_VALUE(value_column) OVER (\n"
        "        PARTITION BY category_column \n"
        "        ORDER BY date_column\n"
        "        ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING\n"
        "    ) as last_value\n"
        "FROM table_name\n"
        "ORDER BY category_column, date_column;"
    }
};

const FunctionCategory window_category = {
    "Window Functions",
    window_functions,
    sizeof(window_functions) / sizeof(window_functions[0])
};
