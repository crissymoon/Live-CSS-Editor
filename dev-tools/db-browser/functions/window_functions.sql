-- Window Functions for Data Analysis
-- Advanced analytical functions over partitions and frames

-- ============================================
-- ROW_NUMBER
-- ============================================

-- Assign sequential row numbers
SELECT 
    product_name,
    price,
    ROW_NUMBER() OVER (ORDER BY price DESC) as rank
FROM products;

-- Row number within partition
SELECT 
    category,
    product_name,
    price,
    ROW_NUMBER() OVER (PARTITION BY category ORDER BY price DESC) as rank_in_category
FROM products;

-- ============================================
-- RANK AND DENSE_RANK
-- ============================================

-- RANK: Same values get same rank, gaps in ranking
SELECT 
    name,
    score,
    RANK() OVER (ORDER BY score DESC) as rank,
    DENSE_RANK() OVER (ORDER BY score DESC) as dense_rank
FROM scores;

-- Rank by category
SELECT 
    category,
    product_name,
    sales,
    RANK() OVER (PARTITION BY category ORDER BY sales DESC) as sales_rank
FROM product_sales;

-- ============================================
-- NTILE (Quartiles, Deciles, etc.)
-- ============================================

-- Divide into 4 groups (quartiles)
SELECT 
    customer_id,
    total_spent,
    NTILE(4) OVER (ORDER BY total_spent DESC) as quartile
FROM customer_spending;

-- Divide into 10 groups (deciles)
SELECT 
    product_id,
    units_sold,
    NTILE(10) OVER (ORDER BY units_sold DESC) as decile
FROM sales;

-- Top 20% customers
SELECT * FROM (
    SELECT 
        customer_id,
        revenue,
        NTILE(5) OVER (ORDER BY revenue DESC) as quintile
    FROM customer_revenue
)
WHERE quintile = 1;

-- ============================================
-- LAG AND LEAD
-- ============================================

-- Previous value (LAG)
SELECT 
    date,
    value,
    LAG(value, 1) OVER (ORDER BY date) as previous_value,
    value - LAG(value, 1) OVER (ORDER BY date) as change
FROM time_series;

-- Next value (LEAD)
SELECT 
    date,
    value,
    LEAD(value, 1) OVER (ORDER BY date) as next_value
FROM time_series;

-- Compare with value from 2 periods ago
SELECT 
    month,
    revenue,
    LAG(revenue, 2) OVER (ORDER BY month) as revenue_2_months_ago
FROM monthly_revenue;

-- ============================================
-- FIRST_VALUE AND LAST_VALUE
-- ============================================

-- First value in group
SELECT 
    category,
    product_name,
    price,
    FIRST_VALUE(price) OVER (PARTITION BY category ORDER BY price) as lowest_price
FROM products;

-- Last value in group
SELECT 
    category,
    product_name,
    price,
    LAST_VALUE(price) OVER (
        PARTITION BY category 
        ORDER BY price
        ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING
    ) as highest_price
FROM products;

-- ============================================
-- RUNNING TOTALS
-- ============================================

-- Cumulative sum
SELECT 
    date,
    amount,
    SUM(amount) OVER (ORDER BY date) as running_total
FROM transactions;

-- Running total by category
SELECT 
    category,
    date,
    amount,
    SUM(amount) OVER (PARTITION BY category ORDER BY date) as category_running_total
FROM sales;

-- ============================================
-- MOVING AVERAGES
-- ============================================

-- 3-day moving average
SELECT 
    date,
    value,
    AVG(value) OVER (
        ORDER BY date 
        ROWS BETWEEN 2 PRECEDING AND CURRENT ROW
    ) as moving_avg_3day
FROM daily_data;

-- 7-day moving average
SELECT 
    date,
    value,
    AVG(value) OVER (
        ORDER BY date 
        ROWS BETWEEN 6 PRECEDING AND CURRENT ROW
    ) as moving_avg_7day
FROM daily_data;

-- Centered moving average
SELECT 
    date,
    value,
    AVG(value) OVER (
        ORDER BY date 
        ROWS BETWEEN 1 PRECEDING AND 1 FOLLOWING
    ) as centered_avg_3day
