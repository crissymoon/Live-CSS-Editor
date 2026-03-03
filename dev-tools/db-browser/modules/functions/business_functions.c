#include "function_categories.h"

static const SQLFunction business_functions[] = {
    {
        "Revenue Growth Rate",
        "Calculate period-over-period growth",
        "SELECT \n"
        "    period,\n"
        "    revenue,\n"
        "    LAG(revenue) OVER (ORDER BY period) as prev_revenue,\n"
        "    ROUND(((revenue - LAG(revenue) OVER (ORDER BY period)) * 100.0 / \n"
        "           LAG(revenue) OVER (ORDER BY period)), 2) as growth_rate_pct\n"
        "FROM revenue_table\n"
        "ORDER BY period;"
    },
    {
        "Customer Lifetime Value",
        "Calculate total customer spending",
        "SELECT \n"
        "    customer_id,\n"
        "    COUNT(DISTINCT order_id) as total_orders,\n"
        "    SUM(amount) as lifetime_value,\n"
        "    AVG(amount) as avg_order_value,\n"
        "    MIN(order_date) as first_order,\n"
        "    MAX(order_date) as last_order\n"
        "FROM orders\n"
        "GROUP BY customer_id\n"
        "ORDER BY lifetime_value DESC;"
    },
    {
        "Profit Margin",
        "Calculate gross and net margins",
        "SELECT \n"
        "    product_id,\n"
        "    revenue,\n"
        "    cost,\n"
        "    revenue - cost as gross_profit,\n"
        "    ROUND(((revenue - cost) * 100.0 / revenue), 2) as margin_pct\n"
        "FROM products\n"
        "WHERE revenue > 0\n"
        "ORDER BY margin_pct DESC;"
    },
    {
        "Customer Churn Rate",
        "Calculate customer retention metrics",
        "SELECT \n"
        "    period,\n"
        "    customers_start,\n"
        "    customers_lost,\n"
        "    ROUND((customers_lost * 100.0 / customers_start), 2) as churn_rate_pct,\n"
        "    ROUND((100.0 - (customers_lost * 100.0 / customers_start)), 2) as retention_rate_pct\n"
        "FROM customer_metrics\n"
        "ORDER BY period;"
    },
    {
        "RFM Analysis (Recency, Frequency, Monetary)",
        "Segment customers by purchase behavior",
        "SELECT \n"
        "    customer_id,\n"
        "    JULIANDAY('now') - JULIANDAY(MAX(order_date)) as recency_days,\n"
        "    COUNT(DISTINCT order_id) as frequency,\n"
        "    SUM(amount) as monetary_value,\n"
        "    CASE \n"
        "        WHEN JULIANDAY('now') - JULIANDAY(MAX(order_date)) <= 30 AND COUNT(*) >= 5 THEN 'Champions'\n"
        "        WHEN JULIANDAY('now') - JULIANDAY(MAX(order_date)) <= 90 THEN 'Active'\n"
        "        ELSE 'At Risk'\n"
        "    END as segment\n"
        "FROM orders\n"
        "GROUP BY customer_id\n"
        "ORDER BY monetary_value DESC;"
    },
    {
        "Inventory Turnover",
        "Calculate inventory efficiency",
        "SELECT \n"
        "    product_id,\n"
        "    cost_of_goods_sold,\n"
        "    avg_inventory_value,\n"
        "    ROUND(cost_of_goods_sold / avg_inventory_value, 2) as turnover_ratio,\n"
        "    ROUND(365.0 / (cost_of_goods_sold / avg_inventory_value), 0) as days_to_sell\n"
        "FROM inventory_metrics\n"
        "WHERE avg_inventory_value > 0;"
    },
    {
        "Cohort Retention Analysis",
        "Track retention of user cohorts over time",
        "SELECT \n"
        "    STRFTIME('%Y-%m', signup_date) as cohort,\n"
        "    CAST((JULIANDAY(activity_date) - JULIANDAY(signup_date)) / 30 AS INTEGER) as months_since_signup,\n"
        "    COUNT(DISTINCT customer_id) as active_customers\n"
        "FROM user_activity\n"
        "GROUP BY cohort, months_since_signup\n"
        "ORDER BY cohort, months_since_signup;"
    },
    {
        "Break-Even Analysis",
        "Calculate break-even point",
        "SELECT \n"
        "    product_id,\n"
        "    fixed_costs,\n"
        "    price_per_unit,\n"
        "    variable_cost_per_unit,\n"
        "    ROUND(fixed_costs / (price_per_unit - variable_cost_per_unit), 0) as breakeven_units\n"
        "FROM product_costs\n"
        "WHERE (price_per_unit - variable_cost_per_unit) > 0;"
    },
    {
        "Sales Conversion Funnel",
        "Analyze conversion rates through stages",
        "WITH stages AS (\n"
        "    SELECT stage, COUNT(*) as count\n"
        "    FROM funnel_events\n"
        "    GROUP BY stage\n"
        ")\n"
        "SELECT \n"
        "    a.stage,\n"
        "    a.count,\n"
        "    ROUND((a.count * 100.0 / b.count), 2) as conversion_from_prev_pct\n"
        "FROM stages a\n"
        "LEFT JOIN stages b ON a.stage = b.stage + 1\n"
        "ORDER BY a.stage;"
    },
    {
        "ABC Analysis (Pareto)",
        "Classify items by value contribution",
        "WITH totals AS (\n"
        "    SELECT \n"
        "        product_id,\n"
        "        SUM(revenue) as total_revenue\n"
        "    FROM sales\n"
        "    GROUP BY product_id\n"
        "),\n"
        "ranked AS (\n"
        "    SELECT \n"
        "        product_id,\n"
        "        total_revenue,\n"
        "        SUM(total_revenue) OVER (ORDER BY total_revenue DESC) * 100.0 / \n"
        "        SUM(total_revenue) OVER () as cumulative_pct\n"
        "    FROM totals\n"
        ")\n"
        "SELECT \n"
        "    product_id,\n"
        "    total_revenue,\n"
        "    cumulative_pct,\n"
        "    CASE \n"
        "        WHEN cumulative_pct <= 80 THEN 'A'\n"
        "        WHEN cumulative_pct <= 95 THEN 'B'\n"
        "        ELSE 'C'\n"
        "    END as abc_class\n"
        "FROM ranked\n"
        "ORDER BY total_revenue DESC;"
    }
};

const FunctionCategory business_category = {
    "Business Analytics",
    business_functions,
    sizeof(business_functions) / sizeof(business_functions[0])
};
