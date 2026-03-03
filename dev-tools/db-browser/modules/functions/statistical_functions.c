#include "function_categories.h"

static const SQLFunction statistical_functions[] = {
    {
        "Standard Deviation",
        "Calculate standard deviation of values",
        "SELECT \n"
        "    AVG(value_column) as mean,\n"
        "    SQRT(AVG(value_column * value_column) - AVG(value_column) * AVG(value_column)) as std_dev\n"
        "FROM table_name;"
    },
    {
        "Variance",
        "Calculate variance of values",
        "SELECT \n"
        "    AVG(value_column * value_column) - AVG(value_column) * AVG(value_column) as variance\n"
        "FROM table_name;"
    },
    {
        "Median Calculation",
        "Find the median value",
        "SELECT AVG(value_column) as median\n"
        "FROM (\n"
        "    SELECT value_column\n"
        "    FROM table_name\n"
        "    ORDER BY value_column\n"
        "    LIMIT 2 - (SELECT COUNT(*) FROM table_name) % 2\n"
        "    OFFSET (SELECT (COUNT(*) - 1) / 2 FROM table_name)\n"
        ");"
    },
    {
        "Quartiles (Q1, Q2, Q3)",
        "Calculate the first, second, and third quartiles",
        "WITH ordered AS (\n"
        "    SELECT value_column,\n"
        "           ROW_NUMBER() OVER (ORDER BY value_column) as rn,\n"
        "           COUNT(*) OVER () as total\n"
        "    FROM table_name\n"
        ")\n"
        "SELECT \n"
        "    (SELECT value_column FROM ordered WHERE rn = CAST(total * 0.25 AS INTEGER)) as Q1,\n"
        "    (SELECT value_column FROM ordered WHERE rn = CAST(total * 0.50 AS INTEGER)) as Q2,\n"
        "    (SELECT value_column FROM ordered WHERE rn = CAST(total * 0.75 AS INTEGER)) as Q3\n"
        "FROM ordered LIMIT 1;"
    },
    {
        "Z-Score (Standardization)",
        "Calculate z-scores for normalization",
        "WITH stats AS (\n"
        "    SELECT \n"
        "        AVG(value_column) as mean,\n"
        "        SQRT(AVG(value_column * value_column) - AVG(value_column) * AVG(value_column)) as std_dev\n"
        "    FROM table_name\n"
        ")\n"
        "SELECT \n"
        "    id_column,\n"
        "    value_column,\n"
        "    (value_column - stats.mean) / stats.std_dev as z_score\n"
        "FROM table_name, stats\n"
        "ORDER BY z_score DESC;"
    },
    {
        "Correlation Coefficient",
        "Calculate correlation between two variables",
        "WITH stats AS (\n"
        "    SELECT \n"
        "        AVG(x_column) as mean_x,\n"
        "        AVG(y_column) as mean_y,\n"
        "        COUNT(*) as n\n"
        "    FROM table_name\n"
        ")\n"
        "SELECT \n"
        "    SUM((x_column - mean_x) * (y_column - mean_y)) / \n"
        "    (SQRT(SUM((x_column - mean_x) * (x_column - mean_x))) * \n"
        "     SQRT(SUM((y_column - mean_y) * (y_column - mean_y)))) as correlation\n"
        "FROM table_name, stats;"
    },
    {
        "Outlier Detection (IQR Method)",
        "Identify outliers using interquartile range",
        "WITH stats AS (\n"
        "    SELECT \n"
        "        value_column,\n"
        "        NTILE(4) OVER (ORDER BY value_column) as quartile\n"
        "    FROM table_name\n"
        "),\n"
        "quartiles AS (\n"
        "    SELECT \n"
        "        MAX(CASE WHEN quartile = 1 THEN value_column END) as Q1,\n"
        "        MAX(CASE WHEN quartile = 3 THEN value_column END) as Q3\n"
        "    FROM stats\n"
        ")\n"
        "SELECT t.*, (Q3 - Q1) as IQR\n"
        "FROM table_name t, quartiles\n"
        "WHERE t.value_column < Q1 - 1.5 * (Q3 - Q1)\n"
        "   OR t.value_column > Q3 + 1.5 * (Q3 - Q1);"
    }
};

const FunctionCategory statistical_category = {
    "Statistical Functions",
    statistical_functions,
    sizeof(statistical_functions) / sizeof(statistical_functions[0])
};
