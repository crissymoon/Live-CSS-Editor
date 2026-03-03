#include "function_categories.h"

static const SQLFunction mathematical_functions[] = {
    {
        "ROUND - Round Numbers",
        "Round numbers to specified decimal places",
        "SELECT \n"
        "    value_column,\n"
        "    ROUND(value_column) as rounded,\n"
        "    ROUND(value_column, 2) as rounded_2_decimals\n"
        "FROM table_name;"
    },
    {
        "ABS - Absolute Value",
        "Get absolute values (remove negative sign)",
        "SELECT \n"
        "    value_column,\n"
        "    ABS(value_column) as absolute_value\n"
        "FROM table_name;"
    },
    {
        "POWER and SQRT",
        "Calculate powers and square roots",
        "SELECT \n"
        "    value_column,\n"
        "    POWER(value_column, 2) as squared,\n"
        "    POWER(value_column, 3) as cubed,\n"
        "    SQRT(value_column) as square_root\n"
        "FROM table_name\n"
        "WHERE value_column >= 0;"
    },
    {
        "Logarithms",
        "Calculate natural and base-10 logarithms",
        "SELECT \n"
        "    value_column,\n"
        "    LOG(value_column) as natural_log,\n"
        "    LOG10(value_column) as base10_log\n"
        "FROM table_name\n"
        "WHERE value_column > 0;"
    },
    {
        "Trigonometric Functions",
        "Calculate sine, cosine, tangent",
        "SELECT \n"
        "    angle_degrees,\n"
        "    SIN(angle_degrees * 3.14159 / 180) as sine,\n"
        "    COS(angle_degrees * 3.14159 / 180) as cosine,\n"
        "    TAN(angle_degrees * 3.14159 / 180) as tangent\n"
        "FROM table_name;"
    },
    {
        "Random Numbers",
        "Generate random numbers",
        "SELECT \n"
        "    id_column,\n"
        "    ABS(RANDOM() % 100) as random_0_to_99,\n"
        "    (ABS(RANDOM() % 100) + 1) as random_1_to_100\n"
        "FROM table_name;"
    },
    {
        "Normalization (Min-Max Scaling)",
        "Scale values to 0-1 range",
        "WITH stats AS (\n"
        "    SELECT \n"
        "        MIN(value_column) as min_val,\n"
        "        MAX(value_column) as max_val\n"
        "    FROM table_name\n"
        ")\n"
        "SELECT \n"
        "    id_column,\n"
        "    value_column,\n"
        "    (value_column - min_val) * 1.0 / (max_val - min_val) as normalized\n"
        "FROM table_name, stats\n"
        "WHERE (max_val - min_val) > 0;"
    },
    {
        "Distance Calculation (Euclidean)",
        "Calculate distance between two points",
        "SELECT \n"
        "    a.id as id1,\n"
        "    b.id as id2,\n"
        "    SQRT(POWER(a.x - b.x, 2) + POWER(a.y - b.y, 2)) as distance\n"
        "FROM table_name a, table_name b\n"
        "WHERE a.id < b.id\n"
        "ORDER BY distance;"
    }
};

const FunctionCategory mathematical_category = {
    "Mathematical Functions",
    mathematical_functions,
    sizeof(mathematical_functions) / sizeof(mathematical_functions[0])
};