FROM daily_data;

-- ============================================
-- RUNNING MIN/MAX
-- ============================================

-- Running maximum
SELECT 
    date,
    value,
    MAX(value) OVER (ORDER BY date) as running_max
FROM time_series;

-- Running minimum
SELECT 
    date,
    value,
    MIN(value) OVER (ORDER BY date) as running_min
FROM time_series;

-- ============================================
-- PERCENT_RANK
-- ============================================

-- Relative rank as percentage (0 to 1)
SELECT 
    name,
    score,
    PERCENT_RANK() OVER (ORDER BY score) as percentile_rank,
    ROUND(PERCENT_RANK() OVER (ORDER BY score) * 100, 2) as percentile
FROM test_scores;

-- ============================================
-- CUME_DIST
-- ============================================

-- Cumulative distribution (percentage of values <= current value)
SELECT 
    value,
    CUME_DIST() OVER (ORDER BY value) as cumulative_dist,
    ROUND(CUME_DIST() OVER (ORDER BY value) * 100, 2) as cumulative_pct
FROM dataset;

-- ============================================
-- WINDOW FRAME SPECIFICATIONS
-- ============================================

-- Last 3 rows
SELECT 
    date,
    value,
    AVG(value) OVER (
        ORDER BY date 
        ROWS BETWEEN 2 PRECEDING AND CURRENT ROW
    ) as avg_last_3
FROM data;

-- Next 3 rows
SELECT 
    date,
    value,
    AVG(value) OVER (
        ORDER BY date 
        ROWS BETWEEN CURRENT ROW AND 2 FOLLOWING
    ) as avg_next_3
FROM data;

-- All rows in partition
SELECT 
    category,
    value,
    AVG(value) OVER (
        PARTITION BY category
        ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING
    ) as category_average
FROM products;

-- Range-based window (by value range)
SELECT 
    date,
    value,
    AVG(value) OVER (
        ORDER BY value
        RANGE BETWEEN 10 PRECEDING AND 10 FOLLOWING
    ) as avg_nearby_values
FROM data;

-- ============================================
-- GROWTH RATE CALCULATIONS
-- ============================================

-- Period-over-period growth rate
SELECT 
    month,
    revenue,
    LAG(revenue) OVER (ORDER BY month) as prev_revenue,
    ((revenue - LAG(revenue) OVER (ORDER BY month)) / 
     LAG(revenue) OVER (ORDER BY month)) * 100 as growth_rate_pct
FROM monthly_revenue;

-- Year-over-year comparison
SELECT 
    year,
    month,
    revenue,
    LAG(revenue, 12) OVER (ORDER BY year, month) as revenue_last_year,
    ((revenue - LAG(revenue, 12) OVER (ORDER BY year, month)) / 
     LAG(revenue, 12) OVER (ORDER BY year, month)) * 100 as yoy_growth_pct
FROM monthly_data;

-- ============================================
-- TOP N PER GROUP
-- ============================================

-- Top 3 products per category by sales
SELECT * FROM (
    SELECT 
        category,
        product_name,
        sales,
        ROW_NUMBER() OVER (PARTITION BY category ORDER BY sales DESC) as rank
    FROM product_sales
)
WHERE rank <= 3;

-- ============================================
-- RUNNING PERCENTAGE
-- ============================================

-- Percentage of running total
SELECT 
    item,
    value,
    SUM(value) OVER (ORDER BY value DESC) as running_total,
    (SUM(value) OVER (ORDER BY value DESC) * 100.0 / 
     SUM(value) OVER ()) as pct_of_total
FROM items;

-- ============================================
-- CONSECUTIVE SEQUENCES
-- ============================================

-- Identify consecutive groups
SELECT 
    date,
    value,
    ROW_NUMBER() OVER (ORDER BY date) - 
    ROW_NUMBER() OVER (PARTITION BY value ORDER BY date) as sequence_group
FROM daily_status;

-- ============================================
-- GAPS AND ISLANDS
-- ============================================

-- Find date gaps
WITH date_gaps AS (
    SELECT 
        date,
        LEAD(date) OVER (ORDER BY date) as next_date,
        JULIANDAY(LEAD(date) OVER (ORDER BY date)) - JULIANDAY(date) as days_gap
    FROM events
)
SELECT * FROM date_gaps
WHERE days_gap > 1;

-- ============================================
-- MOVING SUM
-- ============================================

-- Rolling 7-day sum
SELECT 
    date,
    amount,
    SUM(amount) OVER (
        ORDER BY date 
        ROWS BETWEEN 6 PRECEDING AND CURRENT ROW
    ) as rolling_7day_sum
FROM daily_sales;

-- ============================================
-- RATIO TO TOTAL
-- ============================================

-- Percentage of total
SELECT 
    category,
    sales,
    SUM(sales) OVER () as total_sales,
    (sales * 100.0 / SUM(sales) OVER ()) as pct_of_total
FROM category_sales;

-- Percentage within partition
SELECT 
    region,
    category,
    sales,
    (sales * 100.0 / SUM(sales) OVER (PARTITION BY region)) as pct_of_region_sales
FROM regional_sales;

-- ============================================
-- RANGE BETWEEN ROWS
-- ============================================

-- Count events in time window
SELECT 
    event_time,
    COUNT(*) OVER (
        ORDER BY event_time 
        RANGE BETWEEN INTERVAL 1 HOUR PRECEDING AND CURRENT ROW
    ) as events_last_hour
FROM events;

-- ============================================
-- MULTIPLE WINDOW FUNCTIONS
-- ============================================

-- Combine multiple window calculations
SELECT 
    date,
    sales,
    -- Running total
    SUM(sales) OVER (ORDER BY date) as running_total,
    -- Moving average
    AVG(sales) OVER (ORDER BY date ROWS BETWEEN 6 PRECEDING AND CURRENT ROW) as moving_avg,
    -- Rank
    RANK() OVER (ORDER BY sales DESC) as sales_rank,
    -- Deviation from average
    sales - AVG(sales) OVER () as deviation_from_mean,
    -- Percentage of total
    (sales * 100.0 / SUM(sales) OVER ()) as pct_of_total
FROM daily_sales;

-- ============================================
-- CONDITIONAL WINDOW FUNCTIONS
-- ============================================

-- Running total of specific values
SELECT 
    date,
    amount,
    category,
    SUM(CASE WHEN category = 'Revenue' THEN amount ELSE 0 END) 
        OVER (ORDER BY date) as running_revenue,
    SUM(CASE WHEN category = 'Expense' THEN amount ELSE 0 END) 
        OVER (ORDER BY date) as running_expenses
FROM transactions;

-- ============================================
-- STANDARD DEVIATION OVER WINDOW
-- ============================================

-- Rolling standard deviation
WITH window_stats AS (
    SELECT 
        date,
        value,
        AVG(value) OVER (ORDER BY date ROWS BETWEEN 29 PRECEDING AND CURRENT ROW) as avg_30day
    FROM daily_data
)
SELECT 
    date,
    value,
    avg_30day,
    (value - avg_30day) as deviation
FROM window_stats;

-- ============================================
-- COMPARING TO AGGREGATE
-- ============================================

-- Compare individual to group average
SELECT 
    employee_name,
    department,
    salary,
    AVG(salary) OVER (PARTITION BY department) as dept_avg_salary,
    salary - AVG(salary) OVER (PARTITION BY department) as diff_from_avg,
    (salary / AVG(salary) OVER (PARTITION BY department)) * 100 as pct_of_avg
FROM employees;

-- ============================================
-- STREAK COUNTING
-- ============================================

-- Count consecutive occurrences
WITH numbered AS (
    SELECT 
        date,
        status,
        ROW_NUMBER() OVER (ORDER BY date) as rn,
        ROW_NUMBER() OVER (PARTITION BY status ORDER BY date) as status_rn
    FROM daily_status
)
SELECT 
    MIN(date) as streak_start,
    MAX(date) as streak_end,
    status,
    COUNT(*) as streak_length
FROM numbered
GROUP BY status, rn - status_rn
HAVING COUNT(*) > 1
ORDER BY streak_length DESC;
